// Copyright (c) Qinetik 2024.

#pragma once

#include "compiler/lab/BackendContext.h"

class ToCAstVisitor;

class ToCBackendContext : public BackendContext {
public:

    ToCAstVisitor* visitor;

    ToCBackendContext(ToCAstVisitor* visitor) : visitor(visitor) {

    }

    void mem_copy(Value *lhs, Value *rhs) override;

};