// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/structures/StructDefinition.h"

class StructValue : public Value {
public:

    StructValue(std::string structName, std::vector<std::pair<std::string, std::unique_ptr<Value>>> values)
            : structName(std::move(structName)), values(std::move(values)) {
        values.shrink_to_fit();
    }

    bool primitive() override {
        return false;
    }

    void prepare(InterpretScope &scope) {
        auto decl = scope.find_node(structName);
        if (decl == nullptr) {
            scope.error("couldn't find struct declaration by name " + structName);
        } else if (decl->as_struct_def() == nullptr) {
            scope.error("declaration by name " + structName + " is not a struct");
        } else {
            definition = decl->as_struct_def();
        }
    }

    Value* call_member(InterpretScope& scope, const std::string &name, std::vector<std::unique_ptr<Value>>& params) override {
        prepare(scope);
        if(definition == nullptr) {
            scope.error("couldn't find struct definition by name " + structName + ", when querying member function by name " + name);
            return nullptr;
        } else {
            auto fn = definition->member(name);
            if(fn == nullptr) {
                scope.error("couldn't find member function by name " + name + " in a struct by name " + structName);
                return nullptr;
            }
#ifdef DEBUG
            if(definition->decl_scope == nullptr) {
                scope.error("declaration scope is nullptr for struct value");
            }
            if(!fn->body.has_value()) {
                scope.error("function doesn't have body in a struct " + name);
                return nullptr;
            }
#endif
            InterpretScope child(definition->decl_scope, scope.global, &fn->body.value(), definition);
            child.declare("this", this);
            auto value = fn->call(&scope, params, &child);
            return value;
        }
    }

    void set_child_value(const std::string &name, Value *value, Operation op) override {
        auto i = index(name);
        if (i == -1) {
            std::cerr << "couldn't find child by name " + name + " in struct";
            return;
        }
        // this is probably gonna delete by itself
        delete values[i].second.release();
        values[i] = std::pair(values[i].first, std::unique_ptr<Value>(value));
    }

    Value *evaluated_value(InterpretScope &scope) override {
        prepare(scope);
        return this;
    }

    Value * initializer_value(InterpretScope &scope) override {
        prepare(scope);
        return copy();
    }

    Value *copy() const override {
        std::vector<std::pair<std::string, std::unique_ptr<Value>>> copied(values.size());
        for (unsigned i = 0; i < values.size(); ++i) {
            copied[i] = std::make_pair(values[i].first, std::unique_ptr<Value>(values[i].second->copy()));
        }
        return new StructValue(structName, std::move(copied));
    }

    unsigned int index(const std::string &name) {
        unsigned i = 0;
        for (const auto &value: values) {
            if (value.first == name) {
                return i;
            }
            i++;
        }
        return -1;
    }

    Value *child(const std::string &name) override {
        auto i = index(name);
        if (i == -1) return nullptr;
        return values[i].second.get();
    }

#ifdef COMPILER_BUILD
    llvm::Value * llvm_pointer(Codegen &gen) override {
        return arr;
    }

    void llvm_allocate(Codegen &gen, const std::string &identifier) override {
        auto arrType = llvm_type(gen);
        arr = gen.builder->CreateAlloca(arrType, nullptr, identifier);
        // filling array with values
        for (size_t i = 0; i < values.size(); ++i) {
            auto index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), i);
            auto elemPtr = gen.builder->CreateGEP(arrType, arr, {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0), index});
            auto elemValue = values[i]->llvm_value(gen);
            gen.builder->CreateStore(elemValue, elemPtr);
        }
    }
    llvm::Value * llvm_value(Codegen &gen) override {
        throw std::runtime_error("cannot allocate an array without an identifier");
    }

    llvm::Type * llvm_elem_type(Codegen &gen) override {
        llvm::Type* elementType;
        if(values.empty()) {
            // get empty array type from the user
            elementType = elemType.value()->llvm_type(gen);
        } else {
            elementType = values[0]->llvm_type(gen);
        }
        return elementType;
    }

    llvm::Type * llvm_type(Codegen &gen) override {
        return llvm::ArrayType::get(llvm_elem_type(gen), array_size());
    }
#endif

    std::string representation() const override {
        std::string rep(structName + " {\n");
        unsigned i = 0;
        while (i < values.size()) {
            rep.append(values[i].first);
            rep.append(" : ");
            rep.append(values[i].second->representation());
            if (i < values.size() - 1) rep.append(",\n");
            i++;
        }
        rep.append("\n}");
        return rep;
    }

    StructValue* as_struct() override {
        return this;
    }

    ValueType value_type() const override {
        return ValueType::Struct;
    }

    std::string structName;
    StructDefinition *definition;
    std::vector<std::pair<std::string, std::unique_ptr<Value>>> values;

#ifdef COMPILER_BUILD
    // TODO this arr value should be stored in code gen since its related to that
    llvm::AllocaInst* arr;
#endif

};