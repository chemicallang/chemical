// Copyright (c) Chemical Language Foundation 2025.

#include "PtrVecCBI.h"
#include <vector>

void* PtrVec_get(std::vector<void*>* vec, unsigned int i) {
    return (*vec)[i];
}

void PtrVec_set(std::vector<void*>* vec, unsigned int i, void* ptr) {
    (*vec)[i] = ptr;
}

void PtrVec_push(std::vector<void*>* vec, void* ptr) {
    vec->emplace_back(ptr);
}

void PtrVec_erase(std::vector<void*>* vec, unsigned int i) {
    vec->erase(vec->begin() + i);
}

std::size_t PtrVec_size(std::vector<void*>* vec) {
    return vec->size();
}