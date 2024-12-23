// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//
#pragma once

#include <string>
#include "StreamPosition.h"
#include "integration/common/Diagnostic.h"
#include <iosfwd>
#include "std/chem_string.h"
#include <istream>
#include <streambuf>
#include "InputSource.h"

class SourceProvider {
public:

    /**
     * the capacity of the buffer
     */
    constexpr static size_t BUFFER_CAPACITY = 1024;

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

    /**
     * the buffer is where we load characters from the input source
     * gradually in small blocks
     */
    char buffer[BUFFER_CAPACITY];

    /**
     * the buffer size is the characters we read into the buffer
     * from the input source, input source may have less characters than
     * capacity, this will always be less than capacity
     */
    size_t bufferSize = 0;

    /**
     * buffer position
     */
    size_t bufferPos = 0;

    /**
     * fills the buffer
     */
    void bufferFill();

    /**
     * handles the character read from the stream
     * changes line number and character number based on the character
     */
    inline void handleCharacterRead(char c) {
        if (c == '\n' || c == '\x0C' || (c == '\r' && peek() != '\n')) {
            // if there's no \n next to \r, the line ending must be CR, so we treat it as line ending
            lineNumber++;
            lineCharacterNumber = 0;
        } else {
            lineCharacterNumber++;
        }
    }

public:

    /**
     * the input source is used to read and fill the buffer
     */
    InputSource* stream;

    /**
     * create a source provider with a stream
     */
    explicit SourceProvider(InputSource* stream) : stream(stream) {

    }

    /**
     * reads a single character and returns it
     * everytime a character is read, it must check if its the line ending character to track lineNumbers
     */
    char readCharacter();

    /**
     * checks the stream is at the end
     * please also use both peek() == -1
     */
    [[nodiscard]]
    bool eof() const;

    /**
     * peaks the character to read
     */
    [[nodiscard]]
    char peek();

    /**
     * is the peak character a number character
     */
    inline bool is_peak_number_char() {
        const auto p = peek();
        return p == '-' || std::isdigit(p);
    }

    /**
     * if char c is present at current pos, increments the stream with character
     * @param c character to look for
     * @return true if incremented by character length = 1, otherwise false
     */
    bool increment(char c);

    /**
     * get zero-based current line number
     */
    [[nodiscard]]
    unsigned int getLineNumber() const;

    /**
     * get zero-based character number
     */
    [[nodiscard]]
    unsigned int getLineCharNumber() const;

    /**
     * gets the stream position at the current position
     * @return
     */
    [[nodiscard]]
    StreamPosition getStreamPosition();

    /**
     * reset the stream
     */
    void reset();

    /**
     * reset the buffer and switch to the given source
     */
    void switch_source(InputSource* source);

    /**
     * reads whitespaces, returns how many whitespaces were read
     */
    unsigned int readWhitespaces();

    /**
     * @return whether there's a newline at current position
     */
    bool hasNewLine();

    /**
     * @return whether new line characters were read
     */
    bool readNewLineChars();

    /**
     * reads all whitespaces along with new lines
     */
    void readWhitespacesAndNewLines();

    /**
     * restores the position of this stream from the given position
     * @param position
     */
    void restore(const StreamPosition &position);

    /**
     * returns the token position at the very current position
     * @return
     */
    inline Position position() {
        return {getLineNumber(), getLineCharNumber()};
    }

};