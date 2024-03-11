// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <string>
#include <optional>
#include "lexer/model/TokenPosition.h"
#include "lexer/model/LexTokenType.h"
#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include "LibLsp/lsp/lsp_completion.h"
//#include "rapidjson\rapidjson.h"
//#include "rapidjson\document.h"		// rapidjson's DOM-style API
//#include "rapidjson/writer.h"
//#include "rapidjson\stringbuffer.h"	// wrapper of C stream for prettywriter as output
//#include "rapidjson\prettywriter.h"	// for stringify JSON

class LexToken {
public:

    TokenPosition position;

    LexToken(const TokenPosition& position) : position(position) {

    }

    inline unsigned int start() {
        return position.position;
    }

    inline unsigned int end() {
        return position.position + length();
    }

    inline unsigned int lineNumber() {
        return position.lineNumber;
    }

    inline unsigned int lineCharNumber() {
        return position.lineCharNumber;
    }

    /**
     * returns the modifiers used by lsp
     * @return
     */
    virtual unsigned int lsp_modifiers() {
        return 0;
    }

    /**
     * if this token corresponds to a declaration token
     * meaning the token defines something, a struct, an enum, or a variable
     * the identifier for that, should be returned (enum name or struct name)
     * @return
     */
    virtual std::optional<std::string> declaration_identifier() {
        return std::nullopt;
    }

//    /**
//     * the declaration identifier returned by this token
//     * can be used to identify the identifier present in a child scope of the current scope (in which this token is present)
//     *
//     * Why is this required, well :
//     * in c++ when a function is written below its usage, a function prototype is required above, so the compiler knows the existence of this function
//     *
//     * the problem is that when lexing, tokens are provided to IDE and lexing is a forward process (we can't look back)
//     * so when its usage appears above, to resolve the token, we store that token as unresolved, later when this token returns a declaration identifier and we know
//     * that a child scope token hasn't been resolved with same identifier, we ask this function if the child scope token should be resolved, if this returns true, we resolve it
//     * @return
//     */
//    virtual bool resolves_child_scope_symbol() {
//        return false;
//    }

    /**
     * If this token requires that the identifier returned should be looked in the current scope
     * so that, its correct semantic type can be identified, the resolution identifier should be returned
     * @return
     */
    virtual std::optional<std::string> resolution_identifier() {
        return std::nullopt;
    }

    /**
     * when this token can't be found in the current scope or in the scopes above
     * you can allow the linker to look below the current scope
     */
    virtual bool resolve_below_current_scope() {
        return false;
    }

    /**
     * string length of the token
     * @return
     */
    virtual unsigned int length() const = 0;

    /**
     * Get the type of token this is
     * @return
     */
    virtual LexTokenType type() const = 0;

    /**
     * lsp  semantic token type
     * @return
     */
    virtual SemanticTokenType lspType() const = 0;

    /**
     * return th completion item label for the lsp
     * @return
     */
    virtual std::optional<std::string> lsp_comp_label() const {
        return std::nullopt;
    }

    /**
     * return the lsp completion item kind
     * @return
     */
    virtual std::optional<lsCompletionItemKind> lsp_comp_kind() const {
        return std::nullopt;
    }

    /**
     * this function returns the actual representation of the token in the source code
     * @return
     */
    virtual std::string representation() const = 0;

    /**
     * type string for token (debugging)
     * @return
     */
    virtual std::string type_string() const = 0;

    /**
     * content of the token, for example string token can contain ('some name')
     * @return
     */
    virtual std::string content() const = 0;

//    /**
//     * deserialize the token
//     * @param obj
//     * @return
//     */
//    virtual bool deserialize(const rapidjson::Value& obj) = 0;
//
//    /**
//     * serialize the token
//     * @param writer
//     * @return
//     */
//    virtual bool serialize(rapidjson::Writer<rapidjson::StringBuffer>* writer) const = 0;

};
