// Copyright (c) Qinetik 2024.

#include <sstream>
#include "lexer/model/tokens/IdentifierToken.h"
#include "lexer/model/CompilerBinder.h"
#include "lexer/model/CompilerBinderTCC.h"

IdentifierToken dummy_token_at_start() {
    return IdentifierToken { Position(0,0),""};
}

void handle_error(void *opaque, const char *msg){
    auto binder = (CompilerBinderTCC*) opaque;
    if(binder->cached_start_token && binder->cached_end_token) {
        binder->diagnoser->error(msg, binder->cached_start_token, binder->cached_end_token);
    } else {
        auto dummy = dummy_token_at_start();
        binder->diagnoser->error(msg, &dummy, &dummy);
    }
}

CompilerBinderTCC::CompilerBinderTCC(CSTDiagnoser* diagnoser) : converter(false), translator(nullptr, ""), diagnoser(diagnoser), resolver("", false) {
    cached_start_token = nullptr;
    cached_end_token = nullptr;
}

void CompilerBinderTCC::init() {
    state = tcc_new();
    auto dummy = dummy_token_at_start();
    if(!state) {
        diagnoser->error("couldn't initialize tcc state in tcc compiler binder", &dummy);
    }
    tcc_set_error_func(state, this, handle_error);
    int result;
    result = tcc_add_include_path(state, "packages/tcc/include");
    if(result == -1) {
        diagnoser->error("couldn't add include path 'packages/tcc/include' in tcc compiler binder", &dummy);
    }
    result = tcc_add_library_path(state, "packages/tcc/lib");
    if(result == -1) {
        diagnoser->error("couldn't add library path 'packages/tcc/lib' in tcc compiler binder", &dummy);
    }
    result = tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
    if(result == -1) {
        diagnoser->error("couldn't set tcc output memory in tcc compiler binder", &dummy);
    }
    translator.inline_struct_members_fn_types = true;
}

bool CompilerBinderTCC::compile(std::vector<std::unique_ptr<CSTToken>>& tokens) {

    if(!state) return false;
    if(tokens.empty()) return false;

    cached_start_token = tokens[0].get();
    cached_end_token = tokens[tokens.size() - 1].get();

    // convert the tokens
    converter.convert(tokens);

    // move the diagnostics
    if(!converter.diagnostics.empty()) {
        diagnoser->diagnostics.insert(diagnoser->diagnostics.end(), std::make_move_iterator(converter.diagnostics.begin()), std::make_move_iterator(converter.diagnostics.end()));
    }
    if(converter.has_errors) {
        return false;
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
        for(auto& err : resolver.errors) {
            diagnoser->error("[SymRes] " + err.message, cached_start_token, cached_end_token);
        }
    }

    // translate nodes to c
    std::ostringstream stream;
    translator.output = &stream;
    translator.translate(nodes);

    // collect nodes
    collected.insert(collected.end(), std::make_move_iterator(nodes.begin()), std::make_move_iterator(nodes.end()));

    int result;

    result = tcc_compile_string(state, stream.str().c_str());
    if(result == -1) {
        diagnoser->error("couldn't compile c code in binder", cached_start_token, cached_end_token);
        return false;
    }

    // relocate the code
    result = tcc_relocate(state);
    if(result == -1) {
        diagnoser->error("couldn't relocate c code in binder", cached_start_token, cached_end_token);
        return false;
    }

    return true;

}

void* CompilerBinderTCC::provide_func(const std::string& funcName) {
    auto found = cached_func.find(funcName);
    if(found != cached_func.end()) {
        return found->second;
    } else {
        auto sym = tcc_get_symbol(state, funcName.c_str());
        cached_func[funcName] = sym;
        return sym;
    }
}

void CompilerBinderTCC::reset_new_file() {
    cached_start_token = nullptr;
    cached_end_token = nullptr;
}

CompilerBinderTCC::~CompilerBinderTCC() {
    if(state) {
        tcc_delete(state);
    }
}