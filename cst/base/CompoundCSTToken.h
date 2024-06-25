// Copyright (c) Qinetik 2024.

#pragma once

#include "CSTToken.h"
#include <utility>
#include <vector>
#include <memory>
#include "integration/common/Position.h"

class CompoundCSTToken : public CSTToken {
public:

    /**
     * cst tokens vecotr
     */
    std::vector<std::unique_ptr<CSTToken>> tokens;

    /**
     * the constructor for CompoundCSTToken
     */
    CompoundCSTToken(std::vector<std::unique_ptr<CSTToken>> tokens) : tokens(std::move(tokens)) {
        tokens.shrink_to_fit();
    }

};