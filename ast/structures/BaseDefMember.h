// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AnnotableNode.h"
#include "ordered_map.h"

class BaseDefMember : public AnnotableNode {
public:

    std::string name;

    BaseDefMember(
        std::string name
    );

    virtual bool requires_destructor() = 0;

    virtual bool requires_clear_fn() = 0;

    virtual bool requires_move_fn() = 0;

    virtual bool requires_copy_fn() = 0;

    virtual BaseDefMember* copy_member(ASTAllocator& allocator) = 0;

    virtual Value* default_value() {
        return nullptr;
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_load(Codegen &gen) override;

#endif

};

typedef tsl::ordered_map<std::string, std::unique_ptr<BaseDefMember>> VariablesMembersType;