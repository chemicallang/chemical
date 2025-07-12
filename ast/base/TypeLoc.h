// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "BaseType.h"

/**
 * a wrapper of base type, that also contains the location
 * BaseType doesn't contain location because it gets reused
 */
class TypeLoc {
private:

    // Pointer to the underlying type (immutable)
    BaseType const *type_;
    // Source location where this type was written
    SourceLocation loc_;

public:

    /**
     * Construct from a BaseType pointer and a SourceLocation
     */
    constexpr TypeLoc(BaseType const *type, SourceLocation loc) noexcept
            : type_(type), loc_(loc) {}

    // Construct a null TypeLoc (no type, default location 0)
    constexpr TypeLoc(std::nullptr_t) noexcept
            : type_(nullptr), loc_(0) {}

    /**
     * Implicit conversion to BaseType pointer
     */
    constexpr operator BaseType*() const noexcept { return const_cast<BaseType*>(type_); }

    /**
     * Pointer-like access to BaseType
     */
    constexpr inline BaseType* operator->() const noexcept { return const_cast<BaseType*>(type_); }

    /**
     * Pointer-like access to BaseType
     */
    constexpr inline BaseType& operator*()  const noexcept { return const_cast<BaseType&>(*type_); }

    /**
     * nullptr comparisons
     */
    constexpr bool operator==(std::nullptr_t) const noexcept { return type_ == nullptr; }
    /**
     * nullptr comparisons
     */
    constexpr bool operator!=(std::nullptr_t) const noexcept { return type_ != nullptr; }

    /**
     * Access the stored SourceLocation
     */
    [[nodiscard]]
    constexpr inline SourceLocation getLocation() const noexcept { return loc_; }

    /**
     * get the encoded location
     */
    [[nodiscard]]
    constexpr inline SourceLocation encoded_location() const noexcept { return loc_; }

    /**
     * If you need the raw pointer explicitly
     */
    [[nodiscard]]
    constexpr inline BaseType const* getType() const noexcept { return type_; }

    /**
     * copy this type on allocator
     */
    constexpr inline TypeLoc copy(ASTAllocator& allocator) const noexcept {
        return { type_->copy(allocator), loc_ };
    }

};