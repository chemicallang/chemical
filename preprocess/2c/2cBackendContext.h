// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/lab/BackendContext.h"

class ToCAstVisitor;

class ToCBackendContext : public BackendContext {
public:

    ToCAstVisitor* visitor;

    ToCBackendContext(ToCAstVisitor* visitor) : visitor(visitor) {

    }

    void mem_copy(Value *lhs, Value *rhs) final;

    bool supports(CompilerFeatureKind kind) final {
        switch(kind) {
            case CompilerFeatureKind::Float128:
                return false;
            default:
                return true;
        }
    }

};