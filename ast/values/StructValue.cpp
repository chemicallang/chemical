// Copyright (c) Chemical Language Foundation 2025.

#include "StructValue.h"

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

    for (auto &value: values) {
        auto& value_ptr = value.second.value;
        auto variable = child_type_w_index(value.first);
        if (variable.second == -1) {
            gen.error(this) << "couldn't get struct child " << value.first << " in definition with name " << definition->name_view();
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
                    continue;
                }
            }

            value_ptr->store_in_struct(gen, this, inst, parent_type, idx, elem_index, pure_type);

        }
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

void StructValue::llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) {
    const auto known_type = lhs->getType();
    if(known_type->kind() == BaseTypeKind::Dynamic && known_type->linked_node()->as_interface_def()) {
        const auto value = llvm_allocate(gen, "", nullptr);
        gen.assign_store(lhs, lhsPtr, this, value, encoded_location());
    } else if(lhs->as_deref_value()) {
        if(!definition->destructor_func() && allows_direct_init()) {
            const auto deref = lhs->as_deref_value();
            const auto deref_type = deref->getValue()->getType();
            if (!deref_type->pure_type(gen.allocator)->is_pointer_or_ref()) {
                gen.error("expected a pointer or a reference", lhs);
                return;
            }
            const auto allocated = deref->llvm_pointer(gen);
            initialize_alloca(allocated, gen, nullptr);
        } else {
            gen.error("definition has either a destructor function or does not allow direct init", lhs);
        }
    } else {
        initialize_alloca(lhsPtr, gen, nullptr);
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

bool StructValue::resolve_container(
        GenericInstantiatorAPI& instantiator,
        BaseType* containerType,
        bool specialize_generics
) {
    auto& diagnoser = instantiator.getDiagnoser();
    switch (containerType->kind()) {
        case BaseTypeKind::Struct:
            container = containerType->as_struct_type_unsafe();
            break;
        case BaseTypeKind::Union:
            container = containerType->as_union_type_unsafe();
            break;
        default: {
            const auto found = containerType->linked_node();
            if (!found) {
                diagnoser.error(this) << "couldn't find struct definition for struct name " << containerType->representation();
                return false;
            }
            switch (found->kind()) {
                case ASTNodeKind::GenericStructDecl: {
                    auto gen_args = create_generic_list();
                    const auto gen_decl = found->as_gen_struct_def_unsafe();
                    if (specialize_generics) {
                        const auto instantiatedType = gen_decl->instantiate_type(instantiator, gen_args, encoded_location());
                        if(instantiatedType == nullptr) {
                            return false;
                        }
                        definition = instantiatedType;
                    } else {
                        definition = gen_decl->master_impl;
                    }
                    container = definition;
                    break;
                }
                case ASTNodeKind::GenericUnionDecl: {
                    auto gen_args = create_generic_list();
                    const auto gen_decl = found->as_gen_union_decl_unsafe();
                    if (specialize_generics) {
                        const auto instantiatedType = gen_decl->instantiate_type(instantiator, gen_args, encoded_location());
                        if(instantiatedType == nullptr) {
                            return false;
                        }
                        definition = instantiatedType;
                    } else {
                        definition = gen_decl->master_impl;
                    }
                    container = definition;
                    break;
                }
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
                    return resolve_container(instantiator, found->as_typealias_unsafe()->actual_type, specialize_generics);
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
        std::cerr << "unresolved child by name '" + name.str() + "' in struct";
        return;
    }
    ptr->second.value = value;
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
    for (auto &value: values) {
        struct_value->values.emplace(value.first, StructMemberInitializer { value.first, value.second.value->scope_value(scope) });
    }
//    struct_value->generic_list.reserve(generic_list.size());
//    for(const auto& arg : generic_list) {
//        struct_value->generic_list.emplace_back(arg->copy(scope.allocator));
//    }
    return struct_value;
}

void StructValue::declare_default_values(
        std::unordered_map<chem::string_view, StructMemberInitializer> &into,
        InterpretScope &scope
) {
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
//    struct_value->generic_list.reserve(generic_list.size());
//    for(const auto& arg : generic_list) {
//        struct_value->generic_list.emplace_back(arg->copy(allocator));
//    }
    return struct_value;
}

Value *StructValue::child(InterpretScope &scope, const chem::string_view &name) {
    auto value = values.find(name);
    if (value == values.end()) {
        auto func = definition->member(name);
        if(func) {
            return this;
        } else {
            return nullptr;
        }
    }
    return value->second.value;
}