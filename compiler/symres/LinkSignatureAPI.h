// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/TypeLoc.h"
#include <vector>
#include <utility>

class ASTNode;

class SymbolResolver;

class Scope;

class VariablesContainer;

/**
 * result can contain things that can be passed to next processor in pipeline
 */
struct SymResSignatureResult {

    std::vector<std::pair<TypealiasStatement*, std::vector<TypeLoc>>> inline_instantiations;

};

/**
 * symbol resolve signature and get back a result
 */
SymResSignatureResult sym_res_signature(SymbolResolver& resolver, Scope* scope);

/**
 * must run after link signature
 */
void sym_res_after_signature(SymbolResolver& resolver, Scope* scope);

class TopLevelLinkSignature;

SymbolResolver* SymResLinkSignaturegetSymbolResolver(TopLevelLinkSignature* visitor);

void SymResLinkSignaturevisitNode(TopLevelLinkSignature* visitor, ASTNode* node);

void SymResLinkSignaturevisitValue(TopLevelLinkSignature* visitor, Value* value);

void SymResLinkSignaturevisitEmbeddedNode(TopLevelLinkSignature* visitor, EmbeddedNode* node);

void SymResLinkSignaturevisitEmbeddedValue(TopLevelLinkSignature* visitor, EmbeddedValue* value);