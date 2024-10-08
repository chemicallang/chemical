// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/structures/StructDefinition.h"
#include "StructMemberInitializer.h"
#include "ast/statements/VarInit.h"

class StructValue : public Value {
private:
    ExtendableMembersContainerNode *definition = nullptr;
public:

    BaseType* refType;
    std::unordered_map<std::string, StructMemberInitializer*> values;
    int16_t generic_iteration = 0;
    CSTToken* token;
    ASTNode* parent_node;
    ASTNodeKind linked_kind;
#ifdef COMPILER_BUILD
    llvm::AllocaInst* allocaInst = nullptr;
#endif

//    StructValue(
//            std::unique_ptr<Value> ref,
//            std::unordered_map<std::string, std::unique_ptr<Value>> values,
//            StructDefinition *definition = nullptr
//    );

    StructValue(
            BaseType* refType,
            std::unordered_map<std::string, StructMemberInitializer*> values,
            ExtendableMembersContainerNode *definition,
            CSTToken* token,
            ASTNode* parent
    );

    StructValue(
            BaseType* refType,
            std::unordered_map<std::string, StructMemberInitializer*> values,
            ExtendableMembersContainerNode *definition,
            InterpretScope &scope,
            CSTToken* token,
            ASTNode* parent
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

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) override;

    bool diagnose_missing_members_for_init(ASTDiagnoser& diagnoser);

    Value *call_member(
            InterpretScope &scope,
            const std::string &name,
            std::vector<Value*> &params
    ) override;

    void set_child_value(const std::string &name, Value *value, Operation op) override;

    Value *scope_value(InterpretScope &scope) override;

    void declare_default_values(std::unordered_map<std::string, StructMemberInitializer*> &into, InterpretScope &scope);

    StructValue *copy(ASTAllocator& allocator) override;

    ASTNode* child(const std::string& name) {
        return definition->child(name);
    }

    Value *child(InterpretScope &scope, const std::string &name) override;

    ASTNode *linked_node() override;

    bool is_generic();

    void runtime_name(std::ostream& output);

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
        return (StructDefinition*) definition;
    }

    UnionDef* linked_union() {
        return (UnionDef*) definition;
    }

    const std::string& linked_name() {
        return definition->name;
    }

    bool allows_direct_init();

    bool is_union() {
        return linked_kind == ASTNodeKind::UnionDecl || linked_kind == ASTNodeKind::UnnamedUnion;
    }

    std::vector<BaseType*>& generic_list();

    std::vector<BaseType*> create_generic_list();

#ifdef COMPILER_BUILD

    void initialize_alloca(llvm::Value *inst, Codegen& gen, BaseType* expected_type);

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
            Value *parent,
            llvm::Value *allocated,
            llvm::Type *allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType *expected_type
    ) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

    llvm::Value *llvm_arg_value(Codegen &gen, FunctionCall *call, unsigned int index) override;

    llvm::Value * llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) override;

    llvm::Value *llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    BaseType* create_type(ASTAllocator& allocator) override;

    BaseType* known_type() override;

    uint64_t byte_size(bool is64Bit) override;

//    hybrid_ptr<BaseType> get_base_type() override;

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Struct;
    }

};