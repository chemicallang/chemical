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

class SourceProvider {
private:

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
     * saves the position into given position
     */
    void saveInto(StreamPosition &pos) {
        pos.pos = currentPosition();
        pos.line = lineNumber;
        pos.character = lineCharacterNumber;
    }

    /**
     * restores the position of this stream from the given position
     * @param position
     */
    void restore(StreamPosition &position);

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
     * the actual stream being read
     */
    std::istream *stream;

    /**
     * create a source provider with a stream
     */
    explicit SourceProvider(std::istream *stream) : stream(stream) {}

    /**
     * gets the current pos of the stream
     */
    unsigned int currentPosition() const;

    /**
     * reads a single character and returns it
     * everytime a character is read, it must check if its the line ending character to track lineNumbers
     */
    char readCharacter();

    /**
     * checks the stream is at the end
     * please also use both peek() == -1
     */
    bool eof() const;

    /**
     * peaks the character to read
     */
    char peek() const;

    /**
     * peaks the character at current pos + ahead
     */
    char peek(int ahead);

    /**
     * reads the stream until this (stop) character occurs
     * @param stop the stopping character
     * @return everything read until stop character, it doesn't include the stopping character
     */
    std::string readUntil(char stop);

    /**
     * if text is present at current pos in the stream, increments the stream with text.length()
     * @param text to increment
     * @param peek peeks only, doesn't increment
     * @return true if incremented by text length otherwise false
     */
    bool increment(const std::string& text, bool peek = false);

    /**
     * if char c is present at current pos, increments the stream with character
     * @param c character to look for
     * @return true if incremented by character length = 1, otherwise false
     */
    bool increment(char c);

    /**
     * this will read all the text from current position to end in a string and return it
     */
    std::string readAllFromHere();

    /**
     * reads all the text from the stream from the beginning in a string and returns it
     * @return
     */
    std::string readAllFromBeg();

    /**
     * This for debugging, when called, everything from the current position to the end will be printed to cout
     */
    void printAll();

    /**
     * get zero-based current line number
     */
    unsigned int getLineNumber() const;

    /**
     * get zero-based character number
     * @return
     */
    unsigned int getLineCharNumber() const;

    /**
     * gets the stream position at the current position
     * @return
     */
    StreamPosition getStreamPosition() const;

    /**
     * reset the stream
     */
    void reset();

    /**
     * reads until the given ending appears into a string and returns it
     * @param consume, should it also consume the ending text
     */
    std::string readUntil(const std::string& ending, bool consume = true);

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
     * @return the string that was found
     */
    std::string readAnything(char until = ' ');

    /**
     * reads a alphabetical string
     */
    std::string readAlpha();

    /**
     * reads an unsigned integer as string, returns "" if no integer found
     */
    std::string readUnsignedInt();

    /**
     * a number will be read into a chemical string
     */
    void readNumber(chem::string& string);

    /**
     * reads a number from the stream
     */
    std::string readNumber() {
        chem::string content((const char*) nullptr);
        readNumber(content);
        return content.to_std_string();
    }

    /**
     * reads a alphanumeric string
     */
    std::string readAlphaNum();

    /**
     * reads a single identifier
     */
    std::string readIdentifier();

    /**
     * reads a single annotation into given string, this doesn't read '@'
     */
    void readAnnotationIdentifier(std::string& into);

    /**
     * reads a single annotation, this doesn't read '@'
     */
    std::string readAnnotationIdentifier() {
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
     * returns the token position at the very current position
     * @return
     */
    Position position();

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
    Position backPosition(unsigned int back);

};