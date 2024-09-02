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

    Api, // functions that are exposed to other modules
    Extern, // it means declaration is available in other module, @api is implied, also makes the declaration public by default
    CompTime, // functions that are compile time
    Cpp, // mangle the function name using C++ name mangling scheme

    NoInit, // structs that should not be initialized

    Implicit, // implicit constructor annotation, allows for automatic type conversion

    NotInC, // nodes that should not translated to C

    // constructor or de constructor allow functions to be called automatically
    DirectInit,
    Constructor,
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