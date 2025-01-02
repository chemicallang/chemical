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
    Value* defValue;
    SourceLocation location;
    const bool is_implicit;

    FunctionParam(
            chem::string_view name,
            BaseType* type,
            unsigned int index,
            Value* defValue,
            bool is_implicit,
            FunctionType* func_type,
            SourceLocation location
    ) : BaseFunctionParam(name, type, func_type), index(index),
        defValue(defValue), is_implicit(is_implicit), location(location) {}

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::FunctionParam;
    }

    ASTNode *parent() final {
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

    unsigned int calculate_c_or_llvm_index() final;

    void accept(Visitor *visitor) final;

    Value *holding_value() final {
        return defValue;
    }

    BaseType *known_type() final {
        return type;
    }

    [[nodiscard]]
    FunctionParam *copy(ASTAllocator& allocator) const;

    bool link_param_type(SymbolResolver &linker);

    void declare_and_link(SymbolResolver &linker) final;

};