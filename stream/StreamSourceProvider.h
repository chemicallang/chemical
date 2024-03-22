// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 21/02/2024.
//

#pragma once

#include <iostream>
#include "SourceProvider.h"
#include "StreamPosition.h"

class StreamSourceProvider : public SourceProvider {
public:

    explicit StreamSourceProvider(std::istream &stream) : stream(stream) {}

    unsigned int position() const override {
        return stream.tellg();
    }

    char readCharacter() override {
        auto c = stream.get();
        handleCharacterRead(c);
        return c;
    }

    bool eof() const override {
        return stream.eof();
    }

    char peek() const override {
        return stream.peek();
    }

    char peek(int ahead) override {
        unsigned int pos = stream.tellg();
        stream.seekg(pos + ahead, std::ios::beg);
        char c = stream.get();
        stream.seekg(pos, std::ios::beg);
        return c;
    }

    std::string readUntil(char stop) {
        auto read = "";
        char currChar;
        while (true) {
            currChar = readCharacter();
            if (currChar == stop || eof()) {
                return read;
            } else {
                read += currChar;
            }
        }
    }

    bool increment(char c) override {
        if (stream.get() == c) {
            handleCharacterRead(c);
            return true;
        } else {
            stream.seekg(position() - 1, std::ios::beg);
            return false;
        }
    }

    std::string readAllFromHere() override {
        std::string source;
        while(!eof()) {
            source += readCharacter();
        }
        return source;
    }

    std::string readAllFromBeg()  override  {
        stream.seekg(0, std::ios::beg);
        return readAllFromHere();
    }

    void printAll() override {
        while(!eof()) {
            std::cout << readCharacter();
        }
    }

    bool increment(const std::string &text) override {

        if(stream.peek() != text[0]) {
            return false;
        }

        // Save current pos
        auto prevPosition = getStreamPosition();

        bool result = true;
        int pos = 0;
        while (!stream.eof() && pos < text.size()) {
            char c = readCharacter();
            if (c != text[pos]) {
                result = false;
                break;
            } else {
                pos++;
            }
        }

        // Seek back to original pos
        if (!result) {
            restore(prevPosition);
        }

        return result;
    }

    unsigned int getLineNumber() const override {
        return lineNumber;
    }

    unsigned int getLineCharNumber() const override {
        return lineCharacterNumber;
    }

    void reset() {
        this->stream.seekg(0, std::ios::beg);
        this->lineCharacterNumber = 0;
        this->lineNumber = 0;
    }

    StreamPosition getStreamPosition() const override {
        return StreamPosition{static_cast<unsigned int>(position()), lineNumber, lineCharacterNumber};
    }

private:

    void saveInto(StreamPosition &pos) {
        pos.pos = position();
        pos.line = lineNumber;
        pos.character = lineCharacterNumber;
    }

    void restore(StreamPosition &position) {
        stream.seekg(position.pos, std::ios::beg);
        lineNumber = position.line;
        lineCharacterNumber = position.character;
    }

    inline void handleCharacterRead(char c) {
        if (c == '\n' || c == '\x0C' || (c == '\r' && stream.peek() != '\n')) {
            // if there's no \n next to \r, the line ending must be CR, so we treat it as line ending
            lineNumber++;
            lineCharacterNumber = 0;
        } else {
            lineCharacterNumber++;
        }
    }

    std::istream &stream;

    /**
     * this counts lines, zero-based
     * On every character read, the provider checks if the line has ended and increments
     */
    unsigned int lineNumber = 0;

    /**
     * the current pos (character) to read, relative to the current line (line)
     * zero-based
     */
    unsigned int lineCharacterNumber = 0;

};