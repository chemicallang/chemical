// Copyright (c) Qinetik 2024.

#include "lexer/model/tokens/LexToken.h"
#include "CSTSymbolResolver.h"
#include "cst/utils/CSTUtils.h"
#include "cst/values/AccessChainCST.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"
#include "compiler/PrimitiveTypeMap.h"
#include <iostream>
#include "lexer/model/tokens/RefToken.h"

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

void CSTSymbolResolver::visitTypealias(CompoundCSTToken *cst) {
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

void CSTSymbolResolver::visitAccessChain(AccessChainCST *chain) {
    chain->tokens[0]->accept(this);
    if(chain->tokens.size() == 1) return;
    unsigned i = 1;
    CSTToken* parent = ((RefToken*) chain->tokens[0].get())->linked;
    if(!parent) {
        error("unresolved symbol not found '" + chain->tokens[0]->representation(), chain->tokens[0].get());
        return;
    }
    CSTToken* token;
    while(i < chain->tokens.size()) {
        token = chain->tokens[i].get();
        // TODO only links if token is a variable, skipped index op and function call
        if(token->type() == LexTokenType::Variable) {
            parent = link_child(parent, token);
            if(!parent) {
                error("unresolved symbol not found '" + token->representation() + "'", token);
                break;
            }
        }
        i++;
    }
}

void CSTSymbolResolver::resolve_symbol(RefToken* token) {
    auto found = find(token->value);
    if(found) {
        token->linked = found;
    } else {
        error("unresolved symbol found '" + token->value + "'", token);
    }
}

void CSTSymbolResolver::visitVariableToken(LexToken *token) {
    resolve_symbol((RefToken*) token);
}

void CSTSymbolResolver::visitFunctionCall(CompoundCSTToken *cst) {

}

void CSTSymbolResolver::visitIndexOp(CompoundCSTToken *cst) {

}

void CSTSymbolResolver::visitTypeToken(LexToken *token) {
    auto found = find(token->value);
    if(found) {
        token->as_ref()->linked = found;
    } else {
        auto prim = TypeMakers::PrimitiveMap.find(token->value);
        if(prim == TypeMakers::PrimitiveMap.end()) {
            error("unresolved symbol found '" + token->value + "'", token);
        }
    }
}

void CSTSymbolResolver::resolve(ImportUnit* unit) {
    for (auto& file : unit->files) {
        ::visit(this, file->tokens);
    }
}