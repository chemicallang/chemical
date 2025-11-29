// Copyright (c) Chemical Language Foundation 2025.

#include "StructMember.h"
#include "StructDefinition.h"
#include "FunctionDeclaration.h"
#include "ast/types/LinkedType.h"
#include "ast/types/StructType.h"
#include "InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/types/VoidType.h"
#include "ast/types/PointerType.h"
#include "ast/types/GenericType.h"
#include "ast/values/NullValue.h"
#include "UnnamedStruct.h"
#include "std/except.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/types/LinkedType.h"
#include "compiler/mangler/NameMangler.h"

void StructDefinition::struct_func_gen(
    Codegen& gen,
    InstFuncRange funcs,
    bool declare
) {
    if(declare) {
        for (auto& function: funcs) {
            if (function->is_override()) {

                // Do not declare the function because it overrides another function
                // when a function is being overridden which is already present in an interface
                // interface generates all declarations with entry blocks for it's users

                // BUT interface hasn't been tested to do this across modules

                continue;
            }
            function->code_gen_declare(gen, this);
        }
    } else {
        for (auto& function: funcs) {
            if (function->is_override()) {
                llvm_override(gen, function);
                continue;
            }
            function->code_gen_body(gen, this);
        }
    }
}

// tries to override the function present in interface
// returns true if current function should be skipped because it has been overridden
// or errored out
void StructDefinition::llvm_override(Codegen& gen, FunctionDeclaration* function) {
    const auto inherited_func = inherited_function(function->name_view());
    if(inherited_func == nullptr) {
        gen.error("couldn't find base function", function);
        return;
    }
    const auto func_parent = inherited_func->parent();
    if(func_parent == nullptr || func_parent->kind() != ASTNodeKind::InterfaceDecl) {
        gen.error("couldn't find base function container", function);
        return;
    }
    const auto interface = func_parent->as_interface_def_unsafe();
    // we always assume base container as interface, it could be something else (abstract struct maybe)
    if(interface->is_static()) {
        const auto func = inherited_func->llvm_func(gen);
        const auto interfaceModule = func->getParent();
        if(interfaceModule != gen.module.get()) {
            // interface is present in another module
            // we create a new function with strong linkage in this module
            const auto new_func = gen.create_function(func->getName(), func->getFunctionType(), function, AccessSpecifier::Public);
            function->set_llvm_data(gen, new_func);
            function->code_gen_override(gen, new_func);
        } else {
            // internal interface, present in current module
            // we will implement the interface in place, since its present in current module
            function->set_llvm_data(gen, func);
            if(func->size() == 1) {
                // remove the stub block present in functions internal to module
                auto& stubEntry = func->getEntryBlock();
                stubEntry.removeFromParent();
            }
            const auto final_specifier = is_linkage_public(interface->specifier()) || is_linkage_public(specifier()) ? llvm::GlobalValue::ExternalLinkage : llvm::GlobalValue::PrivateLinkage;
            // change the function's linkage to internal
            func->setLinkage(final_specifier);
            gen.createFunctionBlock(func);
            function->code_gen_override(gen, func);
        }
    } else {
        auto user_itr = interface->users.find(this);
        if(user_itr == interface->users.end()) {
            gen.error("failed to override function, couldn't find given struct as user of the interface", function);
            return;
        }
        auto& user = user_itr.value();
        auto llvm_data = user.find(inherited_func);
        if (llvm_data == user.end()) {
            gen.error("failed to override function, couldn't find the base function and its pointer", function);
            return;
        }
        auto& overridable_info = llvm_data->second;
        // indicate to interface that this function has been overridden
        // so default implementation must not be generated (very important)
        overridable_info.overridden = true;
        // clean the function (any default implementation from interface may be there)
        const auto func = overridable_info.func_pointer;
        if(!(func->size() == 1 && func->front().empty())) {
            while(!func->empty()) {
                auto& bb = func->back();
                bb.dropAllReferences();
                bb.eraseFromParent();
            }
            // create a new entry block
            // ignore the return
            llvm::BasicBlock::Create(*gen.ctx, "entry", func);
        }
        function->set_llvm_data(gen, func);
        function->code_gen_override(gen, func);
    }
}

void StructDefinition::code_gen_function_declare(Codegen& gen, FunctionDeclaration* decl) {
    if(decl->is_override()) {
        return;
    }
    decl->code_gen_declare(gen, this);
}

void StructDefinition::code_gen_function_body(Codegen& gen, FunctionDeclaration* decl) {
    if(decl->is_override()) {
        llvm_override(gen, decl);
        return;
    }
    decl->code_gen_body(gen, this);
}

void StructDefinition::code_gen(Codegen &gen, bool declare) {
    if(is_comptime()) {
        return;
    }
    auto& has_done = declare ? has_declared : has_implemented;
    if(!has_done) {
        struct_func_gen(gen, instantiated_functions(), declare);
        if (!declare) {
            for (auto& inherits: inherited) {
                const auto interface = inherits.type->get_direct_linked_interface();
                if (interface && !interface->is_static()) {
                    interface->llvm_global_vtable(gen, this);
                }
            }
        }
        has_done = true;
    }
}

void StructDefinition::code_gen_external_declare(Codegen &gen) {
    // clear the stored llvm types, so they must be declared again by StructType
    llvm_struct_type = nullptr;
    extendable_external_declare(gen);
}

llvm::Type* StructMember::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Value* child_of_self_ptr(Codegen& gen, BaseDefMember& member, llvm::Value* self_ptr) {
    auto parent_struct = member.parent();
    std::vector<llvm::Value*> idxList { gen.builder->getInt32(0) };
    parent_struct->add_child_index(gen, idxList, member.name);
    return gen.builder->CreateGEP(member.parent()->llvm_type(gen), self_ptr, idxList, "", gen.inbounds);
}

llvm::Value* BaseDefMember::llvm_pointer(Codegen &gen) {
#ifdef DEBUG
    if(!isAnyStructMember(kind())) {
        CHEM_THROW_RUNTIME("BaseDefMember: called llvm_pointer on a non struct member");
    }
#endif
    const auto curr_func = gen.current_func_type->as_function();
    if(curr_func == nullptr) {
        gen.error(encoded_location()) << "current function incapable of providing a pointer to struct member with name '" << name << '\'';
        return NullValue::null_llvm_value(gen);
    }
    if(curr_func->is_constructor_fn()) {
        // TODO hard coded the index for the constructor self param
        auto self_ptr = gen.current_function->getArg(0);
        return child_of_self_ptr(gen, *this, self_ptr);
    } else {
        auto self_param = curr_func->get_self_param();
        if(self_param == nullptr) {
            gen.error(encoded_location()) << "current function incapable of providing a pointer to struct member with name '" << name << '\'' << ", no self parameter";
            return NullValue::null_llvm_value(gen);
        }
        auto self_ptr = gen.current_function->getArg(self_param->calculate_c_or_llvm_index(curr_func));
        return child_of_self_ptr(gen, *this, self_ptr);
    }
}

llvm::Value* BaseDefMember::loadable_llvm_pointer(Codegen& gen, SourceLocation location) {
    if(kind() == ASTNodeKind::StructMember) {
        // already need to do a load always, so just return it
        return llvm_pointer(gen);
    } else {
        return ASTNode::turnPtrValueToLoadablePtr(gen, llvm_pointer(gen), location);
    }
}

llvm::Value* BaseDefMember::llvm_load(Codegen& gen, SourceLocation location) {
    auto pointer = llvm_pointer(gen);
    return Value::load_value(gen, known_type(), llvm_type(gen), pointer, location);
}

llvm::Type* StructMember::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return type->llvm_chain_type(gen, values, index);
}

bool StructMember::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const chem::string_view &childName) {
    auto linked = type->linked_node();
    if (!linked) return false;
    linked->add_child_index(gen, indexes, childName);
    return true;
}

void StructDefinition::llvm_destruct(Codegen &gen, llvm::Value *allocaInst, SourceLocation location) {
    auto func = destructor_func();
    if(func) {
        llvm::Function* func_data = func->llvm_func(gen);
        const auto instr = gen.builder->CreateCall(func_data, func->has_self_param() ? std::initializer_list<llvm::Value*> { allocaInst } : std::initializer_list<llvm::Value*> {});
        gen.di.instr(instr, location);
    }
}

llvm::StructType* StructDefinition::llvm_stored_type(Codegen& gen) {
    return llvm_struct_type;
}

void StructDefinition::llvm_store_type(Codegen& gen, llvm::StructType* type) {
    // auto creation
    llvm_struct_type = type;
}

llvm::Type* StructDefinition::with_elements_type(
        Codegen &gen,
        const std::vector<llvm::Type *>& elements,
        bool anonymous
) {
    if(anonymous) {
        return llvm::StructType::get(*gen.ctx, elements);
    }
    auto stored = llvm_stored_type(gen);
    if(!stored) {
        ScratchString<128> temp_name;
        gen.mangler.mangle(temp_name, this);
        auto new_stored = llvm::StructType::create(*gen.ctx, elements, (std::string_view) temp_name);
        llvm_store_type(gen, new_stored);
        return new_stored;
    }
    return stored;
}

llvm::Type *StructDefinition::llvm_type(Codegen &gen) {
    return with_elements_type(gen, elements_type(gen), is_anonymous());
}

llvm::Type *StructDefinition::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *StructDefinition::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return with_elements_type(gen, elements_type(gen, values, index), true);
}

llvm::Type* UnnamedStruct::llvm_type(Codegen &gen) {
    return llvm::StructType::get(*gen.ctx, elements_type(gen));
}

llvm::Type* UnnamedStruct::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return llvm::StructType::get(*gen.ctx, elements_type(gen, values, index));
}

#endif

void StructDefinition::generate_functions(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* returnNode) {
    bool has_constructor = false;
    bool has_def_constructor = false;
    bool has_destructor = false;
    for(auto& func : non_gen_range()) {
        if(func->is_constructor_fn()) {
            has_constructor = true;
            if(!func->has_explicit_params()) {
                has_def_constructor = true;
            }
            func->ensure_constructor(allocator, diagnoser, returnNode);
        }
        if(func->is_delete_fn()) {
            func->ensure_destructor(allocator, diagnoser, returnNode);
            has_destructor = true;
        }
        if(func->is_copy_fn()) {
            func->ensure_copy_fn(allocator, diagnoser, returnNode);
        }
    }
    if(!has_def_constructor && all_members_def_constructible()) {
        if(create_def_constructor_checking(allocator, diagnoser, name_view(), returnNode)) {
            if (!has_constructor) {
                attrs.is_direct_init = true;
            }
        }
    }
    if(!has_destructor && any_member_has_destructor()) {
        has_destructor = true;
        create_def_destructor(allocator, diagnoser, returnNode);
    }
    if(!has_destructor) {
        // we make the struct copyable by default, if it doesn't have any destructor
        attrs.is_copy = true;
    }
}