// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/AnnotableNode.h"

class BaseDefMember : public AnnotableNode {
public:

    std::string name;
    std::unique_ptr<BaseType> type;

    BaseDefMember(
        std::string name,
        std::unique_ptr<BaseType> type
    );

    virtual Value* default_value() {
        return nullptr;
    }

    virtual BaseDefMember* copy() = 0;

};