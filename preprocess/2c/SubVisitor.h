// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"

class ToCAstVisitor;

class SubVisitor {
public:

    /**
     * c visitor
     */
    ToCAstVisitor& visitor;

    /**
     * constructor
     */
    SubVisitor(ToCAstVisitor& visitor) : visitor(visitor) {

    };

    /**
     * space fn using visitor
     */
    inline void space() const;

    /**
     * write fn using visitor
     */
    inline void write(char value) const;

    /**
     * write fn using visitor
     */
    inline void write(const chem::string_view& value) const;

    /**
     * new line and indent to current indentation level
     */
    inline void new_line_and_indent();

    /**
     * reset the visitor, to translate another set of nodes
     */
    virtual void reset() {
        // does nothing
    }

};