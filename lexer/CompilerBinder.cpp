// Copyright (c) Qinetik 2024.

#include <sstream>
#include <utility>
#include "lexer/model/CompilerBinder.h"
#include "utils/PathUtils.h"
#include "integration/libtcc/LibTccInteg.h"
#include "compiler/ASTProcessor.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/structures/MembersContainer.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "integration/cbi/bindings/CBI.h"
#include "rang.hpp"

void handle_error(void *opaque, const char *msg){
    const auto binder = (CompilerBinder*) opaque;
    binder->diagnostics.emplace_back(msg);
}

CompilerBinder::CompilerBinder(std::string exe_path) : exe_path(std::move(exe_path)) {
    source_provider_symbol_map(interface_maps["SourceProvider"]);
    lexer_symbol_map(interface_maps["Lexer"]);
    build_context_symbol_map(interface_maps["BuildContext"]);
    cst_token_symbol_map(interface_maps["CSTToken"]);
    ast_builder_symbol_map(interface_maps["ASTBuilder"]);
    ptr_vec_symbol_map(interface_maps["PtrVec"]);
    cst_converter_symbol_map(interface_maps["CSTConverter"]);
}

void declare_func(FunctionDeclaration* func, TCCState* state, std::unordered_map<std::string, void*>& sym_map) {
    const auto sym_name = func->runtime_name_str();
    auto sym = tcc_get_symbol(state, sym_name.c_str());
    if(sym) {
        sym_map[sym_name] = sym;
    } else {
        // symbols like printf are defined in other modules or other files that we import
        // we cannot define those, so we must not generate an error here
    }
}

void declare_sym_map(std::unordered_map<std::string, void*>& from_sym_map, std::unordered_map<std::string, void*>& to_sym_map) {
    for(auto& sym : from_sym_map) {
        to_sym_map[sym.first] = sym.second;
    }
}

bool CompilerBinder::import_compiler_interface(const std::string& name, TCCState* state) {
    auto map = interface_maps.find(name);
    if(map != interface_maps.end()) {
        for(auto& sym : map->second) {
            tcc_add_symbol(state, sym.first.data(), sym.second);
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
    CompilerBinder& binder,
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
// this is probably not required because nodes imported from other files are declared using C translation
// in c translation we look for compiler interfaces, and then declare them before importing symbols from other files
//            if(container->has_annotation(AnnotationKind::CompilerInterface)) {
//                auto container_node = container->as_extendable_members_container_node();
//                if(container_node) {
//                    auto map = binder.interface_maps.find(container_node->name);
//                    if(map != binder.interface_maps.end()) {
//                        declare_sym_map(map->second, sym_map);
//                    } else {
//                        std::cerr << rang::fg::red << "[Binder] couldn't find compiler interface by name '" << container_node->name << "'" << rang::fg::reset << std::endl;
//                    }
//                } else {
//                    std::cerr << "[Binder] couldn't find compiler interface by name '";
//                    container->runtime_name(std::cerr);
//                    std::cerr << '\'' << std::endl;
//                }
//                return;
//            }
            declare_functions(container->functions(), state, sym_map);
            return;
        }
        default:
            return;
    }
}

CBIData* CompilerBinder::create_cbi(const std::string& name, unsigned int mod_count) {
    auto found = data.find(name);
    if(found != data.end()) {
        return nullptr;
    }
    auto& mod_data = data[name]; // <------ auto creation
    mod_data.modules.reserve(mod_count);
    return &mod_data;
}

BinderResult CompilerBinder::compile(
    CBIData& cbiData,
    const std::string& program,
    std::vector<std::string_view>& imports,
    std::vector<std::string_view>& current_files,
    const std::vector<std::string>& compiler_interfaces,
    ASTProcessor& processor
) {
    auto state = tcc_new();
    if(!state) {
        return {"couldn't initialize tcc state in tcc compiler binder"};
    }
    tcc_set_error_func(state, this, handle_error);
    auto tcc_dir = resolve_non_canon_parent_path(exe_path, "packages/tcc");
    auto include_dir = resolve_rel_child_path_str(tcc_dir, "include");
    auto lib_dir = resolve_rel_child_path_str(tcc_dir, "lib");
    int result;
    result = tcc_add_include_path(state, include_dir.c_str());
    if(result == -1) {
        return { "couldn't add include path 'packages/tcc/include' in tcc compiler binder" };
    }
    result = tcc_add_library_path(state, lib_dir.c_str());
    if(result == -1) {
        return { "couldn't add library path 'packages/tcc/lib' in tcc compiler binder" };
    }
    result = tcc_set_output_type(state, TCC_OUTPUT_MEMORY);
    if(result == -1) {
        return { "couldn't set tcc output memory in tcc compiler binder" };
    }

    // compile
    result = tcc_compile_string(state, program.c_str());
    if(result == -1) {
        return { "couldn't compile c code in binder" };
    }

    // adding compiler interfaces requested
    for(auto& interface : compiler_interfaces) {
        if(!import_compiler_interface(interface, state)) {
            return { "couldn't import compiler interface by name " + interface };
        }
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
            return { "couldn't import symbol map when importing file '" + abs_path };
        }
    }

    // relocate the code
    result = tcc_relocate(state);
    if(result == -1) {
        return { "couldn't relocate c code in binder"};
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

    cbiData.modules.emplace_back(state);

    return { state };
}

void* CompilerBinder::provide_func(const std::string& cbi_name, const std::string& funcName) {
    auto complete_cached_name = cbi_name + ':' + funcName;
    auto found = cached_func.find(complete_cached_name);
    if(found != cached_func.end()) {
        return found->second;
    } else {
        auto cbi = data.find(cbi_name);
        if(cbi != data.end() && cbi->second.entry_module) {
            auto sym = tcc_get_symbol(cbi->second.entry_module, funcName.c_str());
            if(sym) {
                cached_func[complete_cached_name] = sym;
            }
            return sym;
        } else {
            return nullptr;
        }
    }
}

CompilerBinder::~CompilerBinder() {
    for(auto& unit : data) {
        for(auto& mod : unit.second.modules) {
            tcc_delete(mod);
        }
    }
}