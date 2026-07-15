// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/TypeLoc.h"
#include "core/diag/Diagnostic.h"
#include "compiler/symres/SymbolRange.h"
#include <vector>
#include <utility>

class ASTNode;

class SymbolResolver;

class SymbolTable;

class Scope;

class VariablesContainer;

/**
 * result can contain things that can be passed to next processor in pipeline
 */
struct SymResSignatureResult {

    std::vector<std::pair<TypealiasStatement*, std::vector<TypeLoc>>> inline_instantiations;

    bool has_errors;

    std::vector<Diag> diagnostics;

};

/**
 * symbol resolve signature and get back a result
 * range is used to populate the local table with file-private symbols
 */
SymResSignatureResult sym_res_signature(SymbolResolver& resolver, Scope* scope, const SymbolRange& range);

/**
 * must run after link signature
 */
void sym_res_after_signature(SymbolResolver& resolver, Scope* scope);

class TopLevelLinkSignature;

SymbolResolver* SymResLinkSignaturegetSymbolResolver(TopLevelLinkSignature* visitor);

SymbolTable* SymResLinkSignaturegetSymbolTable(TopLevelLinkSignature* visitor);

void SymResLinkSignaturevisitNode(TopLevelLinkSignature* visitor, ASTNode* node);

void SymResLinkSignaturevisitValue(TopLevelLinkSignature* visitor, Value* value);

void SymResLinkSignaturevisitEmbeddedNode(TopLevelLinkSignature* visitor, EmbeddedNode* node);

void SymResLinkSignaturevisitEmbeddedValue(TopLevelLinkSignature* visitor, EmbeddedValue* value);