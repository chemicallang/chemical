// Copyright (c) Chemical Language Foundation 2025.

#include "GenericStructDecl.h"
#include "compiler/SymbolResolver.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/generics/GenInstantiatorAPI.h"

void GenericStructDecl::declare_top_level(SymbolResolver &linker, ASTNode *&node_ptr) {
    master_impl->generic_parent = this;
    linker.declare(master_impl->name_view(), this);
}

void GenericStructDecl::link_signature(SymbolResolver &linker) {
//    // symbol resolve the master declaration
//    // TODO this creates an extra scope
    linker.scope_start();
    for(auto& param : generic_params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    master_impl->link_signature(linker);
    master_impl->declare_and_link(linker, (ASTNode*&) master_impl);
    linker.scope_end();
}

void GenericStructDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
////    // symbol resolve the master declaration
////    // TODO this creates an extra scope
//    linker.scope_start();
//    for(auto& param : generic_params) {
//        param->declare_and_link(linker, (ASTNode*&) param);
//    }
//    master_impl->declare_and_link(linker, (ASTNode*&) master_impl);
//    linker.scope_end();
}

BaseType* GenericStructDecl::create_value_type(ASTAllocator &allocator) {
    return master_impl->create_value_type(allocator);
}

StructDefinition* GenericStructDecl::register_generic_args(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, std::vector<BaseType*>& types) {

    const auto types_size = types.size();
    std::vector<BaseType*> generic_args(types_size);
    unsigned i = 0;
    for(auto& type : types) {
        generic_args[i] = type;
        i++;
    }
    const auto itr = register_generic_usage(astAllocator, generic_params, generic_args);
    if(itr.second) {
        // register iteration to subscribers
    } else {
        return instantiations[itr.first];
    }

    const auto created = Instantiate(astAllocator, diagnoser, this, itr.first);

    if(itr.first != instantiations.size()) {
#ifdef DEBUG
        throw std::runtime_error("not the index we expected");
#endif
    }

    instantiations.emplace_back(created);

    created->generic_parent = this;
    created->generic_instantiation = itr.first;

    return created;

}

#ifdef COMPILER_BUILD

void GenericStructDecl::code_gen_declare(Codegen &gen) {
    for(const auto inst : instantiations) {
        inst->code_gen_declare(gen);
    }
}

void GenericStructDecl::code_gen(Codegen &gen) {
    for(const auto inst : instantiations) {
        inst->code_gen(gen);
    }
}

void GenericStructDecl::code_gen_external_declare(Codegen &gen) {
    for(const auto inst : instantiations) {
        inst->code_gen_external_declare(gen);
    }
}

#endif