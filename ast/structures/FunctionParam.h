// Copyright (c) Qinetik 2024.

#pragma once

#include <optional>
#include "BaseFunctionParam.h"

class FunctionType;

class FunctionParam : public BaseFunctionParam {
public:

    unsigned int index;
    std::optional<std::unique_ptr<Value>> defValue;

    FunctionParam(
            std::string name,
            std::unique_ptr<BaseType> type,
            unsigned int index,
            std::optional<std::unique_ptr<Value>> defValue,
            FunctionType* func_type = nullptr
      );

    ASTNodeKind kind() override {
        return ASTNodeKind::FunctionParam;
    }

    ASTNode *parent() override {
        return (ASTNode*) func_type;
    }

    unsigned int calculate_c_or_llvm_index() override;

    void accept(Visitor *visitor) override;

    FunctionParam *as_func_param() override {
        return this;
    }

    Value *holding_value() override {
        return defValue.has_value() ? defValue.value().get() : nullptr;
    }

    BaseType *known_type() override {
        return type.get();
    }

    [[nodiscard]] FunctionParam *copy() const;

};