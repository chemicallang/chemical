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

    // we store the reference to container, if the linked container
    // is not a extendable members container which is that it cannot supply
    VariablesContainer* container = nullptr;

    // we only store pointer to definition, if found
    ExtendableMembersContainerNode *definition = nullptr;

public:

    BaseType* refType;
    std::unordered_map<chem::string_view, StructMemberInitializer> values;
    int16_t generic_iteration = 0;
    SourceLocation location;
    ASTNode* parent_node;
#ifdef COMPILER_BUILD
    llvm::AllocaInst* allocaInst = nullptr;
#endif

    StructValue(
            BaseType* refType,
            SourceLocation location,
            ASTNode* parent
    ) : refType(refType), definition(nullptr), container(nullptr), location(location), parent_node(parent)
    {

    }

    StructValue(
            BaseType* refType,
            ExtendableMembersContainerNode *definition,
            VariablesContainer* container,
            SourceLocation location,
            ASTNode* parent
    ) : refType(refType), definition(definition), container(container), location(location), parent_node(parent)
    {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::StructValue;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    inline VariablesContainer* variables() {
        return container;
    }

    bool primitive() final;

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    bool diagnose_missing_members_for_init(ASTDiagnoser& diagnoser);

    Value *call_member(
            InterpretScope &scope,
            const chem::string_view &name,
            std::vector<Value*> &params
    ) final;

    void set_child_value(InterpretScope& scope, const chem::string_view &name, Value *value, Operation op);

    StructValue* initialized_value(InterpretScope& scope);

    Value *evaluated_value(InterpretScope &scope) final;

    void declare_default_values(std::unordered_map<chem::string_view, StructMemberInitializer> &into, InterpretScope &scope);

    StructValue *copy(ASTAllocator& allocator) final;

    ASTNode* child(const chem::string_view& name) {
        if(definition) {
            return definition->child(name);
        } else {
            auto& variables = container->variables;
            auto found = variables.find(name);
            return found == variables.end() ? nullptr : found->second;
        }
    }

    Value *child(InterpretScope &scope, const chem::string_view &name) final;

    ASTNode *linked_node() final;

    bool is_generic();

    void runtime_name(std::ostream& output);

    std::string runtime_name_str();

    int16_t get_active_iteration() {
        return definition->active_iteration;
    }

    void set_active_iteration(int16_t itr) {
        definition->active_iteration = itr;
    }

    BaseDefMember* child_member(const chem::string_view& name){
        return container->child_member(name);
    }

    ExtendableMembersContainerNode* linked_extendable() {
        return definition;
    }

    StructDefinition* linked_struct() {
        return (StructDefinition*) definition;
    }

    const chem::string_view& linked_name_view();

    bool allows_direct_init();

    bool is_union() {
        if(definition) {
            const auto k = definition->kind();
            return k == ASTNodeKind::UnionDecl || k == ASTNodeKind::UnnamedUnion;
        } else {
            return refType->kind() == BaseTypeKind::Union;
        }
    }

    std::vector<BaseType*>& generic_list();

    std::vector<BaseType*> create_generic_list();

#ifdef COMPILER_BUILD

    void initialize_alloca(llvm::Value *inst, Codegen& gen, BaseType* expected_type);

    llvm::AllocaInst *llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) final;

    llvm::Value *llvm_pointer(Codegen &gen) final;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) final;

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

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Value *llvm_arg_value(Codegen &gen, BaseType* expected_type) final;

    void llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) final;

    llvm::Value *llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

    BaseType* create_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    uint64_t byte_size(bool is64Bit) final;

//    hybrid_ptr<BaseType> get_base_type() final;

};