//
// Created by wakaz on 13/02/2024.
//

#pragma once

#include <vector>
#include <iostream>
#include "lexer/model/LexToken.h"
#include "LibLsp/lsp/textDocument/SemanticTokens.h"

void printTokens(const std::vector<std::unique_ptr<LexToken>> &lexed);

void printTokens(const std::vector<SemanticToken> &tokens);