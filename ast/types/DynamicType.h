// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "LinkedType.h"

class DynamicType : public BaseType {
public:

    enum AutoTrait : uint8_t {
        AutoTraitNone = 0,
        AutoTraitSend = 1u << 0,
        AutoTraitSync = 1u << 1,
    };

    BaseType* referenced;

    /** Explicit `dyn I + Send` / `dyn I + Sync` bounds (compile-time only). */
    uint8_t auto_traits = AutoTraitNone;

    /**
     * constructor
     */
    constexpr DynamicType(BaseType* referenced, uint8_t auto_traits = AutoTraitNone)
        : referenced(referenced), auto_traits(auto_traits), BaseType(BaseTypeKind::Dynamic) {

    }

    bool has_auto_trait(AutoTrait trait) const {
        return (auto_traits & static_cast<uint8_t>(trait)) != 0;
    }

    void add_auto_trait(AutoTrait trait) {
        auto_traits |= static_cast<uint8_t>(trait);
    }

    bool is_same(BaseType* type) final {
        return type->kind() == BaseTypeKind::Dynamic &&
               ((DynamicType*) type)->auto_traits == auto_traits &&
               ((DynamicType*) type)->referenced->is_same(referenced);
    }

    bool satisfies(BaseType *type) final;

    [[nodiscard]]
    DynamicType* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<DynamicType>()) DynamicType(
            referenced->copy(allocator), auto_traits);
    }

    ASTNode* linked_node() final {
        return referenced->linked_node();
    }

    uint64_t byte_size(const TargetData& target) final;


};