// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include "ordered_map.h"
#include "ast/structures/VariablesContainer.h"

class UnionType : public BaseType, public VariablesContainer {
public:

    UnionType() = default;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Union;
    }

    uint64_t byte_size(bool is64Bit) override {
        uint64_t size = 0;
        uint64_t previous;
        for(auto& mem : variables) {
            previous = mem.second->byte_size(is64Bit);
            if(previous > size) {
                size = previous;
            }
        }
        return size;
    }

    bool equals(UnionType *type) const {
        return type->byte_size(true) == const_cast<UnionType*>(this)->byte_size(true);
    }

    bool is_same(BaseType *type) const override {
        return kind() == type->kind() && equals(static_cast<UnionType *>(type));
    }

    virtual BaseType *copy() const {
        throw std::runtime_error("copy called on union type");
    }

    bool satisfies(ValueType type) const override {
        for(auto& member : variables) {
            auto mem_type = member.second->create_value_type();
            if(mem_type->satisfies(type)) {
                return true;
            }
        }
        return false;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

};