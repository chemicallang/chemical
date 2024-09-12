// Copyright (c) Qinetik 2024.

#pragma once

#include <optional>
#include "BaseFunctionParam.h"

class FunctionType;

class FunctionParam : public BaseFunctionParam {
private:
    bool has_moved = false;
public:

    unsigned int index;
    std::unique_ptr<Value> defValue;
    CSTToken* token;

    FunctionParam(
            std::string name,
            std::unique_ptr<BaseType> type,
            unsigned int index,
            std::unique_ptr<Value> defValue,
            FunctionType* func_type,
            CSTToken* token
    );

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::FunctionParam;
    }

    ASTNode *parent() override {
        return (ASTNode*) func_type;
    }

    void moved() {
        has_moved = true;
    }

    void unmove() {
        has_moved = false;
    }

    [[nodiscard]]
    bool get_has_moved() const {
        return has_moved;
    }

    unsigned int calculate_c_or_llvm_index() override;

    void accept(Visitor *visitor) override;

    Value *holding_value() override {
        return defValue ? defValue.get() : nullptr;
    }

    BaseType *known_type() override {
        return type.get();
    }

    [[nodiscard]] FunctionParam *copy() const;

};