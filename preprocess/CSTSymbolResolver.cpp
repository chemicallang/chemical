// Copyright (c) Qinetik 2024.

#include "lexer/model/tokens/LexToken.h"
#include "CSTSymbolResolver.h"
#include "cst/utils/CSTUtils.h"
#include "cst/values/AccessChainCST.h"
#include "lexer/model/tokens/VariableToken.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"

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
    ::visit(this, cst->tokens, 2);
}

void CSTSymbolResolver::visitEnumDecl(CompoundCSTToken *cst) {
    declare(cst->tokens[1].get(), cst);
    ::visit(this, cst->tokens, 2);
}

void CSTSymbolResolver::visitInterface(CompoundCSTToken *cst) {
    declare(cst->tokens[1].get(), cst);
    ::visit(this, cst->tokens, 2);
}

void CSTSymbolResolver::visitStructDef(CompoundCSTToken *cst) {
    declare(cst->tokens[1].get(), cst);
    ::visit(this, cst->tokens, 2);
}

void CSTSymbolResolver::visitAccessChain(AccessChainCST *cst) {
    if(cst->tokens.size() == 1) {
        cst->tokens[0]->accept(this);
    } else {
        // TODO implement complete access chain
    }
}

void CSTSymbolResolver::visitVariableToken(LexToken *token) {
    auto found = find(token->value);
    if(found) {
        ((VariableToken*) token)->linked = found;
    }
}

void CSTSymbolResolver::resolve(ImportUnit* unit) {
    for (auto& file : unit->files) {
        ::visit(this, file->tokens);
    }
}