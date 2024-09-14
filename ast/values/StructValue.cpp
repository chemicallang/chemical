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

void StructValue::initialize_alloca(llvm::Value *inst, Codegen& gen) {
    auto& current_func_type = *gen.current_func_type;
    const auto parent_type = llvm_type(gen);
    for (const auto &value: values) {
        auto& value_ptr = value.second->value;
        auto variable = definition->variable_type_index(value.first);
        if (variable.first == -1) {
            gen.error("couldn't get struct child " + value.first + " in definition with name " + definition->name, this);
        } else {
            auto movable_value = current_func_type.movable_value(gen, value_ptr.get());
            if(movable_value == nullptr) {
                // couldn't move struct
                std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
                value_ptr->store_in_struct(gen, this, inst, parent_type, idx, is_union() ? 0 : variable.first,
                                           variable.second);
            } else {
                // since it will be moved, we will std memcpy it into current pointer
                std::vector<llvm::Value*> idx{gen.builder->getInt32(0)};
                auto elementPtr = Value::get_element_pointer(gen, parent_type, inst, idx, is_union() ? 0 : variable.first);
                current_func_type.move_by_memcpy(gen, variable.second, value_ptr.get(), elementPtr, movable_value);
            }
        }
    }
}

llvm::AllocaInst *StructValue::llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) {
    allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr);
    initialize_alloca(allocaInst, gen);
    return allocaInst;
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
    const auto interface = expected_type ? expected_type->linked_dyn_interface() : nullptr;
    if(interface) {
        auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
        const auto struct_alloc = llvm_allocate(gen, "", nullptr);
        if(!gen.assign_dyn_obj(this, expected_type, elementPtr, struct_alloc)) {
            gen.error("couldn't assign dyn object struct " + representation() + " to expected type " + expected_type->representation() + " in parent " + parent->representation(), this);
        }
    } else {
        auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
        initialize_alloca(elementPtr, gen);
    }
    return index + values.size();
}

unsigned int StructValue::store_in_array(
        Codegen &gen,
        ArrayValue *parent,
        llvm::AllocaInst *ptr,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType* expected_type
) {
    if (index == -1) {
        gen.error(
                "can't store struct value " + representation() + " array value " + ((Value*) parent)->representation() + " with an unknown index" +
                " where current definition name " + definition->name, this);
        return index + 1;
    }
    const auto interface = expected_type ? expected_type->linked_dyn_interface() : nullptr;
    if(interface) {
        auto elementPtr = Value::get_element_pointer(gen, ((Value*) parent)->llvm_type(gen), ptr, idxList, index);
        const auto struct_alloc = llvm_allocate(gen, "", nullptr);
        if(!gen.assign_dyn_obj(this, expected_type, elementPtr, struct_alloc)) {
            gen.error("couldn't assign dyn object struct " + representation() + " to expected type " + expected_type->representation() + " in parent " + ((Value*) parent)->representation(), this);
        }
    } else {
        idxList.emplace_back(gen.builder->getInt32(index));
        for (const auto &value: values) {
            auto& value_ptr = value.second->value;
            auto currIndex = definition->variable_type_index(value.first);
            value_ptr->store_in_array(gen, parent, ptr, idxList, currIndex.first, currIndex.second);
        }
    }
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
        if(!definition->destructor_func() && allows_direct_init()) {
            const auto deref = lhs->as_deref_value();
            if (deref->value->value_type() == ValueType::Pointer) {
                auto allocated = deref->llvm_pointer(gen);
                initialize_alloca(allocated, gen);
                return nullptr;
            }
        }
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
    initialize_alloca(structPassed, gen);
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
        std::unique_ptr<Value> ref,
        std::unordered_map<std::string, std::unique_ptr<StructMemberInitializer>> values,
        std::vector<std::unique_ptr<BaseType>> generic_list,
        ExtendableMembersContainerNode *definition,
        CSTToken* token,
        ASTNode* parent_node
) : ref(std::move(ref)), values(std::move(values)), definition(definition), generic_list(std::move(generic_list)), token(token), parent_node(parent_node) {}

StructValue::StructValue(
        std::unique_ptr<Value> ref,
        std::unordered_map<std::string, std::unique_ptr<StructMemberInitializer>> values,
        ExtendableMembersContainerNode *definition,
        InterpretScope &scope,
        CSTToken* token,
        ASTNode* parent_node
) : ref(std::move(ref)), values(std::move(values)), definition(definition), token(token), parent_node(parent_node) {
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

bool StructValue::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    ref->link(linker, ref);
    auto found = ref->linked_node();
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
                linker.error("given struct name is not a struct definition : " + ref->representation(), this);
                return false;
        }
        linked_kind = k;
        definition = (ExtendableMembersContainerNode*) found;
        if(!allows_direct_init()) {
            linker.error("struct value with a constructor cannot be initialized, name '" + definition->name + "' has a constructor", this);
            return false;
        }
        for(auto& arg : generic_list) {
            arg->link(linker, arg);
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
            val_ptr->link(linker, this, val.first);
            auto member = definition->variables.find(val.first);
            if(definition->variables.end() != member) {
                const auto mem_type = member->second->get_value_type();
                auto implicit = mem_type->implicit_constructor_for(val_ptr.get());
                current_func_type.mark_moved_value(val.second->value.get(), mem_type.get(), linker);
                if(implicit) {
                    if(linker.preprocess) {
                        val_ptr = call_with_arg(implicit, std::move(val_ptr), linker);
                    } else {
                        link_with_implicit_constructor(implicit, linker, val_ptr.get());
                    }
                }
            }
        }
        // setting iteration back
        if(!definition->generic_params.empty()) {
            definition->set_active_iteration(prev_itr);
        }
    } else {
        linker.error("couldn't find struct definition for struct name " + ref->representation(), this);
        return false;
    };
    return true;
}

ASTNode *StructValue::linked_node() {
    return definition;
}

bool StructValue::is_generic() {
    return linked_struct()->is_generic();
}

Value *StructValue::call_member(
        InterpretScope &scope,
        const std::string &name,
        std::vector<std::unique_ptr<Value>> &params
) {
    auto fn = definition->member(name);
    if (fn == nullptr) {
        scope.error("couldn't find member function by name " + name + " in a struct by name " + ref->representation());
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
    ptr->second->value = std::unique_ptr<Value>(value);
}

Value *StructValue::scope_value(InterpretScope &scope) {
    auto struct_value = new StructValue(
            std::unique_ptr<Value>(ref->copy()),
            std::unordered_map<std::string, std::unique_ptr<StructMemberInitializer>>(),
            std::vector<std::unique_ptr<BaseType>>(),
            definition,
            token,
            parent_node
    );
    declare_default_values(struct_value->values, scope);
    struct_value->values.reserve(values.size());
    for (const auto &value: values) {
        struct_value->values[value.first] = std::make_unique<StructMemberInitializer>(
                value.first,
                std::unique_ptr<Value>(value.second->value->initializer_value(scope)),
                struct_value,
                value.second->member

        );
    }
    struct_value->generic_list.reserve(generic_list.size());
    for(const auto& arg : generic_list) {
        struct_value->generic_list.emplace_back(arg->copy());
    }
    return struct_value;
}

void StructValue::declare_default_values(
        std::unordered_map<std::string,
        std::unique_ptr<StructMemberInitializer>> &into,
        InterpretScope &scope
) {
    Value* defValue;
    for (const auto &field: definition->variables) {
        defValue = field.second->default_value();
        if (into.find(field.second->name) == into.end() && defValue) {
            into[field.second->name]->value = std::unique_ptr<Value>(defValue->initializer_value(scope));
        }
    }
}

StructValue *StructValue::copy() {
    auto struct_value = new StructValue(
        std::unique_ptr<Value>(ref->copy()),
        std::unordered_map<std::string, std::unique_ptr<StructMemberInitializer>>(),
        std::vector<std::unique_ptr<BaseType>>(),
        definition,
        token,
        parent_node
    );
    struct_value->values.reserve(values.size());
    for (const auto &value: values) {
        struct_value->values[value.first] = value.second->copy_unique();
    }
    struct_value->generic_list.reserve(generic_list.size());
    for(const auto& arg : generic_list) {
        struct_value->generic_list.emplace_back(arg->copy());
    }
    return struct_value;
}

std::unique_ptr<BaseType> StructValue::create_type() {
    if(!definition) return nullptr;
    if(!definition->generic_params.empty()) {
       auto gen_type = std::make_unique<GenericType>(std::make_unique<LinkedType>(definition->name, definition, nullptr), generic_iteration);
       for(auto& type : generic_list) {
           gen_type->types.emplace_back(type->copy());
       }
       return gen_type;
    } else {
        auto type = std::make_unique<LinkedType>(ref->representation(), nullptr);
        type->linked = definition;
        return type;
    }
}

BaseType* StructValue::known_type() {
    if(!struct_type) {
        struct_type = create_type();
    }
    return struct_type.get();
}

hybrid_ptr<BaseType> StructValue::get_base_type() {
    if(!struct_type) {
        struct_type = create_type();
    }
    return hybrid_ptr<BaseType>{ struct_type.get(), false };
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
    return value->second->value.get();
}

StructMemberInitializer::StructMemberInitializer(
        std::string name,
        std::unique_ptr<Value> value,
        StructValue* struct_value,
        ASTNode* member
) : name(std::move(name)), value(std::move(value)), struct_value(struct_value), member(member) {

}

StructMemberInitializer* StructMemberInitializer::copy() {
    return new StructMemberInitializer(name, value->copy_unique(), struct_value, member);
}

CSTToken *StructMemberInitializer::cst_token() {
    return value->cst_token();
}

ASTNode* StructMemberInitializer::parent() {
    return struct_value->parent_node;
}