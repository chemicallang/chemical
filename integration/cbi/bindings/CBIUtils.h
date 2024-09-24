// Copyright (c) Qinetik 2024.

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