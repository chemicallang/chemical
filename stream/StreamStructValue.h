// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/utils/GlobalFunctions.h"
#include "SourceProvider.h"
#include "ast/utils/InterpretValues.h"

class StreamStructValue : public Value {
public:

    SourceProvider &provider;
    std::unordered_map<std::string, MemValueFn> &members;

    StreamStructValue(
            SourceProvider &provider,
            std::unordered_map<std::string, MemValueFn> &members
    ) : provider(provider), members(members) {

    }

    void accept(Visitor &visitor) override {
        // no visitor can visit this
    }

    // This is a primitive value, meaning it will be copied when provided as a parameter
    // and deleted afterward, this makes it so that this value is not moved anywhere
    // user can access it from anywhere and make copied of it
    Value *copy(InterpretScope& scope) override {
        return new StreamStructValue(provider, members);
    }

    Value * evaluated_value(InterpretScope &scope) override {
        return this;
    }

    Value *
    call_member(InterpretScope &scope, const std::string &name, std::vector<std::unique_ptr<Value>> &values) override {
        auto fn = members.find(name);
        if (fn != members.end()) {
            return fn->second(scope, this, values);
        } else {
            scope.error("couldn't find function name " + name + " on SourceStream");
            return nullptr;
        }
    }

};

void define_source_stream_fns(GlobalInterpretScope &global);