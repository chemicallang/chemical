// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/types/PointerType.h"

class CapturedVariable : public ASTNode {
public:

    bool capture_by_ref;
    std::string name;
    unsigned int index;
    LambdaFunction *lambda;
    ASTNode *linked;
    CSTToken* token;
    PointerType ptrType;


    CapturedVariable(
        std::string name,
        unsigned int index,
        bool capture_by_ref,
        CSTToken* token
    );

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::CapturedVariable;
    }

    ASTNode *parent() override {
        return nullptr;
    }

    void accept(Visitor *visitor) override {
        // no visit
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

    BaseType* create_value_type(ASTAllocator &allocator) override;

    BaseType* known_type() override;

    [[nodiscard]]
    ValueType value_type() const override;

    [[nodiscard]]
    BaseTypeKind type_kind() const override;

};