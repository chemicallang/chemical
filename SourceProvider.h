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
    virtual int position() = 0;
    /**
     * reads a single character and returns it
     * @return
     */
    virtual char readCharacter() = 0;
    /**
     * checks the stream is not at the end
     * @return
     */
    virtual bool eof() = 0;
    /**
     * peaks the character to read
     * @return
     */
    virtual char peek() = 0;
    /**
     * peaks the character at current position + ahead
     * @param ahead
     * @return
     */
    virtual char peek(int ahead) = 0;
    /**
     * if text is present at current position in the stream, increments the stream with text.length()
     * otherwise returns false
     * @param text to increment
     * @return true if incremented by text length otherwise false
     */
    virtual bool increment(const std::string& text) = 0;
};


#endif //COMPILER_SOURCEPROVIDER_H
