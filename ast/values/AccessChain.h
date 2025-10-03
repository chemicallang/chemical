// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/ASTNode.h"
#include "ast/base/ChainValue.h"
#include <unordered_map>

struct AccessChainAttributes {

    bool is_moved;

};

/**
 * access chain represents a way to access things in programming language for example
 * x.y.z is an access chain, z member of the y of the x is being accessed
 * x.y.z() where z is a function call, z function is assumed to be present in y and y in z
 * z.y.z[0] similarly z is an index operator here
 */
class AccessChain : public ChainValue {
public:

    std::vector<ChainValue*> values;
    AccessChainAttributes attrs;

    /**
     * constructor
     */
    constexpr AccessChain(
        SourceLocation location
    ) : ChainValue(ValueKind::AccessChain, location), attrs(false) {

    }

    /**
     * constructor
     */
    constexpr AccessChain(
            BaseType* chain_type,
            SourceLocation location
    ) : ChainValue(ValueKind::AccessChain, chain_type, location), attrs(false) {

    }

    /**
     * constructor
     */
    constexpr AccessChain(
        std::vector<ChainValue*> values,
        SourceLocation location
    ) : ChainValue(ValueKind::AccessChain, location), values(std::move(values)), attrs(false) {

    }

    /**
     * constructor
     */
    constexpr AccessChain(
            std::vector<ChainValue*> values,
            BaseType* chain_type,
            SourceLocation location
    ) : ChainValue(ValueKind::AccessChain, chain_type, location), values(std::move(values)), attrs(false) {

    }

    inline bool is_moved() {
        return attrs.is_moved;
    }

    inline void set_is_moved(bool is_moved) {
        attrs.is_moved = is_moved;
    }

    /**
     * will call relink_parent on values starting from second value
     */
    void relink_parent();

    AccessChain *copy(ASTAllocator& allocator) final;

    bool primitive() final;

    bool compile_time_computable() final;

    uint64_t byte_size(bool is64Bit) final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) final;

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

    unsigned int store_in_struct(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type* allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType* expected_type
    ) final;

    unsigned int store_in_array(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type *allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType *expected_type
    ) final;

#endif

    Value *parent(InterpretScope &scope);

    inline Value* parent_value(InterpretScope &scope);

    // set value of this access chain
    void set_value(InterpretScope &scope, Value *rawValue, Operation op, SourceLocation location);

    Value *pointer(InterpretScope &scope);

    Value* evaluated_value(InterpretScope &scope);

    inline std::string chain_representation() {
        return Value::representation();
    }

    ASTNode *linked_node() final;

};

void copy_from(ASTAllocator& allocator, std::vector<ChainValue*>& destination, std::vector<ChainValue*>& source, unsigned from);

Value* evaluate_from(std::vector<ChainValue*>& values, InterpretScope& scope, Value* evaluated, unsigned i);

#ifdef COMPILER_BUILD

/**
 * this creates a GEP (get element ptr instruction), for chain values, the pointer is the parent value
 */
llvm::Value* create_gep(Codegen &gen, std::vector<ChainValue*>& values, unsigned index, llvm::Value* pointer, std::vector<llvm::Value*>& idxList);

#endif