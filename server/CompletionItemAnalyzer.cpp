// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "CompletionItemAnalyzer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include <unordered_set>
#include "cst/structures/BodyCST.h"
#include "cst/statements/VarInitCST.h"
#include "cst/structures/ForLoopCST.h"
#include "cst/structures/WhileCST.h"
#include "cst/structures/DoWhileCST.h"
#include "cst/statements/IfCST.h"

#define DEBUG false

void CompletionItemAnalyzer::put(const std::string &label, lsCompletionItemKind kind) {
    items.emplace_back(label, kind);
}

bool CompletionItemAnalyzer::is_ahead(Position &position) const {
    return position.line > caret_position.first ||
           (position.line == caret_position.first && position.character > caret_position.second);
}

bool CompletionItemAnalyzer::is_caret_inside(CSTToken* token) {
    return is_ahead(token->start_token()->position) && !is_ahead(token->end_token()->position);
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

void CompletionItemAnalyzer::visit(BodyCST *bodyCst) {
    if(is_caret_inside(bodyCst)) {
        visit(bodyCst->tokens);
    }
}

void CompletionItemAnalyzer::visit(VarInitCST *varInit) {
    put(varInit->identifier(), lsCompletionItemKind::Variable);
}

void CompletionItemAnalyzer::visit(FunctionCST *function) {

};

void CompletionItemAnalyzer::visit(IfCST *ifCst) {
    visit(ifCst->tokens);
};

void CompletionItemAnalyzer::visit(WhileCST *whileCst) {
    visit(whileCst->tokens);
};

void CompletionItemAnalyzer::visit(DoWhileCST *doWhileCst) {
    visit(doWhileCst->tokens);
};

void CompletionItemAnalyzer::visit(ForLoopCST *forLoop) {
    if(is_caret_inside(forLoop->tokens[8].get())) {
        forLoop->tokens[2]->accept(this);
        forLoop->tokens[8]->accept(this);
    }
};

void CompletionItemAnalyzer::visit(SwitchCST *switchCst) {

};

void CompletionItemAnalyzer::visit(StructDefCST *structDef) {

};

void CompletionItemAnalyzer::visit(MultilineCommentToken *token) {

};

CompletionList CompletionItemAnalyzer::analyze(std::vector<std::unique_ptr<CSTToken>> &tokens) {
    visit(tokens);
#if defined DEBUG && DEBUG
    for(const auto & item : items) {
            std::cout << item.label << std::endl;
        }
#endif
    return CompletionList{false, std::move(items)};
}