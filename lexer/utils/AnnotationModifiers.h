// Copyright (c) Qinetik 2024.

#pragma once

#include "lexer/Lexer.h"

std::string annotation_str_arg(unsigned index, CSTToken* token);

const std::unordered_map<std::string, AnnotationModifierFn> AnnotationModifiers = {
        { "cbi:global", [](Lexer *lexer, CSTToken* token) -> void {
            if(!lexer->binder) return;
            lexer->isCBICollecting = true;
            lexer->isCBICollectingGlobal = true;
            lexer->current_cbi = annotation_str_arg(0, token);;
        }},
        { "cbi:create", [](Lexer *lexer, CSTToken* token) -> void {
            if(!lexer->binder) return;
            auto n = annotation_str_arg(0, token);
            if(n.empty()) {
                lexer->error("cbi:create called with invalid parameters : " + n);
                return;
            }
            lexer->binder->create_cbi(n);
        }},
        { "cbi:import", [](Lexer *lexer, CSTToken* token) -> void {
            if(!lexer->binder) return;
            auto a = annotation_str_arg(0, token);
            auto b = annotation_str_arg(1, token);
            if(a.empty() || b.empty()) {
                lexer->error("cbi:import called with invalid parameters : " + a + " , " + b);
                return;
            }
            lexer->binder->import_container(a, b);
        }},
        { "cbi:to", [](Lexer *lexer, CSTToken* token) -> void {
            if(!lexer->binder) return;
            lexer->isCBICollecting = true;
            lexer->current_cbi = annotation_str_arg(0, token);
        }},
        { "cbi:begin", [](Lexer *lexer, CSTToken* token) -> void {
            if(!lexer->binder) return;
            lexer->isCBICollecting = true;
            lexer->isCBIKeepCollecting = true;
            lexer->current_cbi = annotation_str_arg(0, token);
        }},
        { "cbi:end", [](Lexer *lexer, CSTToken* token) -> void {
            if(!lexer->binder) return;
            lexer->isCBICollecting = false;
            lexer->isCBIKeepCollecting = false;
            lexer->current_cbi = "";
        }},
        {"cbi:compile", [](Lexer *lexer, CSTToken* token) -> void {
            if(!lexer->binder) return;
            lexer->isCBICollecting = false;
            lexer->isCBIKeepCollecting = false;
            lexer->current_cbi = "";
            auto a = annotation_str_arg(0, token);
            if(a.empty()) {
                lexer->error("cbi:compiler called with invalid parameters : " + a);
                return;
            }
            lexer->binder->compile(a);
        }}
};