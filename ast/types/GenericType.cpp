// Copyright (c) Qinetik 2024.


#include "GenericType.h"
#include "compiler/SymbolResolver.h"
#include "ReferencedType.h"
#include "ast/structures/StructDefinition.h"

GenericType::GenericType(std::unique_ptr<ReferencedType> referenced) : referenced(std::move(referenced)) {

}

GenericType::GenericType(std::unique_ptr<ReferencedType> referenced, int16_t generic_itr) : referenced(std::move(referenced)), generic_iteration(generic_itr) {

}

GenericType::GenericType(std::string base) : referenced(new ReferencedType(std::move(base))) {

}

void GenericType::link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) {
    referenced->link(linker, (std::unique_ptr<BaseType>&) referenced);
    const auto generic_struct = referenced->get_generic_struct();
    if(generic_struct) {
        generic_iteration = generic_struct->register_generic_args(types);
    }
}

void GenericType::report_generic_usage() {

}

BaseType* GenericType::copy() const {
    auto gen = new GenericType(std::unique_ptr<ReferencedType>((ReferencedType*) referenced->copy()));
    gen->generic_iteration = generic_iteration;
    for(auto& type : types) {
        gen->types.emplace_back(type->copy());
    }
    return gen;
}

ValueType GenericType::value_type() const {
    return referenced->value_type();
}

bool GenericType::satisfies(ValueType value_type) {
    return referenced->satisfies(value_type);
}

ASTNode *GenericType::linked_node() {
    return referenced->linked_node();
}