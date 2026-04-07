// Copyright (c) Chemical Language Foundation 2025.

#include "FormatterAnalyzer.h"
#include <string>

std::vector<lsp::TextEdit> FormatterAnalyzer::format(const std::vector<Token>& tokens, const std::string& source) {
    if (tokens.empty()) return {};

    formatted.clear();
    indentLevel = 0;
    atStartOfLine = true;
    pendingSpace = false;
    uint32_t prevLine = 0;
    bool hasPrevToken = false;

    // Skip initial whitespace/newlines
    size_t startIndex = 0;
    while(startIndex < tokens.size() && (tokens[startIndex].type == TokenType::Whitespace || tokens[startIndex].type == TokenType::NewLine)) {
        startIndex++;
    }

    for (size_t i = startIndex; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        
        if (token.type == TokenType::EndOfFile) break;

        // Skip original whitespace and newlines, we generate our own
        if (token.type == TokenType::Whitespace || token.type == TokenType::NewLine) {
            continue;
        }

        // Handle indentation and newlines for Braces
        if (token.type == TokenType::RBrace) {
            indentLevel--;
            if (!atStartOfLine) appendNewline();
            appendIndent();
        } else if (atStartOfLine) {
            appendIndent();
        } else {
            // Check if we should insert a newline based on original source lines
            if (hasPrevToken && token.position.line > prevLine) {
                appendNewline();
                appendIndent();
            }
        }

        // Handle spacing before token for binary operators and other cases
        if (!atStartOfLine) {
            bool needsSpaceBefore = false;
            switch(token.type) {
                case TokenType::VarKw:
                case TokenType::ConstKw:
                case TokenType::FuncKw:
                case TokenType::StructKw:
                case TokenType::NamespaceKw:
                case TokenType::EnumKw:
                case TokenType::IfKw:
                case TokenType::WhileKw:
                case TokenType::ForKw:
                case TokenType::ReturnKw:
                case TokenType::ImportKw:
                case TokenType::ExportKw:
                case TokenType::PlusSym:
                case TokenType::MinusSym:
                case TokenType::MultiplySym:
                case TokenType::DivideSym:
                case TokenType::EqualSym:
                case TokenType::DoubleEqualSym:
                case TokenType::NotEqualSym:
                case TokenType::LessThanSym:
                case TokenType::LessThanOrEqualSym:
                case TokenType::GreaterThanSym:
                case TokenType::GreaterThanOrEqualSym:
                case TokenType::LogicalAndSym:
                case TokenType::LogicalOrSym:
                case TokenType::LambdaSym:
                case TokenType::LBrace:
                    needsSpaceBefore = true;
                    break;
                default: break;
            }
            
            if (pendingSpace || needsSpaceBefore) {
                formatted += ' ';
            }
        }
        pendingSpace = false;

        // Add the token value
        formatted += std::string(token.value.data(), token.value.size());
        atStartOfLine = false;
        prevLine = token.position.line;
        hasPrevToken = true;

        // Deciding on what follows the token
        switch (token.type) {
            case TokenType::LBrace:
                indentLevel++;
                appendNewline();
                break;
            case TokenType::RBrace:
                if (i + 1 < tokens.size() && tokens[i+1].type != TokenType::EndOfFile) {
                    appendNewline();
                }
                break;
            case TokenType::SemiColonSym:
                if (i + 1 < tokens.size() && tokens[i+1].type != TokenType::EndOfFile) {
                    appendNewline();
                }
                break;
            case TokenType::CommaSym:
                pendingSpace = true;
                break;
            case TokenType::ColonSym:
                // var x: int -> space after, no space before
                pendingSpace = true; 
                break;
            case TokenType::PlusSym:
            case TokenType::MinusSym:
            case TokenType::MultiplySym:
            case TokenType::DivideSym:
            case TokenType::EqualSym:
            case TokenType::DoubleEqualSym:
            case TokenType::NotEqualSym:
            case TokenType::LessThanSym:
            case TokenType::LessThanOrEqualSym:
            case TokenType::GreaterThanSym:
            case TokenType::GreaterThanOrEqualSym:
            case TokenType::LogicalAndSym:
            case TokenType::LogicalOrSym:
            case TokenType::LambdaSym: // =>
                pendingSpace = true;
                break;
            case TokenType::IfKw:
            case TokenType::WhileKw:
            case TokenType::ForKw:
            case TokenType::SwitchKw:
            case TokenType::CatchKw:
                pendingSpace = true; // if (condition)
                break;
            case TokenType::VarKw:
            case TokenType::ConstKw:
            case TokenType::FuncKw:
            case TokenType::StructKw:
            case TokenType::NamespaceKw:
            case TokenType::ReturnKw:
            case TokenType::UsingKw:
            case TokenType::ImportKw:
            case TokenType::ExportKw:
                pendingSpace = true;
                break;
            default:
                break;
        }
        
        // Remove the redundant check at the end since we handle it in the next iteration now
    }

    // Create a single TextEdit for the entire file
    lsp::TextEdit edit;
    edit.newText = formatted;
    
    // Find the end of the file (simplified)
    int lastLine = 0;
    int lastChar = 0;
    for (char c : source) {
        if (c == '\n') {
            lastLine++;
            lastChar = 0;
        } else {
            lastChar++;
        }
    }

    edit.range.start = {0, 0};
    edit.range.end = { (uint32_t)lastLine, (uint32_t)lastChar };

    return {edit};
}

void FormatterAnalyzer::append(std::string_view str) {
    formatted += str;
}

void FormatterAnalyzer::appendSpace() {
    formatted += ' ';
}

void FormatterAnalyzer::appendNewline() {
    formatted += '\n';
    atStartOfLine = true;
    pendingSpace = false;
}

void FormatterAnalyzer::appendIndent() {
    for (int i = 0; i < indentLevel; ++i) {
        formatted += "    "; // 4 spaces
    }
}
