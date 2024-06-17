// Copyright (c) Qinetik 2024.

#pragma once

#include <optional>
#include "BaseFunctionParam.h"

class BaseFunctionType;

class FunctionParam : public BaseFunctionParam {
public:

    unsigned int index;
    std::optional<std::unique_ptr<Value>> defValue;

    FunctionParam(
            std::string name,
            std::unique_ptr<BaseType> type,
            unsigned int index,
            std::optional<std::unique_ptr<Value>> defValue,
            BaseFunctionType* func_type = nullptr
      );

    unsigned int calculate_c_or_llvm_index() override;

    void accept(Visitor *visitor) override;

    FunctionParam *as_func_param() override {
        return this;
    }

    Value *holding_value() override {
        return defValue.has_value() ? defValue.value().get() : nullptr;
    }

    BaseType *holding_value_type() override {
        return type.get();
    }

    FunctionParam *copy() const;

};