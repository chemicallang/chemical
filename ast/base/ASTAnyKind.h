// Copyright (c) Qinetik 2024.

#pragma once

#include <cstdint>

/**
 * @brief Enum class representing kind of ASTAny
 */
enum class ASTAnyKind : uint8_t {

    Value,
    Type,
    Node

};