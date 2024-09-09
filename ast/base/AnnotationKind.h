// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

enum class AnnotationKind {

    // Inline or Size related annotations
    Inline,
    AlwaysInline,
    NoInline,
    InlineHint,
    OptSize,
    MinSize,

    Extern, // it means declaration is available in other module
    CompTime, // functions that are compile time
    Cpp, // mangle the function name using C++ name mangling scheme

    NoInit, // structs that should not be initialized

    Implicit, // implicit constructor annotation, allows for automatic type conversion

    // constructor or de constructor allow functions to be called automatically
    DirectInit,
    Constructor,
    // a move function is triggered on the object that has been moved (it's not like C++ move constructor which is called on the newly object being constructed)
    // it means to say, function that defines what happens when the object is moved and NOT how to construct an object from another object without copying everything
    Move,
    Copy,
    Destructor,

    // the function overrides another present above in a struct or interface
    Override,

    // structs or unions can be declared anonymous
    Anonymous,

    // this allows us to propagate symbols in 'using namespace' statement
    Propagate,

    IndexInlineStart=Inline,
    IndexInlineEnd=MinSize

};

std::string to_string(AnnotationKind kind);