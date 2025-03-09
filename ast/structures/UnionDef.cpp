// Copyright (c) Chemical Language Foundation 2025.

#include "UnnamedUnion.h"
#include "UnionDef.h"
#include "FunctionDeclaration.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/LinkedType.h"
#include "ast/base/ChainValue.h"

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
    if(itr_ptr == 0) {
        func_gen(gen, declare);
        itr_ptr++;
    }
}

void UnionDef::code_gen_external_declare(Codegen &gen) {
    llvm_struct_type = nullptr;
}

llvm::Type *UnionDef::llvm_type(Codegen &gen) {
    auto largest = largest_member();
    if(!largest) {
        gen.error(this) << "Couldn't determine the largest member of the union with name " << name_view();
        return nullptr;
    }
    auto stored = llvm_union_get_stored_type();
    if(!stored) {
        std::vector<llvm::Type*> members {largest->llvm_type(gen)};
        if(is_anonymous()) {
            return llvm::StructType::get(*gen.ctx, members);
        }
        stored = llvm::StructType::create(*gen.ctx, members, "union." + name_view().str());
        llvm_union_type_store(stored);
        return stored;
    }
    return stored;
}

llvm::Type* llvm_union_chain_type(VariablesContainer* container, Codegen& gen, std::vector<ChainValue*> &values, unsigned int index) {
    if(index + 1 < values.size()) {
        auto linked = values[index + 1]->linked_node();
        if(linked) {
            for (auto &member : container->variables) {
                if (member.second == linked) {
                    std::vector<llvm::Type *> struct_type{member.second->llvm_chain_type(gen, values, index + 1)};
                    return llvm::StructType::get(*gen.ctx, struct_type);
                }
            }
        }
    }
    return nullptr;
}

llvm::Type *UnionDef::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    const auto type = llvm_union_chain_type(this, gen, values, index);
    if(type) return type;
    return llvm_type(gen);
}

llvm::Type *UnnamedUnion::llvm_type(Codegen &gen) {
    auto largest = largest_member();
    if(!largest) {
        gen.error("Couldn't determine the largest member of the unnamed union", this);
        return nullptr;
    }
    std::vector<llvm::Type*> members {largest->llvm_type(gen)};
    return llvm::StructType::get(*gen.ctx, members);
}

llvm::Type *UnnamedUnion::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    const auto type = llvm_union_chain_type(this, gen, values, index);
    if(type) return type;
    return llvm_type(gen);
}

#endif

//BaseType *UnionDef::copy(ASTAllocator& allocator) const {
//    return new (allocator.allocate<LinkedType>()) LinkedType(name_view(), (ASTNode*) this, location);
//}

VariablesContainer *UnionDef::copy_container(ASTAllocator& allocator) {
    auto container = new (allocator.allocate<UnionDef>()) UnionDef(identifier, parent(), encoded_location());
    for(auto& variable : variables) {
        container->variables[variable.first] = variable.second->copy_member(allocator);
    }
    return container;
}

BaseType* UnnamedUnion::create_value_type(ASTAllocator &allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(name, (ASTNode*) this, encoded_location());
}

//BaseType *UnnamedUnion::copy(ASTAllocator& allocator) const {
//    return new (allocator.allocate<LinkedType>()) LinkedType(name, (ASTNode*) this, location);
//}

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

VariablesContainer *UnnamedUnion::copy_container(ASTAllocator& allocator) {
    return (VariablesContainer*) copy_member(allocator);
}