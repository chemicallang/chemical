// Copyright (c) Qinetik 2024.

#include "chem_string.h"
#include <iostream>

std::ostream& chem::operator<<(std::ostream& os, const chem::string& str) {
    os << str.data();
    return os;
}