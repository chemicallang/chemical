// Copyright (c) Chemical Language Foundation 2025.

#pragma once

enum class SymResScopeKind : int {
    /**
     * a global namespace is much like default scope, however
     * only a single global scope exists, It's never created by the user
     */
    Global,
    /**
     * a module is a collection of files, that export into an object files
     */
    Module,
    /**
     * a file scope is just like a default scope, however with minor
     * differences, a file scope helps also to distinguish between files
     */
    File,
    /**
     * a default scope, symbols can be declared and retrieved
     * the retrieval takes into account symbols declared in scopes above
     */
    Default,
};