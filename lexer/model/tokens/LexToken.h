// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <string>
#include "lexer/minLsp/SemanticTokens.h"
#include "lexer/model/TokenPosition.h"
#include "lexer/model/LexTokenType.h"
//#include "rapidjson\rapidjson.h"
//#include "rapidjson\document.h"		// rapidjson's DOM-style API
//#include "rapidjson/writer.h"
//#include "rapidjson\stringbuffer.h"	// wrapper of C stream for prettywriter as output
//#include "rapidjson\prettywriter.h"	// for stringify JSON

class LexToken {
public:

    TokenPosition position;

    unsigned modifiers = 0;

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
    virtual LspSemanticTokenType lspType() const = 0;

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
