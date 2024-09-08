// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/VarInit.h"

class UnionValue : public Value {
private:
    UnionDef *definition = nullptr;
public:

    std::unique_ptr<Value> ref;
    std::vector<std::unique_ptr<BaseType>> generic_list;
    std::pair<std::string, std::unique_ptr<Value>> value;
    int16_t generic_iteration = 0;
    CSTToken* token;
#ifdef COMPILER_BUILD
    llvm::AllocaInst* allocaInst;
#endif
    // the type that represents this struct value, cached !
    std::unique_ptr<BaseType> struct_type = nullptr;

//    UnionValue(
//            std::unique_ptr<Value> ref,
//            std::pair<std::string, std::unique_ptr<Value>> value,
//            UnionDef *definition = nullptr
//    );

    UnionValue(
            std::unique_ptr<Value> ref,
            std::pair<std::string, std::unique_ptr<Value>> value,
            std::vector<std::unique_ptr<BaseType>> generic_list,
            UnionDef *definition,
            CSTToken* token
    );

    UnionValue(
            std::unique_ptr<Value> ref,
            std::pair<std::string, std::unique_ptr<Value>> value,
            UnionDef *definition,
            InterpretScope &scope,
            CSTToken* token
    );

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::UnionValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool primitive() override;

    bool link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    BaseDefMember* child_member(const std::string& name);

    Value *call_member(
            InterpretScope &scope,
            const std::string &name,
            std::vector<std::unique_ptr<Value>> &params
    ) override;

    void set_child_value(const std::string &name, Value *value, Operation op) override;

    Value *scope_value(InterpretScope &scope) override;

    void declare_default_values(std::unordered_map<std::string, std::unique_ptr<Value>> &into, InterpretScope &scope);

    UnionValue *copy() override;

    Value *child(InterpretScope &scope, const std::string &name) override;

    ASTNode *linked_node() override;

    bool is_generic();

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
        return ValueType::Union;
    }

};