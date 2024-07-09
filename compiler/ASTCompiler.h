// Copyright (c) Qinetik 2024.

#pragma once

#include "ASTProcessor.h"

class ASTCompiler : public ASTProcessor {
public:

    using ASTProcessor::ASTProcessor;

    void compile_nodes(
        Codegen* gen,
        ShrinkingVisitor& shrinker,
        const FlatIGFile &file
    );

};