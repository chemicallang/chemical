// Copyright (c) Chemical Language Foundation 2025.

#include "StructValue.h"

#include <memory>
#include "ast/structures/StructMember.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "compiler/SymbolResolver.h"
#include "ast/utils/ASTUtils.h"
#include "ast/types/GenericType.h"
#include "ast/types/LinkedType.h"
#include "StructMemberInitializer.h"
#include "ast/values/DereferenceValue.h"
#include <sstream>

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

            bool moved = false;
            if(value_ptr->is_ref_moved()) {
                // since it will be moved, we will std memcpy it into current pointer
                auto elementPtr = Value::get_element_pointer(gen, parent_type, inst, idx, elem_index);
                moved = gen.move_by_memcpy(variable.second, value_ptr, elementPtr, value_ptr->llvm_value(gen));
            }
            if(!moved) {
                if (gen.requires_memcpy_ref_struct(variable.second, value_ptr)) {
                    auto elementPtr = Value::get_element_pointer(gen, parent_type, inst, idx, elem_index);
                    gen.memcpy_struct(value_ptr->llvm_type(gen), elementPtr, value_ptr->llvm_value(gen, nullptr), value_ptr->encoded_location());
                } else {
                    // couldn't move struct
                    value_ptr->store_in_struct(gen, this, inst, parent_type, idx, elem_index, variable.second);
                }
            }
        }
    }

    // storing default values
    const auto isUnion = is_union();
    for(auto& value : container->variables) {
        auto found = values.find(value.first);
        if(found == values.end()) {
            auto defValue = value.second->default_value();
            if (defValue) {
                auto variable = container->variable_type_index(value.first);
                std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
                defValue->store_in_struct(gen, this, inst, parent_type, idx, is_union() ? 0 : variable.first, variable.second);
            } else if(!isUnion) {
                gen.error(this) << "expected a default value for '" << value.first << "'";
            }
        }
    }

}

llvm::AllocaInst *StructValue::llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) {
    auto dyn_obj = (llvm::AllocaInst*) gen.allocate_dyn_obj_based_on_type(expected_type);
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

void StructValue::llvm_destruct(Codegen &gen, llvm::Value *givenAlloca) {
    if(definition) {
        int16_t prev_itr;
        if (is_generic()) {
            prev_itr = definition->active_iteration;
            definition->set_active_iteration(generic_iteration);
        }
        definition->llvm_destruct(gen, givenAlloca, encoded_location());
        if (is_generic()) {
            definition->set_active_iteration(prev_itr);
        }
    } else {
        switch(refType->kind()) {
            case BaseTypeKind::Union:
            case BaseTypeKind::Struct:
                // TODO we must do this here
                return;
            default:
                return;
        }
    }
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
        if(!definition->destructor_func() && !definition->clear_func() && allows_direct_init()) {
            const auto deref = lhs->as_deref_value();
            const auto deref_type = deref->value->create_type(gen.allocator);
            if (deref_type->pure_type()->is_pointer()) {
                auto allocated = deref->llvm_pointer(gen);
                initialize_alloca(allocated, gen, nullptr);
                return;
            }
        }
    } else {
        if(!lhs->is_ref_moved()) {
            llvm::Function* func_data;
            auto destr = gen.determine_destructor_for(definition->known_type(), func_data);
            if(destr) {
                const auto callInst = gen.builder->CreateCall(func_data, { lhsPtr });
                gen.di.instr(callInst, this);
            }
        }
        initialize_alloca(lhsPtr, gen, nullptr);
        return;
    }
#ifdef DEBUG
    throw std::runtime_error("cannot allocate a struct without an identifier");
#endif
    return;
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
        if (definition->generic_params.empty()) {
            return definition->llvm_type(gen);
        } else {
            return definition->llvm_type(gen, generic_iteration);
        }
    } else {
        switch(refType->kind()) {
            case BaseTypeKind::Struct:
                return refType->as_struct_type_unsafe()->llvm_type(gen);
            case BaseTypeKind::Union:
                return refType->as_union_type_unsafe()->llvm_type(gen);
            default:
                return nullptr;
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

uint64_t StructValue::byte_size(bool is64Bit) {
    if(definition->generic_params.empty()) {
        return definition->byte_size(is64Bit);
    } else {
        return definition->byte_size(is64Bit, generic_iteration);
    }
}

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
        const auto k = definition->kind();
        switch (k) {
            case ASTNodeKind::StructDecl:
                return !definition->as_struct_def_unsafe()->has_constructor() || definition->as_struct_def_unsafe()->is_direct_init();
            case ASTNodeKind::UnionDecl:
                return !definition->as_union_def_unsafe()->has_constructor();
            case ASTNodeKind::UnnamedStruct:
            case ASTNodeKind::UnnamedUnion:
                return true;
            default:
                return false;
        }
    } else {
        const auto k = refType->kind();
        switch(k) {
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
    for(auto& mem : container->variables) {
        if(mem.second->default_value() == nullptr) {
            auto val = values.find(mem.second->name);
            if (val == values.end()) {
                missing.emplace_back(mem.second->name);
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

bool StructValue::link(SymbolResolver& linker, Value*& value_ptr, BaseType* expected_type) {
    if(refType) {
        if(!refType->link(linker)) {
            return false;
        }
        auto found = refType->linked_node();
        if(!found) {
            linker.error(this) << "couldn't find struct definition for struct name " << refType->representation();
            return false;
        }
        definition = (ExtendableMembersContainerNode*) found;
        container = definition;
        const auto k = found->kind();
        switch (k) {
            case ASTNodeKind::UnionDecl:
            case ASTNodeKind::UnnamedUnion:
                if(values.size() > 1) {
                    linker.error("initializing multiple values inside a union is not allowed", this);
                    return false;
                }
                break;
            case ASTNodeKind::UnnamedStruct:
            case ASTNodeKind::StructDecl:
                break;
            case ASTNodeKind::StructType:{
                const auto structType = (StructType*) found; //StructType::castASTNode(found);
                refType = structType;
                definition = nullptr;
                container = structType;
                break;
            }
            case ASTNodeKind::UnionType: {
                const auto unionType = (UnionType*) found; // UnionType::castASTNode(found);
                refType = unionType;
                definition = nullptr;
                container = unionType;
                break;
            }
            default:
                linker.error(this) << "given struct name is not a struct definition : " << refType->representation();
                return false;
        }
    } else {
        if(!expected_type) {
            linker.error("unnamed struct value cannot link without a type", this);
            return false;
        }
        switch(expected_type->kind()) {
            case BaseTypeKind::Struct:
                refType = expected_type;
                container = refType->as_struct_type_unsafe();
                break;
            case BaseTypeKind::Union:
                refType = expected_type;
                container = refType->as_union_type_unsafe();
                break;
            default:
                const auto node = linked_node();
                if(node) {
                    const auto nodeKind = node->kind();
                    switch(nodeKind) {
                        case ASTNodeKind::UnnamedUnion:
                            if(values.size() > 1) {
                                linker.error("initializing multiple values inside a union is not allowed", this);
                                return false;
                            }
                            container = node->as_unnamed_union_unsafe();
                            break;
                        case ASTNodeKind::UnnamedStruct:
                            container = node->as_unnamed_struct_unsafe();
                            break;
                        case ASTNodeKind::StructDecl:
                            definition = node->as_struct_def_unsafe();
                            container = definition;
                            break;
                        case ASTNodeKind::UnionDecl:
                            if(values.size() > 1) {
                                linker.error("initializing multiple values inside a union is not allowed", this);
                                return false;
                            }
                            definition = node->as_union_def_unsafe();
                            container = definition;
                            break;
                        default:
                            linker.error("unnamed struct value cannot link with an unknown node", this);
                            return false;
                    }
                } else {
                    linker.error("unnamed struct value cannot link without a node", this);
                    return false;
                }
        }
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
    int16_t prev_itr;
    // setting active generic iteration
    if(definition && !definition->generic_params.empty()) {
        prev_itr = definition->active_iteration;
        generic_iteration = definition->register_value(linker, this);
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
        auto child_type = child_node->get_value_type(linker.allocator);
        const auto val_linked = val_ptr->link(linker, val_ptr, child_type);
        auto member = container->variables.find(val.first);
        if(val_linked && container->variables.end() != member) {
            const auto mem_type = member->second->get_value_type(linker.allocator);
            auto implicit = mem_type->implicit_constructor_for(linker.allocator, val_ptr);
            current_func_type.mark_moved_value(linker.allocator, val.second.value, mem_type, linker);
            if(implicit) {
                link_with_implicit_constructor(implicit, linker, val_ptr);
            } else if(!mem_type->satisfies(linker.allocator, value, false)) {
                linker.unsatisfied_type_err(value, mem_type);
            }
        }
    }
    // setting iteration back
    if(definition && !definition->generic_params.empty()) {
        definition->set_active_iteration(prev_itr);
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

bool StructValue::is_generic() {
    if(!definition) return false;
    switch(definition->kind()) {
        case ASTNodeKind::UnionDecl:
            return definition->as_union_def_unsafe()->is_generic();
        case ASTNodeKind::StructDecl:
            return definition->as_struct_def_unsafe()->is_generic();
        default:
            return false;
    }
}

void StructValue::runtime_name(std::ostream& output) {
    if(definition) {
        switch (definition->kind()) {
            case ASTNodeKind::UnionDecl: {
                const auto uni = definition->as_union_def_unsafe();
                if (uni->is_generic()) {
                    auto prev = uni->active_iteration;
                    uni->set_active_iteration(generic_iteration);
                    uni->runtime_name(output);
                    uni->set_active_iteration(prev);
                } else {
                    uni->runtime_name(output);
                }
                return;
            }
            case ASTNodeKind::StructDecl: {
                const auto decl = definition->as_struct_def_unsafe();
                if (decl->is_generic()) {
                    auto prev = decl->active_iteration;
                    decl->set_active_iteration(generic_iteration);
                    decl->runtime_name(output);
                    decl->set_active_iteration(prev);
                } else {
                    decl->runtime_name(output);
                }
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

std::string StructValue::runtime_name_str() {
    std::stringstream stream;
    runtime_name(stream);
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
            encoded_location(),
            parent_node
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
    for (const auto &field: definition->variables) {
        const auto defValue = field.second->default_value();
        if (into.find(field.second->name) == into.end() && defValue) {
            into.emplace(field.second->name, StructMemberInitializer { field.second->name, defValue->scope_value(scope) });
        }
    }
}

StructValue *StructValue::copy(ASTAllocator& allocator) {
    auto struct_value = new (allocator.allocate<StructValue>()) StructValue(
        refType->copy(allocator),
        definition,
        container,
        encoded_location(),
        parent_node
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