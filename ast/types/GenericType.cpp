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
    for(auto& type : types) {
        type->link(linker, type);
    }
    report_generic_usage(linker);
}

bool GenericType::subscribe_to_parent_generic() {
    for(auto& type : types) {
        if(type->kind() == BaseTypeKind::Referenced) {
            const auto gen_param = type->linked_node()->as_generic_type_param();
            if(gen_param) {
                gen_param->parent_node->subscribe(this);
                return true;
            }
        }
    }
    return false;
}

int16_t GenericType::report_generic_args(SymbolResolver &linker, std::vector<std::unique_ptr<BaseType>>& gen_args) {
    const auto generic_struct = referenced->get_generic_struct();
    if(generic_struct) {
        return generic_struct->register_generic_args(linker, gen_args);
    }
    return -1;
}

void GenericType::report_generic_usage(SymbolResolver& linker) {
    if(!subscribe_to_parent_generic()) {
        generic_iteration = report_generic_args(linker, types);
    }
}

void GenericType::report_parent_usage(SymbolResolver &linker, int16_t parent_itr) {
    std::vector<std::unique_ptr<BaseType>> generic_args;
    for(auto& type : types) {
        if(type->kind() == BaseTypeKind::Referenced) {
            const auto gen_param = type->linked_node()->as_generic_type_param();
            if(gen_param) {
                generic_args.emplace_back(gen_param->usage.back());
                continue;
            }
        }
        // completely specialized type
        generic_args.emplace_back(type.get());
    }
    subscribed_map[parent_itr] = report_generic_args(linker, generic_args);
    // release all generic args as they are all references
    for(auto& arg : generic_args) {
        arg.release();
    }
}

void GenericType::set_parent_iteration(int16_t parent_itr) {
    if(parent_itr == -1) {
        generic_iteration = -1;
        return;
    }
    auto found = subscribed_map.find(parent_itr);
    if(found != subscribed_map.end()) {
        generic_iteration = found->second;
    } else {
#ifdef DEBUG
        throw std::runtime_error("couldn't find a registered generic iteration for parent generic iteration");
#else
        std::cerr << "Generic type with " + representation() + " cannot set iteration using parent generic iteration " + std::to_string(parent_itr) << std::endl;
#endif
    }
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