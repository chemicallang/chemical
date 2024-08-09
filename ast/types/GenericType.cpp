// Copyright (c) Qinetik 2024.


#include "GenericType.h"
#include "compiler/SymbolResolver.h"
#include "ReferencedType.h"
#include "ast/structures/StructDefinition.h"

GenericType::GenericType(std::unique_ptr<ReferencedType> referenced) : referenced(std::move(referenced)) {

}

GenericType::GenericType(std::unique_ptr<ReferencedType> referenced, std::vector<std::unique_ptr<BaseType>> types) : referenced(std::move(referenced)), types(std::move(types)) {

}

GenericType::GenericType(std::unique_ptr<ReferencedType> referenced, int16_t generic_itr) : referenced(std::move(referenced)), generic_iteration(generic_itr) {

}

GenericType::GenericType(std::string base) : referenced(new ReferencedType(std::move(base))) {

}

void GenericType::link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) {
    referenced->link(linker, (std::unique_ptr<BaseType>&) referenced);
    const auto generic_struct = referenced->get_generic_struct();
    if(generic_struct) {
        generic_iteration = generic_struct->register_generic_args(linker, types);
    }
    for(auto& type : types) {
        type->link(linker, type);
    }
}

void GenericType::report_generic_usage() {
    // we did this when linking
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

bool GenericType::satisfies(Value *value) {
    const auto value_pure_type = value->get_pure_type();
    if(value_pure_type->kind() == BaseTypeKind::Generic) {
        const auto gen_type = (GenericType*) value_pure_type.get();
        return referenced->is_same(gen_type->referenced.get()) && gen_type->generic_iteration == generic_iteration;
    } else {
        return false;
    }
}

ASTNode *GenericType::linked_node() {
    return referenced->linked_node();
}