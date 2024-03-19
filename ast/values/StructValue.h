// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include "ast/base/Value.h"

class StructValue : public Value {
public:

    StructValue(std::string structName, std::vector<std::pair<std::string, std::unique_ptr<Value>>> values)
            : structName(std::move(structName)), values(std::move(values)) {
        values.shrink_to_fit();
    }

    void prepare(InterpretScope &scope) {
        auto decl = scope.global->nodes.find(structName);
        if (decl == scope.global->nodes.end()) {
            scope.error("couldn't find struct declaration by name " + structName);
        } else if (decl->second->as_struct_def() == nullptr) {
            scope.error("declaration by name " + structName + " is not a struct");
        } else {
            definition = decl->second->as_struct_def();
        }
    }

    Value *evaluated_value(InterpretScope &scope) override {
        prepare(scope);
        return this;
    }

    Value *copy() const override {
        std::vector<std::pair<std::string, std::unique_ptr<Value>>> copied(values.size());
        for (const auto &value: values) {
            copied.emplace_back(value.first, value.second->copy());
        }
        return new StructValue(structName, std::move(copied));
    }

    Value *child(const std::string &name) override {
        for (const auto &value: values) {
            if (value.first == name) {
                return value.second.get();
            }
        }
        return nullptr;
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

    std::string structName;
    StructDefinition *definition;
    std::vector<std::pair<std::string, std::unique_ptr<Value>>> values;

#ifdef COMPILER_BUILD
    // TODO this arr value should be stored in code gen since its related to that
    llvm::AllocaInst* arr;
#endif

};