// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class CapturedVariable : public ASTNode {
public:

    bool capture_by_ref;
    std::string name;
    unsigned int index;
    LambdaFunction *lambda;
    ASTNode *linked;


    CapturedVariable(std::string name, unsigned int index, bool capture_by_ref) : name(std::move(name)), index(index), capture_by_ref(capture_by_ref) {

    }

    void accept(Visitor *visitor) override {
        // no visit
    }

    CapturedVariable *as_captured_var() override {
        return this;
    }

    void declare_and_link(SymbolResolver &linker) override;

    ASTNode *child(const std::string &name) override {
        return linked->child(name);
    }

    ASTNode *child(int index) override {
        return linked->child(index);
    }

    int child_index(const std::string &name) override {
        return linked->child_index(name);
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override {
        return linked->add_child_index(gen, indexes, name);
    }

    llvm::Value *llvm_load(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override {
        return linked->llvm_func_type(gen);
    }

    llvm::Type *llvm_elem_type(Codegen &gen) override {
        return linked->llvm_elem_type(gen);
    }

#endif

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    ValueType value_type() const override;

    BaseTypeKind type_kind() const override;

};