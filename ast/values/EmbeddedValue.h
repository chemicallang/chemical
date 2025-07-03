// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

class EmbeddedValue;

typedef bool(EmbeddedValueSymbolResolveFunc)(SymbolResolver* resolver, EmbeddedValue* value);

typedef Value*(EmbeddedValueReplacementFunc)(ASTAllocator* allocator, EmbeddedValue* value);

typedef BaseType*(EmbeddedValueTypeCreationFunc)(ASTAllocator *allocator, EmbeddedValue* value);

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
     * type creation function is used to create a type for this value
     */
    EmbeddedValueTypeCreationFunc* type_creation_fn;

    /**
     * traversal function allows us to traverse this embedded value
     */
    EmbeddedValueTraversalFunc* traversal_fn;

    /**
     * constructor
     */
    EmbeddedValue(
        void* data_ptr,
        EmbeddedValueSymbolResolveFunc* sym_res_fn,
        EmbeddedValueReplacementFunc* replacement_fn,
        EmbeddedValueTypeCreationFunc* type_creation_fn,
        EmbeddedValueTraversalFunc* traversal_fn,
        SourceLocation loc
    ) : Value(ValueKind::EmbeddedValue, loc), data_ptr(data_ptr), sym_res_fn(sym_res_fn), replacement_fn(replacement_fn),
        type_creation_fn(type_creation_fn), traversal_fn(traversal_fn)
    {

    }

    /**
     * shallow copy is performed
     */
    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<EmbeddedValue>()) EmbeddedValue(
               data_ptr,
               sym_res_fn,
               replacement_fn,
               type_creation_fn,
               traversal_fn,
               encoded_location()
        );
    }

    /**
     * link
     */
    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) override {
          return sym_res_fn(&linker, this);
    }

    /**
     * create the type
     */
    BaseType* create_type(ASTAllocator &allocator) override {
        return type_creation_fn(&allocator, this);
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

#endif



};