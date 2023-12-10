//
// Created by wakaz on 10/12/2023.
//

#ifndef COMPILER_SOURCEPROVIDER_H
#define COMPILER_SOURCEPROVIDER_H
#include <string>

class SourceProvider {
public:
    virtual int position() = 0;
    virtual char readCharacter() = 0;
    virtual bool eof() = 0;
    virtual char peek() = 0;
    virtual char peek(int ahead) = 0;
    virtual bool increment(const std::string& text) = 0;
};


#endif //COMPILER_SOURCEPROVIDER_H
