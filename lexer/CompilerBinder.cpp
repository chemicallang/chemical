// Copyright (c) Qinetik 2024.

#include <sstream>
#include <utility>
#include "cst/base/CSTToken.h"
#include "lexer/model/CompilerBinder.h"
#include "lexer/model/CompilerBinderTCC.h"
#include "utils/PathUtils.h"
#include "lexer/model/CompilerBinderCommon.h"
#include "ast/base/ASTNode.h"

CSTToken dummy_token_at_start() {
    return CSTToken { LexTokenType::Identifier, Position(0,0),""};
}

void error(CSTDiagnoser* diagnoser, const std::string& msg) {
    auto dummy = dummy_token_at_start();
    diagnoser->error(msg, &dummy);
}

void handle_error(void *opaque, const char *msg){
    auto binder = (CompilerBinderTCC*) opaque;
    error(binder->diagnoser, msg);
}

CompilerBinderCommon::CompilerBinderCommon(
    CSTDiagnoser* diagnoser,
    ASTAllocator& job_allocator,
    ASTAllocator& mod_allocator
) : converter("", false, "binder", global, job_allocator, mod_allocator), diagnoser(diagnoser), resolver(global, false, mod_allocator, job_allocator), global(nullptr, nullptr, mod_allocator) {

}

CompilerBinderTCC::CompilerBinderTCC(
    CSTDiagnoser* diagnoser,
    std::string exe_path,
    ASTAllocator& job_allocator,
    ASTAllocator& mod_allocator
) : CompilerBinderCommon(diagnoser, job_allocator, mod_allocator), translator(global, nullptr, mod_allocator), exe_path(std::move(exe_path)) {
    translator.comptime_scope.prepare_top_level_namespaces(resolver);
}

std::vector<ASTNode*> CompilerBinderCommon::parse(std::vector<CSTToken*>& tokens) {

    if(tokens.empty()) {
        return {};
    }

    // convert the tokens
    converter.convert(tokens);

    // move the diagnostics
    if(!converter.diagnostics.empty()) {
        diagnoser->diagnostics.insert(diagnoser->diagnostics.end(), std::make_move_iterator(converter.diagnostics.begin()), std::make_move_iterator(converter.diagnostics.end()));
    }
    if(converter.has_errors) {
        return {};
    }

    // convert the nodes
    auto nodes = std::move(converter.nodes);

    // symbol resolution
    for(auto& node : nodes) {
        node->declare_top_level(resolver);
    }
    for(auto& node : nodes) {
        node->declare_and_link(resolver);
    }
    if(resolver.has_errors) {
        resolver.print_diagnostics("unknown_path", "Binder");
    }

    return nodes;

}

void to_cbi(CompilerBinderCommon* binder, const std::string& cbi_name, std::vector<ASTNode*>& nodes) {
    auto found_cbi = binder->cbi.find(cbi_name);
    if(found_cbi != binder->cbi.end()) {
        for(auto& node : nodes) {
            found_cbi->second.emplace_back(node);
        }
    }
}

void CompilerBinderCommon::collect(const std::string& name, std::vector<CSTToken*> &tokens, bool err_no_found) {
    auto nodes = parse(tokens);
    auto found = collected.find(name);
    if(found == collected.end()) {
        if(err_no_found) {
            auto dummy = dummy_token_at_start();
            diagnoser->error("Couldn't find container for collection by name " + name, &dummy);
        } else {
            to_cbi(this, name, nodes);
            collected[name] = std::move(nodes);
        }
    } else {
        to_cbi(this, name, nodes);
        auto& container = found->second;
        container.insert(container.end(), std::make_move_iterator(nodes.begin()), std::make_move_iterator(nodes.end()));
    }
}

void CompilerBinderCommon::create_cbi(const std::string &cbi_name) {
    auto found = collected.find(cbi_name);
    auto found_cbi = cbi.find(cbi_name);
    if(found != collected.end() || found_cbi != cbi.end()) {
        auto dummy = dummy_token_at_start();
        diagnoser->error("CBI with name " + cbi_name + " already exists", &dummy);
    } else {
        collected[cbi_name] = std::vector<ASTNode*> {};
        cbi[cbi_name] = {};
    }
}

void CompilerBinderCommon::import_container(const std::string &cbi_name, const std::string &container) {
    auto found = collected.find(container);
    if(found == collected.end()) {
        auto dummy = dummy_token_at_start();
        diagnoser->error("Couldn't import container by name " + container, &dummy);
    } else {
        auto cbi_found = cbi.find(cbi_name);
        if(cbi_found != cbi.end()) {
            for(auto& node : found->second) {
                cbi_found->second.emplace_back(node);
            }
        } else {
            auto dummy = dummy_token_at_start();
            diagnoser->error("Couldn't find CBI by name " + cbi_name, &dummy);
        }
    }
}

bool CompilerBinderTCC::compile(const std::string& cbi_name) {

    auto found = compiled.find(cbi_name);
    if(found != compiled.end()) {
        error(diagnoser, "cbi has already been compiled " + cbi_name);
        return false;
    }
    auto cbi_vec = cbi.find(cbi_name);
    if(cbi_vec == cbi.end()) {
        error(diagnoser, "couldn't find CBI by name " + cbi_name);
        return false;
    }
    if(cbi_vec->second.empty()) {
        error(diagnoser, "cannot compile an empty CBI by name " + cbi_name);
        return false;
    }
    auto state = tcc_new();
    if(!state) {
        error(diagnoser, "couldn't initialize tcc state in tcc compiler binder");
        return false;
    }
    tcc_set_error_func(state, this, handle_error);
    auto tcc_dir = resolve_non_canon_parent_path(exe_path, "packages/tcc");
    auto include_dir = resolve_rel_child_path_str(tcc_dir, "include");
    auto lib_dir = resolve_rel_child_path_str(tcc_dir, "lib");
    int result;
    result = tcc_add_include_path(state, include_dir.c_str());
    if(result == -1) {
        error(diagnoser, "couldn't add include path 'packages/tcc/include' in tcc compiler binder");
        return false;
    }
    result = tcc_add_library_path(state, lib_dir.c_str());
    if(result == -1) {
        error(diagnoser, "couldn't add library path 'packages/tcc/lib' in tcc compiler binder");
        return false;
    }
    result = tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
    if(result == -1) {
        error(diagnoser, "couldn't set tcc output memory in tcc compiler binder");
        return false;
    }

    // translate to c
    std::ostringstream stream;
    translator.output = &stream;
    translator.prepare_translate();
    translator.translate(cbi_vec->second);

    // compile
    result = tcc_compile_string(state, stream.str().c_str());
    if(result == -1) {
        error(diagnoser, "couldn't compile c code in binder for cbi " + cbi_name + ", where c code:\n" + stream.str());
        return false;
    }

    // relocate the code
    result = tcc_relocate(state);
    if(result == -1) {
        error(diagnoser, "couldn't relocate c code in binder for cbi " + cbi_name);
        return false;
    }

    compiled[cbi_name] = state;
    return true;
}

void* CompilerBinderTCC::provide_func(const std::string& cbi_name, const std::string& funcName) {
    auto complete_cached_name = cbi_name + ':' + funcName;
    auto found = cached_func.find(complete_cached_name);
    if(found != cached_func.end()) {
        return found->second;
    } else {
        auto cbi = compiled.find(cbi_name);
        if(cbi != compiled.end()) {
            auto sym = tcc_get_symbol(cbi->second, funcName.c_str());
            cached_func[complete_cached_name] = sym;
            return sym;
        } else {
            return nullptr;
        }
    }
}

CompilerBinderTCC::~CompilerBinderTCC() {
    for(auto& unit : compiled) {
        tcc_delete(unit.second);
    }
}