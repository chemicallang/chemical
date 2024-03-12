// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#pragma once

#include "stream/StreamPosition.h"
#include <string>
#include <utility>
#include <iostream>

class LexError {
public:

    /**
     * The start position of the error
     */
    StreamPosition start;

    /**
     * the position of the stream where the error stopped
     * this is usually the position where the error actually occurred
     */
    StreamPosition end;

    /**
     * specific sourceId that identifies the source code
     * for example a file path can be a sourceId
     */
    std::string sourceId;

    /**
     * the message associated with the error
     */
    std::string message;

    /**
     * constructor
     * @param start
     * @param position
     * @param identifier
     * @param message
     */
    LexError(StreamPosition start, StreamPosition position, std::string& identifier, std::string message) : start(start), end(position), sourceId(identifier), message(std::move(message)) {

    }

    /**
     * representation of the error so it can be printed in the console
     * @return
     */
    std::string representation() const {
        return "[Lexer] " + message + " ; at " + sourceId + '#' + std::to_string(end.line) + ':' + std::to_string(end.character);
    };

    inline std::string getFilePath() {
        return sourceId;
    }

};