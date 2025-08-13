#include "SourceProvider.h"
#include <iostream>

void SourceProvider::skipWhitespaces() {
    while(true) {
        switch(peek()) {
            case ' ':
            case '\t':
                increment();
                continue;
            case '\0':
            default:
                return;
        }
    }
}

unsigned int SourceProvider::readWhitespaces() {
    unsigned int whitespaces = 0;
    auto p = peek();
    while (p == ' ' || p == '\t') {
        increment();
        if(p == ' ') {
            whitespaces++;
        } else if(p == '\t') {
            whitespaces += 4;
        }
        p = peek();
    }
    return whitespaces;
}

bool SourceProvider::readNewLineChars() {
    auto p = peek();
    if (p == '\n') {
        increment();
        return true;
    } else if (p == '\r') {
        // consuming the \r
        increment();
        // consume also the next \n
        if (peek() == '\n') increment();
        return true;
    } else {
        return false;
    }
}

void SourceProvider::readWhitespacesAndNewLines() {
    auto p = peek();
    while (p == ' ' || p == '\t' || p == '\n' || p == '\r') {
        increment();
        p = peek();
    }
}