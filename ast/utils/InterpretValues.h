// Copyright (c) Qinetik 2024.

/*
 * This class defines some values that are used within interpretation only
 * These values are initialized by global functions
 */

#pragma once

#include "ast/base/Value.h"
#include <functional>
#include <map>

using MemValueFn = std::function<Value *(InterpretScope &, Value *, std::vector<std::unique_ptr<Value>> &)>;

class MemberFnsValue : public Value {
public:

    std::unordered_map<std::string, MemValueFn> members;

    MemberFnsValue(std::unordered_map<std::string, MemValueFn> members) : members(std::move(members)) {

    }

    void accept(Visitor &visitor) override {
        // no visitor can visit this
    }

};

class InterpretVectorValue : public Value {
public:

    std::vector<std::unique_ptr<Value>> values;
    std::unordered_map<std::string, MemValueFn> &members;

    InterpretVectorValue(
            std::vector<std::unique_ptr<Value>> values,
            std::unordered_map<std::string, MemValueFn> &members
    ) : values(std::move(values)), members(members) {

    }

    void accept(Visitor &visitor) override {
        // no visitor can visit this
    }

    Value *child(InterpretScope& scope, const std::string &name) override {
        return nullptr;
    }

    Value *initializer_value(InterpretScope &scope) override {
        return this;
    }

    std::string representation() const override {
        std::string rep;
        unsigned i = 0;
        while(i < values.size()){
            rep.append(values[i]->representation());
            if(i < values.size() - 1) rep.append(1, ',');
            i++;
        }
        return rep;
    }

    Value *
    call_member(InterpretScope &scope, const std::string &name, std::vector<std::unique_ptr<Value>> &vals) override {
        auto fn = members.find(name);
        if (fn == members.end()) {
            scope.error("function with name " + name + " doesn't exist on vector");
            return nullptr;
        } else {
            return fn->second(scope, this, vals);
        }
    }

    Value *index(InterpretScope& scope, int i) override {
        if (values[i]->primitive()) {
            return values[i]->copy();
        } else {
            return values[i].get();
        }
    }

    Value *copy() override {
        return (Value *) this;
    }

    ValueType value_type() const override {
        return ValueType::Vector;
    }

    InterpretVectorValue * as_vector() override {
        return this;
    }

};

class InterpretUnorderedMap : public Value {
public:

    std::unordered_map<std::string, std::unique_ptr<Value>> values;
    std::unordered_map<std::string, MemValueFn> *members;

    InterpretUnorderedMap(
            std::unordered_map<std::string, std::unique_ptr<Value>> values,
            std::unordered_map<std::string, MemValueFn> *members
    ) : values(std::move(values)), members(members) {

    }

    void accept(Visitor &visitor) override {
        // no visitor can visit this
    }

    Value *child(InterpretScope& scope, const std::string &name) override {
        auto val = values.find(name);
        if (val != values.end()) {
            auto act = val->second.get();
            if (act->primitive()) {
                return act->copy();
            } else {
                return act;
            }
        } else {
            return nullptr;
        }
    }

    Value *
    call_member(InterpretScope &scope, const std::string &name, std::vector<std::unique_ptr<Value>> &vals) override {
        auto fn = members->find(name);
        if (fn == members->end()) {
            scope.error("function with name " + name + " doesn't exist on unordered_map");
        } else {
            return fn->second(scope, this, vals);
        }
    }

    Value *copy() override {
        return (Value *) this;
    }

    Value *evaluated_value(InterpretScope &scope) override {
        return this;
    }

};

class InterpretMapValue : public Value {
public:

    std::map<std::string, std::unique_ptr<Value>> values;
    std::unordered_map<std::string, MemValueFn> &members;

    InterpretMapValue(
            std::map<std::string, std::unique_ptr<Value>> values,
            std::unordered_map<std::string, MemValueFn> &members
    ) : values(std::move(values)), members(members) {

    }

    void accept(Visitor &visitor) override {
        // no visitor can visit this
    }

    Value *child(InterpretScope& scope, const std::string &name) override {
        auto val = values.find(name);
        if (val != values.end()) {
            auto act = val->second.get();
            if (act->primitive()) {
                return act->copy();
            } else {
                return act;
            }
        } else {
            return nullptr;
        }
    }

    Value *
    call_member(InterpretScope &scope, const std::string &name, std::vector<std::unique_ptr<Value>> &vals) override {
        auto fn = members.find(name);
        if (fn == members.end()) {
            scope.error("function with name " + name + " doesn't exist on map");
        } else {
            return fn->second(scope, this, vals);
        }
    }

    Value *copy() override {
        return (Value *) this;
    }

    Value *evaluated_value(InterpretScope &scope) override {
        return this;
    }

};