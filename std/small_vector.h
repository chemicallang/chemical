// Copyright (c) Chemical Language Foundation 2025.

#pragma once

//#include <iostream>
#include <cassert>
#include <memory>
#include <algorithm>

template<typename T, std::size_t N>
class SmallVector {
public:
    SmallVector()
            : size_(0), capacity_(N), data_(stack_data_) {}

    ~SmallVector() {
        if (data_ != stack_data_) {
            delete[] data_;
        }
    }

    void push_back(const T& value) {
        if (size_ >= capacity_) {
            grow();
        }
        data_[size_++] = value;
    }

    void pop_back() {
        assert(size_ > 0 && "pop_back() called on empty vector");
        --size_;
    }

    T& operator[](std::size_t index) {
        assert(index < size_ && "Index out of bounds");
        return data_[index];
    }

    const T& operator[](std::size_t index) const {
        assert(index < size_ && "Index out of bounds");
        return data_[index];
    }

    std::size_t size() const {
        return size_;
    }

    std::size_t capacity() const {
        return capacity_;
    }

    bool empty() const {
        return size_ == 0;
    }

private:
    void grow() {
        capacity_ = capacity_ * 2;
        T* new_data = new T[capacity_];
        std::copy(data_, data_ + size_, new_data);
        if (data_ != stack_data_) {
            delete[] data_;
        }
        data_ = new_data;
    }

    std::size_t size_;
    std::size_t capacity_;
    T* data_;
    T stack_data_[N];
};