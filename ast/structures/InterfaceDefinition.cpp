// Copyright (c) Chemical Language Foundation 2025.

#include "InterfaceDefinition.h"
#include "StructDefinition.h"
#include "ImplDefinition.h"
#include "StructMember.h"
#include "compiler/mangler/NameMangler.h"
#include "ast/types/LinkedType.h"
#include <sstream>
#include "std/except.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void InterfaceDefinition::code_gen_for_users(Codegen& gen, FunctionDeclaration* func) {
    const auto prev_user = active_user;
    for(const auto user : users) {
        const auto key = TraitImplFuncMapKey { .interface = this,.for_ = user, .func = func };
        auto found = gen.trait_impl_func_map.find(key);
        if (found == gen.trait_impl_func_map.end()) {
            // this should not happen
            // we try to declare and override the function
            // function should have been declared
            active_user = user;
            func->code_gen_declare(gen, this);
            const auto func_ptr = func->get_llvm_data(gen);
            func->code_gen_override(gen, func_ptr);
            gen.trait_impl_func_map.emplace(key, func_ptr);
        } else {
            active_user = user;
            func->code_gen_override(gen, found->second);
        }
    }
    active_user = prev_user;
}

void InterfaceDefinition::code_gen_function_declare(Codegen& gen, FunctionDeclaration* decl) {
    code_gen_for_users(gen, decl);
}

void InterfaceDefinition::code_gen_function_body(Codegen& gen, FunctionDeclaration* decl) {
    // this function is called by function declaration, when code_gen is called on a function
    // however since interface doesn't generate any body so we do nothing
}

void InterfaceDefinition::code_gen_declare_for_user(Codegen& gen, ExtendableMembersContainerNode* node) {
    const auto prev_user = active_user;
    active_user = node;
    for (const auto func: instantiated_functions()) {
        const auto key = TraitImplFuncMapKey{ .interface = this, .for_ = node, .func = func };
        auto found = gen.trait_impl_func_map.find(key);
        if(found == gen.trait_impl_func_map.end()) {
            func->code_gen_declare(gen, this);
            gen.trait_impl_func_map.emplace(key, func->get_llvm_data(gen));
        } else {
            // impl probably came first and basically set the function pointer
        }
    }
    // going over inherited interfaces and calling the same function
    for (auto& inh : inherited) {
        const auto can = inh.type->get_direct_linked_interface();
        if (can) {
            can->code_gen_declare_for_user(gen, node);
        }
    }
    active_user = prev_user;
}

void InterfaceDefinition::code_gen_declare(Codegen &gen) {
    if (users.empty()) {
        if (is_static()) {
            code_gen_declare_for_user(gen, nullptr);
        }
    } else {
        for(const auto use : users) {
            code_gen_declare_for_user(gen, use);
        }
    }
}

void InterfaceDefinition::code_gen(Codegen &gen) {
    if(is_static()) {
        // nothing to be done at the moment (unsure)
    } else {
        if(generates_vtable()) {
            // generating vtables for each user struct
            for (const auto user: users) {
                llvm_build_vtable(gen, user);
            }
        }
    }
}

void InterfaceDefinition::code_gen_external_declare_for_user(Codegen& gen, ExtendableMembersContainerNode* user) {
    const auto prev_active_user = user;
    active_user = user;
    const auto mod_scope = user->get_mod_scope();
    if (mod_scope == nullptr) {
        gen.error("couldn't find module scope", this);
        gen.warn("couldn't find module scope of user struct when declaring interface (of external module) implementations", user);
    }
    for(const auto func : instantiated_functions()) {
        const auto key = TraitImplFuncMapKey{ this, user, func };
        auto found = gen.trait_impl_func_map.find(key);
        if (found == gen.trait_impl_func_map.end()) {
            // if user is of current module
            // it means it hasn't ever been implemented (because this interface is external & its a new user)
            // we must declare the function (create the stub), so impl can generate body later on
            if (gen.current_module == mod_scope) {
                // we declare and generate the (stub) body so users (structs) can override it)
                func->code_gen_declare(gen, this);
            } else {
                // external function (needs redeclaration, llvm pointer from another module becomes invalid)
                func->code_gen_external_declare(gen);
            }
            gen.trait_impl_func_map.emplace(key, func->get_llvm_data(gen));
        }
    }
    // going over inherited interfaces and calling the same function
    for (auto& inh : inherited) {
        const auto can = inh.type->get_direct_linked_interface();
        if (can) {
            can->code_gen_external_declare_for_user(gen, user);
        }
    }
    active_user = prev_active_user;
}

void declare_static_interface(Codegen& gen, InterfaceDefinition* interface) {
    const auto prev_user = interface->active_user;
    interface->active_user = nullptr;
    for (const auto func: interface->instantiated_functions()) {
        const auto key = TraitImplFuncMapKey{ .interface = interface, .for_ = nullptr, .func = func };
        auto found = gen.trait_impl_func_map.find(key);
        if(found == gen.trait_impl_func_map.end()) {
            gen.trait_impl_func_map.emplace(key, func->get_llvm_data(gen));
        }
    }
    // going over inherited interfaces and calling the same function
    for (auto& inh : interface->inherited) {
        const auto can = inh.type->get_direct_linked_interface();
        if (can) {
            declare_static_interface(gen, can);
        }
    }
    interface->active_user = prev_user;
}

void InterfaceDefinition::code_gen_external_declare(Codegen &gen) {
    if (is_static()) {
        if (users.empty()) {
            extendable_external_declare(gen);
            declare_static_interface(gen, this);
            return;
        }
    }
    for (const auto user: users) {
        code_gen_external_declare_for_user(gen, user);
    }
    if(generates_vtable()) {
        // we generate the vtables, for which vtables exist we declare them, otherwise we create (implement) vtables
        // it can be a new user (present in current module), if that's the case, vtable is created for it
        for (const auto user: users) {
            create_global_vtable(gen, user, vtable_pointers.contains(user));
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

void InterfaceDefinition::llvm_vtable_type(Codegen& gen, std::vector<llvm::Type*>& struct_types) {
    struct_types.reserve(struct_types.size() + evaluated_nodes().size());
    for (const auto node : evaluated_nodes()) {
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
            case ASTNodeKind::GenericFuncDecl:
                struct_types.emplace_back(gen.builder->getPtrTy());
            default:
                continue;
        }
    }
}

llvm::StructType* InterfaceDefinition::llvm_vtable_type(Codegen& gen) {
    std::vector<llvm::Type*> struct_types;
    llvm_build_inherited_vtable_type(gen, struct_types);
    llvm_vtable_type(gen, struct_types);
    return llvm::StructType::get(*gen.ctx, struct_types);
}

void InterfaceDefinition::llvm_build_vtable(Codegen& gen, ExtendableMembersContainerNode* for_struct, std::vector<llvm::Constant*>& llvm_pointers) {
    for(const auto func : instantiated_functions()) {
        const auto key = TraitImplFuncMapKey { .interface = this, .for_ = for_struct, .func = func };
        auto found = gen.trait_impl_func_map.find(key);
        if (found != gen.trait_impl_func_map.end()) {
            llvm_pointers.emplace_back(found->second);
        } else {
            gen.error((AnnotableNode*) func) << "couldn't find function impl pointer, name '" << func->name_view() << "' for struct '" << for_struct->name_view() << "' for interface '" << name_view() << "'";
        }
    }
}

llvm::Constant* InterfaceDefinition::llvm_build_vtable(Codegen& gen, ExtendableMembersContainerNode* for_struct, llvm::StructType* vtable_type) {
    std::vector<llvm::Constant*> llvm_pointers;
    llvm_build_inherited_vtable(gen, for_struct, llvm_pointers);
    llvm_build_vtable(gen, for_struct, llvm_pointers);
    return llvm::ConstantStruct::get(vtable_type, llvm_pointers);
}

static llvm::GlobalValue::LinkageTypes to_linkage(AccessSpecifier specifier) {
    switch(specifier) {
        case AccessSpecifier::Public:
        case AccessSpecifier::Protected:
        default:
            return llvm::GlobalValue::ExternalLinkage;
        case AccessSpecifier::Internal:
        case AccessSpecifier::Private:
            return llvm::GlobalValue::PrivateLinkage;
    }
}

llvm::Value* InterfaceDefinition::create_global_vtable(Codegen& gen, ExtendableMembersContainerNode* for_struct, bool declare_only) {
    // building vtable
    const auto constant = declare_only ? nullptr : llvm_build_vtable(gen, for_struct);
    const auto vtable_type = declare_only ? llvm_vtable_type(gen) : constant->getType();
    const auto linkage = to_linkage(specifier());
    ScratchString<128> temp_name;
    gen.mangler.mangle_vtable_name(temp_name, this, for_struct);
    auto table = new llvm::GlobalVariable(
            *gen.module,
            vtable_type,
            true,
            linkage,
            constant,
            (std::string_view) temp_name
    );
    vtable_pointers[for_struct] = table;
    return table;
}

llvm::Constant* primitive_impl_vtable(Codegen& gen, InterfaceDefinition* iDef, ImplDefinition* def) {
    std::vector<llvm::Constant*> llvm_pointers;
    for(const auto func : def->instantiated_functions()) {
        llvm_pointers.emplace_back(func->known_func(gen));
    }
    return llvm::ConstantStruct::get(iDef->llvm_vtable_type(gen), llvm_pointers);
}

llvm::Value* InterfaceDefinition::create_global_vtable(Codegen& gen, ImplDefinition* implDef, BaseType* implType, bool declare_only) {
    // building vtable
    const auto constant = declare_only ? nullptr : primitive_impl_vtable(gen, this, implDef);
    const auto vtable_type = declare_only ? llvm_vtable_type(gen) : constant->getType();
    const auto linkage = to_linkage(specifier());
    ScratchString<128> temp_name;
    gen.mangler.mangle_vtable_name(temp_name, this, implType);
    auto table = new llvm::GlobalVariable(
            *gen.module,
            vtable_type,
            true,
            linkage,
            constant,
            (std::string_view) temp_name
    );
    return table;
}

#endif

int InterfaceDefinition::vtable_function_index(FunctionDeclaration* decl) {
    int i = 0;
    for(const auto func : evaluated_nodes()) {
        if(func == decl) {
            return i;
        }
        i++;
    }
    return -1;
}

uint64_t InterfaceDefinition::byte_size(const TargetData& target) {
#ifdef DEBUG
    CHEM_THROW_RUNTIME("InterfaceDefinition::byte_size interface byte_size called");
#endif
    return 0;
}

