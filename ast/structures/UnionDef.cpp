// Copyright (c) Chemical Language Foundation 2025.

#include "UnnamedUnion.h"
#include "UnionDef.h"
#include "FunctionDeclaration.h"
#include "ast/types/LinkedType.h"
#include "ast/base/Value.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void UnionDef::code_gen_function_declare(Codegen &gen, FunctionDeclaration* decl) {
    decl->code_gen_declare(gen, this);
}

void UnionDef::code_gen_function_body(Codegen &gen, FunctionDeclaration* decl) {
    decl->code_gen_body(gen, this);
}

void UnionDef::code_gen(Codegen &gen, bool declare) {
    if(is_comptime()) {
        return;
    }
    auto& has_done = declare ? has_declared : has_implemented;
    if(!has_done) {
        if(declare) {
            for (auto& node: evaluated_nodes()) {
                switch (node->kind()) {
                    case ASTNodeKind::FunctionDecl:
                        node->as_function_unsafe()->code_gen_declare(gen, this);
                        break;
                    case ASTNodeKind::GenericFuncDecl: {
                        for (const auto func : node->as_gen_func_decl_unsafe()->instantiations) {
                            func->code_gen_declare(gen, this);
                        }
                        break;
                    }
                    default:
                        node->code_gen_declare(gen);
                        break;
                }
            }
        } else {
            for (auto& node: evaluated_nodes()) {
                switch (node->kind()) {
                    case ASTNodeKind::FunctionDecl:
                        node->as_function_unsafe()->code_gen_body(gen, this);
                        break;
                    case ASTNodeKind::GenericFuncDecl: {
                        for (const auto func : node->as_gen_func_decl_unsafe()->instantiations) {
                            func->code_gen_body(gen, this);
                        }
                        break;
                    }
                    default:
                        node->code_gen(gen);
                        break;
                }
            }
        }
        has_done = true;
    }
}

void UnionDef::code_gen_external_declare(Codegen &gen) {
    extendable_external_declare(gen);
}

llvm::StructType* UnionDef::llvm_get_stored_type(Codegen& gen) {
    auto found = gen.ctx_ptr_cache.find(this);
    return found != gen.ctx_ptr_cache.end() ? ((llvm::StructType*) found->second) : nullptr;
}

void UnionDef::llvm_store_type(Codegen& gen, llvm::StructType* type) {
    gen.ctx_ptr_cache[this] = type;
}

llvm::Type *UnionDef::llvm_type(Codegen &gen) {
    const auto stored = llvm_get_stored_type(gen);
    if (stored) return stored;
    auto largest = largest_member();
    if(!largest) {
        gen.error(this) << "Couldn't determine the largest member of the union with name " << name_view();
        return nullptr;
    }
    std::vector<llvm::Type*> members {largest->llvm_type(gen)};
    if(is_anonymous()) {
        return llvm::StructType::get(*gen.ctx, members);
    }
    const auto new_stored = llvm::StructType::create(*gen.ctx, members, "union." + name_view().str());
    llvm_store_type(gen, new_stored);
    return new_stored;
}

llvm::Type *UnionDef::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type* llvm_union_chain_type(VariablesContainerBase* container, Codegen& gen, std::vector<Value*> &values, unsigned int index) {
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

llvm::Type *UnionDef::llvm_chain_type(Codegen &gen, std::vector<Value*> &values, unsigned int index) {
    const auto type = llvm_union_chain_type(this, gen, values, index);
    if(type) return type;
    return llvm_type(gen);
}

llvm::StructType* UnnamedUnion::llvm_get_stored_type(Codegen& gen) {
    auto found = gen.ctx_ptr_cache.find(this);
    return found != gen.ctx_ptr_cache.end() ? ((llvm::StructType*) found->second) : nullptr;
}

void UnnamedUnion::llvm_store_type(Codegen& gen, llvm::StructType* type) {
    gen.ctx_ptr_cache[this] = type;
}

llvm::Type *UnnamedUnion::llvm_type(Codegen &gen) {
    if (const auto found = llvm_get_stored_type(gen)) return found;
    auto largest = largest_member();
    if(!largest) {
        gen.error("Couldn't determine the largest member of the unnamed union", this);
        return nullptr;
    }
    std::vector<llvm::Type*> members {largest->llvm_type(gen)};
    const auto created = llvm::StructType::get(*gen.ctx, members);
    llvm_store_type(gen, created);
    return created;
}

llvm::Type *UnnamedUnion::llvm_chain_type(Codegen &gen, std::vector<Value*> &values, unsigned int index) {
    const auto type = llvm_union_chain_type(this, gen, values, index);
    if(type) return type;
    return llvm_type(gen);
}

#endif