// Copyright (c) Qinetik 2024.

#pragma once

#include "ValueType.h"
#include <string>
#include "compiler/Codegen.h"
#ifdef COMPILER_BUILD
#include "llvm/IR/Type.h"
#endif
/**
 * BaseType is a base class for all the types there are
 */
class BaseType {
public:

    /**
     * constructor
     */
    BaseType() {

    }

    /**
     * this basically tells whether the given value type would satisfy this type
     * @param type
     * @return
     */
    virtual bool satisfies(ValueType type) const = 0;

    /**
     * representation is representation of the type
     * @return
     */
    virtual std::string representation() const = 0;

#ifdef COMPILER_BUILD
    /**
     * return the llvm type that corresponds to this base type
     * @param gen
     * @return
     */
    virtual llvm::Type* llvm_type(Codegen& gen) const = 0;
#endif

};