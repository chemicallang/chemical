// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#pragma once

#include "StreamPosition.h"
#include <string>
#include <utility>
#include <iostream>

class LexError {

public:

    /**
     * the position of the lexer where the error occurred
     */
    StreamPosition position;

    /**
     * specific sourceId that identifies the source code
     * for example a file path can be a sourceId
     */
    std::string sourceId;

    /**
     * the message associated with the error
     */
    std::string message;

    LexError(StreamPosition position, std::string& identifier, std::string message) : position(position), sourceId(identifier), message(std::move(message)) {

    }

    void display() {
        std::cerr << "[Error] " << message << " ; at " << sourceId << '#' << position.line << ':' << position.character << '\n';
    }

    LexError wrap(const std::string& prefix, const std::string& suffix) {
        return {position, sourceId, prefix + " " + message + " " + suffix};
    }

    LexError wrap(const std::string& prefix) {
        return {position, sourceId, prefix + " " + message};
    }

    inline std::string getFilePath() {
        return sourceId;
    }

};