// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"

class EmbeddedValue;

class ASTBuilder;

typedef bool(EmbeddedValueSymbolResolveFunc)(SymbolResolver* resolver, EmbeddedValue* value);

typedef Value*(EmbeddedValueReplacementFunc)(ASTBuilder* builder, EmbeddedValue* value);

typedef void(EmbeddedValueTraversalFunc)(EmbeddedValue* value, void* data, bool(*traverse)(void* data, ASTAny* item));

class EmbeddedValue : public Value {
public:

    /**
     * user can store his ast in this pointer
     */
    void* data_ptr;

    /**
     * the symbol resolve function is called by the symbol resolver
     */
    EmbeddedValueSymbolResolveFunc* sym_res_fn;

    /**
     * the replacement function is called to replace it as a value by the backend
     */
    EmbeddedValueReplacementFunc* replacement_fn;

    /**
     * traversal function allows us to traverse this embedded value
     */
    EmbeddedValueTraversalFunc* traversal_fn;

    /**
     * constructor
     */
    EmbeddedValue(
        void* data_ptr,
        BaseType* type,
        EmbeddedValueSymbolResolveFunc* sym_res_fn,
        EmbeddedValueReplacementFunc* replacement_fn,
        EmbeddedValueTraversalFunc* traversal_fn,
        SourceLocation loc
    ) : Value(ValueKind::EmbeddedValue, type, loc), data_ptr(data_ptr), sym_res_fn(sym_res_fn),
        replacement_fn(replacement_fn), traversal_fn(traversal_fn)
    {

    }

    /**
     * shallow copy is performed
     */
    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<EmbeddedValue>()) EmbeddedValue(
               data_ptr,
               getType()->copy(allocator),
               sym_res_fn,
               replacement_fn,
               traversal_fn,
               encoded_location()
        );
    }

    /**
     * create the type
     */
    BaseType* create_type(ASTAllocator &allocator) override {
        return getType()->copy(allocator);
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

#endif



};