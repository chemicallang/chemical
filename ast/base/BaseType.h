// Copyright (c) Qinetik 2024.

#pragma once

#include "ValueType.h"
#include <string>
#include "compiler/Codegen.h"
#include "compiler/ASTLinker.h"

#ifdef COMPILER_BUILD
#include "compiler/llvmfwd.h"
#endif

class PointerType;

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

    virtual BaseType* copy() const = 0;

    /**
     * a type, or a referenced type, can link itself with its definition
     */
    virtual void link(ASTLinker& linker) {
        // does nothing by default
    }

    /**
     * just return the linked node
     */
    virtual ASTNode* linked_node() {
        return nullptr;
    }

    /**
     * this basically tells whether the given value type would satisfy this type
     * @param type
     * @return
     */
    virtual bool satisfies(ValueType type) const = 0;

    /**
     * pointer type returns pointer type
     */
    virtual PointerType* pointer_type() {
        return nullptr;
    }

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

    virtual ~BaseType() = default;

};