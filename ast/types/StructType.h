// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "ast/base/BaseType.h"
#include <memory>
#include "ast/structures/VariablesContainer.h"

class StructType : public BaseType, public ASTNode, public VariablesContainer {
public:

    chem::string_view name;


    StructType(
        chem::string_view name,
        ASTNode* parent,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::StructType, parent, location), BaseType(BaseTypeKind::Struct, location), name(name) {

    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<StructType>()) StructType(name, parent(), BaseType::encoded_location());
    }

    bool equals(StructType *type);

    bool is_same(BaseType *type) final {
        return BaseType::kind() == type->kind() && equals(static_cast<StructType *>(type));
    }

    bool link(SymbolResolver &linker) override;

    ASTNode* child(const chem::string_view &name) override {
        return VariablesContainer::direct_child(name);
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