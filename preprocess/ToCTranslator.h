// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "compiler/ASTProcessorOptions.h"
#include "utils/fwd/functional.h"

/**
 * this allows you to control the compilation process
 */
class ToCTranslatorOptions : public ASTProcessorOptions {
public:

    /**
     * the path to the output, this is a path to a directory
     */
    std::string output_path;

    /**
     * is the target triple 64bit
     */
    bool is64Bit;

    /**
     * constructor
     */
    ToCTranslatorOptions(std::string exe_path, std::string output_path, bool is64Bit);

};

class ToCAstVisitor;

bool translate(const std::string& path, ToCTranslatorOptions* options, const std::function<void(ToCAstVisitor*)>& prepare);