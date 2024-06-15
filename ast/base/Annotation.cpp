// Copyright (c) Qinetik 2024.

#include "Annotation.h"
#include "ast/base/Value.h"

Annotation::Annotation(std::string name) : name(std::move(name)) {

}

Annotation* Annotation::get_annotation(const std::string& expected) {
    if(name == expected) {
        return this;
    }
    for(auto& ann : extends) {
        if(ann.has_annotation(expected)) {
            return &ann;
        }
    }
    return nullptr;
}