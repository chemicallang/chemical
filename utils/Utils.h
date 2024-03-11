// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include "lexer/model/tokens/LexToken.h"

void printToken(LexToken *token);

void printTokens(const std::vector<std::unique_ptr<LexToken>> &lexed);

void printTokens(const std::vector<std::unique_ptr<LexToken>> &lexed, const std::unordered_map<unsigned int, unsigned int> &linked);