// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/ASTNode.h"
#include "ast/base/ChainValue.h"
#include "TypeLinkedValue.h"
#include <unordered_map>

/**
 * access chain represents a way to access things in programming language for example
 * x.y.z is an access chain, z member of the y of the x is being accessed
 * x.y.z() where z is a function call, z function is assumed to be present in y and y in z
 * z.y.z[0] similarly z is an index operator here
 */
class AccessChain : public ASTNode, public ChainValue, public TypeLinkedValue {
public:

    std::vector<std::unique_ptr<ChainValue>> values;
    ASTNode* parent_node;
    bool is_node;
    bool is_moved = false;
    CSTToken* token;

    AccessChain(ASTNode* parent_node, bool is_node, CSTToken* token) : parent_node(parent_node), is_node(is_node), token(token) {

    }

    AccessChain(std::vector<std::unique_ptr<ChainValue>> values, ASTNode* parent_node, bool is_node, CSTToken* token);

    CSTToken* cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::AccessChain;
    }

    ValueKind val_kind() override {
        return ValueKind::AccessChain;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    bool link(SymbolResolver &linker, BaseType *type, std::unique_ptr<Value>* value_ptr);

    bool link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr, BaseType *type) override;

    bool link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    bool link(SymbolResolver &linker, ReturnStatement *returnStmt) override {
        return TypeLinkedValue::link(linker, returnStmt);
    }

    bool link(SymbolResolver &linker, StructValue *value, const std::string &name) override {
        return TypeLinkedValue::link(linker, value, name);
    }

    bool link(SymbolResolver &linker, FunctionCall *call, unsigned int index) override {
        return TypeLinkedValue::link(linker, call, index);
    }

    void relink_after_generic(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr, BaseType *expected_type) override {
        link(linker, value_ptr, expected_type);
    }

    bool link(SymbolResolver &linker, VarInitStatement *stmnt) override {
        return TypeLinkedValue::link(linker, stmnt);
    }

    bool link(SymbolResolver &linker, ArrayValue *value, unsigned int index) override {
        return TypeLinkedValue::link(linker, value, index);
    }

    bool link(SymbolResolver &linker, AssignStatement *stmnt, bool lhs) override {
        return TypeLinkedValue::link(linker, stmnt, lhs);
    }

    bool find_link_in_parent(ChainValue *parent, SymbolResolver &resolver) override;

    /**
     * will call relink_parent on values starting from second value
     */
    void relink_parent();

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void accept(Visitor *visitor) override;

    AccessChain *copy() override;

    bool primitive() override;

    bool reference() override;

    bool compile_time_computable() override;

    void interpret(InterpretScope &scope) override;

    std::unique_ptr<BaseType> create_type() override;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    uint64_t byte_size(bool is64Bit) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type, llvm::Value** parent_pointer);

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) override;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    unsigned int store_in_struct(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type* allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType* expected_type
    ) override;

    unsigned int store_in_array(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type *allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType *expected_type
    ) override;

#endif

    AccessChain *as_access_chain() override {
        return this;
    }

    void evaluate_children(InterpretScope &scope) override;

    Value *parent(InterpretScope &scope);

    inline hybrid_ptr<Value> parent_value(InterpretScope &scope);

    void set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) override;

    Value *pointer(InterpretScope &scope);

    Value *scope_value(InterpretScope &scope) override;

    hybrid_ptr<Value> evaluated_value(InterpretScope &scope);

    std::unique_ptr<Value> create_evaluated_value(InterpretScope &scope) override;

    /**
     * for every chain value that is connected to a generic struct value, we get the iteration
     * from the struct value and set it to corresponding struct definition
     * @param active_iterations a map is given so that previous iterations can be saved to this map
     */
    void set_generic_iterations(std::unordered_map<uint16_t, int16_t>& active_iterations);

    /**
     * set the active iterations from a saved map
     */
    void restore_active_iterations(std::unordered_map<uint16_t, int16_t>& restore);

    inline std::string chain_representation() {
        return Value::representation();
    }

    ASTNode *linked_node() override;

    [[nodiscard]]
    ValueType value_type() const override;

    [[nodiscard]]
    BaseTypeKind type_kind() const override;

};

ChainValue* get_grandpa_value(std::vector<std::unique_ptr<ChainValue>> &chain_values, unsigned int index);

std::pair<StructDefinition*, int16_t> get_grandpa_generic_struct(std::vector<std::unique_ptr<ChainValue>>& chain_values, unsigned int index);

#ifdef COMPILER_BUILD

/**
 * get loaded value at given index, if index is not within a nullptr is returned
 * up until the given index objects are destructed properly
 * the object at supplied index is not destructed because you must use it
 * after that destruction should be done manually
 */
llvm::Value* llvm_load_chain_until(
        Codegen& gen,
        std::vector<std::unique_ptr<ChainValue>>& chain,
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
        std::vector<std::unique_ptr<ChainValue>> chain,
        unsigned int index,
        llvm::Value* grandpa_value,
        llvm::Value* parent_value,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
);

/**
 * this creates a GEP (get element ptr instruction), for chain values, the pointer is the parent value
 */
llvm::Value* create_gep(Codegen &gen, std::vector<std::unique_ptr<ChainValue>>& values, unsigned index, llvm::Value* pointer, std::vector<llvm::Value*>& idxList);

#endif