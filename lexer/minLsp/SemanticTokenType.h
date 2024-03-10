// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#pragma once

enum class LspSemanticTokenType {
    ls_namespace = 0,// 'namespace',
    /**
     * Represents a generic type. Acts as a fallback for types which
     * can't be mapped to a specific type like class or enum.
     */
    ls_type,// 'type',
    ls_class,// 'class',
    ls_enum,// 'enum',
    ls_interface,// 'interface',
    ls_struct,// 'struct',
    ls_typeParameter,// 'typeParameter',
    ls_parameter,// 'parameter',
    ls_variable,// 'variable',
    ls_property,// 'property',
    ls_enumMember,// 'enumMember',
    ls_event,// 'event',
    ls_function,// 'function',
    ls_method,// 'method',
    ls_macro,// 'macro',
    ls_keyword,// 'keyword',
    ls_modifier,// 'modifier',
    ls_comment,// 'comment',
    ls_string,// 'string',
    ls_number,// 'number',
    ls_regexp,// 'regexp',
    ls_operator,// 'operator'
    lastKind = ls_operator
};