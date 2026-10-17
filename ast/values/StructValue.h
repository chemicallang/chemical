// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ordered_map.h"
#include "ast/structures/StructDefinition.h"
#include "StructMemberInitializer.h"
#include "ast/statements/VarInit.h"
#include "compiler/generics/InstantiationsContainer.h"

class NameMangler;

class StructValue : public Value {
private:

    // we store the reference to container, if the linked container
    // is not a extendable members container which is that it cannot supply
    VariablesContainerBase* container = nullptr;

    // we only store pointer to definition, if found
    ExtendableMembersContainerNode *definition = nullptr;
    // Variant member index (-1 if not a variant), stored directly on the struct
    // so that copies automatically carry the index. Previously stored in a global
    // map keyed by pointer, which failed for copies.
    int64_t variant_member_index = -1;

public:

    tsl::ordered_map<chem::string_view, StructMemberInitializer> values;

    StructValue(
        BaseType* refType,
        SourceLocation location
    ) : Value(ValueKind::StructValue, refType, location), definition(nullptr), container(nullptr)
    {

    }

    StructValue(
        BaseType* refType,
        ExtendableMembersContainerNode *definition,
        VariablesContainerBase* container,
        SourceLocation location
    ) : Value(ValueKind::StructValue, refType, location), definition(definition), container(container)
    {

    }

    inline VariablesContainerBase* variables() {
        return container;
    }

    bool primitive() final;

    bool resolve_container(ASTNode* node);

    bool resolve_container(ASTDiagnoser& diagnoser, BaseType* containerType);

    bool ensure_specialized_container(GenericInstantiatorAPI& instantiator, ASTDiagnoser& diagnoser, BaseType* containerType, InstantiationRequirement requirement = InstantiationRequirement::SignatureFinalization);

    inline BaseType* getRefType() {
        return Value::getType();
    }

    inline void setRefType(BaseType* type) {
        Value::setType(type);
    }

    inline TypeLoc getRefTypeLoc() {
        return TypeLoc(Value::getType(), encoded_location());
    }

    inline bool resolve_container(ASTDiagnoser& diagnoser) {
        return resolve_container(diagnoser, getRefType());
    }

    inline bool ensure_specialized_container(GenericInstantiatorAPI& instantiator, ASTDiagnoser& diagnoser, InstantiationRequirement requirement = InstantiationRequirement::SignatureFinalization) {
        return ensure_specialized_container(instantiator, diagnoser, getRefType(), requirement);
    }

    bool diagnose_missing_members_for_init(ASTDiagnoser& diagnoser);

    StructValue* initialized_value(InterpretScope& scope);

    Value *evaluated_value(InterpretScope &scope) final;

    void declare_default_values(tsl::ordered_map<chem::string_view, StructMemberInitializer> &into, InterpretScope &scope);

    StructValue *copy(ASTAllocator& allocator) final;

    std::pair<BaseType*, long> child_type_w_index(const chem::string_view& name) {
        if(definition) {
            return definition->variable_type_w_index(name);
        } else {
            return container->variable_type_w_index_no_inherited(name);
        }
    }

    ASTNode* child(const chem::string_view& name) {
        if(definition) {
            return definition->ASTNode::child(name);
        } else {
            // TODO: we used to use direct_child, however container now is container base
            return container->any_child(name);
        }
    }

    ASTNode* linked_member_or_struct_of(const chem::string_view& name) {
        if(definition) {
            return definition->child_member_or_inherited_struct(name);
        } else {
            return container->any_child_variable(name);
        }
    }

    BaseDefMember* direct_variable(const chem::string_view& name) {
        if(definition) {
            return definition->direct_variable(name);
        } else {
            return container->any_child_variable(name);
        }
    }

    Value *child(InterpretScope &scope, const chem::string_view &name) final;

    ASTNode *linked_node() final;

    BaseDefMember* child_member(const chem::string_view& name){
        return container->child_member(name);
    }

    ExtendableMembersContainerNode* linked_extendable() {
        return definition;
    }

    void set_variant_member_index(int64_t idx) {
        variant_member_index = idx;
    }

    int64_t get_variant_member_index() const {
        return variant_member_index;
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
            return getRefType()->isUnionType();
        }
    }

    std::vector<TypeLoc>& generic_list();

    std::vector<TypeLoc> create_generic_list();


    uint64_t byte_size(const TargetData& target) final {
        return definition->byte_size(target);
    }

};