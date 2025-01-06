// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include "ast/base/BaseType.h"
#include <memory>
#include "ast/structures/VariablesContainer.h"

class StructType : public BaseType, public ASTNode, public VariablesContainer {
public:

    chem::string_view name;
    ASTNode* parent_node;
    SourceLocation location;

    StructType(chem::string_view name, ASTNode* parent, SourceLocation location) : name(name), parent_node(parent), location(location) {

    }

    SourceLocation encoded_location() override {
        return location;
    }

    ASTNode* parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Struct;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::StructType;
    }

    [[nodiscard]]
    ValueType value_type() const {
        return ValueType::Struct;
    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<StructType>()) StructType(name, parent_node, location);
    }

    bool equals(StructType *type);

    bool is_same(BaseType *type) final {
        return BaseType::kind() == type->kind() && equals(static_cast<StructType *>(type));
    }

    bool satisfies(ValueType type) final {
        return type == ValueType::Struct;
    }

    bool link(SymbolResolver &linker) override;

    ASTNode* child(const chem::string_view &name) override {
        const auto found = variables.find(name);
        if(found != variables.end()) {
            return found->second;
        } else {
            return nullptr;
        }
    }

    int child_index(const chem::string_view &name) override {
        return VariablesContainer::variable_index(name, false);
    }

    ASTNode* linked_node() override {
        return this;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen);

    llvm::Type *llvm_param_type(Codegen &gen);

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index);

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &name) override {
        return llvm_struct_child_index(gen, indexes, name);
    }

#endif

};