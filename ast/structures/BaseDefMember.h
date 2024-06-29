// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AnnotableNode.h"

class BaseDefMember : public AnnotableNode {
public:

    std::string name;

    BaseDefMember(
        std::string name
    );

    virtual bool requires_destructor() = 0;

    virtual Value* default_value() {
        return nullptr;
    }

};