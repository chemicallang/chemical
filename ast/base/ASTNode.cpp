// Copyright (c) Qinetik 2024.

#include "ASTNode.h"
#include "preprocess/RepresentationVisitor.h"
#include <sstream>

std::string ASTNode::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}