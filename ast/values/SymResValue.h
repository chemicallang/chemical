// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Value.h"

/**
 * the function called to replace sym res value during symbol resolution
 */
typedef Value*(*SymResValueReplacementFn)(ASTAllocator* allocator, SymbolResolver* resolver, void* data);

/**
 * sym res value replaces itself during symbol resolution
 */
class SymResValue : public Value {
public:

    /**
     * The allocator is stored during parsing to allocate in symbol resolution
     * This is because during parsing allocator is used based on passed parameter
     * Because public nodes should be allocated on job allocator and non public on module allocator
     */
    ASTAllocator* allocator;

    /**
     * this method is called during symbol linkage, so we can get real value, which
     * will replace the current sym res value
     */
    SymResValueReplacementFn fn;

    /**
     * the data pointer is any pointer that is stored for holding information
     * that is needed during symbol resolution, just for information passing between
     * parsing phase and symbol resolution
     */
    void* data_ptr;

    /**
     * constructor
     */
    inline SymResValue(
        ASTAllocator* allocator,
        SymResValueReplacementFn fn,
        void* data_ptr,
        SourceLocation location
    ) : Value(ValueKind::SymResValue, location), allocator(allocator), fn(fn), data_ptr(data_ptr) {

    }

//    Value* copy(ASTAllocator &alloc) override {
//        return new (alloc.allocate<SymResValue>()) SymResValue(
//            this->allocator, fn, data_ptr, location
//        );
//    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override {
        // replacing
        const auto replacement = fn(allocator, &linker, data_ptr);
        if(replacement) {
            value_ptr = replacement;
            return true;
        } else {
            return false;
        }
    }

};