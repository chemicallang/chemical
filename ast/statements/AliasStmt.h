// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class AliasStmt : public ASTNode {
public:

    AccessSpecifier specifier;
    chem::string_view alias_name;
    Value* value;

    AliasStmt(
        AccessSpecifier specifier,
        chem::string_view alias_name,
        Value* value,
        ASTNode* parent,
        SourceLocation loc
    ) : ASTNode(ASTNodeKind::AliasStmt, parent, loc), specifier(specifier), alias_name(alias_name), value(value) {

    }

    ASTNode* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<AliasStmt>()) AliasStmt(
            specifier, alias_name, value->copy(allocator), parent(), encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {

    }

#endif

};