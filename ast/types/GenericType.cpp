// Copyright (c) Qinetik 2024.


#include "GenericType.h"
#include "compiler/SymbolResolver.h"
#include "LinkedType.h"
#include "ast/structures/StructDefinition.h"

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
    report_generic_usage(*linker.ast_allocator, linker);
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

int16_t GenericType::report_generic_args(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, std::vector<BaseType*>& gen_args) {
    const auto members_container = referenced->linked_node()->as_members_container();
    if(members_container) {
        return members_container->register_generic_args(astAllocator, diagnoser, gen_args);
    }
    return -1;
}

void GenericType::report_generic_usage(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser) {
    if(!subscribe_to_parent_generic()) {
        generic_iteration = report_generic_args(astAllocator, diagnoser, types);
    }
}

void GenericType::report_parent_usage(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, int16_t parent_itr) {
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
    subscribed_map[parent_itr] = report_generic_args(astAllocator, diagnoser, generic_args);
}

void GenericType::set_parent_iteration(int16_t parent_itr) {
    if(parent_itr == -1) {
        generic_iteration = -1;
        return;
    }
    auto found = subscribed_map.find(parent_itr);
    if(found != subscribed_map.end()) {
        generic_iteration = found->second;
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