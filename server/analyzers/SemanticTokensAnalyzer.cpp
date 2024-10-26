// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "SemanticTokensAnalyzer.h"
#include <unordered_set>
#include "cst/base/CSTToken.h"
#include "cst/utils/CSTUtils.h"
#include "ast/base/ASTNode.h"

#define DEBUG false

void SemanticTokensAnalyzer::put(
        unsigned int lineNumber,
        unsigned int lineCharNumber,
        unsigned int length,
        unsigned int tokenType,
        unsigned int tokenModifiers
) {
    tokens.emplace_back(
            lineNumber - prev_token_line_num, (
                    lineNumber == prev_token_line_num ? (
                            // on the same line
                            lineCharNumber - prev_token_char_num
                    ) : (
                            // on a different line
                            lineCharNumber
                    )
            ), length, tokenType, tokenModifiers
    );
    prev_token_char_num = lineCharNumber;
    prev_token_line_num = lineNumber;
}

void SemanticTokensAnalyzer::put(CSTToken *token, unsigned int tokenType, unsigned int tokenModifiers) {
    put(token->lineNumber(), token->lineCharNumber(), token->length(), tokenType, tokenModifiers);
}

void SemanticTokensAnalyzer::visitCommon(CSTToken *token) {
#ifdef DEBUG
    throw std::runtime_error("[SemanticTokensAnalyzer] VISIT_COMMON called ! It shouldn't have when it's overridden");
#endif
}

void SemanticTokensAnalyzer::put_auto(CSTToken* token) {
    switch (token->type()) {
        case LexTokenType::Keyword:
        case LexTokenType::Bool:
        case LexTokenType::Null:
            put(token, SemanticTokenType::ls_keyword);
            return;
        case LexTokenType::CharOperator:
        case LexTokenType::StringOperator:
        case LexTokenType::Operation:
            put(token, SemanticTokenType::ls_operator);
            return;
        case LexTokenType::Char:
        case LexTokenType::String:
            put(token, SemanticTokenType::ls_string);
            return;
        case LexTokenType::Comment:
            put(token, SemanticTokenType::ls_comment);
            return;
        case LexTokenType::Annotation:
            put(token, SemanticTokenType::ls_macro);
            return;
        case LexTokenType::Number:
            put(token, SemanticTokenType::ls_number);
            return;
        case LexTokenType::Variable:
        case LexTokenType::Identifier:
        case LexTokenType::Type:
            if(token->any) {
                const auto linked = token->any->get_ref_linked_node();
                if(linked) {
                    const auto linked_kind = linked->kind();
                    switch(linked_kind) {
                        case ASTNodeKind::FunctionParam:
                        case ASTNodeKind::VariantMemberParam:
                        case ASTNodeKind::ExtensionFuncReceiver:
                            put(token, SemanticTokenType::ls_parameter);
                            return;
                        case ASTNodeKind::GenericTypeParam:
                            put(token, SemanticTokenType::ls_typeParameter);
                            return;
                        case ASTNodeKind::StructDecl:
                        case ASTNodeKind::UnnamedStruct:
                        case ASTNodeKind::UnionDecl:
                        case ASTNodeKind::UnnamedUnion:
                        case ASTNodeKind::VariantDecl:
                            put(token, SemanticTokenType::ls_struct);
                            return;
                        case ASTNodeKind::EnumDecl:
                            put(token, SemanticTokenType::ls_enum);
                            return;
                        case ASTNodeKind::EnumMember:
                            put(token, SemanticTokenType::ls_enumMember);
                            return;
                        case ASTNodeKind::NamespaceDecl:
                            put(token, SemanticTokenType::ls_namespace);
                            return;
                        case ASTNodeKind::InterfaceDecl:
                            put(token, SemanticTokenType::ls_interface);
                            return;
                        case ASTNodeKind::ExtensionFunctionDecl:
                        case ASTNodeKind::FunctionDecl: {
                            const auto parent = linked->parent();
                            if(parent) {
                                const auto parent_kind = parent->kind();
                                if(parent_kind == ASTNodeKind::VariantDecl || parent_kind == ASTNodeKind::StructDecl || parent_kind == ASTNodeKind::UnionDecl) {
                                    put(token, SemanticTokenType::ls_method);
                                    return;
                                }
                            }
                            put(token, SemanticTokenType::ls_function);
                            return;
                        }
                        case ASTNodeKind::VarInitStmt:
                            put(token, SemanticTokenType::ls_variable);
                            return;
                        case ASTNodeKind::TypealiasStmt:
                            put(token, SemanticTokenType::ls_type);
                            return;
                        default:
                            break;
                    }
                }
            }
            if(token->type() == LexTokenType::Type) {
                put(token, SemanticTokenType::ls_type);
            } else {
                put(token, SemanticTokenType::ls_variable);
            }
            return;
        default:
            std::string s;
            token->append_representation(s);
            std::cerr << "[SemanticTokensAnalyzer] : Token with representation " << s
                      << " and type " << token->type_string()
                      << " called put_auto, has been appended as a comment token" << std::endl;
            put(token, SemanticTokenType::ls_comment);
            return;
    }
}

void SemanticTokensAnalyzer::visitLexTokenCommon(CSTToken *token) {
    put_auto(token);
}

void SemanticTokensAnalyzer::visit(std::vector<CSTToken*> &tokens_vec, unsigned start, unsigned till) {
    auto i = start;
    while (i < till) {
        tokens_vec[i]->accept(this);
        i++;
    }
}

void SemanticTokensAnalyzer::visitCompoundCommon(CSTToken* compound) {
    visit(compound->tokens);
}

void SemanticTokensAnalyzer::visitBody(CSTToken* bodyCst) {
    analyze(bodyCst->tokens);
}

void SemanticTokensAnalyzer::visitEnumDecl(CSTToken* enumDecl) {
    const auto i = enum_name_index(enumDecl);
    visit(enumDecl->tokens, 0, i);
    put(enumDecl->tokens[i]->start_token(), SemanticTokenType::ls_enum);
    const auto tokens_size = enumDecl->tokens.size();
    auto j = i + 1;
    while(j < tokens_size) {
        const auto token = enumDecl->tokens[j];
        if(token->is_identifier()) {
            put(token, SemanticTokenType::ls_enumMember);
        } else {
            token->accept(this);
        }
        j++;
    }
}

void SemanticTokensAnalyzer::visitFunction(CSTToken* function) {
    auto i = func_name_index(function);
    visit(function->tokens, 0, i);
    auto& name_token = function->tokens[i];
    put(name_token, SemanticTokenType::ls_function);
    visit(function->tokens, i + 1);
};

void SemanticTokensAnalyzer::visitInterface(CSTToken* cst) {
    const auto name_ind = interface_name_index(cst);
    visit(cst->tokens, 0, name_ind);
    put(cst->tokens[name_ind], SemanticTokenType::ls_interface);
    visit(cst->tokens, name_ind + 1);
}

void SemanticTokensAnalyzer::visitImpl(CSTToken* cst) {
    bool has_for = is_keyword(cst->tokens[2], "for");
    cst->tokens[0]->accept(this);
    put(cst->tokens[1], SemanticTokenType::ls_interface);
    cst->tokens[2]->accept(this);
    if(has_for) {
        put(cst->tokens[3], SemanticTokenType::ls_struct);
        visit(cst->tokens, 4);
    } else {
        visit(cst->tokens, 3);
    }
}

void SemanticTokensAnalyzer::visitStructDef(CSTToken* cst) {
    const auto name_ind = struct_name_index(cst);
    visit(cst->tokens, 0, name_ind);
    put(cst->tokens[name_ind], SemanticTokenType::ls_struct);
    visit(cst->tokens, name_ind + 1);
};

/**
 * comment tokens must be divided for different lines
 * since vs code doesn't yet support a single multiline token
 */
void SemanticTokensAnalyzer::visitMultilineComment(CSTToken *token) {
    auto lineStart = token->position().line;
    auto charStart = token->position().character;
    const auto& val = token->value();
    const auto total_length = val.size();
    unsigned lengthCovered = 0;
    unsigned i = 0;
    while(i < total_length) {
        if(val[i] == '\n') {
            // comment from previous start
            put(lineStart, charStart, i - lengthCovered, SemanticTokenType::ls_comment, 0);
            // update the next token start
            lineStart++;
            charStart = 0;
            lengthCovered = i;
        } else if(val[i] == '\r') {
            // comment from previous start
            put(lineStart, charStart, i - lengthCovered, SemanticTokenType::ls_comment, 0);
            // consume the next line ending
            if(val[i + 1] == '\n') {
                i++;
            }
            // update the next token start
            lineStart++;
            charStart = 0;
            lengthCovered = i;
        }
        i++;
    }
    if(lengthCovered < total_length) {
        put(lineStart, charStart, total_length - lengthCovered, SemanticTokenType::ls_comment, 0);
    }
};