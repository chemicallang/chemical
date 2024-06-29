// Copyright (c) Qinetik 2024.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"

class UnnamedStruct : public BaseDefMember, public VariablesContainer {
public:

    explicit UnnamedStruct(
        std::string name
    );

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void declare_and_link(SymbolResolver &linker) override;

    // TODO destructor support for struct
    bool requires_destructor() override {
        return false;
    }

    ASTNode *child(const std::string &name) override {
        return VariablesContainer::child_def_member(name);
    }

    uint64_t byte_size(bool is64Bit) override {
        return total_byte_size(is64Bit);
    }

    hybrid_ptr<BaseType> get_value_type() override;

    UnnamedStruct *as_unnamed_struct() override {
        return this;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    bool add_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const std::string &name
    ) override {
        return VariablesContainer::llvm_struct_child_index(gen, indexes, name);
    }

#endif

    ValueType value_type() const override {
        return ValueType::Struct;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Struct;
    }

};