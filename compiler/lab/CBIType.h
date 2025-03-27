// Copyright (c) Chemical Language Foundation 2025.

#pragma once

enum class CBIType : int {

    /**
     * lexer for a macro, the cbi must contain a initializeLexer function
     */
    MacroLexer = 0,
    /**
     * parser for a macro, the cbi must contain parseMacroNode, parseMacroValue functions
     */
    MacroParser = 1,

    /**
     * index to last element
     */
    IndexLast = MacroParser,

};