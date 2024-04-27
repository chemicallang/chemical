// Copyright (c) Qinetik 2024.

#include "InterfaceDefinition.h"
#include "StructMember.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void InterfaceDefinition::code_gen(Codegen &gen) {
    for(auto& function : functions) {
        function.second->code_gen_interface(gen);
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
        std::string name
) : name(std::move(name)) {

}

std::string InterfaceDefinition::representation() const {
    std::string ret("interface " + name + " {\n");
    ret.append(MembersContainer::representation());
    ret.append("\n}");
    return ret;
}