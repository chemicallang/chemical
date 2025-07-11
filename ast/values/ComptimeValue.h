// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Value.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/InterpretScope.h"

/**
 * comptime value value replaces itself during symbol resolution by evaluating
 * the contained value
 */
class ComptimeValue : public Value {
public:

    /**
     * the actual value
     */
    Value* value;

    /**
     * constructor
     */
    inline ComptimeValue(
        Value* value
    ) : Value(ValueKind::ComptimeValue, value->encoded_location()), value(value) {

    }

    bool link(SymbolResolver &linker, BaseType *expected_type) override {
        return value->link(linker, expected_type);
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

    BaseType* create_type(ASTAllocator &allocator) override {
        return value->create_type(allocator);
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &name) override {
        return value->add_child_index(gen, indexes, name);
    }

#endif

};