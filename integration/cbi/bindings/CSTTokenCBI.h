// Copyright (c) Qinetik 2024.

#pragma once

#include "lexer/model/LexTokenType.h"
#include <vector>

class Position;

class CSTToken;

class CSTConverter;

extern "C" {

    void CSTTokenget_value(CSTToken* token, chem::string* into);

    LexTokenType CSTTokentype(CSTToken* token);

    Position* CSTTokenposition(CSTToken* token);

    std::vector<CSTToken*>* CSTTokentokens(CSTToken* token);

    void CSTTokenaccept(CSTToken* token, CSTConverter* converter);

}