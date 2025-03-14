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

    bool is_node;

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
        bool is_node,
        SourceLocation location
    ) : ChainValue(ValueKind::AccessChain, location), attrs(is_node, false) {

    }

    /**
     * constructor
     */
    constexpr AccessChain(
        std::vector<ChainValue*> values,
        bool is_node,
        SourceLocation location
    ) : ChainValue(ValueKind::AccessChain, location), values(std::move(values)), attrs(is_node, false) {

    }

    inline bool is_node() {
        return attrs.is_node;
    }

    inline bool is_moved() {
        return attrs.is_moved;
    }

    inline void set_is_node(bool is_node) {
        attrs.is_node = is_node;
    }

    inline void set_is_moved(bool is_moved) {
        attrs.is_moved = is_moved;
    }


    /**
     * simply links the access chain, the end_offset is used to control linking last values of the chain
     * if end_offset is 1, the last value won't be linked, if it's 2, two last values won't be linked
     * end_offset may not be taken into account, if chain has a single value or two must link values
     */
    bool link(
        SymbolResolver &linker,
        BaseType *type,
        Value** value_ptr,
        bool check_validity,
        bool assign
    );

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *type) final {
        return link(linker, type, &value_ptr, true, false);
    }

    inline bool link_assign(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) {
        return link(linker, expected_type, &value_ptr, true, true);
    }

    /**
     * will call relink_parent on values starting from second value
     */
    void relink_parent();

    AccessChain *copy(ASTAllocator& allocator) final;

    bool primitive() final;

    bool compile_time_computable() final;

    void interpret(InterpretScope &scope) final;

    BaseType* create_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    uint64_t byte_size(bool is64Bit) final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value* llvm_value_no_itr(Codegen &gen, BaseType* expected_type);

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