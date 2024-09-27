// Copyright (c) Qinetik 2024.

#include "StructValue.h"

#include <memory>
#include "ast/structures/StructMember.h"
#include "ast/structures/UnionDef.h"
#include "compiler/SymbolResolver.h"
#include "ast/utils/ASTUtils.h"
#include "ast/types/GenericType.h"
#include "ast/types/LinkedType.h"
#include "StructMemberInitializer.h"
#include "ast/values/DereferenceValue.h"

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
            allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr);
        }
        if(!gen.assign_dyn_obj(this, expected_type, inst, allocaInst)) {
            gen.error("couldn't assign dyn object struct " + representation() + " to expected type " + expected_type->representation(), this);
        }
        inst = allocaInst;
    }

    auto& current_func_type = *gen.current_func_type;
    for (const auto &value: values) {
        auto& value_ptr = value.second->value;
        auto variable = definition->variable_type_index(value.first);
        if (variable.first == -1) {
            gen.error("couldn't get struct child " + value.first + " in definition with name " + definition->name, this);
        } else {

            // replace values that call implicit constructors
            auto implicit = variable.second->implicit_constructor_for(gen.allocator, value_ptr);
            if(implicit) {
                value_ptr = call_with_arg(implicit, value_ptr, gen.allocator);
            }

            std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
            bool moved = false;
            if(value_ptr->is_ref_moved()) {
                // since it will be moved, we will std memcpy it into current pointer
                auto elementPtr = Value::get_element_pointer(gen, parent_type, inst, idx, is_union() ? 0 : variable.first);
                moved = gen.move_by_memcpy(variable.second, value_ptr, elementPtr, value_ptr->llvm_value(gen));
            }
            if(!moved) {
                if (gen.requires_memcpy_ref_struct(variable.second, value_ptr)) {
                    auto elementPtr = Value::get_element_pointer(gen, parent_type, inst, idx, is_union() ? 0 : variable.first);
                    gen.memcpy_struct(value_ptr->llvm_type(gen), elementPtr, value_ptr->llvm_value(gen, nullptr));
                } else {
                    // couldn't move struct
                    value_ptr->store_in_struct(gen, this, inst, parent_type, idx, is_union() ? 0 : variable.first, variable.second);
                }
            }
        }
    }
    for(auto& value : definition->variables) {
        auto found = values.find(value.first);
        if(found == values.end()) {
            auto defValue = value.second->default_value();
            if (defValue) {
                auto variable = definition->variable_type_index(value.first);
                std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
                defValue->store_in_struct(gen, this, inst, parent_type, idx, is_union() ? 0 : variable.first, variable.second);
            } else if(linked_kind != ASTNodeKind::UnionDecl) {
                gen.error("expected a default value for '" + value.first + "'", this);
            }
        }
    }

}

llvm::AllocaInst *StructValue::llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) {
    auto dyn_obj = (llvm::AllocaInst*) gen.allocate_dyn_obj_based_on_type(expected_type);
    allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr);
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
    int16_t prev_itr;
    if(is_generic()) {
        prev_itr = definition->active_iteration;
        definition->set_active_iteration(generic_iteration);
    }
    definition->llvm_destruct(gen, givenAlloca);
    if(is_generic()) {
        definition->set_active_iteration(prev_itr);
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
        gen.error(
                "can't store struct value '" + representation() + "' into parent struct value '" + parent->representation() + "' with an unknown index" +
                " where current definition name '" + definition->name + "'", this);
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
        gen.error(
                "can't store struct value " + representation() + " array value " + ((Value*) parent)->representation() + " with an unknown index" +
                " where current definition name " + definition->name, this);
        return index + 1;
    }
    auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    initialize_alloca(elementPtr, gen, expected_type);
    return index + 1;
}

llvm::Value *StructValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    throw std::runtime_error("cannot allocate a struct without an identifier");
}

llvm::Value *StructValue::llvm_assign_value(Codegen &gen, Value *lhs) {
    const auto known_type = lhs->known_type();
    if(known_type->kind() == BaseTypeKind::Dynamic && known_type->linked_node()->as_interface_def()) {
        return llvm_allocate(gen, "", nullptr);
    } else if(lhs->as_deref_value()) {
        if(!definition->destructor_func() && !definition->clear_func() && allows_direct_init()) {
            const auto deref = lhs->as_deref_value();
            if (deref->value->value_type() == ValueType::Pointer) {
                auto allocated = deref->llvm_pointer(gen);
                initialize_alloca(allocated, gen, nullptr);
                return nullptr;
            }
        }
    } else {
        auto ptr = lhs->llvm_pointer(gen);
        if(!lhs->is_ref_moved()) {
            llvm::FunctionType* func_type;
            llvm::Value* func_callee;
            auto destr = gen.determine_destructor_for(definition->known_type(), func_type, func_callee);
            if(destr) {
                gen.builder->CreateCall(func_type, func_callee, { ptr });
            }
        }
        initialize_alloca(ptr, gen, nullptr);
        return nullptr;
    }
#ifdef DEBUG
    throw std::runtime_error("cannot allocate a struct without an identifier");
#endif
    return nullptr;
}

llvm::Value *StructValue::llvm_arg_value(Codegen &gen, FunctionCall *call, unsigned int index) {
    return llvm_allocate(gen, "", nullptr);
}

llvm::Value *StructValue::llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) {
    // TODO make sure this argument corresponds to the struct
    auto structPassed = gen.current_function->getArg(0);
    initialize_alloca(structPassed, gen, nullptr);
    return nullptr;
}

llvm::Type *StructValue::llvm_elem_type(Codegen &gen) {
    throw std::runtime_error("llvm_elem_type: called on a struct");
}

llvm::Type *StructValue::llvm_type(Codegen &gen) {
    if(definition->generic_params.empty()) {
        return definition->llvm_type(gen);
    } else {
        return definition->llvm_type(gen, generic_iteration);
    }
}

bool StructValue::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return definition->add_child_index(gen, indexes, name);
}

#endif

//StructValue::StructValue(
//        std::unique_ptr<Value> ref,
//        std::unordered_map<std::string, std::unique_ptr<Value>> values,
//        StructDefinition *definition
//) : ref(std::move(ref)), values(std::move(values)), definition(definition) {}

StructValue::StructValue(
        BaseType* refType,
        std::unordered_map<std::string, StructMemberInitializer*> values,
        ExtendableMembersContainerNode *definition,
        CSTToken* token,
        ASTNode* parent_node
) : refType(refType), values(std::move(values)), definition(definition), token(token), parent_node(parent_node) {}

StructValue::StructValue(
        BaseType* refType,
        std::unordered_map<std::string, StructMemberInitializer*> values,
        ExtendableMembersContainerNode *definition,
        InterpretScope &scope,
        CSTToken* token,
        ASTNode* parent_node
) : refType(refType), values(std::move(values)), definition(definition), token(token), parent_node(parent_node) {
    declare_default_values(this->values, scope);
}

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

bool StructValue::allows_direct_init() {
    switch(linked_kind) {
        case ASTNodeKind::StructDecl:
            return !definition->as_struct_def_unsafe()->has_constructor() || definition->as_struct_def_unsafe()->is_direct_init;
        case ASTNodeKind::UnionDecl:
            return !definition->as_union_def_unsafe()->has_constructor();
        case ASTNodeKind::UnnamedStruct:
        case ASTNodeKind::UnnamedUnion:
            return true;
        default:
            return false;
    }
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
    if(linked_kind == ASTNodeKind::UnionDecl) {
        if(values.size() != 1) {
            diagnoser.error("union '" + definition->name + "' must be initialized with a single member value", this);
            return false;
        } else {
            return true;
        }
    }
    if(values.size() < definition->init_values_req_size()) {
        std::vector<std::string> missing;
        for(auto& mem : definition->inherited) {
            auto& type = *mem->type;
            if(type.get_direct_linked_struct()) {
                auto& ref_type_name = mem->ref_type_name();
                auto val = values.find(ref_type_name);
                if (val == values.end()) {
                    missing.emplace_back(ref_type_name);
                }
            }
        }
        for(auto& mem : definition->variables) {
            if(mem.second->default_value() == nullptr) {
                auto val = values.find(mem.second->name);
                if (val == values.end()) {
                    missing.emplace_back(mem.second->name);
                }
            }
        }
        if(!missing.empty()) {
            for (auto& miss: missing) {
                diagnoser.error(
                        "couldn't find value for member '" + miss + "' for initializing struct '" + definition->name +
                        "'", this);
            }
            return true;
        }
    }
    return false;
}

bool StructValue::link(SymbolResolver& linker, Value*& value_ptr, BaseType* expected_type) {
    refType->link(linker);
    auto found = refType->linked_node();
    auto& current_func_type = *linker.current_func_type;
    if(found) {
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
            default:
                linker.error("given struct name is not a struct definition : " + refType->representation(), this);
                return false;
        }
        linked_kind = k;
        definition = (ExtendableMembersContainerNode*) found;
        diagnose_missing_members_for_init(linker);
        if(!allows_direct_init()) {
            linker.error("struct value with a constructor cannot be initialized, name '" + definition->name + "' has a constructor", this);
            return false;
        }
        auto refTypeKind = refType->kind();
        if(refTypeKind == BaseTypeKind::Generic) {
            for (auto& arg: generic_list()) {
                arg->link(linker);
            }
        }
        int16_t prev_itr;
        // setting active generic iteration
        if(!definition->generic_params.empty()) {
            prev_itr = definition->active_iteration;
            generic_iteration = definition->register_value(linker, this);
            definition->set_active_iteration(generic_iteration);
        }
        // linking values
        for (auto &val: values) {
            auto& val_ptr = val.second->value;
            auto child_node = definition->direct_child_member(val.first);
            if(!child_node) {
                linker.error("couldn't find child " + val.first + " in struct declaration", this);
                continue;
            }
            auto child_type = child_node->get_value_type(linker.allocator);
            val_ptr->link(linker, val_ptr, child_type);
            auto member = definition->variables.find(val.first);
            if(definition->variables.end() != member) {
                const auto mem_type = member->second->get_value_type(linker.allocator);
                auto implicit = mem_type->implicit_constructor_for(linker.allocator, val_ptr);
                current_func_type.mark_moved_value(linker.allocator, val.second->value, mem_type, linker);
                if(implicit) {
                    link_with_implicit_constructor(implicit, linker, val_ptr);
                }
            }
        }
        // setting iteration back
        if(!definition->generic_params.empty()) {
            definition->set_active_iteration(prev_itr);
        }
    } else {
        linker.error("couldn't find struct definition for struct name " + refType->representation(), this);
        return false;
    };
    return true;
}

ASTNode *StructValue::linked_node() {
    return definition;
}

bool StructValue::is_generic() {
    switch(linked_kind) {
        case ASTNodeKind::UnionDecl:
            return linked_union()->is_generic();
        case ASTNodeKind::StructDecl:
        default:
            return linked_struct()->is_generic();
    }
}

void StructValue::runtime_name(std::ostream& output) {
    switch(linked_kind) {
        case ASTNodeKind::UnionDecl: {
            const auto uni = linked_union();
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
        default:
        case ASTNodeKind::StructDecl:{
            const auto decl = linked_struct();
            if (decl->is_generic()) {
                auto prev = decl->active_iteration;
                decl->set_active_iteration(generic_iteration);
                decl->runtime_name(output);
                decl->set_active_iteration(prev);
            } else {
                decl->runtime_name(output);
            }
        }
    }
}

Value *StructValue::call_member(
        InterpretScope &scope,
        const std::string &name,
        std::vector<Value*> &params
) {
    auto fn = definition->member(name);
    if (fn == nullptr) {
        scope.error("couldn't find member function by name " + name + " in a struct by name " + refType->representation());
        return nullptr;
    }
#ifdef DEBUG
    if (!fn->body.has_value()) {
        scope.error("function doesn't have body in a struct " + name);
        return nullptr;
    }
#endif
    InterpretScope child(nullptr, scope.global);
    child.declare("this", this);
    auto value = fn->call(&scope, params, this, &child);
    return value;
}

void StructValue::set_child_value(const std::string &name, Value *value, Operation op) {
    auto ptr = values.find(name);
    if (ptr == values.end()) {
        std::cerr << "couldn't find child by name " + name + " in struct";
        return;
    }
    ptr->second->value = value;
}

Value *StructValue::scope_value(InterpretScope &scope) {
    auto struct_value = new (scope.allocate<StructValue>()) StructValue(
            refType->copy(scope.allocator),
            std::unordered_map<std::string, StructMemberInitializer*>(),
            definition,
            token,
            parent_node
    );
    declare_default_values(struct_value->values, scope);
    struct_value->values.reserve(values.size());
    for (const auto &value: values) {
        struct_value->values[value.first] = new (scope.allocate<StructMemberInitializer>()) StructMemberInitializer(
                value.first,
                value.second->value->scope_value(scope),
                struct_value,
                value.second->member
        );
    }
//    struct_value->generic_list.reserve(generic_list.size());
//    for(const auto& arg : generic_list) {
//        struct_value->generic_list.emplace_back(arg->copy(scope.allocator));
//    }
    return struct_value;
}

void StructValue::declare_default_values(
        std::unordered_map<std::string, StructMemberInitializer*> &into,
        InterpretScope &scope
) {
    Value* defValue;
    for (const auto &field: definition->variables) {
        defValue = field.second->default_value();
        if (into.find(field.second->name) == into.end() && defValue) {
            into[field.second->name]->value = defValue->scope_value(scope);
        }
    }
}

StructValue *StructValue::copy(ASTAllocator& allocator) {
    auto struct_value = new (allocator.allocate<StructValue>()) StructValue(
        refType->copy(allocator),
        std::unordered_map<std::string, StructMemberInitializer*>(),
        definition,
        token,
        parent_node
    );
    struct_value->values.reserve(values.size());
    for (const auto &value: values) {
        struct_value->values[value.first] = value.second->copy(allocator);
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

Value *StructValue::child(InterpretScope &scope, const std::string &name) {
    auto value = values.find(name);
    if (value == values.end()) {
        auto func = definition->member(name);
        if(func) {
            return this;
        } else {
            return nullptr;
        }
    }
    return value->second->value;
}

StructMemberInitializer::StructMemberInitializer(
        std::string name,
        Value* value,
        StructValue* struct_value,
        ASTNode* member
) : name(std::move(name)), value(value), struct_value(struct_value), member(member) {

}

StructMemberInitializer* StructMemberInitializer::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<StructMemberInitializer>()) StructMemberInitializer(name, value->copy(allocator), struct_value, member);
}

CSTToken *StructMemberInitializer::cst_token() {
    return value->cst_token();
}

ASTNode* StructMemberInitializer::parent() {
    return struct_value->parent_node;
}