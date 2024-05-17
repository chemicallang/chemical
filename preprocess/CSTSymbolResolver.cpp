// Copyright (c) Qinetik 2024.

#include "lexer/model/tokens/LexToken.h"
#include "CSTSymbolResolver.h"
#include "cst/utils/CSTUtils.h"
#include "cst/values/AccessChainCST.h"
#include "lexer/model/tokens/VariableToken.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"
#include <iostream>

void CSTSymbolResolver::declare(LexToken *token, CSTToken *node) {
    auto &last = current.back();
    auto found = last.find(token->value);
    if (found == last.end()) {
        last[token->value] = node;
    } else {
        error("duplicate symbol being declared " + token->value + " symbol already exists", token);
    }
}

void CSTSymbolResolver::visitCompoundCommon(CompoundCSTToken *compound) {
    ::visit(this, compound->tokens);
}

void CSTSymbolResolver::visitBody(CompoundCSTToken *cst) {
    scope_start();
    ::visit(this, cst->tokens);
    scope_end();
}

void CSTSymbolResolver::visitVarInit(CompoundCSTToken *cst) {
    declare(cst->tokens[1].get(), cst);
    ::visit(this, cst->tokens, 2);
}

void CSTSymbolResolver::visitFunction(CompoundCSTToken *cst) {
    declare(cst->tokens[1].get(), cst);
    scope_start();
    ::visit(this, cst->tokens, 2);
    scope_end();
}

void CSTSymbolResolver::visitEnumDecl(CompoundCSTToken *cst) {
    declare(cst->tokens[1].get(), cst);
    ::visit(this, cst->tokens, 2);
}

void CSTSymbolResolver::visitInterface(CompoundCSTToken *cst) {
    declare(cst->tokens[1].get(), cst);
    scope_start();
    ::visit(this, cst->tokens, 2);
    scope_end();
}

void CSTSymbolResolver::visitStructDef(CompoundCSTToken *cst) {
    declare(cst->tokens[1].get(), cst);
    scope_start();
    ::visit(this, cst->tokens, 2);
    scope_end();
}

void CSTSymbolResolver::visitImpl(CompoundCSTToken *impl) {
    bool has_for = is_keyword(impl->tokens[2].get(), "for");
    scope_start();
    ::visit(this, impl->tokens, has_for ? 4 : 2);
    scope_end();
}

void CSTSymbolResolver::visitAccessChain(AccessChainCST *cst) {
    if(cst->tokens.size() == 1) {
        cst->tokens[0]->accept(this);
    } else {
        cst->tokens[0]->accept(this);
        CSTToken* previous = cst->tokens[0].get();
        CSTToken* current;
        unsigned i = 1;
        while(i < cst->tokens.size()) {
            current = cst->tokens[i].get();
            switch(current->type()) {
                case LexTokenType::CharOperator:
                    continue;
                case LexTokenType::Variable:
                    ((VariableToken*) current)->linked = previous;
                    previous = current;
                    break;
                case LexTokenType::CompFunctionCall:
                    ((VariableToken*) ((CompoundCSTToken*) current)->tokens[0].get())->linked = previous;
                    previous = current;
                    break;
                case LexTokenType::CompIndexOp:
                    ((VariableToken*) ((CompoundCSTToken*) current)->tokens[0].get())->linked = previous;
                    previous = current;
                default:
                    std::cerr << "unknown token found in access chain !!" << std::endl;
            }
            i++;
        }
    }
}

void CSTSymbolResolver::resolve_symbol(VariableToken* token) {
    auto found = find(token->value);
    if(found) {
        token->linked = found;
    } else {
        error("unresolved symbol found '" + token->value + "'", token);
    }
}

void CSTSymbolResolver::visitVariableToken(LexToken *token) {
    resolve_symbol((VariableToken*) token);
}

void CSTSymbolResolver::visitFunctionCall(CompoundCSTToken *cst) {
    resolve_symbol((VariableToken*) cst->tokens[0].get());
}

void CSTSymbolResolver::visitIndexOp(CompoundCSTToken *cst) {
    resolve_symbol((VariableToken*) cst->tokens[0].get());
}

void CSTSymbolResolver::resolve(ImportUnit* unit) {
    for (auto& file : unit->files) {
        ::visit(this, file->tokens);
    }
}