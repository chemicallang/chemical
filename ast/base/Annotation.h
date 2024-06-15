// Copyright (c) Qinetik 2024.

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
    std::vector<std::unique_ptr<Value>> values;

    /**
     * constructor
     */
    Annotation(
            AnnotationKind kind
    );

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

};