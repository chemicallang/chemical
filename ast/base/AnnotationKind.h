// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

enum class AnnotationKind {

    Implicit, // implicit constructor annotation, allows for automatic type conversion
    NoReturn,
    Constructor,
    // a move function is triggered on the object that has been moved (it's not like C++ move constructor which is called on the newly object being constructed)
    // it means to say, function that defines what happens when the object is moved and NOT how to construct an object from another object without copying everything
    Copy,
    Clear,
    Move,
    Delete,

    // the function overrides another present above in a struct or interface
    Unsafe,
    Override,

    // this allows us to propagate symbols in 'using namespace' statement
    Propagate,

};

std::string to_string(AnnotationKind kind);