// Copyright (c) Qinetik 2024.

#pragma once

#include "VariableIdentifier.h"

class NamespaceIdentifier : public VariableIdentifier {
public:

    using VariableIdentifier::VariableIdentifier;

    bool can_link_with_namespace() override {
        return true;
    }

};