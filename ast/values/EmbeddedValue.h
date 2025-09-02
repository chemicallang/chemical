// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"

class EmbeddedValue : public Value {
public:

    /**
     * the name corresponds to the hook, for example #html has 'html' as name
     */
    chem::string_view name;

    /**
     * user can store his ast in this pointer
     */
    void* data_ptr;

    /**
     * constructor
     */
    EmbeddedValue(
        chem::string_view name,
        void* data_ptr,
        BaseType* type,
        SourceLocation loc
    ) : Value(ValueKind::EmbeddedValue, type, loc), name(name), data_ptr(data_ptr)
    {

    }

    /**
     * shallow copy is performed
     */
    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<EmbeddedValue>()) EmbeddedValue(
               name,
               data_ptr,
               getType()->copy(allocator),
               encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

#endif



};