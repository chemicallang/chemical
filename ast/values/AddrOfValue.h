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
    bool is_mutable = false;
    // When evaluated_value() wraps a FunctionCall result, stores the inner
    // evaluated result (the struct temp) so enclosing expressions can
    // destruct it. Set during evaluated_value().
    Value* innerEvaluatedResult = nullptr;

    /**
     * constructor
     */
    constexpr AddrOfValue(
        Value* value,
        bool is_mutable,
        PointerType* ptrType,
        SourceLocation location
    ) : Value(ValueKind::AddrOfValue, ptrType, location), value(value), is_mutable(is_mutable) {

    }

    inline PointerType* getType() {
        return (PointerType*) Value::getType();
    }

    uint64_t byte_size(TargetData& target) final;

    AddrOfValue *copy(ASTAllocator& allocator) final {
        return  new (allocator.allocate<AddrOfValue>()) AddrOfValue(
                value->copy(allocator),
                is_mutable,
                getType()->copy(allocator),
                encoded_location()
        );
    }

    void determine_type();

    Value* evaluated_value(InterpretScope &scope) override;

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