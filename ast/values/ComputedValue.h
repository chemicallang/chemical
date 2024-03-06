// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "WrapperValue.h"

/**
 * A computed value stores a value that must be deleted at the end of the scope
 */
class ComputedValue : public WrapperValue {
public:

    ComputedValue(Value* value) : WrapperValue(value) {

    }

    bool delete_value() const override {
        return true;
    }

    ~ComputedValue(){
        delete value;
    }

};