// Copyright (c) Qinetik 2024.

#include "UnionValue.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/StructMember.h"
#include "compiler/SymbolResolver.h"
#include "ast/utils/ASTUtils.h"
#include "ast/types/GenericType.h"
#include "ast/types/ReferencedType.h"
#include "ast/values/DereferenceValue.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "IntValue.h"

void UnionValue::initialize_alloca(llvm::Value *inst, Codegen& gen) {
    const auto parent_type = llvm_type(gen);
    auto variable = definition->variable_type_index(value.first);
    if (variable.first == -1) {
        gen.error("couldn't get union child " + value.first + " in definition with name " + definition->name, this);
    } else {
        std::vector<llvm::Value*> idx {gen.builder->getInt32(0)};
        value.second->store_in_struct(gen, this, inst, parent_type, idx, 0, variable.second);
    }
}

llvm::AllocaInst *UnionValue::llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) {
    allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr);
    initialize_alloca(allocaInst, gen);
    return allocaInst;
}

llvm::Value *UnionValue::llvm_pointer(Codegen &gen) {
    return allocaInst;
}

void UnionValue::llvm_destruct(Codegen &gen, llvm::Value *givenAlloca) {
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

unsigned int UnionValue::store_in_struct(
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
        return index + 1;
    }
    const auto interface = expected_type ? expected_type->linked_dyn_interface() : nullptr;
    if(interface) {
        auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
        const auto struct_alloc = llvm_allocate(gen, "", nullptr);
        if(!gen.assign_dyn_obj(this, expected_type, elementPtr, struct_alloc)) {
            gen.error("couldn't assign dyn object struct " + representation() + " to expected type " + expected_type->representation() + " in parent " + parent->representation(), this);
        }
    } else {
        const auto parent_type = llvm_type(gen);
        auto child_index = definition->variable_type_index(value.first);
        auto currIndex = index + child_index.first;
        value.second->store_in_struct(gen, this, allocated, parent_type, idxList, currIndex, child_index.second);
    }
    return index + 1;
}

unsigned int UnionValue::store_in_array(
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
        auto currIndex = definition->variable_type_index(value.first);
        value.second->store_in_array(gen, parent, ptr, idxList, currIndex.first, currIndex.second);
    }
    return index + 1;
}

llvm::Value *UnionValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    throw std::runtime_error("cannot allocate a struct without an identifier");
}

llvm::Value *UnionValue::llvm_assign_value(Codegen &gen, Value *lhs) {
    const auto known_type = lhs->known_type();
    if(known_type->kind() == BaseTypeKind::Dynamic && known_type->linked_node()->as_interface_def()) {
        return llvm_allocate(gen, "", nullptr);
    } else if(lhs->as_deref_value()) {
        if(!definition->destructor_func() && (!definition->has_constructor() || definition->is_direct_init)) {
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

llvm::Value *UnionValue::llvm_arg_value(Codegen &gen, FunctionCall *call, unsigned int index) {
    return llvm_allocate(gen, "", nullptr);
}

llvm::Value *UnionValue::llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) {
    // TODO make sure this argument corresponds to the struct
    auto structPassed = gen.current_function->getArg(0);
    initialize_alloca(structPassed, gen);
    return nullptr;
}

llvm::Type *UnionValue::llvm_elem_type(Codegen &gen) {
    throw std::runtime_error("llvm_elem_type: called on a struct");
}

llvm::Type *UnionValue::llvm_type(Codegen &gen) {
    if(definition->generic_params.empty()) {
        return definition->llvm_type(gen);
    } else {
        return definition->llvm_type(gen, generic_iteration);
    }
}

bool UnionValue::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return definition->add_child_index(gen, indexes, name);
}

#endif

//UnionValue::UnionValue(
//        std::unique_ptr<Value> ref,
//        std::pair<std::string, std::unique_ptr<Value>> value,
//        UnionDef *definition
//) : ref(std::move(ref)), values(std::move(values)), definition(definition) {}

UnionValue::UnionValue(
        std::unique_ptr<Value> ref,
        std::pair<std::string, std::unique_ptr<Value>> value,
        std::vector<std::unique_ptr<BaseType>> generic_list,
        UnionDef *definition,
        CSTToken* token
) : ref(std::move(ref)), value(std::move(value)), definition(definition), generic_list(std::move(generic_list)), token(token) {}

UnionValue::UnionValue(
        std::unique_ptr<Value> ref,
        std::pair<std::string, std::unique_ptr<Value>> value,
        UnionDef *definition,
        InterpretScope &scope,
        CSTToken* token
) : ref(std::move(ref)), value(std::move(value)), definition(definition), token(token) {

}

uint64_t UnionValue::byte_size(bool is64Bit) {
    if(definition->generic_params.empty()) {
        return definition->byte_size(is64Bit);
    } else {
        return definition->byte_size(is64Bit, generic_iteration);
    }
}

bool UnionValue::primitive() {
    return false;
}

BaseDefMember* UnionValue::child_member(const std::string& name) {
    return definition->child_member(name);
}

bool UnionValue::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    ref->link(linker, ref);
    auto found = ref->linked_node();
    if(found) {
        auto struct_def = found->as_union_def();
        if (struct_def) {
            definition = struct_def;
            if(definition->has_constructor() && !definition->is_direct_init) {
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
                generic_iteration = definition->register_generic_args(linker, generic_list);
                definition->set_active_iteration(generic_iteration);
            }
            // linking values
            value.second->link(linker, value.second);
            auto member = definition->variables.find(value.first);
            if(definition->variables.end() != member) {
                const auto mem_type = member->second->get_value_type();
                auto implicit = mem_type->implicit_constructor_for(value.second.get());
                if(implicit) {
                    if(linker.preprocess) {
                        value.second = call_with_arg(implicit, std::move(value.second), linker);
                    } else {
                        link_with_implicit_constructor(implicit, linker, value.second.get());
                    }
                }
            }
            // setting iteration back
            if(!definition->generic_params.empty()) {
                definition->set_active_iteration(prev_itr);
            }
        } else {
            linker.error("given struct name is not a struct definition : " + ref->representation(), this);
            return false;
        }
    } else {
        linker.error("couldn't find struct definition for struct name " + ref->representation(), this);
        return false;
    };
    return true;
}

ASTNode *UnionValue::linked_node() {
    return definition;
}

bool UnionValue::is_generic() {
    return definition->is_generic();
}

Value *UnionValue::call_member(
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

void UnionValue::set_child_value(const std::string &name, Value *init, Operation op) {
    value.second = std::unique_ptr<Value>(init);
}

Value *UnionValue::scope_value(InterpretScope &scope) {
    auto struct_value = new UnionValue(
            std::unique_ptr<Value>(ref->copy()),
            std::pair<std::string, std::unique_ptr<Value>>(),
            std::vector<std::unique_ptr<BaseType>>(),
            definition,
            token
    );
    struct_value->value.second = std::unique_ptr<Value>(value.second->initializer_value(scope));
    struct_value->generic_list.reserve(generic_list.size());
    for(const auto& arg : generic_list) {
        struct_value->generic_list.emplace_back(arg->copy());
    }
    return struct_value;
}

void UnionValue::declare_default_values(
        std::unordered_map<std::string,
        std::unique_ptr<Value>> &into,
        InterpretScope &scope
) {
    Value* defValue;
    for (const auto &field: definition->variables) {
        defValue = field.second->default_value();
        if (into.find(field.second->name) == into.end() && defValue) {
            into[field.second->name] = std::unique_ptr<Value>(defValue->initializer_value(scope));
        }
    }
}

UnionValue *UnionValue::copy() {
    auto struct_value = new UnionValue(
        std::unique_ptr<Value>(ref->copy()),
        std::pair<std::string, std::unique_ptr<Value>>(),
        std::vector<std::unique_ptr<BaseType>>(),
        definition,
        token
    );
    struct_value->value.second = std::unique_ptr<Value>(value.second->copy());
    struct_value->generic_list.reserve(generic_list.size());
    for(const auto& arg : generic_list) {
        struct_value->generic_list.emplace_back(arg->copy());
    }
    return struct_value;
}

std::unique_ptr<BaseType> UnionValue::create_type() {
    if(!definition) return nullptr;
    if(!definition->generic_params.empty()) {
       auto gen_type = std::make_unique<GenericType>(std::make_unique<ReferencedType>(definition->name, definition, nullptr), generic_iteration);
       for(auto& type : generic_list) {
           gen_type->types.emplace_back(type->copy());
       }
       return gen_type;
    } else {
        auto type = std::make_unique<ReferencedType>(ref->representation(), nullptr);
        type->linked = definition;
        return type;
    }
}

BaseType* UnionValue::known_type() {
    if(!struct_type) {
        struct_type = create_type();
    }
    return struct_type.get();
}

hybrid_ptr<BaseType> UnionValue::get_base_type() {
    if(!struct_type) {
        struct_type = create_type();
    }
    return hybrid_ptr<BaseType>{ struct_type.get(), false };
}

Value *UnionValue::child(InterpretScope &scope, const std::string &name) {
    return value.second.get();
}