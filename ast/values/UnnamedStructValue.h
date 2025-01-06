// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Value.h"
#include <unordered_map>

class UnnamedStructValue : public Value {
public:

    std::unordered_map<chem::string_view, StructMemberInitializer> values;
    SourceLocation location;
#ifdef COMPILER_BUILD
    llvm::AllocaInst* allocaInst = nullptr;
#endif

    UnnamedStructValue(SourceLocation location) : location(location) {

    }

    ValueKind val_kind() override {
        return ValueKind::UnnamedStructValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    SourceLocation encoded_location() override {
        return location;
    }


};