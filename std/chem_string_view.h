// Copyright (c) Qinetik 2024.

#pragma once

#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace chem {

    class string_view {
    public:
        using value_type = char;
        using size_type = std::size_t;
        using pointer = const char*;
        using reference = const char&;
        using iterator = pointer;
        using const_iterator = const char*;

        constexpr string_view() noexcept: data_(nullptr), size_(0) {}

        constexpr string_view(const char* str)
                : data_(str), size_(str ?
                                    // GCC 7 doesn't have constexpr char_traits. Fall back to __builtin_strlen.
                                    #if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 8
                                    __builtin_strlen(Str)
                                    #else
                                    std::char_traits<char>::length(str)
                                    #endif
                                        : 0) {}

        constexpr string_view(const char* str, size_type len)
                : data_(str), size_(len) {}

        constexpr reference operator[](size_type idx) const {
            if (idx >= size_) throw std::out_of_range("string_view index out of range");
            return data_[idx];
        }

        [[nodiscard]]
        constexpr reference front() const {
            if (size_ == 0) throw std::out_of_range("string_view is empty");
            return data_[0];
        }

        [[nodiscard]]
        constexpr reference back() const {
            if (size_ == 0) throw std::out_of_range("string_view is empty");
            return data_[size_ - 1];
        }

        [[nodiscard]]
        constexpr pointer data() const noexcept { return data_; }

        [[nodiscard]]
        constexpr size_type size() const noexcept { return size_; }

        [[nodiscard]]
        constexpr bool empty() const noexcept { return size_ == 0; }

        [[nodiscard]]
        constexpr iterator begin() const noexcept { return data_; }

        [[nodiscard]]
        constexpr iterator end() const noexcept { return data_ + size_; }

        [[nodiscard]]
        constexpr const_iterator cbegin() const noexcept { return data_; }

        [[nodiscard]]
        constexpr const_iterator cend() const noexcept { return data_ + size_; }

    private:
        const char* data_;
        size_type size_;
    };

}

// Overloaded operators for comparison
inline bool operator==(const chem::string_view& lhs, const chem::string_view& rhs) {
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

inline bool operator!=(const chem::string_view& lhs, const chem::string_view& rhs) {
    return !(lhs == rhs);
}

inline bool operator<(const chem::string_view& lhs, const chem::string_view& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

inline bool operator<=(const chem::string_view& lhs, const chem::string_view& rhs) {
    return !(rhs < lhs);
}

inline bool operator>(const chem::string_view& lhs, const chem::string_view& rhs) {
    return rhs < lhs;
}

inline bool operator>=(const chem::string_view& lhs, const chem::string_view& rhs) {
    return !(lhs < rhs);
}
