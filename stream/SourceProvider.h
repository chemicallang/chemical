// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/12/2023.
//
#pragma once

#include <string>
#include "StreamPosition.h"
#include "core/diag/Diagnostic.h"
#include <iosfwd>
#include "std/chem_string.h"
#include <istream>
#include <streambuf>
#include "NewInputSource.h"

class SourceProvider {
public:

    /**
     * the pointer to data
     */
    const char* data_;

    /**
     * the size of the data
     */
    std::size_t size_;

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
     * create a source provider with a stream
     */
    explicit SourceProvider(NewInputSource& stream) : data_(stream.data()), size_(stream.size()) {

    }

    /**
     * create a source provider with a stream
     */
    explicit SourceProvider(NewInputSource* stream) : data_(stream->data()), size_(stream->size()) {

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
     * doesn't read newlines
     */
    unsigned int readWhitespaces();

    /**
     * skips whitespaces, this doesn't include new lines
     * characters ' ', '\t' are skipped
     */
    void skipWhitespaces();

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