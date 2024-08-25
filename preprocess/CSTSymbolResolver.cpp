// Copyright (c) Qinetik 2024.

#include "cst/base/CSTToken.h"
#include "CSTSymbolResolver.h"
#include "cst/utils/CSTUtils.h"
#include "integration/ide/model/ImportUnit.h"
#include "integration/ide/model/LexResult.h"
#include "compiler/PrimitiveTypeMap.h"
#include <iostream>

void CSTSymbolResolver::declare(CSTToken *token, CSTToken *node) {
    auto &last = current.back();
    auto found = last.find(token->value);
    if (found == last.end()) {
        last[token->value] = node;
    } else {
        error("duplicate symbol being declared " + token->value + " symbol already exists", token);
    }
}

void CSTSymbolResolver::visitCompoundCommon(CSTToken* compound) {
    ::visit(this, compound->tokens);
}

void CSTSymbolResolver::visitBody(CSTToken* cst) {
    scope_start();
    ::visit(this, cst->tokens);
    scope_end();
}

void CSTSymbolResolver::visitVarInit(CSTToken* cst) {
    declare(cst->tokens[1], cst);
    ::visit(this, cst->tokens, 2);
}

void CSTSymbolResolver::visitTypealias(CSTToken* cst) {
    declare(cst->tokens[1], cst);
    ::visit(this, cst->tokens, 2);
}

void CSTSymbolResolver::visitFunction(CSTToken* cst) {
    declare(cst->tokens[1], cst);
    scope_start();
    ::visit(this, cst->tokens, 2);
    scope_end();
}

void CSTSymbolResolver::visitEnumDecl(CSTToken* cst) {
    declare(cst->tokens[1], cst);
    ::visit(this, cst->tokens, 2);
}

void CSTSymbolResolver::visitInterface(CSTToken* cst) {
    declare(cst->tokens[1], cst);
    scope_start();
    ::visit(this, cst->tokens, 2);
    scope_end();
}

void CSTSymbolResolver::visitStructDef(CSTToken* cst) {
    declare(cst->tokens[1], cst);
    scope_start();
    ::visit(this, cst->tokens, 2);
    scope_end();
}

void CSTSymbolResolver::visitImpl(CSTToken* impl) {
    bool has_for = is_keyword(impl->tokens[2], "for");
    scope_start();
    ::visit(this, impl->tokens, has_for ? 4 : 2);
    scope_end();
}

void CSTSymbolResolver::visitAccessChain(CSTToken* chain) {
    chain->tokens[0]->accept(this);
    if(chain->tokens.size() == 1) return;
    unsigned i = 1;
    // TODO
    throw std::runtime_error("TODO");
    CSTToken* parent;// = ((CSTToken*) chain->tokens[0].get())->linked;
    if(!parent) {
        error("unresolved symbol not found '" + chain->tokens[0]->representation(), chain->tokens[0]);
        return;
    }
    CSTToken* token;
    while(i < chain->tokens.size()) {
        token = chain->tokens[i];
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

void CSTSymbolResolver::link(CSTToken* ref, CSTToken* token) {
//    ref->linked = token;
    throw std::runtime_error("TODO");
}

void CSTSymbolResolver::visitVariableToken(CSTToken *token) {
    auto found = find(token->value);
    if(found) {
        throw std::runtime_error("TODO");
//        link(token->as_ref(), found);
    } else {
        error("unresolved symbol found '" + token->value + "'", token);
    }
}

void CSTSymbolResolver::visitFunctionCall(CSTToken* cst) {

}

void CSTSymbolResolver::visitIndexOp(CSTToken* cst) {

}

void CSTSymbolResolver::visitTypeToken(CSTToken *token) {
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