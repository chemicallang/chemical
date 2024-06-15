// Copyright (c) Qinetik 2024.

#include "Annotation.h"
#include "ast/base/Value.h"

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