// Copyright (c) Qinetik 2024.

#include "UnnamedUnion.h"
#include "UnionDef.h"
#include "FunctionDeclaration.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/LinkedType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void UnionDef::code_gen_function_declare(Codegen &gen, FunctionDeclaration* decl) {
    decl->code_gen_declare(gen, this);
}

void UnionDef::code_gen_function_body(Codegen &gen, FunctionDeclaration* decl) {
    decl->code_gen_body(gen, this);
}

void UnionDef::func_gen(Codegen &gen, bool declare) {
    if(declare) {
        for (auto &function: functions()) {
            function->code_gen_declare(gen, this);
        }
    } else {
        for (auto &function: functions()) {
            function->code_gen_body(gen, this);
        }
    }
}

void UnionDef::code_gen(Codegen &gen, bool declare) {
    if(is_comptime()) {
        return;
    }
    auto& itr_ptr = declare ? iterations_declared : iterations_body_done;
    if(generic_params.empty()) {
        if(itr_ptr == 0) {
            func_gen(gen, declare);
            itr_ptr++;
        }
    } else {
        const auto total = total_generic_iterations();
        if(total == 0) return; // generic type was never used
        auto prev_active_iteration = active_iteration;
        auto struct_itr = itr_ptr;
        while(struct_itr < total) {
            set_active_iteration(struct_itr);
            if(declare) {
                early_declare_structural_generic_args(gen);
            }
            func_gen(gen, declare);
            if(declare) {
                acquire_function_iterations(struct_itr);
            }
            struct_itr++;
        }
        itr_ptr = struct_itr;
        set_active_iteration(prev_active_iteration);
    }
}

void UnionDef::code_gen_external_declare(Codegen &gen) {
    llvm_struct_type = nullptr;
}

#endif

BaseType *UnionDef::copy(ASTAllocator& allocator) const {
    return new (allocator.allocate<LinkedType>()) LinkedType(name_view(), (ASTNode*) this, location);
}

VariablesContainer *UnionDef::copy_container(ASTAllocator& allocator) {
    auto container = new (allocator.allocate<UnionDef>()) UnionDef(identifier, parent_node, location);
    for(auto& variable : variables) {
        container->variables[variable.first] = variable.second->copy_member(allocator);
    }
    return container;
}

BaseType* UnnamedUnion::create_value_type(ASTAllocator &allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(name, (ASTNode*) this, location);
}

BaseType *UnnamedUnion::copy(ASTAllocator& allocator) const {
    return new (allocator.allocate<LinkedType>()) LinkedType(name, (ASTNode*) this, location);
}

BaseType* UnionDef::create_value_type(ASTAllocator& allocator) {
    return create_linked_type(name_view(), allocator);
}

BaseType* UnionDef::known_type() {
    return &linked_type;
}

void UnionDef::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare_node(name_view(), this, specifier(), true);
}

void UnionDef::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    MembersContainer::declare_and_link(linker, node_ptr);
}

void UnnamedUnion::redeclare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void UnnamedUnion::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    VariablesContainer::declare_and_link(linker, node_ptr);
    linker.scope_end();
    linker.declare(name, this);
}

BaseDefMember *UnnamedUnion::copy_member(ASTAllocator& allocator) {
    auto unnamed = new (allocator.allocate<UnnamedUnion>()) UnnamedUnion(name, parent_node, location);
    for(auto& variable : variables) {
        unnamed->variables[variable.first] = variable.second->copy_member(allocator);
    }
    return unnamed;
}

VariablesContainer *UnnamedUnion::copy_container(ASTAllocator& allocator) {
    return (VariablesContainer*) copy_member(allocator);
}