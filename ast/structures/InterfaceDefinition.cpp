// Copyright (c) Qinetik 2024.

#include "InterfaceDefinition.h"
#include "StructDefinition.h"
#include "StructMember.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/LinkedType.h"

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
            auto func_res = found->second.find(func);
            if(func_res != found->second.end()) {
                llvm_pointers.emplace_back(func_res->second);
            } else {
                gen.error("couldn't find function impl pointer, name '" + func->name_str() + "' for struct '" + for_struct->name_str() + "' for interface '" + name_str() + "'", (AnnotableNode*) func);
            }
        }
    } else {
        gen.error("couldn't find struct '" + for_struct->name_str() + "' implementation pointers for interface '" + name_str() + "'", (AnnotableNode*) for_struct);
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

BaseType* InterfaceDefinition::create_value_type(ASTAllocator& allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(name_view(), this, location);
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