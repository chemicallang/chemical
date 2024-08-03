// Copyright (c) Qinetik 2024.


#include "GenericType.h"
#include "compiler/SymbolResolver.h"
#include "ReferencedType.h"

GenericType::GenericType(std::unique_ptr<ReferencedType> referenced) : referenced(std::move(referenced)) {

}

GenericType::GenericType(std::string base) : referenced(new ReferencedType(std::move(base))) {

}

void GenericType::link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) {
    referenced->link(linker, (std::unique_ptr<BaseType>&) referenced);
}

BaseType* GenericType::copy() const {
    auto gen = new GenericType(std::unique_ptr<ReferencedType>((ReferencedType*) referenced->copy()));
    for(auto& type : types) {
        gen->types.emplace_back(type->copy());
    }
    return gen;
}

bool GenericType::satisfies(ValueType value_type) {
    return referenced->satisfies(value_type);
}

ASTNode *GenericType::linked_node() {
    return referenced->linked_node();
}