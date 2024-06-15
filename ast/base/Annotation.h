// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <vector>
#include <string>

class Value;

class Annotation {
public:

    std::string name;
    std::vector<Annotation> extends;
    std::vector<std::unique_ptr<Value>> values;

    /**
     * constructor
     */
    Annotation(
            std::string name
    );

    /**
     * gets annotation with name
     */
    Annotation* get_annotation(const std::string& expected);

    /**
     * checks if the given annotation is present
     */
    inline bool has_annotation(const std::string& expected) {
        return get_annotation(expected) != nullptr;
    }

};