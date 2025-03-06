// Copyright (c) Qinetik 2025.

#pragma once

class ASTNode;

class SymbolResolver;

namespace chem {
    class string_view;
}

extern "C" {

    ASTNode* SymbolResolverfind(SymbolResolver* resolver, chem::string_view* name);

}