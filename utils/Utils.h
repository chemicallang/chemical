// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#pragma once

#include <memory>
#include <vector>
#include "lexer/model/tokens/LexToken.h"

void printTokens(const std::vector<std::unique_ptr<LexToken>> &lexed);