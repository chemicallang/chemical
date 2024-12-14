// Copyright (c) Qinetik 2024.

#include "chem_string_view.h"
#include "chem_string.h"
#include <iostream>

std::ostream& chem::operator<<(std::ostream& os, const chem::string& str) {
    return os.write(str.data(), (std::streamsize) str.size());
}

std::ostream& chem::operator<<(std::ostream& os, const chem::string_view& str) {
    return os.write(str.data(), (std::streamsize) str.size());
}