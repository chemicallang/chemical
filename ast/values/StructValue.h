// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/VarInit.h"

class StructValue : public Value {
private:
    StructDefinition *definition = nullptr;
public:

    std::unique_ptr<Value> ref;
    std::vector<std::unique_ptr<BaseType>> generic_list;
    std::unordered_map<std::string, std::unique_ptr<Value>> values;
    int16_t generic_iteration = 0;
    CSTToken* token;
#ifdef COMPILER_BUILD
    llvm::AllocaInst* allocaInst;
#endif
    // the type that represents this struct value, cached !
    std::unique_ptr<BaseType> struct_type = nullptr;

//    StructValue(
//            std::unique_ptr<Value> ref,
//            std::unordered_map<std::string, std::unique_ptr<Value>> values,
//            StructDefinition *definition = nullptr
//    );

    StructValue(
            std::unique_ptr<Value> ref,
            std::unordered_map<std::string, std::unique_ptr<Value>> values,
            std::vector<std::unique_ptr<BaseType>> generic_list,
            StructDefinition *definition,
            CSTToken* token
    );

    StructValue(
            std::unique_ptr<Value> ref,
            std::unordered_map<std::string, std::unique_ptr<Value>> values,
            StructDefinition *definition,
            InterpretScope &scope,
            CSTToken* token
    );

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::StructValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool primitive() override;

    bool link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    Value *call_member(
            InterpretScope &scope,
            const std::string &name,
            std::vector<std::unique_ptr<Value>> &params
    ) override;

    void set_child_value(const std::string &name, Value *value, Operation op) override;

    Value *scope_value(InterpretScope &scope) override;

    void declare_default_values(std::unordered_map<std::string, std::unique_ptr<Value>> &into, InterpretScope &scope);

    StructValue *copy() override;

    ASTNode* child(const std::string& name) {
        return definition->child(name);
    }

    Value *child(InterpretScope &scope, const std::string &name) override;

    ASTNode *linked_node() override;

    bool is_generic();

    int16_t get_active_iteration() {
        return definition->active_iteration;
    }

    void set_active_iteration(int16_t itr) {
        definition->active_iteration = itr;
    }

    BaseDefMember* child_member(const std::string& name){
        return definition->child_member(name);
    }

    ExtendableMembersContainerNode* linked_extendable() {
        return definition;
    }

    StructDefinition* linked_struct() {
        return definition;
    }

    const std::string& linked_name() {
        return definition->name;
    }

#ifdef COMPILER_BUILD

    void initialize_alloca(llvm::Value *inst, Codegen& gen);

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

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
            ArrayValue *parent,
            llvm::AllocaInst *ptr,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType* expected_type
    ) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

    llvm::Value *llvm_arg_value(Codegen &gen, FunctionCall *call, unsigned int index) override;

    llvm::Value *llvm_assign_value(Codegen &gen, Value *lhs) override;

    llvm::Value *llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    std::unique_ptr<BaseType> create_type() override;

    BaseType* known_type() override;

    uint64_t byte_size(bool is64Bit) override;

    hybrid_ptr<BaseType> get_base_type() override;

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Struct;
    }

};