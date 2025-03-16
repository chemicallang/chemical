// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

double get_double_value(Value* value, ValueKind k);

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, double value, SourceLocation location);

Value* pack_by_kind(InterpretScope& scope, ValueKind kind, uint64_t value, SourceLocation location);

/**
 * This class is the base class for integer type value
 * except bool which could be considered an integer type but
 * doesn't inherit int num value
 */
class IntNumValue : public Value {
public:

    /**
     * create a int num value
     */
    static IntNumValue* create_number(
            ASTAllocator& alloc,
            unsigned int bitWidth,
            bool is_signed,
            uint64_t value,
            SourceLocation location
    );

    /**
     * constructor
     */
    inline constexpr IntNumValue(ValueKind k, SourceLocation loc) noexcept : Value(k, loc) {

    }

    /**
     * provide the number of bits used by this value
     */
    virtual unsigned int get_num_bits(bool is64Bit) = 0;

    /**
     * get number value
     */
    [[nodiscard]]
    virtual uint64_t get_num_value() const = 0;

    /**
     * return if this is a unsigned value
     */
    virtual bool is_unsigned() = 0;

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