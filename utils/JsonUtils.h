// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#pragma once

#include <string>
#include "rapidjson/writer.h"
#include <vector>
#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include "rapidjson/filewritestream.h"
#include <iostream>
#include <rapidjson/ostreamwrapper.h>
#include <fstream>
#include <rapidjson/prettywriter.h>
#include "cst/base/CSTToken.h"
#include "rapidjson/document.h"

class JsonUtils {

public:

    static void serialize(const std::string& path, const std::vector<SemanticToken>& tokens) {

        std::ofstream stream("D:/Programming/Chemical/chemical/" + path);
        rapidjson::OStreamWrapper out(stream);
        rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(out);

        rapidjson::Document d;
        d.SetArray();
        auto allocator = d.GetAllocator();

        for (const auto &token: tokens) {
            rapidjson::Value tokenObj(rapidjson::kObjectType);
            tokenObj.AddMember("length", token.length, allocator);
            tokenObj.AddMember("deltaLine", token.deltaLine, allocator);
            tokenObj.AddMember("deltaStart", token.deltaStart, allocator);
            tokenObj.AddMember("tokenModifiers", token.tokenModifiers, allocator);
            tokenObj.AddMember("tokenType", token.tokenType, allocator);
            d.PushBack(tokenObj, allocator);
        }

        d.Accept(writer);
        stream.close();

    };

    static void serialize(const std::string& path, const std::vector<CSTToken*>& tokens) {

        std::ofstream stream("D:/Programming/Chemical/chemical/" + path, std::ios::trunc);
        rapidjson::OStreamWrapper out(stream);
        rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(out);

        rapidjson::Document d;
        d.SetArray();
        auto allocator = d.GetAllocator();

        CSTToken* token;
        for (const auto &t: tokens) {
            if(t->compound()) {
                continue;
            }
            token = t->start_token();
            rapidjson::Value tokenObj(rapidjson::kObjectType);
            rapidjson::Value posValue(token->position().representation().c_str(), static_cast<rapidjson::SizeType>(token->position().representation().length()), allocator);
            tokenObj.AddMember("position", posValue, allocator);
            tokenObj.AddMember("length", token->length(), allocator);
            rapidjson::Value typeValue(token->type_string().c_str(), static_cast<rapidjson::SizeType>(token->type_string().length()), allocator);
            tokenObj.AddMember("type", typeValue, allocator);
            d.PushBack(tokenObj, allocator);
        }

        d.Accept(writer);
        stream.close();

    };

};
