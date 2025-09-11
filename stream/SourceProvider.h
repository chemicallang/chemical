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
     * handles the character read from the stream, when you read a character
     * you can supply it to this function
     */
    inline void handleCharacterRead(char c) noexcept {
        if (c == '\n' || c == '\x0C' || (c == '\r' && peekByte() != '\n')) {
            // if there's no \n next to \r, the line ending must be CR, so we treat it as line ending
            lineNumber++;
            lineCharacterNumber = 0;
        } else {
            lineCharacterNumber++;
        }
    }

    /**
     * handles the codepoint read from the stream, when you read a code point
     * you should use this function to handle read code point
     */
    inline void handleCodepointRead(char32_t cp) noexcept {
        if (cp == U'\n' || cp == U'\f' || (cp == U'\r' && peekByte() != '\n')) {
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
     * this gives the current data pointer (read only)
     */
    [[nodiscard]]
    const char* current_data() const noexcept {
        return data_;
    }

    /**
     * this allows us to peek a utf8 code point
     * it doesn't advance the data pointer, returns length (num of bytes) in out_len
     */
    [[nodiscard]]
    char32_t utf8_decode_peek(std::size_t &out_len) const noexcept {

        if (data_ == end_) {
            out_len = 0;
            return 0;
        }

        const auto* p = reinterpret_cast<const unsigned char*>(data_);
        const unsigned char b0 = *p;

        // ASCII fast path
        if (b0 < 0x80) {
            out_len = 1;
            return static_cast<char32_t>(b0);
        }

        // remaining bytes
        const auto rem = static_cast<std::size_t>(end_ - data_);

        // 2-byte sequence
        if ((b0 & 0xE0) == 0xC0) {
            if (rem >= 2) {
                const unsigned char b1 = p[1];
                if ((b1 & 0xC0) == 0x80 && b0 >= 0xC2) {
                    out_len = 2;
                    return static_cast<char32_t>(((b0 & 0x1F) << 6) | (b1 & 0x3F));
                }
            }
            out_len = 1; // resync by one
            return static_cast<char32_t>(0xFFFD);
        }

        // 3-byte sequence
        if ((b0 & 0xF0) == 0xE0) {
            if (rem >= 3) {
                const unsigned char b1 = p[1], b2 = p[2];
                if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
                    if ((b0 != 0xE0 || b1 >= 0xA0) && (b0 != 0xED || b1 <= 0x9F)) {
                        out_len = 3;
                        return static_cast<char32_t>(((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F));
                    }
                }
            }
            out_len = 1;
            return static_cast<char32_t>(0xFFFD);
        }

        // 4-byte sequence
        if ((b0 & 0xF8) == 0xF0) {
            if (rem >= 4) {
                const unsigned char b1 = p[1], b2 = p[2], b3 = p[3];
                if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
                    if ((b0 != 0xF0 || b1 >= 0x90) && (b0 != 0xF4 || b1 <= 0x8F) && b0 <= 0xF4) {
                        out_len = 4;
                        return static_cast<char32_t>(((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F));
                    }
                }
            }
            out_len = 1;
            return static_cast<char32_t>(0xFFFD);
        }

        out_len = 1;
        return static_cast<char32_t>(0xFFFD);

    }

    /**
     * reads a utf8 code point and returns it (consuming it)
     */
    [[nodiscard]]
    char32_t readCodePoint() noexcept {
        std::size_t len = 0;
        char32_t cp = utf8_decode_peek(len);
        if (len == 0) return 0; // EOF
        const auto rem = static_cast<std::size_t>(end_ - data_);
        const auto step = (len <= rem) ? len : rem;
        data_ += step;
        handleCodepointRead(cp); // increment line number
        return cp;
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
        const auto read = *data_++;
        handleCharacterRead(read);
        return read;
    }

    /**
     * increment a single character forward
     */
    void increment() noexcept {
        if(data_ == end_) return;
        handleCharacterRead(*data_++);
    }

    /**
     * increment a peeked code point
     */
    void incrementCodepoint(char32_t cp, std::size_t len) noexcept {
        handleCodepointRead(cp); // increment line number
        const auto rem = static_cast<std::size_t>(end_ - data_);
        const auto step = (len <= rem) ? len : rem;
        data_ += step;
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
     * peaks a unsigned char to read
     */
    [[nodiscard]]
    inline unsigned char peekByte() const noexcept {
        return (data_ < end_) ? static_cast<unsigned char>(*data_) : 0u;
    }

    /**
     * if char c is present at current pos, increments the stream with character
     * @param c character to look for
     * @return true if incremented by character length = 1, otherwise false
     */
    bool increment(char c) noexcept {
        return (data_ < end_ && *data_ == c) ? (handleCharacterRead(c), data_++, true) : false;
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
    unsigned int readWhitespaces() noexcept {
        unsigned int whitespaces = 0;
        while(true) {
            switch(peek()) {
                case ' ':
                    increment();
                    whitespaces++;
                    continue;
                case '\t':
                    increment();
                    whitespaces += 4;
                    continue;
                default:
                    return whitespaces;
            }
        }
    }

    /**
     * skips whitespaces, this doesn't include new lines
     * characters ' ', '\t' are skipped
     */
    void skipWhitespaces() noexcept {
        while(true) {
            switch(peek()) {
                case ' ':
                case '\t':
                    increment();
                    continue;
                default:
                    return;
            }
        }
    }

    /**
     * @return whether there's a newline at current position
     */
    [[nodiscard]]
    inline bool hasNewLine() const noexcept {
        const auto p = peek();
        return p == '\n' || p == '\r';
    }

    /**
     * @return whether new line characters were read
     */
    bool readNewLineChars() noexcept {
        switch(peek()) {
            case '\n':
                increment();
                return true;
            case '\r':
                // consuming the \r
                increment();
                // consume also the next \n
                if (peek() == '\n') increment();
                return true;
            default:
                return false;
        }
    }

    /**
     * reads all whitespaces along with new lines
     */
    void readWhitespacesAndNewLines() noexcept {
        while(true) {
            switch(peek()) {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    increment();
                    continue;
                default:
                    return;
            }
        }
    }

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