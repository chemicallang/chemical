// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>

/**
 * we'll use this someday to travel the AST together
 */
class ASTPointer {

    /**
     * nodes are deeply nested in their data structure
     * what this allows us to do is, have a position pointer at the node currently
     * being worked on
     * for example there are three high level nodes, for loop, var statement and while loop
     *
     * in for loop there are two more var statements and in while loop there's one, like this
     * for {
     *    var a = 0;
     *    var b = 0;
     *    var c = 0;
     * }
     * var d = 0;
     * while {
     *    var e = 0;
     * }
     *
     * so when processing for loop, the scope begins, we push 0 onto this,
     * [0, 0] which means we are pointing to first statement in the for loop
     *
     * first statement is processed, increment is called, we have [0, 1]
     * 0 points to -> for loop, 1 points to -> 2nd statement inside for loop
     * similarly, statement_increment will be called for last statement and [0, 2]
     *
     * for loop ends, end_scope is called, pop the last element & increment the last element
     * we have [1], 1 points to -> var statement
     *
     * var statement ends, statement_increment is called, we have [2]
     * which points to while loop, begin_scope is called, push 0, we have [2,0]
     * pointing to fist statement inside while loop, while loop ends, pop last element & increment
     * we have [3] and that's the end
     */
    std::vector<unsigned int> deep_position = {0};

    /**
     * this must be called when a scope begins
     */
    inline void begin_scope() {
        deep_position.push_back(0);
    }

    /**
     * this must be called when a scope ends
     */
    inline void end_scope() {
        deep_position.pop_back();
        statement_increment();
    }

    /**
     * this will increment the deep_position for a single statement
     */
    inline void statement_increment() {
        deep_position[deep_position.size() - 1]++;
    }

};