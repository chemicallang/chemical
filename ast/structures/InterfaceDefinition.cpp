// Copyright (c) Chemical Language Foundation 2025.

#include "InterfaceDefinition.h"
#include "StructDefinition.h"
#include "StructMember.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/LinkedType.h"
#include <sstream>

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
    if(!decl->has_self_param() && (attrs.has_implementation || !users.empty())) {
        decl->code_gen_declare(gen, this);
        decl->code_gen_body(gen, this);
        return;
    }
    code_gen_for_users(gen, decl);
}

void InterfaceDefinition::code_gen_function_body(Codegen& gen, FunctionDeclaration* decl) {
    // this function is called by function declaration, when code_gen is called on a function
    // however since interface doesn't generate any (body) so we do nothing
}

void declare_generic_static_interface(Codegen &gen, InterfaceDefinition* def) {
    const auto total = def->total_generic_iterations();
    if(total == 0) return; // generic type was never used
    auto prev_active_iteration = def->active_iteration;
    auto& itr_ptr = def->iterations_declared;
    auto struct_itr = itr_ptr;
    while(struct_itr < total) {
        // generating code and copying iterations
        def->set_active_iteration(struct_itr);
        def->early_declare_structural_generic_args(gen);
        for (auto& func: def->functions()) {
            func->code_gen_declare(gen, def);
        }
        def->acquire_function_iterations(struct_itr);
        struct_itr++;
    }
    itr_ptr = struct_itr;
    def->set_active_iteration(prev_active_iteration);
}

void InterfaceDefinition::code_gen(Codegen &gen) {
    if(is_static()) {
        if(is_generic()) {
            declare_generic_static_interface(gen, this);
        } else {
            for (auto& func: functions()) {
                func->code_gen_declare(gen, this);
            }
        }
    } else {
        for (auto& func: functions()) {
            if(!func->has_self_param() && (attrs.has_implementation || !users.empty())) {
                func->code_gen_declare(gen, this);
                func->code_gen_body(gen, this);
            }
        }
        for (const auto& function: functions()) {
            code_gen_for_users(gen, function);
        }
        // generating vtables for each user struct
        for(auto& user : users) {
            llvm_build_vtable(gen, user.first);
        }
    }
}

void InterfaceDefinition::code_gen_external_declare(Codegen &gen) {
    if(is_static()) {
        extendable_external_declare(gen);
    } else {
        // for functions that don't take a self parameter, we just straight up declare them
        // because they have already been generated in other module
        for (auto& func: functions()) {
            if(!func->has_self_param() && (attrs.has_implementation || !users.empty())) {
                func->code_gen_external_declare(gen);
            }
        }
        // for each function:
        // we find their users, which contain function pointers, if function pointer exists, we declare the function
        // if no function pointer exists, we have to assume no implementation exists, and generate as usual, so users can override it
        for(auto& func : functions()) {
            for (auto& use: users) {
                auto& user = users[use.first];
                active_user = use.first;
                auto found = user.find(func);
                if (func->has_self_param()) {
                    if((found == user.end() || found->second == nullptr)) {
                        // if no implementation (function pointer exists, we declare and generate the body so users (structs) can override it)
                        func->code_gen_declare(gen, this);
                        func->code_gen_body(gen, this);
                    } else {
                        // since function pointer exists and is not null pointer
                        // however because the function is from other module, this function pointer is invalid in this module
                        // we must declare the function and reset the function pointer for the user
                        func->code_gen_external_declare(gen);
                    }
                }
                user[func] = (llvm::Function*) func->llvm_pointer(gen);
            }
            active_user = nullptr;
        }
        // now we regenerate the vtables, for which vtables exist we declare them, otherwise we rebuilt vtables
        for(auto& use : users) {
            auto found = vtable_pointers.find(use.first);
            // we found the vtable, we must redeclare it in this module otherwise we regenerate it
            create_global_vtable(gen, use.first, found != vtable_pointers.end());
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
    struct_types.reserve(struct_types.size() + functions().size());
    for(auto& func : functions()) {
        struct_types.emplace_back(gen.builder->getPtrTy());
    }
}

llvm::StructType* InterfaceDefinition::llvm_vtable_type(Codegen& gen) {
    std::vector<llvm::Type*> struct_types;
    llvm_build_inherited_vtable_type(gen, struct_types);
    llvm_vtable_type(gen, struct_types);
    return llvm::StructType::get(*gen.ctx, struct_types);
}

void InterfaceDefinition::llvm_build_vtable(Codegen& gen, StructDefinition* for_struct, std::vector<llvm::Constant*>& llvm_pointers) {
    auto found = users.find(for_struct);
    if(found != users.end()) {
        for(auto& func : functions()) {
            auto func_res = found->second.find(func);
            if(func_res != found->second.end()) {
                llvm_pointers.emplace_back(func_res->second);
            } else {
                gen.error((AnnotableNode*) func) << "couldn't find function impl pointer, name '" << func->name_view() << "' for struct '" << for_struct->name_view() << "' for interface '" << name_view() << "'";
            }
        }
    } else {
        gen.error((AnnotableNode*) for_struct) << "couldn't find struct '" << for_struct->name_view() << "' implementation pointers for interface '" << name_view() << "'";
    }
}

llvm::Constant* InterfaceDefinition::llvm_build_vtable(Codegen& gen, StructDefinition* for_struct, llvm::StructType* vtable_type) {
    std::vector<llvm::Constant*> llvm_pointers;
    llvm_build_inherited_vtable(gen, for_struct, llvm_pointers);
    llvm_build_vtable(gen, for_struct, llvm_pointers);
    return llvm::ConstantStruct::get(vtable_type, llvm_pointers);
}

llvm::Value* InterfaceDefinition::create_global_vtable(Codegen& gen, StructDefinition* for_struct, bool declare_only) {
    // building vtable
    const auto constant = declare_only ? nullptr : llvm_build_vtable(gen, for_struct);
    const auto vtable_type = declare_only ? llvm_vtable_type(gen) : constant->getType();
    const auto linkage = specifier() == AccessSpecifier::Public ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::InternalLinkage;
    auto table = new llvm::GlobalVariable(
            *gen.module,
            vtable_type,
            true,
            linkage,
            constant,
            runtime_vtable_name(for_struct)
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

void InterfaceDefinition::runtime_vtable_name(std::ostream& stream, StructDefinition* def) {
    runtime_name(stream);
    def->runtime_name(stream);
}

std::string InterfaceDefinition::runtime_vtable_name(StructDefinition* def) {
    std::ostringstream str;
    runtime_name(str);
    def->runtime_name(str);
    return str.str();
}

BaseType* InterfaceDefinition::create_value_type(ASTAllocator& allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(name_view(), this, encoded_location());
}

int InterfaceDefinition::vtable_function_index(FunctionDeclaration* decl) {
    int i = 0;
    for(auto& func : functions()) {
        if(func == decl) {
            return i;
        }
        i++;
    }
    return -1;
}

uint64_t InterfaceDefinition::byte_size(bool is64Bit) {
#ifdef DEBUG
throw std::runtime_error("InterfaceDefinition::byte_size interface byte_size called");
#endif
    return 0;
}

void InterfaceDefinition::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare_node(name_view(), this, specifier(), false);
}