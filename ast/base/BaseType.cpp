// Copyright (c) Qinetik 2024.

#include "BaseType.h"
#include "preprocess/RepresentationVisitor.h"
#include <sstream>
#include "ASTNode.h"

std::string BaseType::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}

StructDefinition* BaseType::linked_struct_def() {
    if(kind() == BaseTypeKind::Referenced) {
        return linked_node()->as_struct_def();
    }
    return nullptr;
}

BaseType::~BaseType() = default;