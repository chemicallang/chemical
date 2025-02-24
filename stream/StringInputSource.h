// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "InputSource.h"

class StringInputSource : public InputSource {
private:
    const char* data;
    size_t size;
    size_t position = 0;
public:

    StringInputSource(const std::string &str)
            : data(str.data()), size(str.size()) {}

    StringInputSource(const char* str, size_t len)
            : data(str), size(len) {}

    size_t read(char* buffer, size_t size) final {
        size_t bytesToRead = std::min(size, this->size - position);
        std::memcpy(buffer, data + position, bytesToRead);
        position += bytesToRead;
        return bytesToRead;
    }

    off_t seek(off_t offset, int whence) final {
        if (whence == SEEK_SET) {
            position = offset;
        } else if (whence == SEEK_CUR) {
            position += offset;
        } else if (whence == SEEK_END) {
            position = size + offset;
        }
        return position;
    }

    [[nodiscard]] off_t tell() const final {
        return position;
    }

};