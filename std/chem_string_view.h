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
//            if (idx >= size_) throw std::out_of_range("string_view index out of range");
            return data_[idx];
        }

        [[nodiscard]]
        constexpr reference front() const {
//            if (size_ == 0) throw std::out_of_range("string_view is empty");
            return data_[0];
        }

        [[nodiscard]]
        constexpr reference back() const {
//            if (size_ == 0) throw std::out_of_range("string_view is empty");
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

        [[nodiscard]]
        inline constexpr std::string str() const noexcept {
            return { data_, size_ };
        }

        [[nodiscard]]
        inline constexpr std::string_view view() const noexcept {
            return { data_, size_ };
        }

        friend bool operator==(const string_view& lhs, const string_view& rhs) {
            return lhs.size() == rhs.size() &&
                   std::equal(lhs.begin(), lhs.end(), rhs.begin());
        }

        friend bool operator!=(const string_view& lhs, const string_view& rhs) {
            return !(lhs == rhs);
        }

        friend bool operator<(const string_view& lhs, const string_view& rhs) {
            return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }

        friend bool operator<=(const string_view& lhs, const string_view& rhs) {
            return !(rhs < lhs);
        }

        friend bool operator>(const string_view& lhs, const string_view& rhs) {
            return rhs < lhs;
        }

        friend bool operator>=(const string_view& lhs, const string_view& rhs) {
            return !(lhs < rhs);
        }

    private:
        const char* data_;
        size_type size_;
    };

}

// Hash specialization for my_string_view
namespace std {
    template <>
    struct hash<chem::string_view> {
        // Hash Specialization Using FNV-1a
        std::size_t operator()(const chem::string_view& s) const noexcept {
            std::size_t hash = 0xcbf29ce484222325; // FNV offset basis
            std::size_t prime = 0x100000001b3;     // FNV prime
            for (char c : s) {
                hash ^= static_cast<std::size_t>(c);
                hash *= prime;
            }
            return hash;
        }
    };
}