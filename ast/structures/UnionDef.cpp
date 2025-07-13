// Copyright (c) Chemical Language Foundation 2025.

#include "UnnamedUnion.h"
#include "UnionDef.h"
#include "FunctionDeclaration.h"
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
        for (auto &function: instantiated_functions()) {
            function->code_gen_declare(gen, this);
        }
    } else {
        for (auto &function: instantiated_functions()) {
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

llvm::Type *UnionDef::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type* llvm_union_chain_type(VariablesContainer* container, Codegen& gen, std::vector<ChainValue*> &values, unsigned int index) {
    if(index + 1 < values.size()) {
        auto linked = values[index + 1]->linked_node();
        if(linked) {
            for (const auto member : container->variables()) {
                if (member == linked) {
                    std::vector<llvm::Type *> struct_type{member->llvm_chain_type(gen, values, index + 1)};
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

BaseType* UnionDef::known_type() {
    return &linked_type;
}