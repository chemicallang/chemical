#include "SourceProvider.h"
#include <iostream>

void SourceProvider::restore(const StreamPosition &position) {
    const auto curr_pos = stream->tell();
    if(curr_pos != position.pos) {
        stream->seek(position.pos, SEEK_SET);
        bufferFill();
    }
    lineNumber = position.line;
    lineCharacterNumber = position.character;
    bufferPos = position.bufferPos;
}

void SourceProvider::bufferFill() {
    size_t bytesRead = stream->read(buffer, BUFFER_CAPACITY);
    bufferSize = bytesRead;
    bufferPos = 0;
}

char SourceProvider::readCharacter() {
    if (bufferPos >= bufferSize) {
        bufferFill();
        if (bufferSize == 0) {
            return EOF;
        }
    }
    char c = buffer[bufferPos++];
    handleCharacterRead(c);
    return c;
}

bool SourceProvider::eof() const {
    if (bufferPos < bufferSize) {
        return false; // Still data in the buffer
    }
    if(stream->tell() == -1) {
        return true;
    }
    // At this point, the buffer is exhausted. Check if the input source itself has reached EOF.
    char dummy;
    // seek back
    if(stream->read(&dummy, 1) == 0) {
        return true;
    } else {
        stream->seek(-1, SEEK_CUR);
        return false;
    }
}

char SourceProvider::peek() {
    if (bufferPos >= bufferSize) {
        bufferFill();
        if (bufferSize == 0) {
            return EOF;
        }
    }
    return buffer[bufferPos];
}

bool SourceProvider::increment(char c) {
    if (peek() == c) {
        readCharacter();
        return true;
    }
    return false;
}

unsigned int SourceProvider::getLineNumber() const {
    return lineNumber;
}

unsigned int SourceProvider::getLineCharNumber() const {
    return lineCharacterNumber;
}

StreamPosition SourceProvider::getStreamPosition() {
    if(bufferPos >= bufferSize) {
        bufferFill();
    }
    return StreamPosition {
            stream->tell(), lineNumber, lineCharacterNumber, bufferPos
    };
}

void SourceProvider::reset() {
    stream->seek(0, SEEK_SET);
    lineCharacterNumber = 0;
    lineNumber = 0;
    bufferPos = 0;
}

void SourceProvider::switch_source(InputSource* source) {
    stream = source;
    lineCharacterNumber = 0;
    lineNumber = 0;
    bufferPos = 0;
}

unsigned int SourceProvider::readWhitespaces() {
    unsigned int whitespaces = 0;
    auto p = peek();
    while (p == ' ' || p == '\t') {
        readCharacter();
        if(p == ' ') {
            whitespaces++;
        } else if(p == '\t') {
            whitespaces += 4;
        }
        p = peek();
    }
    return whitespaces;
}

bool SourceProvider::hasNewLine() {
    const auto p = peek();
    return p == '\n' || p == '\r';
}

bool SourceProvider::readNewLineChars() {
    auto p = peek();
    if (p == '\n') {
        readCharacter();
        return true;
    } else if (p == '\r') {
        // consuming the \r
        readCharacter();
        // consume also the next \n
        if (peek() == '\n') readCharacter();
        return true;
    } else {
        return false;
    }
}

void SourceProvider::readWhitespacesAndNewLines() {
    auto p = peek();
    while (p == ' ' || p == '\t' || p == '\n' || p == '\r') {
        readCharacter();
        p = peek();
    }
}