// Copyright (c) Qinetik 2024.

#include "BaseType.h"
#include "preprocess/RepresentationVisitor.h"
#include <sstream>

std::string BaseType::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}

BaseType::~BaseType() = default;