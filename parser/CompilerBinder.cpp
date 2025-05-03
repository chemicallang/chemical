// Copyright (c) Chemical Language Foundation 2025.

#include <sstream>
#include <utility>
#include "compiler/cbi/model/CompilerBinder.h"
#include "utils/PathUtils.h"
#include "integration/libtcc/LibTccInteg.h"
#include "compiler/ASTProcessor.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/structures/MembersContainer.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "compiler/cbi/bindings/CBI.h"
#include "rang.hpp"

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
    switch(index.fn_type) {
        case CBIFunctionType::InitializeLexer:
            initializeLexerFunctions[index.key.to_chem_view()] = (UserLexerInitializeFn) sym;
            break;
        case CBIFunctionType::ParseMacroValue:
            parseMacroValueFunctions[index.key.to_chem_view()] = (UserParserParseMacroValueFn) sym;
            break;
        case CBIFunctionType::ParseMacroNode:
            parseMacroNodeFunctions[index.key.to_chem_view()] = (UserParserParseMacroNodeFn) sym;
            break;
    }
    return nullptr;
}

CompilerBinder::~CompilerBinder() {
    for(auto& unit : data) {
        if(unit.second.module != nullptr) {
            tcc_delete(unit.second.module);
        }
    }
}