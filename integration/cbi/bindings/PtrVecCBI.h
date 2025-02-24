// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>

class CSTToken;

extern "C" {

    void* PtrVec_get(std::vector<void*>* vec, unsigned int i);

    void PtrVec_set(std::vector<void*>* vec, unsigned int i, void*);

    void PtrVec_push(std::vector<void*>* vec, void*);

    void PtrVec_erase(std::vector<void*>* vec, unsigned int i);

    std::size_t PtrVec_size(std::vector<void*>* vec);

}