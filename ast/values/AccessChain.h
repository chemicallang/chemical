// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/ASTNode.h"
#include "ast/base/ChainValue.h"
#include <unordered_map>

/**
 * access chain represents a way to access things in programming language for example
 * x.y.z is an access chain, z member of the y of the x is being accessed
 * x.y.z() where z is a function call, z function is assumed to be present in y and y in z
 * z.y.z[0] similarly z is an index operator here
 */
class AccessChain : public ASTNode, public ChainValue {
public:

    std::vector<ChainValue*> values;
    ASTNode* parent_node;
    bool is_node;
    bool is_moved = false;
    SourceLocation location;

    AccessChain(ASTNode* parent_node, bool is_node, SourceLocation location);

    AccessChain(std::vector<ChainValue*> values, ASTNode* parent_node, bool is_node, SourceLocation location);

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::AccessChain;
    }

    ValueKind val_kind() final {
        return ValueKind::AccessChain;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
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

    void declare_and_link(SymbolResolver &linker) final {
        link(linker, (BaseType*) nullptr, nullptr, 0, true, false);
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

    BaseType* create_value_type(ASTAllocator& allocator) final;

//    hybrid_ptr<BaseType> get_base_type() final;

    BaseType* known_type() final;

//    hybrid_ptr<BaseType> get_value_type() final;

    uint64_t byte_size(bool is64Bit) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Value* llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type, llvm::Value** parent_pointer);

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) final;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) final;

    llvm::FunctionType *llvm_func_type(Codegen &gen) final;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) final;

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

    AccessChain *as_access_chain() final {
        return this;
    }

    void evaluate_children(InterpretScope &scope) final;

    Value *parent(InterpretScope &scope);

    inline Value* parent_value(InterpretScope &scope);

    void set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) final;

    Value *pointer(InterpretScope &scope);

    Value *scope_value(InterpretScope &scope) final;

    Value* evaluated_value(InterpretScope &scope);

    /**
     * for every chain value that is connected to a generic struct value, we get the iteration
     * from the struct value and set it to corresponding struct definition
     * @param active_iterations a map is given so that previous iterations can be saved to this map
     */
    void set_generic_iterations(ASTAllocator& allocator, std::unordered_map<uint16_t, int16_t>& active_iterations);

    /**
     * set the active iterations from a saved map
     */
    void restore_active_iterations(std::unordered_map<uint16_t, int16_t>& restore);

    inline std::string chain_representation() {
        return Value::representation();
    }

    ASTNode *linked_node() final;

    [[nodiscard]]
    ValueType value_type() const final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};

ChainValue* get_grandpa_value(std::vector<ChainValue*> &chain_values, unsigned int index);

std::pair<StructDefinition*, int16_t> get_grandpa_generic_struct(ASTAllocator& allocator, std::vector<ChainValue*>& chain_values, unsigned int index);

#ifdef COMPILER_BUILD

/**
 * get loaded value at given index, if index is not within a nullptr is returned
 * up until the given index objects are destructed properly
 * the object at supplied index is not destructed because you must use it
 * after that destruction should be done manually
 */
llvm::Value* llvm_load_chain_until(
        Codegen& gen,
        std::vector<ChainValue*>& chain,
        int until,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
);

/**
 * given an access chain and parent value, it get's the next value in chain
 * the parent value is queued for destruction if required
 *
 * the next value is present inside the parent_value, grandpa_value is used to pass
 * self to function calls if required
 *
 * the index of the required value should be supplied
 * the index - 1 is expected to exist
 */
llvm::Value* llvm_next_value(
        Codegen& gen,
        std::vector<ChainValue*> chain,
        unsigned int index,
        llvm::Value* grandpa_value,
        llvm::Value* parent_value,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
);

/**
 * this creates a GEP (get element ptr instruction), for chain values, the pointer is the parent value
 */
llvm::Value* create_gep(Codegen &gen, std::vector<ChainValue*>& values, unsigned index, llvm::Value* pointer, std::vector<llvm::Value*>& idxList);

#endif