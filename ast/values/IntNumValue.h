// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

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
            CSTToken* token = nullptr
    );

    /**
     * provide the number of bits used by this value
     */
    virtual unsigned int get_num_bits() = 0;

    /**
     * get number value
     */
    [[nodiscard]]
    virtual int64_t get_num_value() const = 0;

    /**
     * return if this is a unsigned value
     */
    virtual bool is_unsigned() = 0;

    /**
     * returns true for is int n
     */
    bool is_int_n() override {
        return true;
    }

#ifdef COMPILER_BUILD

    /**
     * would provide llvm type based on num bits provided in getNumBits
     */
    llvm::Type *llvm_type(Codegen &gen) override;

    /**
     * would provide llvm value based on num bits provided in getNumBits
     */
    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif


};