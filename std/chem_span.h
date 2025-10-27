// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstddef>
#include <stdexcept>
#include <iterator>
#include <algorithm>

namespace chem {

    template<typename T>
    class span {
    public:
        using element_type = T;
        using value_type = std::remove_cv_t<T>;
        using size_type = std::size_t;
        using pointer = T*;
        using reference = T&;
        using iterator = pointer;
        using const_iterator = const T*;

        span() noexcept: data_(nullptr), size_(0) {}

        span(T* ptr, size_type count) : data_(ptr), size_(count) {}

        template<std::size_t N>
        span(T (& arr)[N]) noexcept : data_(arr), size_(N) {}

        template<typename Container>
        span(Container& container)
                : data_(container.data()), size_(container.size()) {}

        reference operator[](size_type idx) const {
            return data_[idx];
        }

        reference front() const {
            return data_[0];
        }

        reference back() const {
            return data_[size_ - 1];
        }

        pointer data() const noexcept { return data_; }

        size_type size() const noexcept { return size_; }

        bool empty() const noexcept { return size_ == 0; }

        iterator begin() const noexcept { return data_; }

        iterator end() const noexcept { return data_ + size_; }

        const_iterator cbegin() const noexcept { return data_; }

        const_iterator cend() const noexcept { return data_ + size_; }

    private:
        T* data_;
        size_type size_;
    };

}

// Overloaded operators for comparison
template <typename T>
bool operator==(const chem::span<T>& lhs, const chem::span<T>& rhs) {
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template <typename T>
bool operator!=(const chem::span<T>& lhs, const chem::span<T>& rhs) {
    return !(lhs == rhs);
}

template <typename T>
bool operator<(const chem::span<T>& lhs, const chem::span<T>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T>
bool operator<=(const chem::span<T>& lhs, const chem::span<T>& rhs) {
    return !(rhs < lhs);
}

template <typename T>
bool operator>(const chem::span<T>& lhs, const chem::span<T>& rhs) {
    return rhs < lhs;
}

template <typename T>
bool operator>=(const chem::span<T>& lhs, const chem::span<T>& rhs) {
    return !(lhs < rhs);
}
