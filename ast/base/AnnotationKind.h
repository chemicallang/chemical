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

    CompTime, // functions that are compile time

    // constructor or de constructor allow functions to be called automatically
    Constructor,
    Destructor,

    // structs or unions can be declared anonymous
    Anonymous,

    IndexInlineStart=Inline,
    IndexInlineEnd=MinSize

};

std::string to_string(AnnotationKind kind);