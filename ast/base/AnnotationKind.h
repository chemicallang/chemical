// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

enum class AnnotationKind {

    // Inline related annotations
    Inline,
    AlwaysInline,
    NoInline,
    InlineHint,
    // compiler inline is the strictest inline
    CompilerInline,
    // Size related annotations
    OptSize,
    MinSize,

    Extern, // it means declaration is available in other module
    CompilerInterface, // it means that struct is a compiler interface
    Cpp, // mangle the function name using C++ name mangling scheme
    Deprecated, // deprecated annotation

    NoInit, // structs that should not be initialized

    Implicit, // implicit constructor annotation, allows for automatic type conversion
    NoReturn,
    Constructor,
    // a move function is triggered on the object that has been moved (it's not like C++ move constructor which is called on the newly object being constructed)
    // it means to say, function that defines what happens when the object is moved and NOT how to construct an object from another object without copying everything
    Copy,
    Clear,
    Move,
    Delete,

    // when a struct is marked with this annotation, it means after calling
    // the move function, it can still be used, compiler won't complain about it
    UseAfterMove,

    // the function overrides another present above in a struct or interface
    Unsafe,
    Override,

    // structs or unions can be declared anonymous
    Anonymous,

    // this allows us to propagate symbols in 'using namespace' statement
    Propagate,

    IndexInlineStart=Inline,
    IndexInlineEnd=MinSize

};

std::string to_string(AnnotationKind kind);