// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include "cst/base/CSTToken.h"

void printToken(CSTToken *token);

void printTokens(const std::vector<CSTToken*> &lexed);