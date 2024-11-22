// Copyright (c) Qinetik 2024.

#pragma once

#include "parser/Lexer.h"
#include <unordered_map>

const std::unordered_map<std::string, AnnotationModifierFn> AnnotationModifiers = {
//        { "cbi:global", [](Lexer *lexer, CSTToken* token) -> void {

//        }},
};