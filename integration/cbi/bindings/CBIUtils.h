// Copyright (c) Chemical Language Foundation 2025.

#pragma once

namespace chem {
    class string;
}

class dispose_string {
public:
    chem::string* ptr;
    ~dispose_string();
};

chem::string* init_chem_string(chem::string* str);