// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#ifdef DEBUG

extern "C" void abort(void);

#define CHECK_CAST(expected) if(kind() != (expected)) abort()

#define CHECK_COND(expected) if(!(expected)) abort()

#else

#define CHECK_CAST(expected)

#define CHECK_COND(expected)

#define CHECK_DYN_CAST(Type) ((Type*) this)

#endif