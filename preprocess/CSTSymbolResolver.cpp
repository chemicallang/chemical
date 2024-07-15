// Copyright (c) Qinetik 2024.

#include "lexer/model/tokens/LexToken.h"
#include "CSTSymbolResolver.h"
#include "cst/utils/CSTUtils.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"
#include "compiler/PrimitiveTypeMap.h"
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

void CSTSymbolResolver::visitAccessChain(CompoundCSTToken *chain) {
    chain->tokens[0]->accept(this);
    if(chain->tokens.size() == 1) return;
    unsigned i = 1;
    // TODO
    throw std::runtime_error("TODO");
    CSTToken* parent;// = ((LexToken*) chain->tokens[0].get())->linked;
    if(!parent) {
        error("unresolved symbol not found '" + chain->tokens[0]->representation(), chain->tokens[0].get());
        return;
    }
    CSTToken* token;
    while(i < chain->tokens.size()) {
        token = chain->tokens[i].get();
        if(token->type() == LexTokenType::Variable) {
            parent = link_child(parent, token);
            // TODO ref token died due to performance fire
            // token->as_ref()->linked = parent;
            throw std::runtime_error("TODO");
        } else if(token->type() == LexTokenType::CompIndexOp) {
            parent = get_linked_from_node(parent);
        } else if(token->type() == LexTokenType::CompFunctionCall) {
            parent = get_linked_from_node(parent);
            if(!parent && i == chain->tokens.size() - 1) {
                // function call is last in chain
                // it probably returns void
                break;
            }
        }
        if (!parent) {
            error("unresolved symbol not found '" + token->representation() + "'", token);
            break;
        }
        i++;
    }
}

void CSTSymbolResolver::link(LexToken* ref, CSTToken* token) {
//    ref->linked = token;
    throw std::runtime_error("TODO");
}

void CSTSymbolResolver::visitVariableToken(LexToken *token) {
    auto found = find(token->value);
    if(found) {
        throw std::runtime_error("TODO");
//        link(token->as_ref(), found);
    } else {
        error("unresolved symbol found '" + token->value + "'", token);
    }
}

void CSTSymbolResolver::visitFunctionCall(CompoundCSTToken *cst) {

}

void CSTSymbolResolver::visitIndexOp(CompoundCSTToken *cst) {

}

void CSTSymbolResolver::visitTypeToken(LexToken *token) {
    auto found = find(token->value);
    if(found) {
        throw std::runtime_error("TODO");
//        link(token->as_ref(), found);
    } else {
        auto prim = TypeMakers::PrimitiveMap.find(token->value);
        if(prim == TypeMakers::PrimitiveMap.end()) {
            error("unresolved symbol found '" + token->value + "'", token);
        }
    }
}

void CSTSymbolResolver::resolve(ImportUnit* unit) {
    unsigned i = 0;
    auto size = unit->files.size();
    while(i < size) {
        // clear the diagnostics before last file
        // so that only last file's diagnostics are considered
        if(i == size - 1) diagnostics.clear();
        ::visit(this, unit->files[i]->tokens);
        i++;
    }
}