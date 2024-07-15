// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"
#include "ast/utils/Operation.h"

/**
 * Its named OperationToken because it holds a operation
 */
class OperationToken : public LexToken {
public:

    using LexToken::LexToken;

};