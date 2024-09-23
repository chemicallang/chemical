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

bool SourceProvider::bufferStretch(uint16_t read_size) {
    const auto curr_size = buffer.size();
    buffer.resize(curr_size + read_size);
    auto bytesRead = stream->read(buffer.data() + curr_size, read_size);
    if(bytesRead < read_size) {
        buffer.resize(curr_size + bytesRead);
    }
    return bytesRead > 0;
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

char SourceProvider::peek(unsigned int ahead) {
    const size_t targetPos = bufferPos + ahead;
    if (targetPos >= buffer.size()) {
        bufferStretch();
    }
    return buffer[targetPos];
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

bool SourceProvider::increment_spaced(char c) {
    if(peek() == c && (peek(1) == ' ' || peek(1) == '\t')) {
        readCharacter();
        return true;
    }
    return false;
}

bool SourceProvider::increment_spaced(const std::string& text) {
    if(increment(text, true) && (peek(text.size()) == ' ' || peek(text.size()) == '\t')) {
        increment_amount(text.size());
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

bool SourceProvider::increment(const std::string &text, bool peek) {
    if(bufferPos >= buffer.size()) {
        bufferFill();
        if(buffer.empty()) {
            return false;
        }
    }
    unsigned int pos = bufferPos;
    for (char c : text) {
        if (pos >= buffer.size()) {
            if (!bufferStretch()) {
                return false;
            }
        }
        if (buffer[pos++] != c) {
            return false;
        }
    }
    if (!peek) {
        increment_amount(text.size());
    }
    return true;
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

std::string SourceProvider::readUntil(const std::string &ending, bool consume) {
    std::string content;
    while (!eof() && peek() != -1) {
        if (peek() == ending[0] && increment(ending, !consume)) {
            break;
        }
        content += readCharacter();
    }
    return content;
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

bool keep_reading(const char c, bool& appearedDot, bool& first_char) {
    if (first_char) {
        first_char = false;
        if (c == '-') {
            return true;
        }
    }
    if (c >= '0' && c <= '9') {
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
    auto appearedDot = false;
    auto first_char = true;
    while (!eof() && keep_reading(peek(), appearedDot, first_char)) {
        string->append(readCharacter());
    }
}

void SourceProvider::readNumber(std::string& str) {
    const auto p = peek();
    if (p != '-' && !std::isdigit(p)) return;
    auto appearedDot = false;
    auto first_char = true;
    while (!eof() && keep_reading(peek(), appearedDot, first_char)) {
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
        while (std::isalnum(p) || p == '_' || p == ':') {
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
        whitespaces++;
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

Position SourceProvider::position() {
    return {getLineNumber(), getLineCharNumber()};
}

Position SourceProvider::backPosition(unsigned int back) {
    return {getLineNumber(), getLineCharNumber() - back};
}