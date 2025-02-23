// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/PointerType.h"

class AddrOfValue : public Value {
public:

    Value* value;
    PointerType _ptr_type;
    bool is_value_mutable = false;

    explicit AddrOfValue(
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::AddrOfValue, location), value(value), _ptr_type(nullptr, location) {

    }


    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    AddrOfValue *copy(ASTAllocator& allocator) final;

    BaseType* create_type(ASTAllocator& allocator) final;

    BaseType* known_type() final {
        return &_ptr_type;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    ASTNode *linked_node() final {
        return value->linked_node();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

    BaseTypeKind type_kind() const final {
        return BaseTypeKind::Pointer;
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

};