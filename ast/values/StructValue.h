// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/VarInit.h"

class StructValue : public Value {
public:

    StructValue(std::string structName, std::unordered_map<std::string, std::unique_ptr<Value>> values, StructDefinition* definition = nullptr)
            : structName(std::move(structName)), values(std::move(values)), definition(definition) {}

    StructValue(std::string structName, std::unordered_map<std::string, std::unique_ptr<Value>> values, StructDefinition* definition, InterpretScope& scope)
            : structName(std::move(structName)), values(std::move(values)), definition(definition) {
        declare_default_values(this->values, scope);
    }

    bool primitive() override {
        return false;
    }

    bool prepare(InterpretScope &scope) {
        if (definition != nullptr) return true;
        auto decl = scope.find_node(structName);
        if (decl == nullptr) {
            scope.error("couldn't find struct declaration by name " + structName);
        } else if (decl->as_struct_def() == nullptr) {
            scope.error("declaration by name " + structName + " is not a struct");
        } else {
            definition = decl->as_struct_def();
            return true;
        }
        return false;
    }

    Value *
    call_member(InterpretScope &scope, const std::string &name, std::vector<std::unique_ptr<Value>> &params) override {
        if (!prepare(scope)) return nullptr;
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

    void set_child_value(const std::string &name, Value *value, Operation op) override {
        auto ptr = values.find(name);
        if (ptr == values.end()) {
            std::cerr << "couldn't find child by name " + name + " in struct";
            return;
        }
        // this is probably gonna delete by itself
        delete ptr->second.release();
        ptr->second = std::unique_ptr<Value>(value);
    }

    Value *evaluated_value(InterpretScope &scope) override {
        prepare(scope);
        return this;
    }

    Value *initializer_value(InterpretScope &scope) override {
        return copy(scope);
    }

    void declare_default_values(std::unordered_map<std::string, std::unique_ptr<Value>>& into, InterpretScope& scope) {
        for (const auto &field: definition->variables) {
            if (into.find(field.second->identifier) == into.end() && field.second->value.has_value()) {
                into[field.second->identifier] = std::unique_ptr<Value>(field.second->value.value()->initializer_value(scope));
            }
        }
    }

    Value *copy(InterpretScope &scope) override {
        if (!prepare(scope)) return nullptr;
        std::unordered_map<std::string, std::unique_ptr<Value>> copied(values.size());
        for (const auto &value: values) {
            copied[value.first] = std::unique_ptr<Value>(value.second->initializer_value(scope));
        }
        declare_default_values(copied, scope);
        return new StructValue(structName, std::move(copied), definition);
    }

    Value *child(InterpretScope &scope, const std::string &name) override {
        auto value = values.find(name);
        if (value == values.end()) return nullptr;
        return value->second.get();
    }

#ifdef COMPILER_BUILD
    llvm::Value * llvm_pointer(Codegen &gen) override;

    void llvm_allocate(Codegen &gen, const std::string &identifier) override;

    llvm::Value * llvm_value(Codegen &gen) override;

    llvm::Type * llvm_elem_type(Codegen &gen) override;

    llvm::Type * llvm_type(Codegen &gen) override;
#endif

    std::string representation() const override {
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

    StructValue *as_struct() override {
        return this;
    }

    ValueType value_type() const override {
        return ValueType::Struct;
    }

    std::string structName;
    StructDefinition *definition;
    std::unordered_map<std::string, std::unique_ptr<Value>> values;

#ifdef COMPILER_BUILD
    // TODO this arr value should be stored in code gen since its related to that
    llvm::AllocaInst* allocaInst;
#endif

};