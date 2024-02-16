//
// Created by wakaz on 10/12/2023.
//

#include <istream>
#include "SourceProvider.h"

class StreamSourceProvider : public SourceProvider {
public:

    explicit StreamSourceProvider(std::istream &stream) : stream(stream) {}

    int position() override {
        return stream.tellg();
    }

    char readCharacter() override {
        auto c = stream.get();
        if (c == '\n' || c == '\x0C' || (c == '\r' && stream.peek() != '\n')) {
            // if there's no \n next to \r, the line ending must be CR, so we treat it as line ending
            lineNumber++;
        }
        return c;
    }

    bool eof() override {
        return stream.eof();
    }

    char peek() override {
        return stream.peek();
    }

    char peek(int ahead) override {
        int pos = stream.tellg();
        stream.seekg(pos + ahead, std::ios::beg);
        char c = stream.get();
        stream.seekg(pos);
        return c;
    }

    bool increment(char c) override {
        if (stream.peek() == c) {
            stream.seekg(position() + 1, std::ios::beg);
            return true;
        }
        return false;
    }

    bool increment(const std::string &text) override {

        // Save current position
        std::streampos peekPos = stream.tellg();
        int prevLineNumber = lineNumber;

        bool result = true;
        int pos = 0;
        while (!stream.eof() && pos < text.size()) {
            char c = readCharacter();
            if (c != text[pos]) {
//                std::cout << "NotFound:" << text[pos] << ":" << c << std::endl;
                result = false;
                break;
            } else {
                pos++;
            }
        }

        // Seek back to original position
        if (!result) {
            stream.seekg(peekPos, std::ios::beg);
            lineNumber = prevLineNumber;
        }

        return result;
    }

private:
    std::istream &stream;

    /**
     * By default, this counts lines
     * On every character read, the provider checks if the line has ended and increments
     */
    int lineNumber;

};
