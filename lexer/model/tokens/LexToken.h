// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <string>
#include <optional>
#include "lexer/model/LexTokenType.h"
#include "common/Diagnostic.h"
#include "cst/base/CSTToken.h"

//#include "rapidjson\rapidjson.h"
//#include "rapidjson\document.h"		// rapidjson's DOM-style API
//#include "rapidjson/writer.h"
//#include "rapidjson\stringbuffer.h"	// wrapper of C stream for prettywriter as output
//#include "rapidjson\prettywriter.h"	// for stringify JSON

class LexToken : public CSTToken {
public:

    Position position;

    LexToken(const Position &position) : position(position) {

    }

    inline unsigned int lineNumber() {
        return position.line;
    }

    inline unsigned int lineCharNumber() {
        return position.character;
    }

    bool is_abs_string() {
        auto t = type();
        return t >= LexTokenType::IndexAbsStrStart && t <= LexTokenType::IndexAbsStrEnd;
    }

    LexToken *start_token() override {
        return this;
    }

    LexToken *end_token() override {
        return this;
    }

    /**
     * string length of the token
     */
    virtual unsigned int length() const = 0;

#ifdef DEBUG

    Position start() override {
        return position;
    }

#endif

    /**
     * content of the token, for example string token can contain ('some name')
     */
    virtual std::string content() const = 0;

    /**
     * lex token virtual destructor
     */
    virtual ~LexToken() = default;

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
