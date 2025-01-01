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
    SourceLocation location;
    PointerType ptrType;


    CapturedVariable(
        std::string name,
        unsigned int index,
        bool capture_by_ref,
        SourceLocation location
    );

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::CapturedVariable;
    }

    ASTNode *parent() final {
        return nullptr;
    }

    void accept(Visitor *visitor) final {
        // no visit
    }

    void declare_and_link(SymbolResolver &linker) final;

    ASTNode *child(const std::string &name) final {
        return linked->child(name);
    }

    ASTNode *child(int index) final {
        return linked->child(index);
    }

    int child_index(const std::string &name) final {
        return linked->child_index(name);
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) final {
        return linked->add_child_index(gen, indexes, name);
    }

    llvm::Value *llvm_load(Codegen &gen) final;

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::FunctionType *llvm_func_type(Codegen &gen) final {
        return linked->llvm_func_type(gen);
    }

#endif

    BaseType* create_value_type(ASTAllocator &allocator) final;

    BaseType* known_type() final;

    [[nodiscard]]
    ValueType value_type() const final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};