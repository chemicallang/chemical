// Copyright (c) Qinetik 2024.

#include "StructValue.h"
#include "ast/structures/StructMember.h"
#include "compiler/SymbolResolver.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "IntValue.h"

void StructValue::initialize_alloca(llvm::Value *inst, Codegen& gen) {
    for (const auto &value: values) {
        auto index = definition->child_index(value.first);
        if (index == -1) {
            gen.error("couldn't get struct child " + value.first + " in definition with name " + definition->name);
        } else {
            std::vector<llvm::Value*> idx {gen.builder->getInt32(0)};
            value.second->store_in_struct(gen, this, inst, idx, index);
        }
    }
}

llvm::AllocaInst *StructValue::llvm_allocate(Codegen &gen, const std::string &identifier) {
    allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr);
    initialize_alloca(allocaInst, gen);
    return allocaInst;
}

llvm::Value *StructValue::llvm_pointer(Codegen &gen) {
    return allocaInst;
}

void StructValue::llvm_destruct(Codegen &gen, llvm::Value *givenAlloca) {
    definition->llvm_destruct(gen, givenAlloca);
}

unsigned int StructValue::store_in_struct(
        Codegen &gen,
        StructValue *parent,
        llvm::Value *allocated,
        std::vector<llvm::Value*> idxList,
        unsigned int index
) {
    for (const auto &value: values) {
        auto currIndex = index + definition->child_index(value.first);
        if (index == -1) {
            gen.error(
                    "couldn't get embedded struct child " + value.first + " in definition of name " + definition->name +
                    " with parent of name " + parent->definition->name);
        } else {
            value.second->store_in_struct(gen, this, allocated, {}, currIndex);
        }
    }
    return index + values.size();
}

unsigned int StructValue::store_in_array(
        Codegen &gen,
        ArrayValue *parent,
        llvm::AllocaInst *ptr,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    idxList.emplace_back(gen.builder->getInt32(index));
    for (const auto &value: values) {
        auto currIndex = definition->child_index(value.first);
        if (index == -1) {
            gen.error(
                    "couldn't get embedded struct child " + value.first + " in definition of name " + definition->name +
                    " with parent of name " + definition->name);
        } else {
            value.second->store_in_array(gen, parent, ptr, idxList, currIndex);
        }
    }
    return index + 1;
}

llvm::Value *StructValue::llvm_value(Codegen &gen) {
    throw std::runtime_error("cannot allocate a struct without an identifier");
}

llvm::Value *StructValue::llvm_arg_value(Codegen &gen, FunctionCall *call, unsigned int index) {
    return llvm_allocate(gen, "");
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
    return definition->llvm_type(gen);
}

bool StructValue::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return definition->add_child_index(gen, indexes, name);
}

#endif

StructValue::StructValue(
        std::unique_ptr<Value> ref,
        std::unordered_map<std::string, std::unique_ptr<Value>> values,
        StructDefinition *definition
) : ref(std::move(ref)), values(std::move(values)), definition(definition) {}

StructValue::StructValue(
        std::unique_ptr<Value> ref,
        std::unordered_map<std::string, std::unique_ptr<Value>> values,
        StructDefinition *definition,
        InterpretScope &scope
) : ref(std::move(ref)), values(std::move(values)), definition(definition) {
    declare_default_values(this->values, scope);
}

uint64_t StructValue::byte_size(bool is64Bit) {
    return definition->byte_size(is64Bit);
}

bool StructValue::primitive() {
    return false;
}

void StructValue::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    ref->link(linker, ref);
    auto found = ref->linked_node();
    if(found) {
        auto struct_def = found->as_struct_def();
        if (struct_def) {
            definition = struct_def;
            unsigned i = 0;
            for (const auto &val: values) {
                val.second->link(linker, this, val.first);
            }
        } else {
            linker.error("given struct name is not a struct definition : " + ref->representation());
        }
    } else {
        linker.error("couldn't find struct definition for struct name " + ref->representation());
    };
}

ASTNode *StructValue::linked_node() {
    return definition;
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
    ptr->second = std::unique_ptr<Value>(value);
}

Value *StructValue::scope_value(InterpretScope &scope) {
    std::unordered_map<std::string, std::unique_ptr<Value>> copied(values.size());
    declare_default_values(copied, scope);
    for (const auto &value: values) {
        copied[value.first] = std::unique_ptr<Value>(value.second->initializer_value(scope));
    }
    return new StructValue(std::unique_ptr<Value>(ref->copy()), std::move(copied), definition);
}

void StructValue::declare_default_values(
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

Value *StructValue::copy() {
    std::unordered_map<std::string, std::unique_ptr<Value>> copied(values.size());
    for (const auto &value: values) {
        copied[value.first] = std::unique_ptr<Value>(value.second->copy());
    }
    return new StructValue(std::unique_ptr<Value>(ref->copy()), std::move(copied), definition);
}

std::unique_ptr<BaseType> StructValue::create_type() {
    auto type = std::make_unique<ReferencedType>(ref->representation());
    type->linked = definition;
    return type;
}

hybrid_ptr<BaseType> StructValue::get_base_type() {
    auto type = new ReferencedType(ref->representation());
    type->linked = definition;
    return hybrid_ptr<BaseType> { type };
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
    return value->second.get();
}

StructValue *StructValue::as_struct() {
    return this;
}