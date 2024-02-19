//
// Created by wakaz on 10/12/2023.
//

#ifndef COMPILER_SOURCEPROVIDER_H
#define COMPILER_SOURCEPROVIDER_H
#include <string>

class SourceProvider {
public:
    /**
     * gets the current position of the stream
     * @return
     */
    virtual int position() const = 0;

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
     * peaks the character at current position + ahead
     * @param ahead
     * @return
     */
    virtual char peek(int ahead) = 0;

    /**
     * reads the stream until this (stop) character occurs
     * @param stop the stopping character
     * @param consumeStop when true, stream will position after the stopping character, otherwise before
     * @return everything read until stop character, it doesn't include the stopping character
     */
    virtual std::string readUntil(char stop, bool consumeStop = false) = 0;

    /**
     * if text is present at current position in the stream, increments the stream with text.length()
     * @param text to increment
     * @return true if incremented by text length otherwise false
     */
    virtual bool increment(const std::string& text) = 0;

    /**
     * if char c is present at current position, increments the stream with character
     * @param c character to look for
     * @return true if incremented by character length = 1, otherwise false
     */
    virtual bool increment(char c) = 0;

    /**
     * get the current line number
     */
    virtual unsigned int getLineNumber() const = 0;

};


#endif //COMPILER_SOURCEPROVIDER_H
