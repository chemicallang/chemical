// Copyright (c) Qinetik 2024.

#include <string>
#include "compiler/ASTProcessorOptions.h"

/**
 * this allows you to control the compilation process
 */
class SourceVerifierOptions : public ASTProcessorOptions {
public:

    using ASTProcessorOptions::ASTProcessorOptions;

};

/**
 * will verify the source from the given path
 */
bool verify(const std::string& path, SourceVerifierOptions* options);