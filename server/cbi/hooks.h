// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class SemanticTokensAnalyzer;

class FoldingRangeAnalyzer;

/**
 * this function is called by the semantic tokens analyzer for providing support
 * for embedded semantic tokens put
 */
typedef void(*EmbeddedSemanticTokensPut)(SemanticTokensAnalyzer* analyzer);

/**
 * folding range analyzer is passed to folding ranges put
 */
typedef Token*(*EmbeddedFoldingRangesPut)(FoldingRangeAnalyzer* analyzer, Token* start, Token* end, void* dataPtr);