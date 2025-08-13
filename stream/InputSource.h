// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstddef>

class InputSource {
protected:

    // platform-independent state
    const char* data_;
    std::size_t size_;

    InputSource() : data_(nullptr), size_(0) {

    }

public:

    inline InputSource(
            const char* data_,
            std::size_t size_
    ) : data_(data_), size_(size_) {

    }

    [[nodiscard]] inline const char* data() const noexcept {
        return data_;
    }

    [[nodiscard]] inline std::size_t size() const noexcept {
        return size_;
    }

};