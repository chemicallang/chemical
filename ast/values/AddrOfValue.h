// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/PointerType.h"

class AddrOfValue : public Value {
public:

    Value* value;
    PointerType _ptr_type;
    bool is_mutable = false;

    /**
     * constructor
     */
    constexpr AddrOfValue(
        Value* value,
        bool is_mutable,
        SourceLocation location
    ) : Value(ValueKind::AddrOfValue, &_ptr_type, location), value(value), is_mutable(is_mutable), _ptr_type(value->getType(), is_mutable) {

    }


    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    AddrOfValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<AddrOfValue>()) AddrOfValue(
                value->copy(allocator),
                is_mutable,
                encoded_location()
        );
    }

    void determine_type();

    BaseType* known_type() final;

    ASTNode *linked_node() final {
        return value->linked_node();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

};