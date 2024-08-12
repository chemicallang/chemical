// Copyright (c) Qinetik 2024.

#pragma once

#include <cstdint>

enum class AccessSpecifier : uint8_t {
    Private = 0,
    Public,
    Protected,
    Internal
};