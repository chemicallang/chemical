// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//
#pragma once

#include <string>
#include "StreamPosition.h"

class SourceProvider {
public:
    /**
     * gets the current pos of the stream
     * @return
     */
    virtual unsigned int position() const = 0;

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
     * @return true if incremented by text length otherwise false
     */
    virtual bool increment(const std::string& text) = 0;

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

};