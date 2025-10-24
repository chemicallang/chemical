// Copyright (c) Chemical Language Foundation 2025.

#include <sstream>
#include <utility>
#include "compiler/cbi/model/CompilerBinder.h"
#include "utils/PathUtils.h"
#include "integration/libtcc/LibTccInteg.h"
#include "compiler/cbi/bindings/CBI.h"

CompilerBinder::CompilerBinder(std::string exe_path) : exe_path(std::move(exe_path)) {
    prepare_cbi_maps(interface_maps);
}

void CompilerBinder::import_compiler_interface(const std::span<const std::pair<chem::string_view, void*>>& interface, TCCState* state) {
    for(auto& sym : interface) {
        tcc_add_symbol(state, sym.first.data(), sym.second);
    }
}

const char* CompilerBinder::index_function(CBIFunctionIndex& index, TCCState* state) {
    const auto sym = tcc_get_symbol(state, index.fn_name.data());
    if(!sym) {
        return "function with this name doesn't exist";
    }
    registerHook(index.fn_type, index.key.to_chem_view(), sym);
    return nullptr;
}

CompilerBinder::~CompilerBinder() {
    for(auto& unit : data) {
        if(unit.second.module != nullptr) {
            tcc_delete(unit.second.module);
        }
    }
}