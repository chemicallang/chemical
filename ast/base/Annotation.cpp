// Copyright (c) Chemical Language Foundation 2025.

#include "Annotation.h"
#include "ast/base/Value.h"
#include "AnnotationParent.h"
#include <functional>

Annotation::Annotation(AnnotationKind kind) : kind(kind) {

}

void Annotation::get_all(std::vector<Annotation*>& into, AnnotationKind expected, bool consider_self) {
    if(consider_self && kind == expected) {
        into.push_back(this);
    }
    for(auto& ann : extends) {
        ann.get_all(into, expected, true);
    }
}

Annotation* Annotation::get_annotation(AnnotationKind expected, bool consider_self) {
    if(consider_self && kind == expected) {
        return this;
    }
    for(auto& ann : extends) {
        if(ann.has_annotation(expected)) {
            return &ann;
        }
    }
    return nullptr;
}

Annotation::~Annotation() = default;

void AnnotationParent::get_all(std::vector<Annotation*>& into, AnnotationKind expected) {
    for(auto& ann : annotations) {
        ann.get_all(into, expected);
    }
}

Annotation* AnnotationParent::get_annotation(AnnotationKind expected) {
    Annotation* got = nullptr;
    for(auto& ann : annotations) {
        got = ann.get_annotation(expected);
        if(got) {
            break;
        }
    }
    return got;
}