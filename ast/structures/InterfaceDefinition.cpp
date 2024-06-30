// Copyright (c) Qinetik 2024.

#include "InterfaceDefinition.h"
#include "StructMember.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ReferencedType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void InterfaceDefinition::code_gen(Codegen &gen) {
    std::unordered_map<std::string, llvm::Function*> unimplemented;
    for(auto& function : functions) {
        function.second->code_gen_interface(gen, this);
        if(!function.second->body.has_value()) {
            unimplemented[function.second->name] = (llvm::Function *) function.second->llvm_pointer(gen);
        }
    }
    if(!unimplemented.empty()) {
        gen.unimplemented_interfaces[name] = unimplemented;
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

void InterfaceDefinition::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}