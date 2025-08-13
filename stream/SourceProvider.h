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
#include "InputSource.h"

class SourceProvider {
private:

    /**
     * the pointer to data
     */
    const char* data_;

    /**
     * the size of the data
     */
    std::size_t size_;

    /**
     * the end point which should not be read
     */
    const char* const end_;

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

public:

    /**
     * handles the character read from the stream
     * changes line number and character number based on the character
     */
    inline void handleCharacterRead(char c) noexcept {
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
    explicit SourceProvider(InputSource& stream) : data_(stream.data()), size_(stream.size()), end_(stream.data() + stream.size()) {

    }

    /**
     * create a source provider with a stream
     */
    explicit SourceProvider(const char* data, std::size_t size) : data_(data), size_(size), end_(data + size) {

    }

    /**
     * reads a single character and returns it
     * everytime a character is read, it must check if its the line ending character to track lineNumbers
     */
    [[nodiscard]]
    char readCharacter() noexcept {
        if(data_ == end_) {
            return '\0';
        }
        const auto read = *data_;
        handleCharacterRead(read);
        data_++;
        return read;
    }

    /**
     * increment a single character forward
     */
    void increment() noexcept {
        if(data_ == end_) return;
        handleCharacterRead(*data_);
        data_++;
    }

    /**
     * checks the stream is at the end
     * please also use both peek() == -1
     */
    [[nodiscard]]
    inline bool eof() const noexcept {
        return data_ == end_;
    }

    /**
     * peaks the character to read
     */
    [[nodiscard]]
    inline char peek() const noexcept {
        return (data_ < end_) ? *data_ : '\0';
    }

    /**
     * if char c is present at current pos, increments the stream with character
     * @param c character to look for
     * @return true if incremented by character length = 1, otherwise false
     */
    [[nodiscard]]
    bool increment(char c) noexcept {
        return (data_ < end_ && *data_ == c) ? (handleCharacterRead(*data_++), true) : false;
    }

    /**
     * get zero-based current line number
     */
    [[nodiscard]]
    inline unsigned int getLineNumber() const noexcept {
        return lineNumber;
    }

    /**
     * get zero-based character number
     */
    [[nodiscard]]
    inline unsigned int getLineCharNumber() const noexcept {
        return lineCharacterNumber;
    }

    /**
     * reset the stream
     */
    void reset() {
        lineNumber = 0;
        lineCharacterNumber = 0;
        data_ = end_ - size_;
    }

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
    [[nodiscard]]
    inline bool hasNewLine() const {
        const auto p = peek();
        return p == '\n' || p == '\r';
    }

    /**
     * @return whether new line characters were read
     */
    bool readNewLineChars();

    /**
     * reads all whitespaces along with new lines
     */
    void readWhitespacesAndNewLines();

    /**
     * get the position of the stream, which you can restore later
     */
    [[nodiscard]]
    StreamPosition getStreamPosition() {
        return StreamPosition { data_, getLineNumber(), getLineCharNumber() };
    }

    /**
     * restores the position of this stream from the given position
     * @param position
     */
    void restore(const StreamPosition &position) {
        data_ = position.data;
        lineNumber = position.line;
        lineCharacterNumber = position.character;
    }

    /**
     * returns the token position at the very current position
     */
    inline Position position() {
        return { getLineNumber(), getLineCharNumber() };
    }

};