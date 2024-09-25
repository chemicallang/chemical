// Copyright (c) Qinetik 2024.

#include "CBIUtils.h"
#include "CSTTokenCBI.h"
#include "cst/base/CSTToken.h"
#include "std/chem_string.h"

void CSTTokenget_value(CSTToken* token, chem::string* into) {
    into->append(token->value());
}

LexTokenType CSTTokentype(CSTToken* token) {
    return token->type();
}

Position* CSTTokenposition(CSTToken* token) {
    return &token->flat.position;
}

std::vector<CSTToken*>* CSTTokentokens(CSTToken* token) {
    return &token->tokens;
}

void CSTTokenaccept(CSTToken* token, CSTConverter* converter) {
    token->accept((CSTVisitor*) converter);
}