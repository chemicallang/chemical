// Copyright (c) Qinetik 2024.

#include "ASTNode.h"
#include "BaseType.h"
#include "Value.h"
#include "preprocess/RepresentationVisitor.h"
#include <sstream>

std::string ASTNode::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}

uint64_t ASTNode::byte_size(bool is64Bit) {
    auto holdingType = holding_value_type();
    if(holdingType) return holdingType->byte_size(is64Bit);
    auto holdingValue = holding_value();
    if(holdingValue) return holdingValue->byte_size(is64Bit);
    throw std::runtime_error("unknown byte size for linked node");
}

std::unique_ptr<BaseType> ASTNode::create_value_type() {
    return nullptr;
}

hybrid_ptr<BaseType> ASTNode::get_value_type() {
    return hybrid_ptr<BaseType> { nullptr, false };
}

ASTNode::~ASTNode() = default;