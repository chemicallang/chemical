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
        return stream.get();
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

    bool increment(const std::string& text) override {
        // Save current position
        std::streampos peekPos = stream.tellg();

        bool result = true;
        int pos = 0;
        while(!stream.eof() && pos < text.size()) {
            char c = stream.get();
            if(c != text[pos]) {
//                std::cout << "NotFound:" << text[pos] << ":" << c << std::endl;
                result = false;
                break;
            } else {
                pos++;
            }
        }

        // Seek back to original position
        if(!result) {
            stream.seekg(peekPos, std::ios::beg);
        }

        return result;
    }

private:
    std::istream &stream;

};
