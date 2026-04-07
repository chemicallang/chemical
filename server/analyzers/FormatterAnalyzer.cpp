// Copyright (c) Chemical Language Foundation 2025.

#include "FormatterAnalyzer.h"
#include <string>

std::vector<lsp::TextEdit> FormatterAnalyzer::format(const std::vector<Token>& tokens, const std::string& source) {
    if (tokens.empty()) return {};

    formatted.clear();
    indentLevel = 0;
    bracketLevel = 0;
    atStartOfLine = true;
    pendingSpace = false;
    uint32_t prevLine = 0;
    int consecutiveNewlines = 0;
    bool hasPrevToken = false;

    // Skip initial whitespace/newlines
    size_t startIndex = 0;
    while(startIndex < tokens.size() && (tokens[startIndex].type == TokenType::Whitespace || tokens[startIndex].type == TokenType::NewLine)) {
        startIndex++;
    }

    // Initialize prevLine from the first useful token
    if (startIndex < tokens.size()) {
        prevLine = tokens[startIndex].position.line;
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
                // Limit to maximum of 2 newlines (1 empty line)
                uint32_t lineDiff = token.position.line - prevLine;
                if (lineDiff > 2) lineDiff = 2;
                
                for(uint32_t j = 0; j < lineDiff; ++j) {
                    appendNewline();
                }
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
                case TokenType::PublicKw:
                case TokenType::PrivateKw:
                case TokenType::ProtectedKw:
                case TokenType::InternalKw:
                case TokenType::MutKw:
                case TokenType::ComptimeKw:
                case TokenType::DynKw:
                case TokenType::AliasKw:
                case TokenType::TypeKw:
                    needsSpaceBefore = true;
                    break;
                case TokenType::PlusSym:
                case TokenType::MinusSym:
                case TokenType::MultiplySym:
                case TokenType::DivideSym:
                case TokenType::ModSym:
                case TokenType::EqualSym:
                case TokenType::DoubleEqualSym:
                case TokenType::NotEqualSym:
                case TokenType::LessThanSym:
                case TokenType::LessThanOrEqualSym:
                case TokenType::GreaterThanSym:
                case TokenType::GreaterThanOrEqualSym:
                case TokenType::LogicalAndSym:
                case TokenType::LogicalOrSym:
                case TokenType::BitNotSym:
                case TokenType::PipeSym:
                case TokenType::CaretUpSym:
                case TokenType::AmpersandSym:
                case TokenType::LeftShiftSym:
                case TokenType::RightShiftSym:
                case TokenType::LambdaSym:
                case TokenType::LBrace:
                case TokenType::DoubleColonSym:
                case TokenType::QuestionMarkSym:
                    needsSpaceBefore = true;
                    break;
                default: break;
            }
            
            if (pendingSpace || needsSpaceBefore) {
                formatted += ' ';
            }
        }
        pendingSpace = false;

        // Add the token value with reconstruction if needed
        std::string val(token.value.data(), token.value.size());
        switch (token.type) {
            case TokenType::String:
                formatted += "\"" + val + "\"";
                break;
            case TokenType::Char:
                formatted += "'" + val + "'";
                break;
            case TokenType::MultilineString:
                formatted += "\"\"\"" + val + "\"\"\"";
                break;
            case TokenType::BacktickString:
                formatted += "`" + val + "`";
                break;
            case TokenType::SingleLineComment:
                formatted += "//" + val;
                break;
            case TokenType::MultiLineComment:
                formatted += "/*" + val + "*/";
                break;
            default:
                formatted += val;
                break;
        }
        
        atStartOfLine = false;
        prevLine = token.position.line;
        hasPrevToken = true;

        // Deciding on what follows the token
        switch (token.type) {
            case TokenType::SingleLineComment:
                appendNewline();
                break;
            case TokenType::LBrace:
                indentLevel++;
                appendNewline();
                break;
            case TokenType::RBrace:
                if (i + 1 < tokens.size() && tokens[i+1].type != TokenType::EndOfFile) {
                    // Suppress newline for "cuddled" keywords
                    TokenType nextType = tokens[i+1].type;
                    if (nextType != TokenType::ElseKw && 
                        nextType != TokenType::CatchKw && 
                        nextType != TokenType::WhileKw) { // while for do-while
                        appendNewline();
                    } else {
                        pendingSpace = true;
                    }
                }
                break;
            case TokenType::LBracket:
                bracketLevel++;
                break;
            case TokenType::RBracket:
                bracketLevel--;
                break;
            case TokenType::SemiColonSym:
                if (bracketLevel == 0 && i + 1 < tokens.size() && tokens[i+1].type != TokenType::EndOfFile) {
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
            case TokenType::ModSym:
            case TokenType::EqualSym:
            case TokenType::DoubleEqualSym:
            case TokenType::NotEqualSym:
            case TokenType::LessThanSym:
            case TokenType::LessThanOrEqualSym:
            case TokenType::GreaterThanSym:
            case TokenType::GreaterThanOrEqualSym:
            case TokenType::LogicalAndSym:
            case TokenType::LogicalOrSym:
            case TokenType::BitNotSym:
            case TokenType::PipeSym:
            case TokenType::CaretUpSym:
            case TokenType::AmpersandSym:
            case TokenType::LeftShiftSym:
            case TokenType::RightShiftSym:
            case TokenType::LambdaSym: // =>
            case TokenType::DoubleColonSym:
            case TokenType::QuestionMarkSym:
                pendingSpace = true;
                break;
            case TokenType::IfKw:
            case TokenType::WhileKw:
            case TokenType::ForKw:
            case TokenType::LoopKw:
            case TokenType::SwitchKw:
            case TokenType::TryKw:
            case TokenType::CatchKw:
            case TokenType::ThrowKw:
                pendingSpace = true; 
                break;
            case TokenType::VarKw:
            case TokenType::ConstKw:
            case TokenType::FuncKw:
            case TokenType::StructKw:
            case TokenType::UnionKw:
            case TokenType::VariantKw:
            case TokenType::InterfaceKw:
            case TokenType::ImplKw:
            case TokenType::NamespaceKw:
            case TokenType::EnumKw:
            case TokenType::ReturnKw:
            case TokenType::UsingKw:
            case TokenType::ImportKw:
            case TokenType::ExportKw:
            case TokenType::PublicKw:
            case TokenType::PrivateKw:
            case TokenType::ProtectedKw:
            case TokenType::InternalKw:
            case TokenType::MutKw:
            case TokenType::ComptimeKw:
            case TokenType::DynKw:
            case TokenType::AliasKw:
            case TokenType::TypeKw:
            case TokenType::AsKw:
            case TokenType::IsKw:
            case TokenType::InKw:
                pendingSpace = true;
                break;
            case TokenType::Annotation:
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
