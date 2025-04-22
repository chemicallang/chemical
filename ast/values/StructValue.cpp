// Copyright (c) Chemical Language Foundation 2025.

#include "StructValue.h"

#include "compiler/mangler/NameMangler.h"
#include <memory>
#include "ast/structures/StructMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/values/VariableIdentifier.h"
#include "compiler/SymbolResolver.h"
#include "ast/utils/ASTUtils.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/types/GenericType.h"
#include "ast/types/LinkedType.h"
#include "ast/utils/GenericUtils.h"
#include "StructMemberInitializer.h"
#include "ast/values/DereferenceValue.h"
#include <sstream>
#include <iostream>

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "IntValue.h"

void StructValue::initialize_alloca(llvm::Value *inst, Codegen& gen, BaseType* expected_type) {

    const auto parent_type = llvm_type(gen);

    // when the expected type is dyn, which means fat pointer has been allocated before, however struct can't be stored inside it
    // so we allocate a struct, and assign dynamic object to the fat pointer, we also initialize the dynamic object
    const auto interface = expected_type ? expected_type->linked_dyn_interface() : nullptr;
    if(interface) {
        if(!allocaInst) {
            const auto alloca1 = gen.builder->CreateAlloca(parent_type, nullptr);
            gen.di.instr(alloca1, this);
            allocaInst = alloca1;
        }
        if(!gen.assign_dyn_obj(this, expected_type, inst, allocaInst, encoded_location())) {
            gen.error(this) << "couldn't assign dyn object struct " << representation() << " to expected type " << expected_type->representation();
        }
        inst = allocaInst;
    }

    for (auto &value: values) {
        auto& value_ptr = value.second.value;
        auto variable = container->variable_type_index(value.first);
        if (variable.first == -1) {
            gen.error(this) << "couldn't get struct child " << value.first << " in definition with name " << definition->name_view();
        } else {

            std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
            const auto elem_index = is_union() ? 0 : variable.first;

            auto implicit = variable.second->implicit_constructor_for(gen.allocator, value_ptr);
            if(implicit) {
                // replace values that call implicit constructors
                value_ptr = (Value*) call_with_arg(implicit, value_ptr, variable.second, gen.allocator, gen);
            } else {
                const auto type_kind = variable.second->kind();
                if(type_kind == BaseTypeKind::Reference) {
                    // store pointer only, since user want's a reference
                    auto elementPtr = Value::get_element_pointer(gen, parent_type, inst, idx, elem_index);
                    const auto ref_pointer = value_ptr->llvm_pointer(gen);
                    const auto storeInst = gen.builder->CreateStore(ref_pointer, elementPtr);
                    gen.di.instr(storeInst, value_ptr);
                    continue;
                }
            }

            value_ptr->store_in_struct(gen, this, inst, parent_type, idx, elem_index, variable.second);

        }
    }

    // storing default values
    const auto isUnion = is_union();
    for(const auto value : container->variables()) {
        auto found = values.find(value->name);
        if(found == values.end()) {
            auto defValue = value->default_value();
            if (defValue) {
                auto variable = container->variable_type_index(value->name);
                std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
                defValue->store_in_struct(gen, this, inst, parent_type, idx, is_union() ? 0 : variable.first, variable.second);
            } else if(!isUnion) {
                gen.error(this) << "expected a default value for '" << value->name << "'";
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
    throw std::runtime_error("cannot allocate a struct without an identifier");
}

void StructValue::llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) {
    const auto known_type = lhs->known_type();
    if(known_type->kind() == BaseTypeKind::Dynamic && known_type->linked_node()->as_interface_def()) {
        const auto value = llvm_allocate(gen, "", nullptr);
        gen.assign_store(lhs, lhsPtr, this, value, encoded_location());
        return;
    } else if(lhs->as_deref_value()) {
        if(!definition->destructor_func() && allows_direct_init()) {
            const auto deref = lhs->as_deref_value();
            const auto deref_type = deref->value->create_type(gen.allocator);
            if (deref_type->pure_type(gen.allocator)->is_pointer()) {
                auto allocated = deref->llvm_pointer(gen);
                initialize_alloca(allocated, gen, nullptr);
                return;
            }
        } else {
            gen.error("definition has either a destructor function or does not allow direct init", lhs);
        }
    } else {
        initialize_alloca(lhsPtr, gen, nullptr);
        return;
    }
    gen.error("cannot assign struct value to pointer", lhs);
#ifdef DEBUG
    throw std::runtime_error("cannot allocate a struct without an identifier");
#endif
}

llvm::Value *StructValue::llvm_arg_value(Codegen &gen, BaseType* expected_type) {
    return llvm_allocate(gen, "", nullptr);
}

llvm::Value *StructValue::llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) {
    // TODO make sure this argument corresponds to the struct
    // TODO hardcoding the struct return index
    auto structPassed = gen.current_function->getArg(0);
    initialize_alloca(structPassed, gen, nullptr);
    return nullptr;
}

llvm::Type *StructValue::llvm_type(Codegen &gen) {
    if(definition) {
        return definition->llvm_type(gen);
    } else {
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
        switch(refType->kind()) {
            case BaseTypeKind::Struct:
            case BaseTypeKind::Union:
                return true;
            default:
                return node_allows_direct_init(linked_node());
        }
    };
}

std::vector<BaseType*>& StructValue::generic_list() {
    return ((GenericType*) refType)->types;
}

std::vector<BaseType*> StructValue::create_generic_list() {
    const auto k = refType->kind();
    if(k == BaseTypeKind::Generic) {
        return ((GenericType*) refType)->types;
    } else {
        return {};
    }
}

bool StructValue::diagnose_missing_members_for_init(ASTDiagnoser& diagnoser) {
    if(is_union()) {
        if(values.size() != 1) {
            diagnoser.error(this) << "union '"
            << definition->name_view() << "' must be initialized with a single member value";
            return false;
        } else {
            return true;
        }
    }
    std::vector<chem::string_view> missing;
    for(auto& mem : container->inherited) {
        auto& type = *mem.type;
        if(type.get_direct_linked_struct()) {
            const auto& ref_type_name = mem.ref_type_name();
            auto val = values.find(ref_type_name);
            if (val == values.end()) {
                missing.emplace_back(ref_type_name);
            }
        }
    }
    for(const auto mem : container->variables()) {
        if(mem->default_value() == nullptr) {
            auto val = values.find(mem->name);
            if (val == values.end()) {
                missing.emplace_back(mem->name);
            }
        }
    }
    if(!missing.empty()) {
        for (auto& miss: missing) {
            diagnoser.error(this) << "couldn't find value for member '" << miss << "' for initializing struct '" << definition->name_view() << "'";
        }
        return true;
    }
    return false;
}

bool StructValue::resolve_container(GenericInstantiatorAPI& instantiator) {
    auto& diagnoser = instantiator.getDiagnoser();
    switch(refType->kind()) {
        case BaseTypeKind::Struct:
            container = refType->as_struct_type_unsafe();
            break;
        case BaseTypeKind::Union:
            container = refType->as_union_type_unsafe();
            break;
        default:
        {
            const auto found = refType->linked_node();
            if(!found) {
                diagnoser.error(this) << "couldn't find struct definition for struct name " << refType->representation();
                return false;
            }
            switch(found->kind()) {
                case ASTNodeKind::GenericStructDecl:{
                    auto gen_args = create_generic_list();
                    const auto gen_decl = found->as_gen_struct_def_unsafe();
                    if(are_all_specialized(gen_args)) {
                        definition = gen_decl->register_generic_args(instantiator, gen_args);
                    } else {
                        definition = gen_decl->master_impl;
                    }
                    container = definition;
                    break;
                }
                case ASTNodeKind::GenericUnionDecl:{
                    auto gen_args = create_generic_list();
                    const auto gen_decl = found->as_gen_union_decl_unsafe();
                    if(are_all_specialized(gen_args)) {
                        definition = gen_decl->register_generic_args(instantiator, gen_args);
                    } else {
                        definition = gen_decl->master_impl;
                    }
                    container = definition;
                    break;
                }
                case ASTNodeKind::UnnamedUnion:
                    if(values.size() > 1) {
                        diagnoser.error("initializing multiple values inside a union is not allowed", this);
                        return false;
                    }
                    container = found->as_unnamed_union_unsafe();
                    break;
                case ASTNodeKind::UnionDecl:
                    if(values.size() > 1) {
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
                default:
                    diagnoser.error("unknown struct/union being initialized via struct value", this);
                    definition = found->as_extendable_members_container_node();
                    container = definition;
                    break;
            }
        }
    }
    return true;
}

bool StructValue::link(SymbolResolver& linker, Value*& value_ptr, BaseType* expected_type) {
    if(refType) {
        if(!refType->link(linker)) {
            return false;
        }
    } else {
        if(!expected_type) {
            linker.error("unnamed struct value cannot link without a type", this);
            return false;
        }
        refType = expected_type;
    }
    if(!resolve_container(linker.genericInstantiator)) {
        return false;
    }
    diagnose_missing_members_for_init(linker);
    if(!allows_direct_init()) {
        linker.error(this) << "struct value with a constructor cannot be initialized, name '" << definition->name_view() << "' has a constructor";
    }
    auto refTypeKind = refType->kind();
    if(refTypeKind == BaseTypeKind::Generic) {
        for (auto& arg: generic_list()) {
            arg->link(linker);
        }
    }
    auto& current_func_type = *linker.current_func_type;
    // linking values
    for (auto &val: values) {
        auto& val_ptr = val.second.value;
        const auto value = val_ptr;
        auto child_node = container->child_member_or_inherited_struct(val.first);
        if(!child_node) {
            linker.error(this) << "unresolved child '" << val.first << "' in struct declaration";
            continue;
        }
        auto child_type = child_node->known_type();
        const auto val_linked = val_ptr->link(linker, val_ptr, child_type);
        const auto member = container->direct_variable(val.first);
        if(val_linked && member) {
            const auto mem_type = member->known_type();
            auto implicit = mem_type->implicit_constructor_for(linker.allocator, val_ptr);
            current_func_type.mark_moved_value(linker.allocator, val.second.value, mem_type, linker);
            if(implicit) {
                link_with_implicit_constructor(implicit, linker, val_ptr);
            } else if(!mem_type->satisfies(linker.allocator, value, false)) {
                linker.unsatisfied_type_err(value, mem_type);
            }
        }
    }
    return true;
}

ASTNode *StructValue::linked_node() {
    if(definition) {
        return definition;
    } else {
        return refType->linked_node();
    }
}

void StructValue::runtime_name(std::ostream& output, NameMangler& mangler) {
    if(definition) {
        switch (definition->kind()) {
            case ASTNodeKind::UnionDecl: {
                const auto uni = definition->as_union_def_unsafe();
                mangler.mangle(output, uni);
                return;
            }
            case ASTNodeKind::StructDecl: {
                const auto decl = definition->as_struct_def_unsafe();
                mangler.mangle(output, decl);
                return;
            }
            default:
                return;
        }
    } else {
        switch(refType->kind()) {
            case BaseTypeKind::Struct:
                output << refType->as_struct_type_unsafe()->name;
                return;
            case BaseTypeKind::Union:
                output << refType->as_union_type_unsafe()->name;
                return;
            default:
                return;
        }
    }
}

std::string StructValue::runtime_name_str(NameMangler& mangler) {
    std::stringstream stream;
    runtime_name(stream, mangler);
    return stream.str();
}

Value *StructValue::call_member(
        InterpretScope &scope,
        const chem::string_view &name,
        std::vector<Value*> &params
) {
    auto fn = definition->member(name);
    if (fn == nullptr) {
        scope.error("couldn't find member function by name " + name.str() + " in a struct by name " + refType->representation(), this);
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
            refType->copy(scope.allocator),
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
        refType->copy(allocator),
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

BaseType* StructValue::create_type(ASTAllocator& allocator) {
    return refType;
}

BaseType* StructValue::known_type() {
    return refType;
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