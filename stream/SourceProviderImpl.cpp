#include "StreamSourceProvider.h"

void SourceProvider::readEscaping(std::string& value, char stopAt) {
    bool skip_one;
    char read;
    while(!eof()) {
        read = readCharacter();
        value.append(1, read);
        if(read == stopAt && !skip_one) {
            break;
        }
        skip_one = read == '\\';
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

void SourceProvider::readAnnotationIdentifier(std::string str){
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