// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>

class Value;

/**
 * we may or may not use this in the future
 * chain part represents a part of access chain, the reason it was created
 * was because of a bug that was really hard to solve, which revolved around
 * access chain and unions access, unions required to make different llvm type
 * in llvm_type function depending on which values were accessed
 * this meant looking at values present in the access chain
 */
struct ChainPart {

    // the values are the values of a access chain
    std::vector<Value*>& values;

    // start is inclusive index into values vector
    unsigned start;

    // end is exclusive index into values vector
    unsigned end;

};