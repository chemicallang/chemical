#include "StreamSourceProvider.h"

std::pair<char, bool> SourceProvider::escape_sequence() {
    char actualChar;
    auto found = true;
    unsigned length = 1;
    switch (peek()) {
        case 'a':
            actualChar = '\a';
            break;
        case 'f':
            actualChar = '\f';
            break;
        case 'r':
            actualChar = '\r';
            break;
        case 'n':
            actualChar = '\n';
            break;
        case '0':
            actualChar = '\0';
            break;
        case 't':
            actualChar = '\t';
            break;
        case 'v':
            actualChar = '\v';
            break;
        case 'b':
            actualChar = '\b';
            break;
        case '\\':
            actualChar = '\\';
            break;
        case '"':
            actualChar = '"';
            break;
        case '?':
            actualChar = '\?';
            break;
        case 'x':
            if (peek(1) == '1' && peek(2) == 'b') {
                actualChar = '\x1b';
                length = 3;
            } else {
                actualChar = 'x';
                found = false;
            }
            break;
        default:
            actualChar = peek();
            found = false;
            break;
    }

    // consuming the escape sequence
    if (found) {
        while (length > 0) {
            readCharacter();
            length--;
        }
    }

    return {actualChar, found};
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

std::string SourceProvider::readAnnotation() {
    if (std::isalpha(peek()) || peek() == '_') {
        std::string str;
        while (!eof() && (std::isalnum(peek()) || peek() == '_' || peek() == ':')) {
            str.append(1, readCharacter());
        }
        return str;
    } else {
        return "";
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

Position SourceProvider::position() {
    return {getLineNumber(), getLineCharNumber()};
}

Position SourceProvider::backPosition(unsigned int back) {
    return {getLineNumber(), getLineCharNumber() - back};
}