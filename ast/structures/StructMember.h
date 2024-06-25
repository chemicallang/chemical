// Copyright (c) Qinetik 2024.

#pragma once

#include <map>
#include "ast/base/Value.h"
#include "ast/base/AnnotableNode.h"

class StructMember : public AnnotableNode {
public:

    std::string name;

    std::unique_ptr<BaseType> type;

    std::optional<std::unique_ptr<Value>> defValue;

    StructMember(std::string name, std::unique_ptr<BaseType> type, std::optional<std::unique_ptr<Value>> defValue);

    void accept(Visitor *visitor) override;

    void declare_and_link(SymbolResolver &linker) override;

    ASTNode *child(const std::string &name) override;

    StructMember *as_struct_member() override {
        return this;
    }

    Value *holding_value() override {
        return defValue.has_value() ? defValue.value().get() : nullptr;
    }

    BaseType *holding_value_type() override {
        return type.get();
    }

#ifdef COMPILER_BUILD

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

#endif

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    ValueType value_type() const override;

    BaseTypeKind type_kind() const override;

};