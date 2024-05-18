// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "CompletionItemAnalyzer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include <unordered_set>
#include "cst/base/CompoundCSTToken.h"
#include "cst/utils/CSTUtils.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"
#include "lexer/model/tokens/VariableToken.h"

#define DEBUG_COMPLETION true

void CompletionItemAnalyzer::put(const std::string &label, lsCompletionItemKind kind) {
    list.items.emplace_back(label, kind);
}

bool CompletionItemAnalyzer::is_ahead(Position &position) const {
    return position.line > caret_position.first ||
           (position.line == caret_position.first && position.character > caret_position.second);
}

bool CompletionItemAnalyzer::is_behind(Position& position) const {
    return position.line < caret_position.first ||
           (position.line == caret_position.first && position.character < caret_position.second);
}

bool CompletionItemAnalyzer::is_eq_caret(Position& position) const {
    return position.line == caret_position.first && position.character == caret_position.second;
}

bool CompletionItemAnalyzer::is_eq_caret(LexToken* token) const {
    return is_eq_caret(token->position);
}

bool CompletionItemAnalyzer::is_ahead(LexToken *token) const {
    return is_ahead(token->position);
}

bool CompletionItemAnalyzer::is_caret_inside(CSTToken *token) {
    return is_behind(token->start_token()->position) && !is_behind(token->end_token()->position);
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

CSTToken* last_direct_parent(CSTToken* token) {
    if(token->compound()) {
        auto last = token->as_compound()->tokens[token->as_compound()->tokens.size() - 1].get();
        if(last->compound()) {
            return last_direct_parent(last);
        } else {
            return token;
        }
    } else {
        return token;
    }
}

CSTToken* CompletionItemAnalyzer::token_before_caret(std::vector<std::unique_ptr<CSTToken>> &tokens) {
    int i = 0;
    while (i < tokens.size()) {
        if (is_caret_eq_or_behind(tokens[i]->start_token())) {
            return last_direct_parent(tokens[i - 1].get());
        }
        i++;
    }
    return nullptr;
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
    put(str_token(function->tokens[1].get()), lsCompletionItemKind::Function);
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

void CompletionItemAnalyzer::visitMultilineComment(LexToken *token) {

}

CompletionList CompletionItemAnalyzer::analyze(std::vector<std::unique_ptr<CSTToken>> &tokens) {
    auto chain = chain_before_caret(tokens);
    if(chain) {
        if(handle_chain_before_caret(chain)) {
            return list;
        } else {
            std::cout << "[Unknown] member access into access chain : " + chain->type_string() << std::endl;
        }
    }
    visit(tokens);
//#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
//    for(const auto & item : items) {
//        std::cout << item.label << std::endl;
//    }
//#endif
    return std::move(list);
}

CompoundCSTToken* CompletionItemAnalyzer::chain_before_caret(std::vector<std::unique_ptr<CSTToken>> &tokens) {
    auto parent = direct_parent(tokens);
    if (parent == nullptr) {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
        std::cout << "Couldn't find direct parent" << std::endl;
#endif
        return nullptr;
    } else {
        auto token = token_before_caret(parent->tokens);
        if (token) {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
            std::cout << "token before index : " + token->representation() << " type " << token->type_string() << " parent type " << parent->type_string() << std::endl;
#endif
            if(token->type() == LexTokenType::CompAccessChain) {
                return (CompoundCSTToken*) token;
            }
            return nullptr;
        } else {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
            std::cout << "no token before the caret position" << std::endl;
#endif
            return nullptr;
        }
    }
}

void CompletionItemAnalyzer::put_identifiers(std::vector<std::unique_ptr<CSTToken>>& tokens, unsigned int start) {
    CSTToken* token;
    while(start < tokens.size()) {
        token = tokens[start].get();
        if(token->type() == LexTokenType::Identifier) {
            put(token->as_lex_token()->value, lsCompletionItemKind::EnumMember);
        }
        start++;
    }
}

void collect_struct_members(
        CompletionItemAnalyzer* analyzer,
        std::vector<std::unique_ptr<CSTToken>>& tokens,
        unsigned i
) {
    CSTToken* token;
    while(i < tokens.size()) {
        token = tokens[i].get();
        if(token->is_var_init()) {
            analyzer->put(str_token(((CompoundCSTToken*) token)->tokens[1].get()), lsCompletionItemKind::Variable);
        } else if(token->is_func_decl()) {
            analyzer->put(str_token(((CompoundCSTToken*) token)->tokens[1].get()), lsCompletionItemKind::Function);
        }
        i++;
    }
}

bool put_children(CompletionItemAnalyzer* analyzer, CSTToken* parent) {
    switch(parent->type()) {
        case LexTokenType::CompEnumDecl:
            analyzer->put_identifiers(parent->as_compound()->tokens, 2);
            return true;
        case LexTokenType::CompStructDef:
        case LexTokenType::CompInterface:
            collect_struct_members(
                    analyzer,
                    parent->as_compound()->tokens,
                    (is_char_op(parent->as_compound()->tokens[2].get(), ':')) ? 5 : 3
            );
            return true;
        default:
            return false;
    }
}

bool put_children_of_ref(CompletionItemAnalyzer* analyzer, CSTToken* parent) {
    switch(parent->type()) {
        case LexTokenType::Variable:{
            auto linked = ((VariableToken *) parent)->linked;
            if(linked) {
                put_children(analyzer, linked);
                return true;
            } else {
                return false;
            }
        }
        default:
            return false;
    }
}

bool CompletionItemAnalyzer::handle_chain_before_caret(CompoundCSTToken* chain) {
    if(!chain->tokens.empty() && is_char_op(chain->tokens[chain->tokens.size() - 1].get(), '.')) {
        return put_children_of_ref(this, chain->tokens[chain->tokens.size() - 2].get());
    }
    return false;
}

CompletionList CompletionItemAnalyzer::analyze(ImportUnit* unit) {

    if(unit->files.size() == 1) return analyze(unit->files[0]->tokens);
    if(unit->files.empty()) return list;

    // check is caret position before a chain
    auto chain = chain_before_caret(unit->files[unit->files.size() - 1]->tokens);
    if(chain) {
        if(handle_chain_before_caret(chain)) {
            return list;
        } else {
            std::cout << "[Unknown] member access into access chain : " + chain->type_string() << std::endl;
        }
    }

    auto prev_caret_position = caret_position;
    unsigned i = 0;
    auto size = unit->files.size();
    while(i < size) {
        auto& file = unit->files[i];
        if(i == size - 1) { // last file
            caret_position = prev_caret_position;
        } else {
            if(!file->tokens.empty()) { // not last file
                // set caret position at the end of file, so all tokens are analyzed
                auto& pos = file->tokens[file->tokens.size() - 1]->end_token()->position;
                caret_position = std::pair(pos.line + 2, 0);
            } else {
                i++;
                continue;
            }
        }
        visit(file->tokens);
        i++;
    }
    return list;
}