// Copyright (c) Qinetik 2024.

#pragma once

#include "Annotation.h"

class AnnotationParent {
public:

    /**
     * Annotations that this node contains
     */
    std::vector<Annotation> annotations;

    /**
     * traverse
     */
    void traverse(const std::function<void(Annotation*)>& traverser);

    /**
     * get annotations by this kind
     */
    void get_all(std::vector<Annotation*>& into, AnnotationKind expected);

    /**
     * gets annotation with name
     */
    Annotation* get_annotation(AnnotationKind expected);

    /**
     * checks if the given annotation is present
     */
    inline bool has_annotation(AnnotationKind expected) {
        return get_annotation(expected) != nullptr;
    }


};