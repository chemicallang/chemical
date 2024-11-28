// Copyright (c) Qinetik 2024.

#pragma once

#include "parser/Parser.h"
#include <unordered_map>

const std::unordered_map<std::string_view, AnnotationModifierFn> AnnotationModifiers = {
//        { "cbi:global", [](Lexer *lexer, CSTToken* token) -> void {

//        }},
};