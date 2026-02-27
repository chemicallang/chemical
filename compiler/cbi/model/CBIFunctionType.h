// Copyright (c) Chemical Language Foundation 2025.

#pragma once

enum class CBIFunctionType : int {

    InitializeLexer,

    ParseMacroValue,

    ParseMacroNode,

    ParseMacroTopLevelNode,

    ParseMacroMemberNode,

    SymResDeclareTopLevelNode,

    SymResLinkSignatureNode,

    SymResLinkSignatureValue,

    SymResNode,

    SymResValue,

    ReplacementNodeDeclare,

    ReplacementNode,

    ReplacementValue,

    SemanticTokensPut,

    FoldingRangesPut,

    TransformerMain

};