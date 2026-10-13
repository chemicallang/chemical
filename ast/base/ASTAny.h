// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ast_fwd.h"
#include "ASTAnyKind.h"
#include <vector>
#include <memory>
#include <string>
#include "core/source/SourceLocation.h"

class Value;


class InterpretScope;

class CSTToken;

class ASTAny {
public:

    /**
     * get the ast any kind for this ast any type
     */
    virtual ASTAnyKind any_kind() = 0;

    /**
     * linked node is only available in Value and BaseType
     */
    ASTNode* get_ref_linked_node();


    /**
     * get the string representation for this node
     */
    std::string representation();

    virtual ~ASTAny();

};