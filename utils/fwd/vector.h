// Copyright (c) Qinetik 2024.

#pragma once

namespace std {

    template <class _Tp>
    class allocator;

    template <class _Ty, class _Alloc>
    class vector; // varying size array of values

}

template <typename _Tp, typename _Alloc = std::allocator<_Tp>>
using vector = std::vector<_Tp, _Alloc>;