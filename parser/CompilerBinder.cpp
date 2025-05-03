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

const char* CompilerBinder::prepare_macro_lexer_from(const chem::string_view& cbiName, TCCState* state) {
    std::string funcName(cbiName.data(), cbiName.size());
    funcName.append("_initializeLexer");
    auto sym = tcc_get_symbol(state, funcName.data());
    if(!sym) {
        return "doesn't contain function 'initializeLexer' prefixed with cbi name";
    }
    initializeLexerFunctions[cbiName] = (UserLexerInitializeFn) sym;
    return nullptr;
}

const char* CompilerBinder::prepare_macro_parser_from(const chem::string_view& cbiName, TCCState* state) {
    std::string funcName(cbiName.data(), cbiName.size());
    funcName.append("_parseMacroValue");
    auto sym2 = tcc_get_symbol(state, funcName.data());
    if(!sym2) {
        return "doesn't contain function 'parseMacroValue' prefixed with cbi name";
    }
    funcName.clear();
    funcName.append(cbiName.data(), cbiName.size());
    funcName.append("_parseMacroNode");
    auto sym3 = tcc_get_symbol(state, funcName.data());
    if(!sym3) {
        return "doesn't contain function 'parseMacroNode' prefixed with cbi name";
    }
    parseMacroValueFunctions[cbiName] = (UserParserParseMacroValueFn) sym2;
    parseMacroNodeFunctions[cbiName] = (UserParserParseMacroNodeFn) sym3;
    return nullptr;
}

const char* CompilerBinder::prepare_with_type(const chem::string_view& cbiName, TCCState* state, CBIType type) {
    switch(type) {
        case CBIType::MacroLexer:
            return prepare_macro_lexer_from(cbiName, state);
        case CBIType::MacroParser:
            return prepare_macro_parser_from(cbiName, state);
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