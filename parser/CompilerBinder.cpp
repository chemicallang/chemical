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
    auto sym = tcc_get_symbol(state, "initializeLexer");
    if(!sym) {
        return "doesn't contain function 'initializeLexer'";
    }
    initializeLexerFunctions[cbiName] = (UserLexerInitializeFn) sym;
    return nullptr;
}

const char* CompilerBinder::prepare_macro_parser_from(const chem::string_view& cbiName, TCCState* state) {
    auto sym2 = tcc_get_symbol(state, "parseMacroValue");
    if(!sym2) {
        return "doesn't contain function 'parseMacroValue'";
    }
    auto sym3 = tcc_get_symbol(state, "parseMacroNode");
    if(!sym3) {
        return "doesn't contain function 'parseMacroNode'";
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

bool CompilerBinder::import_compiler_interface(const std::string& name, TCCState* state) {
    auto map = interface_maps.find(chem::string_view(name));
    if(map != interface_maps.end()) {
        for(auto& sym : map->second) {
            tcc_add_symbol(state, sym.first.data(), sym.second);
        }
        return true;
    } else {
        return false;
    }
}

CompilerBinder::~CompilerBinder() {
    for(auto& unit : data) {
        if(unit.second.module != nullptr) {
            tcc_delete(unit.second.module);
        }
    }
}