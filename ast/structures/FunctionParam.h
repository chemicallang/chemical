// Copyright (c) Qinetik 2024.

#pragma once

#include <optional>
#include "BaseFunctionParam.h"

class FunctionType;

class FunctionParam : public BaseFunctionParam {
private:
    /**
     * has moved is used to indicate that an object at this location has moved
     * destructor is not called on moved objects, once moved, any attempt to access
     * this variable causes an error
     */
    bool has_moved = false;
    /**
     * has moved is used to track that var init statement has a assignment to it
     * if it has, during symbol resolution the assignment statement notifies this
     * var init statement
     */
    bool has_assignment = false;
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

    /**
 * check this variable has been moved
 */
    bool get_has_moved() {
        return has_moved;
    }

    /**
     * call it when this variable has been moved
     */
    void moved() {
        has_moved = true;
    }

    /**
     * call it when this variable should be unmoved
     */
    void unmove() {
        has_moved = false;
    }

    /**
     * get has assignment
     */
    bool get_has_assignment() {
        return has_assignment;
    }

    /**
     * assignment can be set to true
     */
    void set_has_assignment() {
        has_assignment = true;
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