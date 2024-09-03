// Copyright (c) Qinetik 2024.

#pragma once

#include "ASTProcessor.h"

class ASTCompiler : public ASTProcessor {
public:

    using ASTProcessor::ASTProcessor;

    /**
     * called to compile nodes for a first time, this is called once, even
     * if a file is imported in multiple modules
     */
    void compile_nodes(
        Codegen& gen,
        Scope& import_res,
        const FlatIGFile &file
    );

    /**
     * called when file was compiled in another module and now is being
     * imported in another module for the first time
     */
    void declare_nodes(
        Codegen& gen,
        Scope& import_res,
        const FlatIGFile &file
    );

};