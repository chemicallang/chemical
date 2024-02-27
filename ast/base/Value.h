// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

/**
 * @brief Base class for all values in the AST.
 */
class Value {
public:
    /**
     * @brief Construct a new Value object.
     */
    Value() = default;

    /**
     * @brief Destroy the Value object.
     */
    virtual ~Value() = default;
};