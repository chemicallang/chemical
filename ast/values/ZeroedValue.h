// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"

class ZeroedValue : public Value {
public:

    bool is_unsafe = false;

    SourceLocation type_location;

    ZeroedValue(
            TypeLoc type_loc,
            SourceLocation location,
            bool is_unsafe = false
    ) : Value(ValueKind::ZeroedValue, (BaseType*) type_loc.getType(), location), is_unsafe(is_unsafe), type_location(type_loc.getLocation()) {

    }

    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<ZeroedValue>()) ZeroedValue(
            { getType()->copy(allocator), type_location }, encoded_location(), is_unsafe
        );
    }

#ifdef COMPILER_BUILD

    llvm::Value* llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type) override;

    llvm::Value* llvm_arg_value(Codegen &gen, BaseType *expected_type) override;

    llvm::Value* llvm_ret_value(Codegen &gen, Value *returnValue) override;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) override;

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) override;

    unsigned int store_in_struct(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) override;

    unsigned int store_in_array(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) override;

    llvm::Type* llvm_type(Codegen& gen) override;

#endif

};
