// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

/**
 * a parameter is added to the function implicitly when it returns a struct
 * when user returns a struct the returning struct is copied into this implicitly
 * passed struct, with this value user can gain access to implicitly passed struct
 * and then modify that
 */
class RetStructParamValue : public Value {
public:

    CSTToken* token;

    explicit RetStructParamValue(CSTToken* token) : token(token) {

    }

    CSTToken* cst_token() final {
        return token;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    ValueKind val_kind() final {
        return ValueKind::RetStructParamValue;
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

//    hybrid_ptr<BaseType> get_base_type() final;

    BaseType* create_type(ASTAllocator& allocator) final;

    Value *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<RetStructParamValue>()) RetStructParamValue(token);
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::Pointer;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Pointer;
    }

};