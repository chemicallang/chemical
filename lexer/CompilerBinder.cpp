// Copyright (c) Qinetik 2024.

#include <sstream>
#include <utility>
#include "lexer/model/CompilerBinder.h"
#include "lexer/model/CompilerBinderTCC.h"
#include "utils/PathUtils.h"
#include "integration/libtcc/LibTccInteg.h"
#include "compiler/ASTProcessor.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/structures/MembersContainer.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "integration/cbi/bindings/LexerCBI.h"

void handle_error(void *opaque, const char *msg){
    const auto binder = (CompilerBinderTCC*) opaque;
    binder->diagnostics.emplace_back(msg);
}

CompilerBinder::CompilerBinder() {

}

CompilerBinderTCC::CompilerBinderTCC(std::string exe_path) : CompilerBinder(), exe_path(std::move(exe_path)) {
    auto& provider = interface_maps["SourceProvider"];
    source_provide_symbol_map(provider);
    auto& lexer = interface_maps["Lexer"];
    lexer_symbol_map(lexer);
    auto& context = interface_maps["BuildContext"];
    build_context_symbol_map(context);
}

void declare_func(FunctionDeclaration* func, TCCState* state, std::unordered_map<std::string, void*>& sym_map) {
    const auto sym_name = func->runtime_name_str();
    auto sym = tcc_get_symbol(state, sym_name.c_str());
    if(sym) {
        sym_map[sym_name] = sym;
    } else {
#ifdef DEBUG
      throw std::runtime_error("symbol not found");
#endif
    }
}

void declare_sym_map(std::unordered_map<std::string, void*>& from_sym_map, std::unordered_map<std::string, void*>& to_sym_map) {
    for(auto& sym : from_sym_map) {
        to_sym_map[sym.first] = sym.second;
    }
}

bool CompilerBinderTCC::import_compiler_interface(const std::string& name, TCCState* state) {
    auto map = interface_maps.find(name);
    if(map != interface_maps.end()) {
        for(auto& sym : map->second) {
            tcc_add_symbol(state, sym.first.c_str(), sym.second);
        }
        return true;
    } else {
        return false;
    }
}

inline void declare_functions(const std::vector<FunctionDeclaration*>& functions, TCCState* state, std::unordered_map<std::string, void*>& sym_map) {
    for(auto& func : functions) declare_func(func, state, sym_map);
}

void declare_node(
    CompilerBinderTCC& binder,
    ASTNode* node,
    TCCState* state,
    std::unordered_map<std::string, void*>& sym_map
) {
    auto node_kind = node->kind();
    switch(node_kind) {
        case ASTNodeKind::FunctionDecl:
        case ASTNodeKind::ExtensionFunctionDecl:
            declare_func((FunctionDeclaration*) node, state, sym_map);
            return;
        case ASTNodeKind::NamespaceDecl:
            for(auto& child_node : ((Namespace*) node)->nodes) {
                declare_node(binder, child_node, state, sym_map);
            }
            return;
        case ASTNodeKind::MultiFunctionNode:
            declare_functions(((MultiFunctionNode*) node)->functions, state, sym_map);
            return;
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl:
        case ASTNodeKind::UnionDecl: {
            const auto container = (MembersContainer*) node;
            if(container->has_annotation(AnnotationKind::CompilerInterface)) {
                auto container_node = container->as_extendable_members_container_node();
                if(container_node) {
                    auto map = binder.symbol_maps.find(container_node->name);
                    if(map != binder.symbol_maps.end()) {
                        declare_sym_map(map->second, sym_map);
                    } else {
                        std::cerr << "[Binder] couldn't find compiler interface by name '" << container_node->name << "'" << std::endl;
                    }
                } else {
                    std::cerr << "[Binder] couldn't find compiler interface by name '";
                    container->runtime_name(std::cerr);
                    std::cerr << '\'' << std::endl;
                }
                return;
            }
            declare_functions(container->functions(), state, sym_map);
            return;
        }
        default:
            return;
    }
}

BinderResult CompilerBinderTCC::compile(
    const std::string& cbi_name,
    const std::string& program,
    CBIData& cbiData,
    std::vector<std::string_view>& imports,
    std::vector<std::string_view>& current_files,
    ASTProcessor& processor
) {
    auto found = compiled.find(cbi_name);
    if(found != compiled.end()) {
        return BinderResult { 1, "cbi has already been compiled " + cbi_name };
    }
    auto state = tcc_new();
    if(!state) {
        return BinderResult { 1, "couldn't initialize tcc state in tcc compiler binder" };
    }
    tcc_set_error_func(state, this, handle_error);
    auto tcc_dir = resolve_non_canon_parent_path(exe_path, "packages/tcc");
    auto include_dir = resolve_rel_child_path_str(tcc_dir, "include");
    auto lib_dir = resolve_rel_child_path_str(tcc_dir, "lib");
    int result;
    result = tcc_add_include_path(state, include_dir.c_str());
    if(result == -1) {
        return BinderResult { 1, "couldn't add include path 'packages/tcc/include' in tcc compiler binder" };
    }
    result = tcc_add_library_path(state, lib_dir.c_str());
    if(result == -1) {
        return BinderResult { 1, "couldn't add library path 'packages/tcc/lib' in tcc compiler binder" };
    }
    result = tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
    if(result == -1) {
        return BinderResult { 1, "couldn't set tcc output memory in tcc compiler binder" };
    }

    // compile
    result = tcc_compile_string(state, program.c_str());
    if(result == -1) {
        return BinderResult { 1, "couldn't compile c code in binder for cbi " + cbi_name };
    }

    // add functions like malloc and free
    prepare_tcc_state_for_jit(state);

    // adding symbols from other modules
    for(auto& file : imports) {
        const std::string& abs_path = file.data();
        auto sym_map_itr = symbol_maps.find(abs_path);
        if(sym_map_itr != symbol_maps.end()) {
            for(auto& sym : sym_map_itr->second) {
                tcc_add_symbol(state, sym.first.c_str(), sym.second);
            }
        } else {
            return BinderResult { 1, "couldn't import symbol map for cbi '" + cbi_name + "' when importing file '" + abs_path };
        }
    }

    // any other functions user require, he would mention by including cbi types
    // in that case, compiler will expose symbols that correspond to that type
//    for(auto& cbiType : cbiData.cbiTypes) {
//        switch(cbiType.kind) {
//            case CBIImportKind::Lexer:
//                // TODO lexer functions should be declared here
//                break;
//        }
//    }


    // relocate the code
    result = tcc_relocate(state);
    if(result == -1) {
        return BinderResult { 1, "couldn't relocate c code in binder for cbi " + cbi_name };
    }

    // create symbol from current module's files
    for(auto& file : current_files) {
        const std::string& abs_path = file.data();
        auto& sym_map = symbol_maps[abs_path]; // <--- auto creation
        auto unit = processor.shrinked_unit.find(abs_path);
        if(unit != processor.shrinked_unit.end()) {
            auto& nodes = unit->second.scope.nodes;
            for(auto& node : nodes) {
                declare_node(*this, node, state, sym_map);
            }
        }
    }

    compiled[cbi_name] = state;
    return BinderResult { 0, "" };
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
            if(sym) {
                cached_func[complete_cached_name] = sym;
            }
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