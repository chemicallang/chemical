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

void SemanticTokensAnalyzer::put(CSTToken *token, unsigned int tokenType, unsigned int tokenModifiers) {
    tokens.emplace_back(
            token->lineNumber() - prev_token_line_num, (
                    token->lineNumber() == prev_token_line_num ? (
                            // on the same line
                            token->lineCharNumber() - prev_token_char_num
                    ) : (
                            // on a different line
                            token->lineCharNumber()
                    )
            ), token->length(), tokenType, tokenModifiers
    );
    prev_token_char_num = token->lineCharNumber();
    prev_token_line_num = token->lineNumber();
}

void SemanticTokensAnalyzer::visitCommon(CSTToken *token) {
    throw std::runtime_error("[SemanticTokensAnalyzer] VISIT_COMMON called ! It shouldn't have when it's overridden");
}

void SemanticTokensAnalyzer::put_auto(CSTToken* token) {
    switch (token->type()) {
        case LexTokenType::Keyword:
            put(token, SemanticTokenType::ls_keyword);
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

void SemanticTokensAnalyzer::visit(std::vector<CSTToken*> &tokens, unsigned start, unsigned end) {
    unsigned int i = start;
    unsigned int till = end;
    while (i < till) {
        tokens[i]->accept(this);
        i++;
    }
}

void SemanticTokensAnalyzer::visit(std::vector<CSTToken*> &tokens, unsigned start) {
    visit(tokens, start, tokens.size());
}

void SemanticTokensAnalyzer::visitCompoundCommon(CSTToken* compound) {
    visit(compound->tokens);
}

void SemanticTokensAnalyzer::visitBody(CSTToken* bodyCst) {
    analyze(bodyCst->tokens);
}

void SemanticTokensAnalyzer::visitEnumDecl(CSTToken* enumDecl) {
    enumDecl->tokens[0]->accept(this);
    put(enumDecl->tokens[1]->start_token(), SemanticTokenType::ls_enum);
    unsigned i = 2;
    CSTToken* token;
    while(i < enumDecl->tokens.size()) {
        token = enumDecl->tokens[i];
        if(token->is_identifier()) {
            put(token, SemanticTokenType::ls_enumMember);
        } else {
            token->accept(this);
        }
        i++;
    }
}

void SemanticTokensAnalyzer::visitVarInit(CSTToken* varInit) {
    visitCompoundCommon((CSTToken* ) varInit);
}

void SemanticTokensAnalyzer::visitFunction(CSTToken* function) {
    auto i = func_name_index(function);
    unsigned j = 0;
    while(j < i) {
        function->tokens[j]->accept(this);
        j++;
    }
    auto& name_token = function->tokens[i];
    put(name_token, SemanticTokenType::ls_function);
    visit(function->tokens, i + 1);

};

void SemanticTokensAnalyzer::visitIf(CSTToken* ifCst) {
    visitCompoundCommon((CSTToken* ) ifCst);
};

void SemanticTokensAnalyzer::visitWhile(CSTToken* whileCst) {
    visitCompoundCommon((CSTToken* ) whileCst);
};

void SemanticTokensAnalyzer::visitDoWhile(CSTToken* doWhileCst) {
    visitCompoundCommon((CSTToken* ) doWhileCst);
};

void SemanticTokensAnalyzer::visitForLoop(CSTToken* forLoop) {
    visitCompoundCommon((CSTToken* ) forLoop);
};

void SemanticTokensAnalyzer::visitSwitch(CSTToken* switchCst) {
    visitCompoundCommon((CSTToken* ) switchCst);
};

void SemanticTokensAnalyzer::visitInterface(CSTToken* cst) {
    cst->tokens[0]->accept(this);
    put(cst->tokens[1], SemanticTokenType::ls_interface);
    visit(cst->tokens, 2);
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
    cst->tokens[0]->accept(this);
    auto has_specifier = cst->tokens[1]->type() == LexTokenType::Keyword;
    if(has_specifier) {
        cst->tokens[1]->accept(this);
        put(cst->tokens[2], SemanticTokenType::ls_struct);
        visit(cst->tokens, 3);
    } else {
        put(cst->tokens[1], SemanticTokenType::ls_struct);
        visit(cst->tokens, 2);
    }
};

void SemanticTokensAnalyzer::analyze(std::vector<CSTToken*> &cstTokens) {
    for (auto &token: cstTokens) {
        token->accept(this);
    }
}

void SemanticTokensAnalyzer::visitMultilineComment(CSTToken *token) {
    put(token, SemanticTokenType::ls_comment);
};

void SemanticTokensAnalyzer::visitBoolToken(CSTToken *token) {
    put(token, SemanticTokenType::ls_keyword);
};

void SemanticTokensAnalyzer::visitNullToken(CSTToken *token) {
    put(token, SemanticTokenType::ls_keyword);
}

void SemanticTokensAnalyzer::visitCharOperatorToken(CSTToken *token) {
    put(token, SemanticTokenType::ls_operator);
};

void SemanticTokensAnalyzer::visitStringOperatorToken(CSTToken *token) {
    put(token, SemanticTokenType::ls_operator);
}

void SemanticTokensAnalyzer::visitCharToken(CSTToken *token) {
    put(token, SemanticTokenType::ls_string);
};

void SemanticTokensAnalyzer::visitCommentToken(CSTToken *token) {
    put(token, SemanticTokenType::ls_comment);
};

void SemanticTokensAnalyzer::visitMacroToken(CSTToken *token) {
    put(token, SemanticTokenType::ls_macro);
};

void SemanticTokensAnalyzer::visitAnnotationToken(CSTToken *token) {
    put(token, SemanticTokenType::ls_macro);
}

void SemanticTokensAnalyzer::visitNumberToken(NumberToken *token) {
    put((CSTToken *) token, SemanticTokenType::ls_number);
};

void SemanticTokensAnalyzer::visitOperationToken(CSTToken *token) {
    put(token, SemanticTokenType::ls_operator);
};

void SemanticTokensAnalyzer::visitStringToken(CSTToken *token) {
    put(token, SemanticTokenType::ls_string);
};

void SemanticTokensAnalyzer::visitAccessChain(CSTToken *chain) {
    unsigned i = 0;
    const auto size = chain->tokens.size();
    while(i < size) {
        chain->tokens[i]->accept(this);
        i++;
    }
}