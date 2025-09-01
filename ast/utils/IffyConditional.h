// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"

struct IffyBase {
    bool is_id;
};

struct IffyCondId : public IffyBase {
    bool is_negative;
    chem::string_view value;
};

enum class IffyExprOp { And, Or };

struct IffyCondExpr : public IffyBase {
    IffyBase* left;
    IffyBase* right;
    IffyExprOp op;
};