// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <memory>
#include <vector>
#include <string>
#include "AnnotationKind.h"

class Value;

class Annotation {
public:

    AnnotationKind kind;
    std::vector<Annotation> extends;
    std::vector<Value*> values;

    /**
     * constructor
     */
    Annotation(
        AnnotationKind kind
    );

    /**
     * deleted copy constructor
     */
    Annotation(const Annotation& ann) = delete;

    /**
     * default move constructor
     */
    Annotation(Annotation&&) = default;

    /**
     * get annotations by this kind
     */
    void get_all(std::vector<Annotation*>& into, AnnotationKind expected, bool consider_self = true);

    /**
     * gets annotation with name
     */
    Annotation* get_annotation(AnnotationKind expected, bool consider_self = true);

    /**
     * checks if the given annotation is present
     */
    inline bool has_annotation(AnnotationKind expected) {
        return get_annotation(kind) != nullptr;
    }

    ~Annotation();

};