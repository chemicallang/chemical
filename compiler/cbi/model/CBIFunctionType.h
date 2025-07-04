// Copyright (c) Chemical Language Foundation 2025.

#pragma once

enum class CBIFunctionType : int {

    InitializeLexer,

    ParseMacroValue,

    ParseMacroNode,

    ParseMacroTopLevelNode,

    ParseMacroMemberNode,

    SemanticTokensPut,

    FoldingRangesPut

};