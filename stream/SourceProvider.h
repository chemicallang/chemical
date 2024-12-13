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
     * buffered input is used
     */
    std::vector<char> buffer;

    /**
     * buffer position
     */
    size_t bufferPos = 0;

    /**
     * buffer size
     */
    size_t bufferSize = 1024;

    /**
     * fills the buffer
     */
    void bufferFill(size_t size);

    /**
     * fills the buffer
     */
    inline void bufferFill() {
        bufferFill(bufferSize);
    }

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
     * reads the stream until this (stop) character occurs
     * @param stop the stopping character
     * @return everything read until stop character, it doesn't include the stopping character
     */
    void readUntil(chem::string* into, char stop);

    /**
     * reads the stream until this (stop) character occurs
     * @param stop the stopping character
     * @return everything read until stop character, it doesn't include the stopping character
     */
    void readUntil(std::string& into, char stop);

    /**
     * helper
     */
    [[nodiscard]]
    inline std::string readUntil(char stop) {
        std::string str;
        readUntil(str, stop);
        return str;
    }

    /**
     * if char c is present at current pos, increments the stream with character
     * @param c character to look for
     * @return true if incremented by character length = 1, otherwise false
     */
    bool increment(char c);

    /**
     * increment by given amount
     */
    void increment_amount(unsigned amount);

    /**
     * this will read all the text from current position to end in a string and return it
     */
    [[nodiscard]]
    std::string readAllFromHere();

    /**
     * This for debugging, when called, everything from the current position to the end will be printed to cout
     */
    void printAll();

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
     * will read everything to the given string
     *
     * will not stop if the stream doesn't end or there's a backslash before stopAt character
     * useful when reading a string token which must not stop at \"
     *
     * will also append the last stopAt character into value
     */
    void readEscaping(chem::string* value, char stopAt);

    /**
     * will read everything to the given string
     *
     * will not stop if the stream doesn't end or there's a backslash before stopAt character
     * useful when reading a string token which must not stop at \"
     *
     * will also append the last stopAt character into value
     */
    void readEscaping(std::string& value, char stopAt);

    /**
     * reads all characters into a string until char occurs
     */
    void readAnything(chem::string* str, char until = ' ');

    /**
     * reads all characters into a string until char occurs
     */
    void readAnything(std::string& str, char until = ' ');

    /**
     * reads all characters into a string until char occurs
     * @return the string that was found
     */
    [[nodiscard]]
    inline std::string readAnything(char until = ' ') {
        std::string str;
        readAnything(str, until);
        return str;
    }

    /**
    * reads a alphabetical string
    */
    void readAlpha(chem::string* str);

    /**
    * reads a alphabetical string
     */
    void readAlpha(std::string& str);

    /**
     * helper
     */
    [[nodiscard]]
    inline std::string readAlpha() {
        std::string str;
        readAlpha(str);
        return str;
    }

    /**
     * reads an unsigned integer as string, returns "" if no integer found
     */
    void readUnsignedInt(chem::string* str);

    /**
     * reads an unsigned integer as string, returns "" if no integer found
     */
    void readUnsignedInt(std::string& str);

    /**
     * helper
     */
    [[nodiscard]]
    inline std::string readUnsignedInt() {
        std::string str;
        readUnsignedInt(str);
        return str;
    }

    /**
     * a number will be read into a chemical string
     */
    void readNumber(chem::string* string);

    /**
     * a number will be read into a string
     */
    void readNumber(std::string& str);

    /**
     * reads a number from the stream
     */
    [[nodiscard]]
    inline std::string readNumber() {
        std::string content;
        readNumber(content);
        return content;
    }

    /**
     * a number will be read into a chemical string
     * this accepts any numbers, even hex numbers
     */
    void readAnyNumber(chem::string* string);

    /**
     * a number will be read into a string
     * this accepts any numbers, even hex numbers
     */
    void readAnyNumber(std::string& str);

    /**
     * reads a alphanumeric string
     */
    void readAlphaNum(chem::string* str);

    /**
     * reads a alphanumeric string
     */
    void readAlphaNum(std::string& str);

    /**
     * helper
     */
    [[nodiscard]]
    inline std::string readAlphaNum() {
        std::string str;
        readAlphaNum(str);
        return str;
    }

    /**
     * reads a single identifier
     */
    void readIdentifier(chem::string* str);

    /**
     * reads a single identifier
     */
    void readIdentifier(std::string& str);

    /**
     * reads a single identifier
     */
    [[nodiscard]]
    inline std::string readIdentifier() {
        std::string str;
        readIdentifier(str);
        return str;
    }

    /**
     * reads a single annotation into given string, this doesn't read '@'
     */
    void readAnnotationIdentifier(chem::string* into);

    /**
     * reads a single annotation into given string, this doesn't read '@'
     */
    void readAnnotationIdentifier(std::string& into);

    /**
     * reads a single annotation, this doesn't read '@'
     */
    [[nodiscard]]
    inline std::string readAnnotationIdentifier() {
        std::string ret;
        readAnnotationIdentifier(ret);
        return ret;
    }

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

    /**
     * when you have read the character from the stream, you create a position, \n\n
     * it corresponds to the position at the end of the character and not at the start \n\n
     * instead of saving the position in a variable before you read and consume characters \n
     * You should get position after reading the characters and basically subtract the length of the token \n\n
     * You can provide the length of the token to this function \n\n
     * Note that token must be on the same line
     * @param back
     * @return
     */
    inline Position backPosition(unsigned int back) {
        return {getLineNumber(), getLineCharNumber() - back};
    }

};