// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//
#pragma once

#include <string>
#include "StreamPosition.h"
#include "common/Diagnostic.h"

class SourceProvider {
public:
    /**
     * gets the current pos of the stream
     * @return
     */
    virtual unsigned int currentPosition() const = 0;

    /**
     * reads a single character and returns it
     * everytime a character is read, it must check if its the line ending character to track lineNumbers
     * @return
     */
    virtual char readCharacter() = 0;
    /**
     * checks the stream is not at the end
     * @return
     */
    virtual bool eof() const = 0;
    /**
     * peaks the character to read
     * @return
     */
    virtual char peek() const = 0;
    /**
     * peaks the character at current pos + ahead
     * @param ahead
     * @return
     */
    virtual char peek(int ahead) = 0;

    /**
     * reads the stream until this (stop) character occurs
     * @param stop the stopping character
     * @return everything read until stop character, it doesn't include the stopping character
     */
    virtual std::string readUntil(char stop) = 0;

    /**
     * if text is present at current pos in the stream, increments the stream with text.length()
     * @param text to increment
     * @param peek peeks only, doesn't increment
     * @return true if incremented by text length otherwise false
     */
    virtual bool increment(const std::string& text, bool peek = false) = 0;

    /**
     * if char c is present at current pos, increments the stream with character
     * @param c character to look for
     * @return true if incremented by character length = 1, otherwise false
     */
    virtual bool increment(char c) = 0;

    /**
     * this will read all the text from current position to end in a string and return it
     * @return
     */
    virtual std::string readAllFromHere() = 0;

    /**
     * reads all the text from the stream from the beginning in a string and returns it
     * @return
     */
    virtual std::string readAllFromBeg() = 0;

    /**
     * This for debugging, when called, everything from the current position to the end will be printed to cout
     */
    virtual void printAll() = 0;

    /**
     * get zero-based current line number
     */
    virtual unsigned int getLineNumber() const = 0;

    /**
     * get zero-based character number
     * @return
     */
    virtual unsigned int getLineCharNumber() const = 0;

    /**
     * gets the stream position at the current position
     * @return
     */
    virtual StreamPosition getStreamPosition() const = 0;

    /**
     * reads anything as long as when lambda returns true
     * @return
     */
    template<typename TFunc>
    std::string readAnything(TFunc when);

    /**
     * reads until the given ending appears into a string and returns it
     * @param consume, should it also consume the ending text
     */
    std::string readUntil(const std::string& ending, bool consume = true);

    /**
     * escapes a sequence at current position, if 'n' is at current position, returns '\n'
     * you must have consumed a \ (backslash) before calling this function
     * a pair will be returned, first the escaped character
     * second the bool indicating whether an escape sequence was found
     */
    std::pair<char, bool> escape_sequence();

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
     * reads a number from the stream
     */
    std::string readNumber();

    /**
     * reads a alphanumeric string
     */
    std::string readAlphaNum();

    /**
     * reads a single identifier
     */
    std::string readIdentifier();

    /**
     * reads a single annotation, this doesn't read '@'
     */
    std::string readAnnotation();

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

/**
 * The implementation for lexAnything
 * This is required in header because of template usage
 */
template<typename TFunc>
std::string SourceProvider::readAnything(TFunc when) {
    std::string str;
    while (!eof() && when()) {
        str.append(1, readCharacter());
    }
    return str;
}