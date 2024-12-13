#include "SourceProvider.h"
#include <iostream>

void SourceProvider::restore(const StreamPosition &position) {
    const auto curr_pos = stream->tell();
    if(curr_pos != position.pos) {
        stream->seek(position.pos, SEEK_SET);
        bufferFill(position.bufferSize);
    }
    lineNumber = position.line;
    lineCharacterNumber = position.character;
    bufferPos = position.bufferPos;
}

void SourceProvider::bufferFill(size_t size) {
    buffer.clear();
    buffer.resize(size);
    size_t bytesRead = stream->read(buffer.data(), size);
    buffer.resize(bytesRead);
    bufferPos = 0;
}

char SourceProvider::readCharacter() {
    if (bufferPos >= buffer.size()) {
        bufferFill();
        if (buffer.empty()) {
            return EOF;
        }
    }
    char c = buffer[bufferPos++];
    handleCharacterRead(c);
    return c;
}

bool SourceProvider::eof() const {
    if (bufferPos < buffer.size()) {
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
    if (bufferPos >= buffer.size()) {
        bufferFill();
        if (buffer.empty()) {
            return EOF;
        }
    }
    return buffer[bufferPos];
}

void SourceProvider::readUntil(chem::string* into, char stop) {
    char currChar;
    while (true) {
        currChar = readCharacter();
        if (currChar == stop || eof()) {
            return;
        } else {
            into->append(currChar);
        }
    }
}

void SourceProvider::readUntil(std::string& into, char stop) {
    char currChar;
    while (true) {
        currChar = readCharacter();
        if (currChar == stop || eof()) {
            return;
        } else {
            into.append(1, currChar);
        }
    }
}

bool SourceProvider::increment(char c) {
    if (peek() == c) {
        readCharacter();
        return true;
    }
    return false;
}

std::string SourceProvider::readAllFromHere() {
    std::string source;
    while (!eof()) {
        source += readCharacter();
    }
    return source;
}

void SourceProvider::printAll() {
    while (!eof()) {
        std::cout << readCharacter();
    }
}

void SourceProvider::increment_amount(unsigned amount) {
    unsigned i = 0;
    while(i < amount) {
        readCharacter();
        i++;
    };
}

unsigned int SourceProvider::getLineNumber() const {
    return lineNumber;
}

unsigned int SourceProvider::getLineCharNumber() const {
    return lineCharacterNumber;
}

StreamPosition SourceProvider::getStreamPosition() {
    if(bufferPos >= buffer.size()) {
        bufferFill();
    }
    return StreamPosition {
            stream->tell(), lineNumber, lineCharacterNumber, buffer.size(), bufferPos
    };
}

void SourceProvider::reset() {
    stream->seek(0, SEEK_SET);
    lineCharacterNumber = 0;
    lineNumber = 0;
    buffer.clear();
    bufferPos = 0;
}

void SourceProvider::switch_source(InputSource* source) {
    stream = source;
    lineCharacterNumber = 0;
    lineNumber = 0;
    buffer.clear();
    bufferPos = 0;
}

void SourceProvider::readEscaping(chem::string* value, char stopAt) {
    bool skip_one = false;
    char read;
    while (!eof()) {
        read = readCharacter();
        value->append(read);
        if(skip_one) {
            skip_one = false;
        } else {
            if (read == stopAt) {
                break;
            }
            skip_one = read == '\\';
        }
    };
}

void SourceProvider::readEscaping(std::string &value, char stopAt) {
    bool skip_one = false;
    char read;
    while (!eof()) {
        read = readCharacter();
        value.append(1, read);
        if(skip_one) {
            skip_one = false;
        } else {
            if (read == stopAt) {
                break;
            }
            skip_one = read == '\\';
        }
    };
}

void SourceProvider::readAnything(chem::string* str, char until) {
    while (!eof() && peek() != until) {
        str->append(readCharacter());
    }
}

void SourceProvider::readAnything(std::string& str, char until) {
    while (!eof() && peek() != until) {
        str.append(1, readCharacter());
    }
}

void SourceProvider::readUnsignedInt(chem::string* str) {
    if (!std::isdigit(peek())) return;
    while (true) {
        const char p = peek();
        if(!eof() && p >= '0' && p <= '9') {
            str->append(readCharacter());
        } else {
            break;
        }
    }
}

void SourceProvider::readUnsignedInt(std::string& str) {
    if (!std::isdigit(peek())) return;
    while (true) {
        const char p = peek();
        if(!eof() && p >= '0' && p <= '9') {
            str.append(1, readCharacter());
        } else {
            break;
        }
    }
}

bool keep_reading_number(const char c, bool& appearedDot) {
    if (c >= '0' && c <= '9') {
        return true;
    } else if (c == '.' && !appearedDot) {
        appearedDot = true;
        return true;
    } else {
        return false;
    }
}

bool keep_reading_any_number(const char c, bool& appearedDot) {
    if (std::isalnum(c)) {
        return true;
    } else if (c == '.' && !appearedDot) {
        appearedDot = true;
        return true;
    } else {
        return false;
    }
}

void SourceProvider::readNumber(chem::string* string) {
    const auto p = peek();
    if (p != '-' && !std::isdigit(p)) return;
    // read the first character
    string->append(readCharacter());
    auto appearedDot = false;
    while (!eof() && keep_reading_number(peek(), appearedDot)) {
        string->append(readCharacter());
    }
}

void SourceProvider::readNumber(std::string& str) {
    const auto p = peek();
    if (p != '-' && !std::isdigit(p)) return;
    // read the first character
    str.append(1, readCharacter());
    auto appearedDot = false;
    while (!eof() && keep_reading_number(peek(), appearedDot)) {
        str.append(1, readCharacter());
    }
}

void SourceProvider::readAnyNumber(chem::string* string) {
    const auto p = peek();
    if (p != '-' && !std::isdigit(p)) return;
    // read the first character
    string->append(readCharacter());
    auto appearedDot = false;
    while (!eof() && keep_reading_any_number(peek(), appearedDot)) {
        string->append(readCharacter());
    }
}

void SourceProvider::readAnyNumber(std::string& str) {
    const auto p = peek();
    if (p != '-' && !std::isdigit(p)) return;
    // read the first character
    str.append(1, readCharacter());
    auto appearedDot = false;
    while (!eof() && keep_reading_any_number(peek(), appearedDot)) {
        str.append(1, readCharacter());
    }
}

void SourceProvider::readAlpha(chem::string* str) {
    while (std::isalpha(peek())) {
        str->append(readCharacter());
    }
}

void SourceProvider::readAlpha(std::string& str) {
    while (std::isalpha(peek())) {
        str.append(1, readCharacter());
    }
}

void SourceProvider::readAlphaNum(chem::string* str) {
    while (std::isalnum(peek())) {
        str->append(readCharacter());
    }
}

void SourceProvider::readAlphaNum(std::string& str) {
    while (std::isalnum(peek())) {
        str.append(1, readCharacter());
    }
}

void SourceProvider::readIdentifier(chem::string* str) {
    auto p = peek();
    if (std::isalpha(p) || p == '_') {
        while (std::isalnum(p) || p == '_') {
            str->append(readCharacter());
            p = peek();
        }
    }
}

void SourceProvider::readIdentifier(std::string& str) {
    auto p = peek();
    if (std::isalpha(p) || p == '_') {
        while (std::isalnum(p) || p == '_') {
            str.append(1, readCharacter());
            p = peek();
        }
    }
}

void SourceProvider::readAnnotationIdentifier(chem::string* str) {
    auto p = peek();
    if (std::isalpha(p) || p == '_') {
        while (std::isalnum(p) || p == '_' || p == ':' || p == '.') {
            str->append(readCharacter());
            p = peek();
        }
    }
}

void SourceProvider::readAnnotationIdentifier(std::string& str) {
    auto p = peek();
    if (std::isalpha(p) || p == '_') {
        while (std::isalnum(p) || p == '_' || p == ':') {
            str.append(1, readCharacter());
            p = peek();
        }
    }
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