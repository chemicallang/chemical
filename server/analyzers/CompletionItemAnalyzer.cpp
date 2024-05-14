// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "CompletionItemAnalyzer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include <unordered_set>
#include "cst/base/CompoundCSTToken.h"
#include "cst/utils/CSTUtils.h"

#define DEBUG_COMPLETION false

void CompletionItemAnalyzer::put(const std::string &label, lsCompletionItemKind kind) {
    items.emplace_back(label, kind);
}

bool CompletionItemAnalyzer::is_ahead(Position &position) const {
    return position.line > caret_position.first ||
           (position.line == caret_position.first && position.character > caret_position.second);
}

bool CompletionItemAnalyzer::is_ahead(LexToken *token) const {
    return is_ahead(token->position);
}

bool CompletionItemAnalyzer::is_caret_inside(CSTToken *token) {
    return !is_ahead(token->start_token()->position) && is_ahead(token->end_token()->position);
}

CompoundCSTToken *CompletionItemAnalyzer::child_container(CompoundCSTToken *compound) {
    for (auto &token: compound->tokens) {
        if (token->compound() && is_caret_inside(token.get())) {
            return (CompoundCSTToken *) token.get();
        }
    }
    return nullptr;
}

CompoundCSTToken *CompletionItemAnalyzer::direct_parent(std::vector<std::unique_ptr<CSTToken>> &tokens) {
    CompoundCSTToken *child;
    CompoundCSTToken *nested;
    for (auto &token: tokens) {
        if (token->compound() && is_caret_inside(token.get())) {
            child = (CompoundCSTToken *) token.get();
            while (true) {
                nested = child_container(child);
                if (nested == nullptr) {
                    return child;
                } else {
                    child = nested;
                }
            }
        }
    }
    return nullptr;
}

int CompletionItemAnalyzer::token_before_caret(std::vector<std::unique_ptr<CSTToken>> &tokens) {
    int i = 0;
    while (i < tokens.size()) {
        if (is_caret_behind(tokens[i]->start_token())) {
            return i - 1;
        }
        i++;
    }
    return -2;
}

void
CompletionItemAnalyzer::visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned int start, unsigned int end) {
    CSTToken *token;
    while (start < end) {
        token = tokens[start].get();
        if (is_ahead(token->start_token()->position)) {
            break;
        } else {
            token->accept(this);
        }
        start++;
    }
}

void CompletionItemAnalyzer::visitBody(CompoundCSTToken *bodyCst) {
    if (is_caret_inside(bodyCst)) {
        visit(bodyCst->tokens);
    }
}

void CompletionItemAnalyzer::visitVarInit(CompoundCSTToken *varInit) {
    put(str_token(varInit->tokens[1].get()), lsCompletionItemKind::Variable);
    if (3 < varInit->tokens.size() && is_caret_inside(varInit->tokens[3].get())) {
        varInit->tokens[3]->accept(this);
    }
}

void CompletionItemAnalyzer::visitAssignment(CompoundCSTToken *cst) {
    if (is_caret_inside(cst->tokens[2].get())) {
        cst->tokens[2]->accept(this);
    }
}

void CompletionItemAnalyzer::visitFunction(CompoundCSTToken *function) {
    function->tokens[function->tokens.size() - 1]->accept(this);
};

void CompletionItemAnalyzer::visitEnumDecl(CompoundCSTToken *cst) {
    if (is_caret_ahead(cst->tokens[cst->tokens.size() - 1]->end_token())) {
        put(str_token(cst->tokens[1].get()), lsCompletionItemKind::Enum);
    }
}

void CompletionItemAnalyzer::visitStructDef(CompoundCSTToken *cst) {
    auto has_override = is_char_op(cst->tokens[3].get(), ':');
    auto l_brace = has_override ? 4 : 2;
    if (is_caret_ahead(cst->tokens[l_brace]->end_token())) {
        put(str_token(cst->tokens[1].get()), lsCompletionItemKind::Struct);
        if (is_caret_behind(cst->tokens[cst->tokens.size() - 1]->end_token())) {
            visit(cst->tokens, l_brace + 1);
        }
    }
}

void CompletionItemAnalyzer::visitInterface(CompoundCSTToken *cst) {
    if (is_caret_ahead(cst->tokens[2]->start_token())) {
        put(str_token(cst->tokens[1].get()), lsCompletionItemKind::Interface);
        if (is_caret_behind(cst->tokens[cst->tokens.size() - 1]->end_token())) {
            visit(cst->tokens, 3);
        }
    }
}

void CompletionItemAnalyzer::visitImpl(CompoundCSTToken *cst) {
    if (is_caret_ahead(cst->tokens[2]->start_token()) &&
        is_caret_behind(cst->tokens[cst->tokens.size() - 1]->end_token())) {
        visit(cst->tokens, 3);
    }
}

void CompletionItemAnalyzer::visitIf(CompoundCSTToken *ifCst) {
    visit(ifCst->tokens);
};

void CompletionItemAnalyzer::visitWhile(CompoundCSTToken *whileCst) {
    visit(whileCst->tokens);
};

void CompletionItemAnalyzer::visitDoWhile(CompoundCSTToken *doWhileCst) {
    visit(doWhileCst->tokens);
};

void CompletionItemAnalyzer::visitForLoop(CompoundCSTToken *forLoop) {
    if (is_caret_inside(forLoop->tokens[8].get())) {
        forLoop->tokens[2]->accept(this);
        forLoop->tokens[8]->accept(this);
    }
};

void CompletionItemAnalyzer::visitLambda(CompoundCSTToken *cst) {
    cst->tokens[cst->tokens.size() - 1]->accept(this);
}

void CompletionItemAnalyzer::visitStructValue(CompoundCSTToken *cst) {
    if (is_caret_ahead(cst->tokens[2]->start_token()) &&
        is_caret_behind(cst->tokens[cst->tokens.size() - 1]->end_token())) {
        visit(cst->tokens, 3, cst->tokens.size());
    }
}

void CompletionItemAnalyzer::visitArrayValue(CompoundCSTToken *arrayValue) {
    if (is_caret_inside(arrayValue)) {
        visit(arrayValue->tokens);
    }
}

void CompletionItemAnalyzer::visitSwitch(CompoundCSTToken *switchCst) {

};

void CompletionItemAnalyzer::visit(MultilineCommentToken *token) {

};

CompletionList CompletionItemAnalyzer::analyze(std::vector<std::unique_ptr<CSTToken>> &tokens) {
    auto parent = direct_parent(tokens);
    if (parent == nullptr) {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
        std::cout << "Couldn't find direct parent" << std::endl;
#endif
        visit(tokens);
    } else {
        auto before_index = token_before_caret(parent->tokens);
        if (before_index >= 0) {
            auto token = parent->tokens[before_index].get();
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
            std::cout << "token before index : " + token->representation() << " type " << token->type_string() << std::endl;
#endif
            if(token->type() == LexTokenType::CompAccessChain) {
                auto& chain_tokens = ((CompoundCSTToken*) token)->tokens;
                if(!chain_tokens.empty() && is_char_op(chain_tokens[chain_tokens.size() - 1].get(), '.')) {
                    std::cout << "[Unimplemented] member access into access chain : " + token->type_string() << std::endl;
                }
            }
            visit(tokens);
        } else {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
            std::cout << "no token before the caret position" << std::endl;
#endif
            visit(tokens);
        }
    }
//#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
//    for(const auto & item : items) {
//        std::cout << item.label << std::endl;
//    }
//#endif
    return CompletionList{false, std::move(items)};
}