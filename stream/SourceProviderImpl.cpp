#include "SourceProvider.h"
#include <iostream>

void SourceProvider::restore(StreamPosition &position) {
    stream->seekg(position.pos, std::ios::beg);
    lineNumber = position.line;
    lineCharacterNumber = position.character;
}

unsigned int SourceProvider::currentPosition() const {
    return stream->tellg();
}

char SourceProvider::readCharacter() {
    auto c = stream->get();
    handleCharacterRead(c);
    return c;
}

bool SourceProvider::eof() const {
    return stream->eof();
}

char SourceProvider::peek() const {
    return stream->peek();
}

char SourceProvider::peek(int ahead) {
    unsigned int pos = stream->tellg();
    stream->seekg(pos + ahead, std::ios::beg);
    char c = stream->get();
    stream->seekg(pos, std::ios::beg);
    return c;
}

std::string SourceProvider::readUntil(char stop) {
    auto read = "";
    char currChar;
    while (true) {
        currChar = readCharacter();
        if (currChar == stop || eof()) {
            return read;
        } else {
            read += currChar;
        }
    }
}

bool SourceProvider::increment(char c) {
    if (stream->get() == c) {
        handleCharacterRead(c);
        return true;
    } else {
        stream->seekg(currentPosition() - 1, std::ios::beg);
        return false;
    }
}

std::string SourceProvider::readAllFromHere() {
    std::string source;
    while (!eof()) {
        source += readCharacter();
    }
    return source;
}

std::string SourceProvider::readAllFromBeg() {
    stream->seekg(0, std::ios::beg);
    return readAllFromHere();
}

void SourceProvider::printAll() {
    while (!eof()) {
        std::cout << readCharacter();
    }
}

bool SourceProvider::increment(const std::string &text, bool peek) {

    if (stream->peek() != text[0]) {
        return false;
    }

    // Save current pos
    auto prevPosition = getStreamPosition();

    bool result = true;
    int pos = 0;
    while (!stream->eof() && pos < text.size()) {
        char c = readCharacter();
        if (c != text[pos]) {
            result = false;
            break;
        } else {
            pos++;
        }
    }

    // Seek back to original pos
    if (!result || peek) {
        restore(prevPosition);
    }

    return result;
}

unsigned int SourceProvider::getLineNumber() const {
    return lineNumber;
}

unsigned int SourceProvider::getLineCharNumber() const {
    return lineCharacterNumber;
}

StreamPosition SourceProvider::getStreamPosition() const {
    return StreamPosition{static_cast<unsigned int>(currentPosition()), lineNumber, lineCharacterNumber};
}

void SourceProvider::reset() {
    this->stream->seekg(0, std::ios::beg);
    this->lineCharacterNumber = 0;
    this->lineNumber = 0;
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

std::string SourceProvider::readAnything(char until) {
    return readAnything([&]() -> bool {
        return peek() != until;
    });
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

std::string SourceProvider::readUnsignedInt() {
    if (!std::isdigit(peek())) return "";
    return readAnything([&]() -> bool {
        auto p = peek();
        return p >= '0' && p <= '9';
    });
}

std::string SourceProvider::readNumber() {
    if (peek() != '-' && !std::isdigit(peek())) return "";
    auto appearedDot = false;
    auto first_char = true;
    return readAnything([&]() -> bool {
        auto c = peek();
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
    });
}

std::string SourceProvider::readAlpha() {
    std::string str;
    while (!eof() && std::isalpha(peek())) {
        str.append(1, readCharacter());
    }
    return str;
}

std::string SourceProvider::readAlphaNum() {
    std::string str;
    while (!eof() && std::isalnum(peek())) {
        str.append(1, readCharacter());
    }
    return str;
}

std::string SourceProvider::readIdentifier() {
    if (std::isalpha(peek()) || peek() == '_') {
        std::string str;
        while (!eof() && (std::isalnum(peek()) || peek() == '_')) {
            str.append(1, readCharacter());
        }
        return str;
    } else {
        return "";
    }
}

void SourceProvider::readAnnotationIdentifier(std::string str) {
    if (std::isalpha(peek()) || peek() == '_') {
        while (!eof() && (std::isalnum(peek()) || peek() == '_' || peek() == ':')) {
            str.append(1, readCharacter());
        }
    }
}

unsigned int SourceProvider::readWhitespaces() {
    unsigned int whitespaces = 0;
    while (!eof() && (peek() == ' ' || peek() == '\t')) {
        readCharacter();
        whitespaces++;
    }
    return whitespaces;
}

bool SourceProvider::hasNewLine() {
    return peek() == '\n' || peek() == '\r';
}

bool SourceProvider::readNewLineChars() {
    auto peak = peek();
    if (peak == '\n') {
        readCharacter();
        return true;
    } else if (peak == '\r') {
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
    while (!eof() && (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r')) {
        readCharacter();
    }
}

Position SourceProvider::position() {
    return {getLineNumber(), getLineCharNumber()};
}

Position SourceProvider::backPosition(unsigned int back) {
    return {getLineNumber(), getLineCharNumber() - back};
}