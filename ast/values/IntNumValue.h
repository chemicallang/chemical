// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/types/IntNType.h"

class TypeBuilder;

double get_double_value(Value* value, ValueKind k);

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, double value, SourceLocation location);

Value* pack_by_kind(InterpretScope& scope, IntNTypeKind kind, uint64_t value, SourceLocation location);

/**
 * This class is the base class for integer type value
 * except bool which could be considered an integer type but
 * doesn't inherit int num value
 */
class IntNumValue : public Value {
public:

    /**
     * value can be stored in these 32 bits
     */
    uint64_t value;

    /**
     * create a int num value
     */
    static IntNumValue* create_number(
            ASTAllocator& alloc,
            TypeBuilder& typeBuilder,
            unsigned int bitWidth,
            bool is_signed,
            uint64_t value,
            SourceLocation location
    );

    /**
     * constructor
     */
    inline constexpr IntNumValue(
            uint64_t value,
            SourceLocation loc
    ) noexcept : Value(ValueKind::IntN, loc), value(value) {

    }

    /**
     * constructor
     */
    inline constexpr IntNumValue(
            uint64_t value,
            IntNType* type,
            SourceLocation loc
    ) noexcept : Value(ValueKind::IntN, type, loc), value(value) {

    }

    /**
     * get the intn type of the value
     */
    [[nodiscard]]
    inline IntNType* getType() noexcept { return (IntNType*) Value::getType(); }

    /**
     * get the bytesize for this intn value
     */
    [[nodiscard]]
    inline uint64_t byte_size(bool is64Bit) final { return getType()->byte_size(is64Bit); }

    /**
     * provide the number of bits used by this value
     */
    [[nodiscard]]
    inline unsigned int get_num_bits(bool is64Bit) noexcept { return getType()->num_bits(is64Bit); }

    /**
     * get number value
     */
    [[nodiscard]]
    inline uint64_t get_num_value() const noexcept { return value; }

    /**
     * return if this is a unsigned value
     */
    [[nodiscard]]
    inline bool is_unsigned() noexcept { return getType()->is_unsigned(); }

    /**
     * copy this value
     */
    [[nodiscard]]
    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<IntNumValue>()) IntNumValue(
            value, getType(), encoded_location()
        );
    }

#ifdef COMPILER_BUILD

    /**
     * would provide llvm type based on num bits provided in getNumBits
     */
    llvm::Type *llvm_type(Codegen &gen);

    /**
     * would provide llvm value based on num bits provided in getNumBits
     */
    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type);

#endif


};