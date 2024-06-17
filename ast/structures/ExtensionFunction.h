// Copyright (c) Qinetik 2024.

#pragma once

#include "FunctionDeclaration.h"

class ExtensionFunction : public FunctionDeclaration {
public:

    std::string name;
    std::unique_ptr<BaseType> type;

    /**
     * extension function will add references to extendable members container
     */
    void declare_top_level(SymbolResolver &linker) override;

};