// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/AnnotableNode.h"
#include "ordered_map.h"

class BaseDefMember : public ASTNode {
public:

    chem::string_view name;

    constexpr BaseDefMember(
        chem::string_view name,
        ASTNodeKind k,
        ASTNode* parent_node,
        SourceLocation loc
    ) : ASTNode(k, parent_node, loc), name(name) {

    }

    inline const chem::string_view& name_view() const noexcept {
        return name;
    }

    virtual bool get_is_const() = 0;

    virtual BaseDefMember* copy_member(ASTAllocator& allocator) = 0;

    virtual Value* default_value() {
        return nullptr;
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer(Codegen &gen) final;

    llvm::Value* llvm_load(Codegen& gen, SourceLocation location) final;

#endif

};

typedef tsl::ordered_map<std::string, std::unique_ptr<BaseDefMember>> VariablesMembersType;