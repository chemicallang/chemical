// Copyright (c) Qinetik 2024.

#include "InterfaceDefinition.h"
#include "StructDefinition.h"
#include "StructMember.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ReferencedType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void InterfaceDefinition::code_gen_for_users(Codegen& gen, FunctionDeclaration* func) {
    for(auto& use : users) {
        auto& user = users[use.first];
        active_user = use.first;
        auto found = user.find(func);
        if((found == user.end() || found->second == nullptr) && func->has_self_param()) {
            func->code_gen_declare(gen, this);
            func->code_gen_body(gen, this);
        }
        user[func] = (llvm::Function*) func->llvm_pointer(gen);
    }
    active_user = nullptr;
}

void InterfaceDefinition::code_gen_function_declare(Codegen& gen, FunctionDeclaration* decl) {
    if(!decl->has_self_param() && (has_implementation || !users.empty())) {
        decl->code_gen_declare(gen, this);
        decl->code_gen_body(gen, this);
        return;
    }
    code_gen_for_users(gen, decl);
}

void InterfaceDefinition::code_gen_function_body(Codegen& gen, FunctionDeclaration* decl) {

}

void InterfaceDefinition::code_gen(Codegen &gen) {
    for (auto& func: functions()) {
        if(!func->has_self_param() && (has_implementation || !users.empty())) {
            func->code_gen_declare(gen, this);
            func->code_gen_body(gen, this);
        }
    }
    for (const auto& function: functions()) {
        code_gen_for_users(gen, function.get());
    }
}

llvm::Type* InterfaceDefinition::llvm_type(Codegen &gen) {
    // the reason it returns void is that
    // when interface functions require the type in parameter, a pointer to interface would mean
    // a pointer to void, because we haven't setup that 'struct member variables in interfaces can be represented'
    // which also requires that struct type matches completely with interface type because get element pointer won't work otherwise

    // not knowing which struct would implement the function, a void pointer would allow all members to pass through
    // since pointers are opaque, void pointer means just a pointer, which could be to a struct
    // since function will be implemented only when struct type is known, get element pointer instructions would
    // use that struct type
    return gen.builder->getVoidTy();
}

void InterfaceDefinition::llvm_vtable_type(Codegen& gen, std::vector<llvm::Type*>& struct_types) {
    struct_types.reserve(struct_types.size() + functions().size());
    for(auto& func : functions()) {
        struct_types.emplace_back(gen.builder->getPtrTy());
    }
}

llvm::Type* InterfaceDefinition::llvm_vtable_type(Codegen& gen) {
    std::vector<llvm::Type*> types;
    llvm_vtable_type(gen, types);
    return llvm::StructType::get(*gen.ctx, types);
}

void InterfaceDefinition::llvm_build_vtable(Codegen& gen, StructDefinition* for_struct, std::vector<llvm::Constant*>& llvm_pointers) {
    auto found = users.find(for_struct);
    if(found != users.end()) {
        for(auto& func : functions()) {
            auto func_res = found->second.find(func.get());
            if(func_res != found->second.end()) {
                llvm_pointers.emplace_back(func_res->second);
            } else {
                gen.error("couldn't find function impl pointer, name '" + func->name + "' for struct '" + ((ASTNode*) for_struct)->ns_node_identifier() + "' for interface '" + name + "'", (AnnotableNode*) func.get());
            }
        }
    } else {
        gen.error("couldn't find struct '" + ((ASTNode*) for_struct)->ns_node_identifier() + "' implementation pointers for interface '" + name + "'", (AnnotableNode*) for_struct);
    }
}

llvm::Constant* InterfaceDefinition::llvm_build_vtable(Codegen& gen, StructDefinition* for_struct) {
    std::vector<llvm::Constant*> llvm_pointers;
    llvm_build_inherited_vtable(gen, for_struct, llvm_pointers);
    llvm_build_vtable(gen, for_struct, llvm_pointers);
    std::vector<llvm::Type*> struct_types;
    llvm_build_inherited_vtable_type(gen, struct_types);
    llvm_vtable_type(gen, struct_types);
    return llvm::ConstantStruct::get(llvm::StructType::get(*gen.ctx, struct_types), llvm_pointers);
}

llvm::Value* InterfaceDefinition::llvm_global_vtable(Codegen& gen, StructDefinition* for_struct) {
    auto found = vtable_pointers.find(for_struct);
    if(found != vtable_pointers.end()) {
        return found->second;
    }
    // building vtable
    auto constant = llvm_build_vtable(gen, for_struct);
    auto table = new llvm::GlobalVariable(
            *gen.module,
            constant->getType(),
            true,
            llvm::GlobalValue::InternalLinkage,
            constant
    );
    // an alias to the first pointer in the llvm_vtable
    // since we are using structs, we don't need to create an alias to the first pointer
//    std::vector<llvm::Constant*> idx { gen.builder->getInt32(0), gen.builder->getInt32(0) };
//    const auto get_ele_ptr = llvm::ConstantExpr::getGetElementPtr(constant->getType(), table, idx, gen.inbounds);
//    const auto alias = llvm::GlobalAlias::create(gen.builder->getPtrTy(), 0, llvm::GlobalValue::LinkageTypes::InternalLinkage, "", get_ele_ptr, gen.module.get());
    vtable_pointers[for_struct] = table;
    return table;
}

#endif

InterfaceDefinition::InterfaceDefinition(
        std::string name,
        ASTNode* parent_node,
        CSTToken* token,
        AccessSpecifier specifier
) : ExtendableMembersContainerNode(std::move(name)), parent_node(parent_node), token(token), specifier(specifier) {

}

void InterfaceDefinition::runtime_name_no_parent(std::ostream &stream) {
    if(active_user) {
        active_user->runtime_name_no_parent(stream);
    } else {
        ExtendableMembersContainerNode::runtime_name_no_parent(stream);
    }
}

void InterfaceDefinition::runtime_name(std::ostream &stream) {
    if(active_user) {
        active_user->runtime_name(stream);
    } else {
        if(parent_node) {
            parent_node->runtime_name(stream);
        }
        ExtendableMembersContainerNode::runtime_name_no_parent(stream);
    }
}

std::unique_ptr<BaseType> InterfaceDefinition::create_value_type() {
    return std::make_unique<ReferencedType>(name, this, nullptr);
}

hybrid_ptr<BaseType> InterfaceDefinition::get_value_type() {
    return hybrid_ptr<BaseType> { new ReferencedType(name, this, nullptr) };
}

int InterfaceDefinition::vtable_function_index(FunctionDeclaration* decl) {
    int i = 0;
    for(auto& func : functions()) {
        if(func.get() == decl) {
            return i;
        }
        i++;
    }
    return -1;
}

void InterfaceDefinition::declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    linker.declare_node(name, this, specifier, false);
}