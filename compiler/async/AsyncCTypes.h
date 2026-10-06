// Copyright (c) Chemical Language Foundation 2026.
//
// Resolves the concrete `core::async` protocol types for one `FutureHandle<T>`
// by walking the handle's `vtbl` field into `FutureTable<T>` and its `poll`
// field. Shared by the C backends (legacy 2c and MIR).

#pragma once

class BaseType;
class FunctionDeclaration;

struct AsyncCTypes {
    BaseType* inner = nullptr;        // T
    BaseType* handle = nullptr;       // FutureHandle<T>
    BaseType* table = nullptr;        // FutureTable<T>
    BaseType* poll = nullptr;         // Poll<T>
    BaseType* context_ptr = nullptr;  // *mut Context
    bool ok = false;
};

AsyncCTypes resolve_async_c_types_from_handle(BaseType* rt);
AsyncCTypes resolve_async_c_types(FunctionDeclaration* decl);
