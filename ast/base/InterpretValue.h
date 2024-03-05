// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <unordered_map>

using scope_vars = std::unordered_map<std::string, Value*>&;

class InterpretValue {
public:

    virtual std::string* getScopeIdentifier() {
        return nullptr;
    }

    virtual Value* getChild(std::string* name) {
        return nullptr;
    }

    /**
     * This would return the representation of the node
     * @return
     */
    virtual std::string interpret_representation() const {
        return "[InterpretValueRepresentation]";
    };

    virtual void set_in_parent(scope_vars vars, Value* value) {

    }

    /**
     * this function is called with the current scope variables on the first element of the access chain
     * this allows locating the first element of the access chain in the scope above
     * once located, we can travel that variable to locate the rest of the access chain and use that value
     * @param scopeVars
     * @return
     */
    virtual Value * find_in_parent(scope_vars &scopeVars) {
        return nullptr;
    }

    /**
     * This is called by for example, assignment statement, to locate the access chain completely
     * in the scope variables given, so that lhs value can be set
     * It is also called by the function for its parameters to locate them
     * @param scopeVars
     * @return
     */
    virtual InterpretValue* travel(scope_vars scopeVars) {
        return nullptr;
    }

};