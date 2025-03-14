// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class AliasStmt : public ASTNode {
public:

    chem::string_view alias_name;
    Value* value;

    AliasStmt(
        chem::string_view alias_name,
        Value* value,
        ASTNode* parent,
        SourceLocation loc
    ) : ASTNode(ASTNodeKind::AliasStmt, parent, loc), alias_name(alias_name), value(value) {

    }

    ASTNode* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<AliasStmt>()) AliasStmt(
            alias_name, value->copy(allocator), parent(), encoded_location()
        );
    }

    void declare_top_level(SymbolResolver &linker);

    inline void declare_top_level(SymbolResolver &linker, ASTNode *&node_ptr) final {
        declare_top_level(linker);
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {

    }

#endif

};