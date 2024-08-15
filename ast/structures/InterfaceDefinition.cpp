// Copyright (c) Qinetik 2024.

#include "InterfaceDefinition.h"
#include "StructMember.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ReferencedType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void InterfaceDefinition::code_gen(Codegen &gen) {
    for (auto& func: functions()) {
        if(!func->has_self_param() && (has_implementation || !users.empty())) {
            func->code_gen_interface(gen, this);
        }
    }
    for(auto& use : users) {
        for (const auto& function: functions()) {
            auto& user = users[use.first];
            auto found = user.find(function.get());
            if((found == user.end() || found->second == nullptr) && function->has_self_param()) {
                function->code_gen_interface(gen, this);
            }
            user[function.get()] = (llvm::Function*) function->llvm_pointer(gen);
        }
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

llvm::Type* InterfaceDefinition::llvm_vtable_type(Codegen& gen) {
    auto total = functions().size();
    std::vector<llvm::Type*> types(total);
    int i = 0;
    for(auto& func : functions()) {
        types[i] = gen.builder->getPtrTy();
        i++;
    }
    return llvm::StructType::get(*gen.ctx, types);
}

#endif

InterfaceDefinition::InterfaceDefinition(
        std::string name,
        ASTNode* parent_node
) : ExtendableMembersContainerNode(std::move(name)), parent_node(parent_node) {

}

std::unique_ptr<BaseType> InterfaceDefinition::create_value_type() {
    return std::make_unique<ReferencedType>(name, this);
}

hybrid_ptr<BaseType> InterfaceDefinition::get_value_type() {
    return hybrid_ptr<BaseType> { new ReferencedType(name, this) };
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

void InterfaceDefinition::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}