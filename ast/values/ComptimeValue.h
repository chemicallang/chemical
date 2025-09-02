// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/InterpretScope.h"

/**
 * comptime value value replaces itself during symbol resolution by evaluating
 * the contained value
 */
class ComptimeValue : public Value {
private:

    /**
     * the actual value
     */
    Value* value;

public:

    /**
     * constructor
     */
    inline ComptimeValue(
        Value* value
    ) : Value(ValueKind::ComptimeValue, value->getType(), value->encoded_location()), value(value) {

    }

    inline Value* getValue() noexcept {
        return value;
    }

    void setValue(Value* newValue) {
        value = newValue;
        setType(newValue->getType());
    }

    Value* evaluate(ASTAllocator& allocator, GlobalInterpretScope* comptime_scope) {
        InterpretScope scope(nullptr, allocator, comptime_scope);
        // replacing
        const auto eval = value->evaluated_value(scope);
        if(!eval) {
            return nullptr;
        }
        // move the allocated values from interpret scope to the allocator
        // so they are destroyed when the allocator is destroyed
        for(const auto val : scope.allocated) {
            allocator.store_ptr(val);
        }
        scope.allocated.clear();
        return eval;
    }

    BaseType* known_type() override {
        return value->known_type();
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &name) override {
        return value->add_child_index(gen, indexes, name);
    }

#endif

};