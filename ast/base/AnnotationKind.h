// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

enum class AnnotationKind {

    Inline,
    AlwaysInline,
    NoInline,
    InlineHint,
    OptSize,
    MinSize,

    Destructor,

    IndexInlineStart=Inline,
    IndexInlineEnd=MinSize

};

std::string to_string(AnnotationKind kind);