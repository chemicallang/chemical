// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AnnotableNode.h"
#include "ordered_map.h"

class BaseDefMember : public AnnotableNode {
public:

    chem::string_view name;

    BaseDefMember(chem::string_view name) : name(name) {

    }

    virtual bool get_is_const() = 0;

    virtual BaseDefMember* copy_member(ASTAllocator& allocator) = 0;

    virtual Value* default_value() {
        return nullptr;
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer(Codegen &gen) final;

    llvm::Value* llvm_load(Codegen &gen) final;

#endif

};

typedef tsl::ordered_map<std::string, std::unique_ptr<BaseDefMember>> VariablesMembersType;