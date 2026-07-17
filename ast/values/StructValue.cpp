// Copyright (c) Chemical Language Foundation 2025.

#include "StructValue.h"
#include "PointerValue.h"
#include "ArrayValue.h"
#include "ast/types/PointerType.h"

#include "compiler/mangler/NameMangler.h"
#include <memory>
#include "ast/structures/StructMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/utils/ASTUtils.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/types/GenericType.h"
#include "ast/statements/Typealias.h"
#include "ast/types/LinkedType.h"
#include "ast/utils/GenericUtils.h"
#include "StructMemberInitializer.h"
#include "ast/values/DereferenceValue.h"
#include <sstream>
#include <iostream>

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void StructValue::initialize_alloca(llvm::Value *inst, Codegen& gen, BaseType* expected_type) {

    const auto parent_type = llvm_type(gen);

    // default initialize inherited values in struct
    if(definition) {
        unsigned inherited_index = 0;
        for (auto& inh: definition->inherited) {
            const auto node = inh.type->get_direct_linked_canonical_node();
            if (node->kind() == ASTNodeKind::StructDecl) {
                const auto decl = node->as_struct_def_unsafe();
                if (values.find(decl->name_view()) == values.end()) {
                    const auto ptr = gen.builder->CreateGEP(parent_type, inst, {gen.builder->getInt32(inherited_index)});
                    gen.default_initialize_struct(decl, ptr, this);
                }
                // increase the inherited index
                inherited_index++;
            }
        }
    }

    auto current = values.begin();
    while (current != values.end()) {
        auto value = current.value();
        auto& value_ptr = value.value;
        auto variable = child_type_w_index(value.name);
        if (variable.second == -1) {
            gen.error(this) << "couldn't get struct child " << value.name << " in definition with name " << definition->name_view();
        } else {

            const auto pure_type = variable.first->canonical();

            std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
            const auto elem_index = is_union() ? 0 : variable.second;

            auto implicit = variable.first->implicit_constructor_for(value_ptr);
            if(implicit) {
                // replace values that call implicit constructors
                value_ptr = (Value*) call_with_arg(implicit, value_ptr, variable.first, gen.allocator, gen);
            } else {
                const auto type_kind = variable.first->kind();
                if(type_kind == BaseTypeKind::Reference) {
                    // store pointer only, since user want's a reference
                    auto elementPtr = Value::get_element_pointer(gen, parent_type, inst, idx, elem_index);
                    const auto ref_pointer = value_ptr->llvm_pointer(gen);
                    const auto storeInst = gen.builder->CreateStore(ref_pointer, elementPtr);
                    gen.di.instr(storeInst, value_ptr);
                    ++current;
                    continue;
                }
            }

            value_ptr->store_in_struct(gen, this, inst, parent_type, idx, elem_index, pure_type);

        }
        ++current;
    }

    // storing default values for direct variables
    const auto isUnion = is_union();
    for(const auto value : container->variables()) {
        auto found = values.find(value->name);
        if(found == values.end()) {
            const auto defValue = value->default_value();
            if (defValue) {

                auto variable = child_type_w_index(value->name);
                auto value_ptr = defValue;

                auto implicit = variable.first->implicit_constructor_for(value_ptr);
                if(implicit) {
                    // replace values that call implicit constructors
                    value_ptr = (Value*) call_with_arg(implicit, value_ptr, variable.first, gen.allocator, gen);
                }

                std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
                value_ptr->store_in_struct(gen, this, inst, parent_type, idx, is_union() ? 0 : variable.second, variable.first);
            } else if(!isUnion) {
                const auto type = value->known_type();
                const auto node = type->get_direct_linked_canonical_node();
                if (node->kind() == ASTNodeKind::StructDecl) {
                    auto variable = child_type_w_index(value->name);
                    const auto decl = node->as_struct_def_unsafe();
                    const auto ptr = gen.builder->CreateGEP(parent_type, inst, { gen.builder->getInt32(0), gen.builder->getInt32(variable.second) });
                    gen.default_initialize_struct(decl, ptr, this);
                } else {
                    gen.error(this) << "expected a default value for '" << value->name << "'";
                }
            }
        }
    }

}

llvm::AllocaInst *StructValue::llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) {
    auto dyn_obj = (llvm::AllocaInst*) gen.allocate_dyn_obj_based_on_type(expected_type, encoded_location());
    const auto alloca1 = gen.builder->CreateAlloca(llvm_type(gen), nullptr);
    gen.di.instr(alloca1, encoded_location());
    allocaInst = alloca1;
    if(dyn_obj) {
        initialize_alloca(dyn_obj, gen, expected_type);
        return dyn_obj;
    } else {
        initialize_alloca(allocaInst, gen, expected_type);
        return allocaInst;
    }
}

llvm::Value *StructValue::llvm_pointer(Codegen &gen) {
    return allocaInst;
}

unsigned int StructValue::store_in_struct(
        Codegen &gen,
        Value *parent,
        llvm::Value *allocated,
        llvm::Type* allocated_type,
        std::vector<llvm::Value*> idxList,
        unsigned int index,
        BaseType* expected_type
) {
    if (index == -1) {
        gen.error(this) <<
                        "can't store struct value '" << representation() << "' into parent struct value '" << parent->representation() << "' with an unknown index"
                        << " where current definition name '" << definition->name_view() << "'";
        return index + values.size();
    }
    auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    initialize_alloca(elementPtr, gen, expected_type);
    return index + values.size();
}

unsigned int StructValue::store_in_array(
        Codegen &gen,
        Value *parent,
        llvm::Value *allocated,
        llvm::Type *allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType *expected_type
) {
    if (index == -1) {
        gen.error(this)
                << "can't store struct value " << representation() << " array value " << ((Value*) parent)->representation() << " with an unknown index" <<
                " where current definition name " << definition->name_view();
        return index + 1;
    }
    auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    initialize_alloca(elementPtr, gen, expected_type);
    return index + 1;
}

llvm::Value *StructValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    if(gen.current_function == nullptr) {
        std::vector<llvm::Constant*> llvm_vals;

        // handle inherited structs
        if (definition) {
            for (auto& inh : definition->inherited) {
                const auto node = inh.type->get_direct_linked_canonical_node();
                if (node->kind() == ASTNodeKind::StructDecl) {
                    const auto decl = node->as_struct_def_unsafe();
                    const auto found = values.find(decl->name_view());
                    if (found != values.end()) {
                        // user provided a value for the inherited struct
                        const auto final_val = found->second.value->llvm_value(gen, nullptr);
                        if (llvm::isa<llvm::Constant>(final_val)) {
                            llvm_vals.emplace_back((llvm::Constant*) final_val);
                        } else {
                            gen.error("expected value to result in a constant expression", found->second.value);
                        }
                    } else {
                        // default initialize the inherited struct (using recursive default values)
                        llvm_vals.emplace_back(gen.get_default_constant(decl));
                    }
                }
            }
        }

        // handle direct variables
        for (const auto var : container->variables()) {
            const auto found = values.find(var->name);
            Value* value_ptr = nullptr;
            if (found != values.end()) {
                value_ptr = found->second.value;
            } else {
                value_ptr = var->default_value();
            }

            if (value_ptr) {
                const auto final_val = value_ptr->llvm_value(gen, var->known_type());
                if (llvm::isa<llvm::Constant>(final_val)) {
                    llvm_vals.emplace_back((llvm::Constant*) final_val);
                } else {
                    gen.error("expected value to result in a constant expression", value_ptr);
                }
            } else {
                if (is_union()) {
                    // for union we only need one member, if not found we might just put zero for first
                    if (llvm_vals.empty()) {
                         llvm_vals.emplace_back(llvm::ConstantAggregateZero::get(var->llvm_type(gen)));
                    }
                } else {
                    gen.error(this) << "expected a value or a default value for '" << var->name << "' during constant initialization";
                    llvm_vals.emplace_back(llvm::ConstantAggregateZero::get(var->llvm_type(gen)));
                }
            }

            if (is_union()) break; // only first member for union
        }

        return llvm::ConstantStruct::get((llvm::StructType*) llvm_type(gen), llvm_vals);
    } else {
        return llvm_allocate(gen, "", expected_type);
    }
}

void StructValue::llvm_assign_value(Codegen &gen, llvm::Value *storagePtr, Value *lhs, llvm::Value *lhsPtr) {
    const auto known_type = lhs->getType();
    if(known_type->kind() == BaseTypeKind::Dynamic && known_type->linked_node()->as_interface_def()) {
        const auto value = llvm_allocate(gen, "", nullptr);
        gen.assign_store(lhs, storagePtr, this, value, encoded_location());
    } else if(lhs->as_deref_value()) {
        if(!definition->destructor_func() && allows_direct_init()) {
            const auto deref = lhs->as_deref_value();
            const auto deref_type = deref->getValue()->getType();
            if (!deref_type->pure_type(gen.allocator)->is_pointer_or_ref()) {
                gen.error("expected a pointer or a reference", lhs);
                return;
            }
            initialize_alloca(storagePtr, gen, nullptr);
        } else {
            gen.error("definition has either a destructor function or does not allow direct init", lhs);
        }
    } else {
        initialize_alloca(storagePtr, gen, nullptr);
    }
}

llvm::Value *StructValue::llvm_arg_value(Codegen &gen, BaseType* expected_type) {
    return llvm_allocate(gen, "", nullptr);
}

llvm::Value *StructValue::llvm_ret_value(Codegen &gen, Value* returnValue) {
    auto structPassed = gen.current_function->getArg(gen.current_func_type->getStructReturnArgIndex());
    initialize_alloca(structPassed, gen, nullptr);
    return nullptr;
}

llvm::Type *StructValue::llvm_type(Codegen &gen) {
    if(definition) {
        return definition->llvm_type(gen);
    } else {
        const auto refType = getRefType();
        switch(refType->kind()) {
            case BaseTypeKind::Struct:
                return refType->as_struct_type_unsafe()->llvm_type(gen);
            case BaseTypeKind::Union:
                return refType->as_union_type_unsafe()->llvm_type(gen);
            default:
                return refType->linked_node()->llvm_type(gen);
        }
    }
}

bool StructValue::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    if(definition) {
        return definition->add_child_index(gen, indexes, name);
    } else {
        const auto refType = getRefType();
        switch(refType->kind()) {
            case BaseTypeKind::Union:
                return refType->as_union_type_unsafe()->add_child_index(gen, indexes, name);
            case BaseTypeKind::Struct:
                return refType->as_struct_type_unsafe()->add_child_index(gen, indexes, name);
            default:
                return false;
        }
    }
}

#endif

bool StructValue::primitive() {
    return false;
}

bool node_allows_direct_init(ASTNode* node) {
    const auto k = node->kind();
    switch (k) {
        case ASTNodeKind::StructDecl:
            return !node->as_struct_def_unsafe()->has_constructor() || node->as_struct_def_unsafe()->is_direct_init();
        case ASTNodeKind::UnionDecl:
            return !node->as_union_def_unsafe()->has_constructor();
        case ASTNodeKind::UnnamedStruct:
        case ASTNodeKind::UnnamedUnion:
        case ASTNodeKind::StructType:
        case ASTNodeKind::UnionType:
            return true;
        default:
            return false;
    }
}

const chem::string_view& StructValue::linked_name_view() {
    if(definition) {
        return definition->name_view();
    } else {
        const auto refType = getRefType();
        switch(refType->kind()) {
            case BaseTypeKind::Struct:
                return refType->as_struct_type_unsafe()->name;
            case BaseTypeKind::Union:
                return refType->as_union_type_unsafe()->name;
            default:
                return "";
        }
    }
}

bool StructValue::allows_direct_init() {
    if(definition) {
        return node_allows_direct_init(definition);
    } else {
        const auto refType = getRefType();
        switch(refType->kind()) {
            case BaseTypeKind::Struct:
            case BaseTypeKind::Union:
                return true;
            default:
                return node_allows_direct_init(linked_node());
        }
    };
}

std::vector<TypeLoc>& StructValue::generic_list() {
    return ((GenericType*) getRefType())->types;
}

std::vector<TypeLoc> StructValue::create_generic_list() {
    const auto refType = getRefType();
    const auto k = refType->kind();
    if(k == BaseTypeKind::Generic) {
        return ((GenericType*) refType)->types;
    } else {
        return {};
    }
}

bool StructValue::diagnose_missing_members_for_init(ASTDiagnoser& diagnoser) {
    if(is_union()) {
        if (values.size() != 1) {
            auto& diag = diagnoser.error(this);
            diag << "union '";
            if (definition) {
                diag << definition->name_view();
            } else {
                diag << "<unnamed>";
            }
            diag << "' must be initialized with a single member value";
            return false;
        } else {
            return true;
        }
    }
    std::vector<chem::string_view> missing;

    // checking the inherited members
    if(definition) {
        for (auto& mem: definition->inherited) {
            auto& type = *mem.type;
            const auto linked_struct = type.get_direct_linked_struct();
            if (linked_struct) {
                const auto& ref_type_name = mem.ref_type_name();
                auto val = values.find(ref_type_name);
                if (val == values.end()) {
                    if (!linked_struct->getAllMembersDefaultInitialized()) {
                        missing.emplace_back(ref_type_name);
                    }
                }
            }
        }
    }

    // check all variables have been default initialized
    for(const auto mem : container->variables()) {
        if(mem->default_value() == nullptr) {
            auto val = values.find(mem->name);
            if (val == values.end()) {
                const auto kType = mem->known_type();
                const auto kContainer = kType->get_members_container();
                if(kContainer) {
                    if(!kContainer->getAllMembersDefaultInitialized()) {
                        missing.emplace_back(mem->name);
                    }
                } else {
                    missing.emplace_back(mem->name);
                }
            }
        }
    }
    if(!missing.empty()) {
        for (auto& miss: missing) {
            auto& diag = diagnoser.error(this);
            diag << "couldn't find value for member '";
            diag << miss;
            diag << "' for initializing struct '";
            if(definition) {
                diag << definition->name_view() << "'";
            } else {
                diag << "<unnamed>'";
            }
        }
        return true;
    }
    return false;
}

bool StructValue::resolve_container(ASTNode* found) {
    switch (found->kind()) {
        case ASTNodeKind::GenericStructDecl:
            return false;
        case ASTNodeKind::GenericUnionDecl:
            return false;
        case ASTNodeKind::UnnamedUnion:
            container = found->as_unnamed_union_unsafe();
            return true;
        case ASTNodeKind::UnionDecl:
            definition = found->as_union_def_unsafe();
            container = definition;
            return true;
        case ASTNodeKind::UnnamedStruct:
            container = found->as_unnamed_struct_unsafe();
            return true;
        case ASTNodeKind::StructDecl:
            definition = found->as_struct_def_unsafe();
            container = definition;
            return true;
        case ASTNodeKind::StructType:
            container = (StructType*) found;
            return true;
        case ASTNodeKind::UnionType:
            container = (UnionType*) found;
            return true;
        case ASTNodeKind::TypealiasStmt:
            return resolve_container(found->as_typealias_unsafe()->actual_type->get_direct_linked_node());
        default:
            definition = found->as_extendable_member_container();
            container = definition;
            return false;
    }
}

bool StructValue::resolve_container(
        ASTDiagnoser& diagnoser, BaseType* containerType
) {
    switch (containerType->kind()) {
        case BaseTypeKind::Struct:
            container = containerType->as_struct_type_unsafe();
            break;
        case BaseTypeKind::Union:
            container = containerType->as_union_type_unsafe();
            break;
        case BaseTypeKind::Generic: {
            const auto genType = containerType->as_generic_type_unsafe();
            const auto linked = genType->referenced->linked;
            switch (linked->kind()) {
                case ASTNodeKind::GenericStructDecl: {
                    const auto gen_decl = linked->as_gen_struct_def_unsafe();
                    definition = gen_decl->master_impl;
                    container = definition;
                    return true;
                }
                case ASTNodeKind::GenericUnionDecl: {
                    auto gen_args = create_generic_list();
                    const auto gen_decl = linked->as_gen_union_decl_unsafe();
                    definition = gen_decl->master_impl;
                    container = definition;
                    return true;
                }
                case ASTNodeKind::UnionDecl:
                    definition = linked->as_union_def_unsafe();
                    container = definition;
                    return true;
                case ASTNodeKind::StructDecl:
                    definition = linked->as_struct_def_unsafe();
                    container = definition;
                    return true;
                case ASTNodeKind::TypealiasStmt:
                    return resolve_container(diagnoser, linked->as_typealias_unsafe()->actual_type);
                default: {
                    diagnoser.error(this) << "unknown generic declaration that can't be instantiated with generic types";
                    return false;
                }
            }
            return true;
        }
        default: {
            const auto found = containerType->linked_node();
            if (!found) {
                diagnoser.error(this) << "couldn't find struct definition for struct name " << containerType->representation();
                return false;
            }
            switch (found->kind()) {
                case ASTNodeKind::UnnamedUnion:
                    if (values.size() > 1) {
                        diagnoser.error("initializing multiple values inside a union is not allowed", this);
                        return false;
                    }
                    container = found->as_unnamed_union_unsafe();
                    break;
                case ASTNodeKind::UnionDecl:
                    if (values.size() > 1) {
                        diagnoser.error("initializing multiple values inside a union is not allowed", this);
                        return false;
                    }
                    definition = found->as_union_def_unsafe();
                    container = definition;
                    break;
                case ASTNodeKind::UnnamedStruct:
                    container = found->as_unnamed_struct_unsafe();
                    break;
                case ASTNodeKind::StructDecl:
                    definition = found->as_struct_def_unsafe();
                    container = definition;
                    break;
                case ASTNodeKind::StructType:
                    container = (StructType*) found;
                    break;
                case ASTNodeKind::UnionType:
                    container = (UnionType*) found;
                    break;
                case ASTNodeKind::TypealiasStmt:
                    return resolve_container(diagnoser, found->as_typealias_unsafe()->actual_type);
                default:
                    diagnoser.error("unknown struct/union being initialized via struct value", this);
                    definition = found->as_extendable_member_container();
                    container = definition;
                    return false;
            }
        }
    }
    return true;
}

bool StructValue::ensure_specialized_container(GenericInstantiatorAPI& instantiator, ASTDiagnoser& diagnoser, BaseType* containerType, InstantiationRequirement requirement) {
    switch (containerType->kind()) {
        case BaseTypeKind::Generic: {
            const auto genType = containerType->as_generic_type_unsafe();
            const auto linked = genType->referenced->linked;
            switch (linked->kind()) {
                case ASTNodeKind::GenericStructDecl: {
                    const auto gen_decl = linked->as_gen_struct_def_unsafe();
                    const auto instantiatedType = gen_decl->instantiate_type(instantiator, genType->types, encoded_location(), requirement);
                    if(instantiatedType == nullptr) {
                        return false;
                    }
                    definition = instantiatedType;
                    container = definition;
                    return true;
                }
                case ASTNodeKind::GenericUnionDecl: {
                    const auto gen_decl = linked->as_gen_union_decl_unsafe();
                    const auto instantiatedType = gen_decl->instantiate_type(instantiator, genType->types, encoded_location(), requirement);
                    if(instantiatedType == nullptr) {
                        return false;
                    }
                    definition = instantiatedType;
                    container = definition;
                    return true;
                }
                case ASTNodeKind::UnionDecl:
                    definition = linked->as_union_def_unsafe();
                    container = definition;
                    break;
                case ASTNodeKind::StructDecl:
                    definition = linked->as_struct_def_unsafe();
                    container = definition;
                    break;
                case ASTNodeKind::TypealiasStmt:
                    return ensure_specialized_container(instantiator, diagnoser, linked->as_typealias_unsafe()->actual_type, requirement);
                default: {
                    diagnoser.error(this) << "unknown generic declaration that can't be instantiated with generic types";
                    return false;
                }
            }
            return true;
        }
        default:
            return true;
    }
}

ASTNode *StructValue::linked_node() {
    if(definition) {
        return definition;
    } else {
        const auto refType = getRefType();
        return refType ? refType->linked_node() : nullptr;
    }
}

Value *StructValue::call_member(
        InterpretScope &scope,
        const chem::string_view &name,
        std::vector<Value*> &params
) {
    auto fn = definition->member(name);
    if (fn == nullptr) {
        scope.error("couldn't find member function by name " + name.str() + " in a struct by name " + getRefType()->representation(), this);
        return nullptr;
    }
#ifdef DEBUG
    if (!fn->body.has_value()) {
        scope.error("function doesn't have body in a struct " + name.str(), this);
        return nullptr;
    }
#endif
    InterpretScope child(nullptr, scope.allocator, scope.global);
    child.declare("this", this);
    auto value = fn->call(&scope, params, this, &child);
    return value;
}

void StructValue::set_child_value(InterpretScope& scope, const chem::string_view &name, Value *value, Operation op) {
    auto ptr = values.find(name);
    if (ptr == values.end()) {
        // If the child name is not found, insert it rather than silently ignoring.
        // This handles placement new into uninitialized structs/variants where
        // the target struct has empty values but the source provides field data.
        values.emplace(name, StructMemberInitializer { name, value });
        return;
    }
    ptr.value().value = value;
}

Value *StructValue::evaluated_value(InterpretScope &scope) {
    return initialized_value(scope);
}

StructValue* StructValue::initialized_value(InterpretScope& scope) {
    auto struct_value = new (scope.allocate<StructValue>()) StructValue(
            getRefTypeLoc().copy(scope.allocator),
            definition,
            container,
            encoded_location()
    );
    declare_default_values(struct_value->values, scope);
    struct_value->values.reserve(values.size());
    for (auto & value: values) {
        auto val = value.second.value->scope_value(scope);
        // Move semantics: if the initializer references an existing destructible struct
        // variable in the scope, clear the source (move). Uses pointer-matching instead
        // of AST node type checks, because the compiler may have replaced identifiers.
        scope.move_clear_source(val, chem::string_view());
        if(definition && val) {
            // Check if we need implicit constructor conversion
            for(const auto var : definition->variables()) {
                if(var->name == value.first) {
                    auto kt = var->known_type();
                    if(kt) {
                        auto imp_cons = kt->implicit_constructor_for(val);
                        if(imp_cons) {
                            InterpretScope imp_scope(scope.global, scope.allocator, scope.global);
                            std::vector<Value*> imp_args = { val };
                            auto newVal = imp_cons->call(
                                &scope, imp_args, (Value*)nullptr, &imp_scope, true, (Value*)nullptr
                            );
                            if(newVal) val = newVal;
                        }
                    }
                    break;
                }
            }
        }
        // Use erase+emplace to overwrite any default value that was set earlier
        struct_value->values.erase(value.first);
        struct_value->values.emplace(value.first, StructMemberInitializer { value.first, val });
    }
    // Handle inherited struct default initialization
    if(definition) {
        for(auto& inh : definition->inherited) {
            const auto node = inh.type->get_direct_linked_canonical_node();
            if(node && node->kind() == ASTNodeKind::StructDecl) {
                auto parentDef = node->as_struct_def_unsafe();
                auto parentCons = parentDef->default_constructor_func();
                if(parentCons) {
                    // Check if any of the parent's member variables or the parent struct name are already set
                    bool hasParentMembers = false;
                    for(const auto pvar : parentDef->variables()) {
                        if(struct_value->values.find(pvar->name) != struct_value->values.end()) {
                            hasParentMembers = true;
                            break;
                        }
                    }
                    // Also check if the inherited struct type name is in values (literal syntax like
                    // `ExtFuncTestPoint : ExtFuncTestPoint { a: 23, b: 8 }`)
                    if(!hasParentMembers && struct_value->values.find(inh.ref_type_name()) != struct_value->values.end()) {
                        hasParentMembers = true;
                    }
                    if(!hasParentMembers) {
                        InterpretScope nested_scope(scope.global, scope.allocator, scope.global);
                        std::vector<Value*> nested_args;
                        Value* parentVal = parentCons->call(
                            &scope, nested_args, (Value*)nullptr, &nested_scope, true, (Value*)nullptr
                        );
                        if(parentVal && parentVal->kind() == ValueKind::StructValue) {
                            auto parentStructVal = parentVal->as_struct_value_unsafe();
                            for(auto& [pname, pmember] : parentStructVal->values) {
                                if(struct_value->values.find(pname) == struct_value->values.end()) {
                                    struct_value->values.emplace(pname, pmember);
                                }
                            }
                        }
                    }
                }
            }
        }
        // Initialize fields whose types have @make constructors but no explicit value
        for(const auto var : definition->variables()) {
            if(struct_value->values.find(var->name) != struct_value->values.end()) {
                continue; // Already has a value
            }
            auto kt = var->known_type();
            if(!kt) continue;
            const auto linked = kt->get_direct_linked_canonical_node();
            if(linked && linked->kind() == ASTNodeKind::StructDecl) {
                auto fieldDef = linked->as_struct_def_unsafe();
                auto fieldCons = fieldDef->default_constructor_func();
                if(fieldCons) {
                    InterpretScope nested_scope(scope.global, scope.allocator, scope.global);
                    std::vector<Value*> nested_args;
                    Value* fieldVal = fieldCons->call(
                        &scope, nested_args, (Value*)nullptr, &nested_scope, true, (Value*)nullptr
                    );
                    if(fieldVal) {
                        struct_value->values.emplace(var->name, StructMemberInitializer(var->name, fieldVal));
                    }
                }
            }
        }
    }
    return struct_value;
}

void StructValue::declare_default_values(
        tsl::ordered_map<chem::string_view, StructMemberInitializer> &into,
        InterpretScope &scope
) {
    if(!definition) return;
    for (const auto field : definition->variables()) {
        const auto defValue = field->default_value();
        if (into.find(field->name) == into.end() && defValue) {
            into.emplace(field->name, StructMemberInitializer { field->name, defValue->scope_value(scope) });
        }
    }
}

StructValue *StructValue::copy(ASTAllocator& allocator) {
    auto struct_value = new (allocator.allocate<StructValue>()) StructValue(
            getRefTypeLoc().copy(allocator),
            definition,
            container,
            encoded_location()
    );
    struct_value->values.reserve(values.size());
    for (const auto &value: values) {
        struct_value->values.emplace(value.first, StructMemberInitializer { value.first, value.second.value->copy(allocator) });
    }
    struct_value->set_variant_member_index(get_variant_member_index());
//    struct_value->generic_list.reserve(generic_list.size());
//    for(const auto& arg : generic_list) {
//        struct_value->generic_list.emplace_back(arg->copy(allocator));
//    }
    return struct_value;
}

Value *StructValue::child(InterpretScope &scope, const chem::string_view &name) {
    // Check direct values
    auto value = values.find(name);
    if (value != values.end()) {
        auto val = value->second.value;
        // Array-to-pointer decay on access: if the stored value is an ArrayValue
        // and the field type is a pointer, return a PointerValue wrapping the array
        // so pointer arithmetic and field access work correctly.
        if(val && val->val_kind() == ValueKind::ArrayValue && definition) {
            for(const auto var : definition->variables()) {
                if(var->name == name) {
                    auto kt = var->known_type();
                    if(kt && kt->kind() == BaseTypeKind::Pointer) {
                        auto arrVal = (ArrayValue*)val;
                        auto ptrType = kt->as_pointer_type_unsafe();
                        auto elemType = ptrType->type;
                        arrVal->evaluated_value(scope);
                        if(!arrVal->values.empty()) {
                            auto pv = new (scope.allocate<PointerValue>()) PointerValue(
                                (void*)arrVal->values[0], elemType, 0,
                                elemType->byte_size(scope.global->target_data) * arrVal->values.size(),
                                val->encoded_location()
                            );
                            pv->backingArray = arrVal;
                            pv->elementIndex = 0;
                            return pv;
                        }
                    }
                    break;
                }
            }
        }
        return val;
    }
    // Check member functions
    if(definition) {
        auto func = definition->member(name);
        if(func) {
            return this;
        }
        // Check inherited struct values (flattened in the interpreter)
        for(auto& inh : definition->inherited) {
            const auto linked = inh.type->get_direct_linked_canonical_node();
            if(linked && linked->kind() == ASTNodeKind::StructDecl) {
                auto inhDecl = linked->as_struct_def_unsafe();
                auto found = values.find(inhDecl->name_view());
                if(found != values.end() && found->second.value) {
                    // Recursively search the inherited struct's values
                    auto childVal = found->second.value->child(scope, name);
                    if(childVal) {
                        return childVal;
                    }
                }
            }
        }
    }
    return nullptr;
}