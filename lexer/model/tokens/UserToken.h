// Copyright (c) Qinetik 2024.

#pragma once

#include "LexToken.h"

struct UserToken {

    unsigned int line;
    unsigned int character;
    unsigned int length;
#ifdef LSP_BUILD
    SemanticTokenType lsp_type;
#endif

};

class LexUserToken : public LexToken {
public:

    unsigned int len;
#ifdef LSP_BUILD
    SemanticTokenType lsp_type;
#endif

    LexUserToken(UserToken token) : LexToken(
Position{
            token.line,
            token.character
        }
    ), len(token.length) {
#ifdef LSP_BUILD
        this->lsp_type = token.lsp_type;
#endif
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    unsigned int length() const override {
        return this->len;
    }

    unsigned int lsp_modifiers() override {
        return 0;
    }

    LexTokenType type() const override {
        return LexTokenType::UserToken;
    }

#ifdef LSP_BUILD
    SemanticTokenType lspType() const override {
        return this->lsp_type;
    }
#endif

    void append_representation(std::string &rep) const override {
        rep.append("[UserToken]");
    }

    std::string type_string() const {
        return "[UserToken]";
    };

    std::string content() const override {
        return "";
    }

};