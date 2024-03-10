// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#pragma once

enum class LspSemanticTokenModifier {
    ls_declaration = 0,// 'declaration',
    ls_definition,// 'definition',
    ls_readonly,// 'readonly',
    ls_static,// 'static',
    ls_deprecated,// 'deprecated',
    ls_abstract,// 'abstract',
    ls_async,// 'async',
    ls_modification,// 'modification',
    ls_documentation,// 'documentation',
    ls_defaultLibrary,// 'defaultLibrary'
    LastModifier = ls_defaultLibrary
};