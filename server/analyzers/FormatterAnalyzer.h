// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "lexer/Token.h"
#include "lsp/types.h"
#include <vector>
#include <string>
#include <string_view>

class FormatterAnalyzer {
public:
    /**
     * formats the given tokens and returns a list of text edits
     */
    std::vector<lsp::TextEdit> format(const std::vector<Token>& tokens, const std::string& source);

private:
    std::string formatted;
    int indentLevel = 0;
    int bracketLevel = 0;
    bool atStartOfLine = true;
    bool pendingSpace = false;

    void append(std::string_view str);
    void appendSpace();
    void appendNewline();
    void appendIndent();
    
    // helper to get current line/char from original source if needed, 
    // but for simple whole-file replacement we don't need much.
};
