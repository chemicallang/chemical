// Copyright (c) Chemical Language Foundation 2025.

#include <sstream>
#include <utility>
#include "parser/model/CompilerBinder.h"
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
    batch_allocator_symbol_map(interface_maps["BatchAllocator"]);
    serial_str_allocator_symbol_map(interface_maps["SerialStrAllocator"]);
    lexer_symbol_map(interface_maps["Lexer"]);
    parser_symbol_map(interface_maps["Parser"]);
    build_context_symbol_map(interface_maps["BuildContext"]);
    ast_builder_symbol_map(interface_maps["ASTBuilder"]);
    symbol_resolver_symbol_map(interface_maps["SymbolResolver"]);
    ptr_vec_symbol_map(interface_maps["PtrVec"]);
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

    // relocate the code
    result = tcc_relocate(state);
    if(result == -1) {
        return { "couldn't relocate c code in binder"};
    }

    cbiData.modules.emplace_back(state);

    return { state };
}

CompilerBinder::~CompilerBinder() {
    for(auto& unit : data) {
        for(auto& mod : unit.second.modules) {
            tcc_delete(mod);
        }
    }
}