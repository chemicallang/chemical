// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class SemanticTokensAnalyzer;

/**
 * this function is called by the semantic tokens analyzer for providing support
 * for embedded semantic tokens put
 */
typedef void(*EmbeddedSemanticTokensPut)(SemanticTokensAnalyzer* analyzer);