// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#pragma once

#include "SemanticAnalyzer.h"

class ScopeAnalyzer : public SemanticAnalyzer {
public:

    /**
     * constructor
     * @param tokens
     */
    ScopeAnalyzer(std::vector<std::unique_ptr<LexToken>> &tokens) : SemanticAnalyzer(tokens) {

    }

    /**
     * when you call this, this function will call scope_begins and scope_ends
     * everytime a scope begins and ends in the tokens vector
     */
    void analyze_scopes();

    /**
     * will be called when a scope begins
     * @param position
     */
    virtual void scope_begins(unsigned int position) = 0;

    /**
     * will be called when a scope ends
     * @param position
     */
    virtual void scope_ends(unsigned int position) = 0;

};