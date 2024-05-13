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

bool CompletionItemAnalyzer::is_caret_inside(CSTToken* token) {
    return !is_ahead(token->start_token()->position) && is_ahead(token->end_token()->position);
}

void CompletionItemAnalyzer::visit(std::vector<std::unique_ptr<CSTToken>> &tokens) {
    for (auto &token: tokens) {
        if (is_ahead(token->start_token()->position)) {
            break;
        } else {
            token->accept(this);
        }
    }
}

void CompletionItemAnalyzer::visitBody(CompoundCSTToken *bodyCst) {
    if(is_caret_inside(bodyCst)) {
        visit(bodyCst->tokens);
    }
}

void CompletionItemAnalyzer::visitVarInit(CompoundCSTToken *varInit) {
    put(str_token(varInit->tokens[1].get()), lsCompletionItemKind::Variable);
}

void CompletionItemAnalyzer::visitFunction(CompoundCSTToken *function) {
    function->tokens[function->tokens.size() - 1]->accept(this);
};

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
    if(is_caret_inside(forLoop->tokens[8].get())) {
        forLoop->tokens[2]->accept(this);
        forLoop->tokens[8]->accept(this);
    }
};

void CompletionItemAnalyzer::visitSwitch(CompoundCSTToken *switchCst) {

};

void CompletionItemAnalyzer::visitStructDef(CompoundCSTToken *structDef) {

};

void CompletionItemAnalyzer::visit(MultilineCommentToken *token) {

};

CompletionList CompletionItemAnalyzer::analyze(std::vector<std::unique_ptr<CSTToken>> &tokens) {
    visit(tokens);
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
    for(const auto & item : items) {
            std::cout << item.label << std::endl;
        }
#endif
    return CompletionList{false, std::move(items)};
}