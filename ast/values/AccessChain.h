// Copyright (c) Qinetik 2024.

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
    SourceLocation location;
    AccessChainAttributes attrs;

    AccessChain(bool is_node, SourceLocation location);

    AccessChain(std::vector<ChainValue*> values, bool is_node, SourceLocation location);

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

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::AccessChain;
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
        unsigned int end_offset,
        bool check_validity,
        bool assign
    );

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *type) final {
        return link(linker, type, &value_ptr, 0, true, false);
    }

    bool link_assign(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) final {
        return link(linker, expected_type, &value_ptr, 0, true, true);
    }

    void relink_after_generic(SymbolResolver &linker, BaseType *expected_type) final {
        link(linker, (BaseType*) nullptr, nullptr, 0, false, false);
    }

    bool find_link_in_parent(ChainValue *parent, SymbolResolver &resolver, BaseType *expected_type);

    bool link(SymbolResolver &linker, std::vector<ChainValue *> &values, unsigned int index, BaseType *expected_type) final {
        const auto values_size = values.size();
        const auto parent_index = index - 1;
        const auto parent = parent_index < values_size ? values[parent_index] : nullptr;
        if(parent) {
            return find_link_in_parent(parent, linker, expected_type);
        } else {
            return link(linker, (Value*&) values[index], expected_type);
        }
    }

    void fix_generic_iteration(ASTDiagnoser& diagnoser, BaseType* expected_type);

    /**
     * will call relink_parent on values starting from second value
     */
    void relink_parent();

    void accept(Visitor *visitor) final;

    AccessChain *copy(ASTAllocator& allocator) final;

    bool primitive() final;

    bool compile_time_computable() final;

    void interpret(InterpretScope &scope) final;

    BaseType* create_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    uint64_t byte_size(bool is64Bit) final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Value* llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) final;

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) final;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) final;

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

    Value *scope_value(InterpretScope &scope) final;

    Value* evaluated_value(InterpretScope &scope);

    inline std::string chain_representation() {
        return Value::representation();
    }

    ASTNode *linked_node() final;

    [[nodiscard]]
    ValueType value_type() const final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};

void copy_from(ASTAllocator& allocator, std::vector<ChainValue*>& destination, std::vector<ChainValue*>& source, unsigned from);

std::pair<StructDefinition*, int16_t> get_grandpa_generic_struct(ASTAllocator& allocator, ChainValue* parent_val);

Value* evaluate_from(std::vector<ChainValue*>& values, InterpretScope& scope, Value* evaluated, unsigned i);

#ifdef COMPILER_BUILD

/**
 * this creates a GEP (get element ptr instruction), for chain values, the pointer is the parent value
 */
llvm::Value* create_gep(Codegen &gen, std::vector<ChainValue*>& values, unsigned index, llvm::Value* pointer, std::vector<llvm::Value*>& idxList);

#endif