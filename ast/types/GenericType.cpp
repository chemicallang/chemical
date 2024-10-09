// Copyright (c) Qinetik 2024.


#include "GenericType.h"
#include "compiler/SymbolResolver.h"
#include "LinkedType.h"
#include "ast/structures/StructDefinition.h"

GenericType::GenericType(LinkedType* referenced) : referenced(referenced) {

}

GenericType::GenericType(LinkedType* referenced, std::vector<BaseType*> types) : referenced(referenced), types(std::move(types)) {

}

GenericType::GenericType(LinkedType* referenced, int16_t generic_itr) : referenced(referenced), generic_iteration(generic_itr) {

}

GenericType::GenericType(std::string base, CSTToken* token) : referenced(new LinkedType(std::move(base), token)) {

}

CSTToken* GenericType::cst_token() {
    return referenced->cst_token();
}

bool GenericType::link(SymbolResolver &linker) {
    referenced->link(linker);
    if(!referenced->linked) {
        return false;
    }
    for(auto& type : types) {
        if(!type->link(linker)) {
            return false;
        }
    }
    report_generic_usage(linker);
    return true;
}

bool GenericType::subscribe_to_parent_generic() {
    for(auto& type : types) {
        if(type->kind() == BaseTypeKind::Linked) {
            const auto gen_param = type->linked_node()->as_generic_type_param();
            if(gen_param) {
                gen_param->parent_node->subscribe(this);
                return true;
            }
        }
    }
    return false;
}

int16_t GenericType::report_generic_args(SymbolResolver &linker, std::vector<BaseType*>& gen_args) {
    const auto members_container = referenced->linked_node()->as_members_container();
    if(members_container) {
        return members_container->register_generic_args(linker, gen_args);
    }
    return -1;
}

void GenericType::report_generic_usage(SymbolResolver& linker) {
    if(!subscribe_to_parent_generic()) {
        generic_iteration = report_generic_args(linker, types);
    }
}

void GenericType::report_parent_usage(SymbolResolver &linker, int16_t parent_itr) {
    std::vector<BaseType*> generic_args;
    for(auto& type : types) {
        if(type->kind() == BaseTypeKind::Linked) {
            const auto gen_param = type->linked_node()->as_generic_type_param();
            if(gen_param) {
                generic_args.emplace_back(gen_param->usage.back());
                continue;
            }
        }
        // completely specialized type
        generic_args.emplace_back(type);
    }
    subscribed_map[parent_itr] = report_generic_args(linker, generic_args);
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

GenericType* GenericType::copy(ASTAllocator& allocator) const {
    auto gen = new (allocator.allocate<GenericType>()) GenericType((LinkedType*) referenced->copy(allocator));
    gen->generic_iteration = generic_iteration;
    for(auto& type : types) {
        gen->types.emplace_back(type->copy(allocator));
    }
    return gen;
}

ValueType GenericType::value_type() const {
    return referenced->value_type();
}

bool GenericType::satisfies(ValueType value_type) {
    return referenced->satisfies(value_type);
}

bool GenericType::satisfies(BaseType *pure_type) {
    if(pure_type->kind() == BaseTypeKind::Generic) {
        const auto gen_type = (GenericType*) pure_type;
        return referenced->is_same(gen_type->referenced) && gen_type->generic_iteration == generic_iteration;
    } else {
        return false;
    }
}

ASTNode *GenericType::linked_node() {
    return referenced->linked_node();
}