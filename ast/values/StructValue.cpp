// Copyright (c) Qinetik 2024.

#include "StructValue.h"
#include "ast/structures/StructMember.h"
#include "compiler/SymbolResolver.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "IntValue.h"

void llvm_allocate_struct(llvm::Value *inst, Codegen& gen, StructValue* structValue) {
    for (const auto &value: structValue->values) {
        auto index = structValue->definition->child_index(value.first);
        if (index == -1) {
            gen.error("couldn't get struct child " + value.first + " in definition with name " + structValue->definition->name);
        } else {
            std::vector<llvm::Value*> idx {gen.builder->getInt32(0)};
            value.second->store_in_struct(gen, structValue, inst, idx, index);
        }
    }
}

llvm::AllocaInst *StructValue::llvm_allocate(Codegen &gen, const std::string &identifier) {
    auto allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr);
    llvm_allocate_struct(allocaInst, gen, this);
    return allocaInst;
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

llvm::Value *StructValue::llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) {
    // TODO make sure this argument corresponds to the struct
    auto structPassed = gen.current_function->getArg(0);
    llvm_allocate_struct(structPassed, gen, this);
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
        std::string structName,
        std::unordered_map<std::string, std::unique_ptr<Value>> values,
        StructDefinition *definition
) : structName(std::move(structName)), values(std::move(values)), definition(definition) {}

StructValue::StructValue(
        std::string structName,
        std::unordered_map<std::string, std::unique_ptr<Value>> values,
        StructDefinition *definition,
        InterpretScope &scope
) : structName(std::move(structName)), values(std::move(values)), definition(definition) {
    declare_default_values(this->values, scope);
}

uint64_t StructValue::byte_size(bool is64Bit) const {
    return definition->byte_size(is64Bit);
}

bool StructValue::primitive() {
    return false;
}

void StructValue::link(SymbolResolver &linker) {
    auto found = linker.find(structName);
    if(found) {
        auto struct_def = found->as_struct_def();
        if (struct_def) {
            definition = struct_def;
            unsigned i = 0;
            for (const auto &val: values) {
                val.second->link(linker, this, val.first);
            }
        } else {
            linker.error("given struct name is not a struct definition : " + structName);
        }
    } else {
        linker.error("couldn't find struct definition for struct name " + structName);
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
        scope.error("couldn't find member function by name " + name + " in a struct by name " + structName);
        return nullptr;
    }
#ifdef DEBUG
    if (definition->decl_scope == nullptr) {
        scope.error("declaration scope is nullptr for struct value");
    }
    if (!fn->body.has_value()) {
        scope.error("function doesn't have body in a struct " + name);
        return nullptr;
    }
#endif
    InterpretScope child(definition->decl_scope, scope.global, &fn->body.value(), definition);
    child.declare("this", this);
    auto value = fn->call(&scope, params, &child);
    return value;
}

void StructValue::set_child_value(const std::string &name, Value *value, Operation op) {
    auto ptr = values.find(name);
    if (ptr == values.end()) {
        std::cerr << "couldn't find child by name " + name + " in struct";
        return;
    }
    // this is probably gonna delete by itself
    delete ptr->second.release();
    ptr->second = std::unique_ptr<Value>(value);
}

Value *StructValue::evaluated_value(InterpretScope &scope) {
    return this;
}

Value *StructValue::initializer_value(InterpretScope &scope) {
    std::unordered_map<std::string, std::unique_ptr<Value>> copied(values.size());
    for (const auto &value: values) {
        copied[value.first] = std::unique_ptr<Value>(value.second->initializer_value(scope));
    }
    declare_default_values(copied, scope);
    return new StructValue(structName, std::move(copied), definition);
}

void StructValue::declare_default_values(
        std::unordered_map<std::string,
                std::unique_ptr<Value>> &into,
        InterpretScope &scope
) {
    for (const auto &field: definition->variables) {
        if (into.find(field.second->name) == into.end() && field.second->defValue.has_value()) {
            into[field.second->name] = std::unique_ptr<Value>(field.second->defValue.value()->initializer_value(scope));
        }
    }
}

Value *StructValue::copy() {
    std::unordered_map<std::string, std::unique_ptr<Value>> copied(values.size());
    for (const auto &value: values) {
        copied[value.first] = std::unique_ptr<Value>(value.second->copy());
    }
    return new StructValue(structName, std::move(copied), definition);
}

std::unique_ptr<BaseType> StructValue::create_type() const {
    auto type = std::make_unique<ReferencedType>(structName);
    type->linked = definition;
    return type;
}

Value *StructValue::child(InterpretScope &scope, const std::string &name) {
    auto value = values.find(name);
    if (value == values.end()) return nullptr;
    return value->second.get();
}

std::string StructValue::representation() const {
    std::string rep(structName + " {\n");
    unsigned i = 0;
    for (const auto &value: values) {
        rep.append(value.first);
        rep.append(" : ");
        rep.append(value.second->representation());
        if (i < values.size() - 1) rep.append(",\n");
        i++;
    }
    rep.append("\n}");
    return rep;
}

StructValue *StructValue::as_struct() {
    return this;
}