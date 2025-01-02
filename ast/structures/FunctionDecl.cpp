// Copyright (c) Qinetik 2024.

#include <memory>

#include "FunctionParam.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "compiler/SymbolResolver.h"
#include "CapturedVariable.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/statements/VarInit.h"
#include "ast/values/CastedValue.h"
#include "ast/types/LinkedType.h"
#include "ast/structures/InitBlock.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/types/VoidType.h"
#include "ast/values/FunctionCall.h"
#include "ast/statements/Return.h"
#include "ast/statements/Typealias.h"
#include "ast/utils/GenericUtils.h"
#include "ast/types/GenericType.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/utils/ASTUtils.h"
#include <sstream>

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/values/LambdaFunction.h"
#include "ast/utils/ASTUtils.h"

llvm::Type *BaseFunctionParam::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Type *BaseFunctionParam::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return type->llvm_chain_type(gen, values, index);
}

void BaseFunctionParam::code_gen_destruct(Codegen &gen, Value *returnValue) {
    if(!(returnValue && returnValue->linked_node() == this)) {
        type->linked_node()->llvm_destruct(gen, gen.current_function->getArg(calculate_c_or_llvm_index()));
    }
}

llvm::Value *BaseFunctionParam::llvm_pointer(Codegen &gen) {
    auto index = calculate_c_or_llvm_index();
    if(index > gen.current_function->arg_size()) {
        gen.error("couldn't get argument with name " + name.str() + " since function has " + std::to_string(gen.current_function->arg_size()) + " arguments", this);
        return nullptr;
    }
    auto arg = gen.current_function->getArg(index);
    if (arg) {
        return arg;
    } else {
        gen.error("couldn't get argument with name " + name.str(), this);
        return nullptr;
    }
}

llvm::Value *BaseFunctionParam::llvm_load(Codegen &gen) {
    if (gen.current_function != nullptr) {
        return llvm_pointer(gen);
    } else {
        gen.error("cannot provide pointer to a function parameter when not generating code for a function", this);
    }
    return nullptr;
}

llvm::FunctionType *FunctionDeclaration::create_llvm_func_type(Codegen &gen) {
    auto paramTypes = param_types(gen);
    if(paramTypes.empty()) {
        return llvm::FunctionType::get(llvm_func_return(gen, returnType), isVariadic());
    } else {
        return llvm::FunctionType::get(llvm_func_return(gen, returnType), paramTypes, isVariadic());
    }
}

llvm::FunctionType *FunctionDeclaration::known_func_type() {
    if(!llvm_data.empty() && active_iteration < llvm_data.size()) {
        return llvm_data[active_iteration].second;
    } else {
        return nullptr;
    }
}

llvm::FunctionType *FunctionDeclaration::llvm_func_type(Codegen &gen) {
    const auto known = known_func_type();
    if(known) return known;
    return create_llvm_func_type(gen);
}

std::pair<llvm::Value*, llvm::FunctionType*>& FunctionDeclaration::get_llvm_data() {
    if(active_iteration == llvm_data.size() && is_override()) {
        const auto struct_def = parent_node->as_struct_def();
        if(struct_def) {
            const auto overriding = struct_def->get_overriding_info(this);
            if(overriding.first) {
                const auto interface = overriding.first->as_interface_def();
                if(interface) {
                    auto& use = interface->users[struct_def];
                    const auto& found = use.find(overriding.second);
                    if(found != use.end()) {
                        llvm_data.emplace_back(found->second, found->second->getFunctionType());
                        return llvm_data.back();
                    }
                } else {
                    const auto parent_struct = overriding.first->as_struct_def();
                    if(parent_struct) {
                        // TODO: do this;
                    }
                }
            }
        }
    }
    if(parent_node) {
        auto k = parent_node->kind();
        if(k == ASTNodeKind::StructDecl || k == ASTNodeKind::VariantDecl || k == ASTNodeKind::UnionDecl) {
            auto container = parent_node->as_members_container_unsafe();
            if(!container->generic_params.empty()) { // container is generic
                return container->generic_llvm_data[this][container->active_iteration][active_iteration];
            }
        }
    }
    return llvm_data[active_iteration];
}

llvm::Value* FunctionDeclaration::llvm_callee() {
    return get_llvm_data().first;
}

llvm::Function* FunctionDeclaration::llvm_func() {
    return (llvm::Function*)  llvm_callee();
}

void FunctionType::queue_destruct_params(Codegen& gen) {
    for(const auto param : params) {
        if(param->get_has_moved()) continue;
        const auto k = param->type->kind();
        if(k == BaseTypeKind::Linked || k == BaseTypeKind::Generic) {
            const auto def = param->type->linked_node();
            if(def) {
                const auto members_container = def->as_members_container();
                if(members_container && members_container->destructor_func()) {
                    gen.destruct_nodes.emplace_back(param);
                }
            }
        }
    }
}

void initialize_def_struct_values(Codegen &gen, StructDefinition* struct_def, FunctionDeclaration* decl) {
    std::unordered_map<chem::string_view, InitBlockInitializerValue>* initializers = nullptr;
    if(decl->body.has_value() && !decl->body->nodes.empty()) {
        auto block = decl->body->nodes.front()->as_init_block();
        if(block) {
            initializers = &block->initializers;
        }
    }
    auto self_arg = gen.current_function->getArg(0);
    auto parent_type = struct_def->llvm_type(gen);
    for(auto& var : struct_def->variables) {
        const auto defValue = var.second->default_value();
        if(!defValue) continue;
        auto has_not_been_initialized = !initializers || initializers->find(var.first) == initializers->end();
        if(has_not_been_initialized) {
            // couldn't move struct
            auto variable = struct_def->variable_type_index(var.first, false);
            std::vector<llvm::Value*> idx { gen.builder->getInt32(0) };
            defValue->store_in_struct(gen, nullptr, self_arg, parent_type, idx, variable.first, variable.second);
        }
    }
}

void initialize_constructor_def_values(Codegen &gen, FunctionDeclaration* decl) {
    auto parent = decl->parent_node;
    if(parent) {
        if(!decl->is_constructor_fn()) {
            return;
        }
        const auto struct_def = parent->as_struct_def();
        if(struct_def) {
            initialize_def_struct_values(gen, struct_def, decl);
        }
    }
}

void body_gen(Codegen &gen, llvm::Function* funcCallee, std::optional<LoopScope>& body, FunctionDeclaration* func_type) {
    if(body.has_value()) {
        auto prev_func_type = gen.current_func_type;
        auto prev_func = gen.current_function;
        gen.current_func_type = func_type;
        gen.current_function = funcCallee;
        auto prev_destruct_nodes = std::move(gen.destruct_nodes);
        const auto destruct_begin = 0;
        func_type->queue_destruct_params(gen);
        gen.SetInsertPoint(&funcCallee->getEntryBlock());
        initialize_constructor_def_values(gen, func_type);
        body->code_gen(gen, destruct_begin);
        gen.end_function_block();
        gen.destruct_nodes = std::move(prev_destruct_nodes);
        gen.current_function = nullptr;
        gen.current_func_type = prev_func_type;
    }
}

void body_gen(Codegen &gen, FunctionDeclaration* decl, llvm::Function* funcCallee) {
    body_gen(gen, funcCallee, decl->body, decl);
}

void FunctionDeclaration::code_gen_body(Codegen &gen) {
    if(is_comptime()) {
        return;
    }
    if(generic_params.empty()) {
        body_gen(gen, this, llvm_func());
    } else {
        const auto total = total_generic_iterations();
        while(bodies_gen_index < total) {
            set_active_iteration(bodies_gen_index);
            body_gen(gen, this, llvm_func());
            bodies_gen_index++;
        }
        // we set active iteration to -1, so all generics would fail if acessed without setting active iteration
        set_active_iteration(-1);
    }
}

void FunctionDeclaration::code_gen(Codegen &gen) {
    if(is_comptime()) {
        return;
    }
    if(parent_node) {
        auto k = parent_node->kind();
        switch(k) {
            case ASTNodeKind::StructDecl:{
                const auto parent_decl = parent_node->as_struct_def_unsafe();
                parent_decl->code_gen_function_body(gen, this);
                return;
            }
            case ASTNodeKind::VariantDecl: {
                const auto parent_decl = parent_node->as_variant_def_unsafe();
                parent_decl->code_gen_function_body(gen, this);
                return;
            }
            case ASTNodeKind::UnionDecl: {
                const auto parent_decl = parent_node->as_union_def_unsafe();
                parent_decl->code_gen_function_body(gen, this);
                return;
            }
            case ASTNodeKind::InterfaceDecl: {
                const auto parent_decl = parent_node->as_interface_def_unsafe();
                parent_decl->code_gen_function_body(gen, this);
                return;
            }
            case ASTNodeKind::ImplDecl: {
                const auto parent_decl = parent_node->as_impl_def_unsafe();
                parent_decl->code_gen_function_body(gen, this);
                return;
            }
            case ASTNodeKind::NamespaceDecl:
            default:
                code_gen_body(gen);
                return;
        }
    } else {
        code_gen_body(gen);
    }
}

void FunctionDeclaration::code_gen_generic(Codegen &gen) {
    code_gen_declare(gen);
    code_gen_body(gen);
}

void FunctionDeclaration::llvm_attributes(llvm::Function* func) {
    if(attrs.is_inline) {
        // do nothing at the moment
    }
    if(attrs.always_inline) {
        func->addFnAttr(llvm::Attribute::AlwaysInline);
    }
    if(attrs.no_inline) {
        func->addFnAttr(llvm::Attribute::NoInline);
    }
    if(attrs.inline_hint) {
        func->addFnAttr(llvm::Attribute::InlineHint);
    }
    if(attrs.opt_size) {
        func->addFnAttr(llvm::Attribute::OptimizeForSize);
    }
    if(attrs.min_size) {
        func->addFnAttr(llvm::Attribute::MinSize);
    }
}

void llvm_func_def_attr(llvm::Function* func) {
//    func->addFnAttr(llvm::Attribute::UWTable); // this causes error
    func->addFnAttr(llvm::Attribute::NoUnwind);
}

void FunctionDeclaration::set_llvm_data(llvm::Value* func_callee, llvm::FunctionType* func_type) {
#ifdef DEBUG
    if(active_iteration > (int) llvm_data.size()) {
        throw std::runtime_error("decl's generic active iteration is greater than total llvm_data size");
    }
#endif
    if(active_iteration == llvm_data.size()) {
        llvm_data.emplace_back(func_callee, func_type);
    } else {
        llvm_data[active_iteration].first = func_callee;
        llvm_data[active_iteration].second = func_type;
    }
}

std::string FunctionDeclaration::runtime_name_fast(Codegen& gen) {
    if(is_cpp_mangle()) {
        // TODO what about generic functions ?
        return gen.clang.mangled_name(this);
    }
    return runtime_name_fast();
}

void create_non_generic_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name) {
#ifdef DEBUG
    auto existing_func = gen.module->getFunction(name);
    if(existing_func) {
        gen.error("function with name '" + name + "' already exists in the module", (ASTNode*) decl);
    }
#endif
    auto func_type = decl->create_llvm_func_type(gen);
    auto func = gen.create_function(name, func_type, decl->specifier());
    llvm_func_def_attr(func);
    decl->llvm_attributes(func);
    decl->set_llvm_data(func, func->getFunctionType());
}

void create_fn(Codegen& gen, FunctionDeclaration *decl) {
    if(decl->generic_params.empty()) {
        create_non_generic_fn(gen, decl, decl->runtime_name_fast(gen));
    } else {
        const auto total_use = decl->total_generic_iterations();
        auto i = (int16_t) decl->llvm_data.size();
        while(i < total_use) {
            decl->set_active_iteration(i);
            create_non_generic_fn(gen, decl, decl->runtime_name_fast(gen));
            i++;
        }
        // we set active iteration to -1, so all generics would fail without setting active_iteration
        decl->set_active_iteration(-1);
    }
}

void declare_non_gen_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name) {
    auto callee = gen.declare_function(name, decl->create_llvm_func_type(gen));
    decl->set_llvm_data(callee.getCallee(), callee.getFunctionType());
}

void declare_fn(Codegen& gen, FunctionDeclaration *decl) {
    if(decl->generic_params.empty()) {
        declare_non_gen_fn(gen, decl, decl->runtime_name_fast(gen));
    } else {
        const auto total_use = decl->total_generic_iterations();
        auto i = (int16_t) decl->llvm_data.size();
        while(i < total_use) {
            decl->set_active_iteration(i);
            declare_non_gen_fn(gen, decl, decl->runtime_name_fast(gen));
            i++;
        }
        // we set active iteration to -1, so all generics would fail without setting active_iteration
        decl->set_active_iteration(-1);
    }
}

inline void create_or_declare_fn(Codegen& gen, FunctionDeclaration* decl) {
    if (decl->body.has_value()) {
        create_fn(gen, decl);
    } else {
        declare_fn(gen, decl);
    }
}

void FunctionDeclaration::code_gen_declare_normal(Codegen& gen) {
    if(is_comptime()) {
        return;
    }
    create_or_declare_fn(gen, this);
    gen.current_function = nullptr;
}

void FunctionDeclaration::code_gen_declare(Codegen &gen) {
    if(is_comptime()) {
        return;
    }
    if(parent_node) {
        auto k = parent_node->kind();
        switch(k) {
            case ASTNodeKind::StructDecl:{
                const auto parent_decl = parent_node->as_struct_def_unsafe();
                parent_decl->code_gen_function_declare(gen, this);
                return;
            }
            case ASTNodeKind::VariantDecl: {
                const auto parent_decl = parent_node->as_variant_def_unsafe();
                parent_decl->code_gen_function_declare(gen, this);
                return;
            }
            case ASTNodeKind::UnionDecl: {
                const auto parent_decl = parent_node->as_union_def_unsafe();
                parent_decl->code_gen_function_declare(gen, this);
                return;
            }
            case ASTNodeKind::InterfaceDecl: {
                const auto parent_decl = parent_node->as_interface_def_unsafe();
                parent_decl->code_gen_function_declare(gen, this);
                return;
            }
            case ASTNodeKind::ImplDecl: {
                // Implementation doesn't declare any function because it always generates body for functions that
                // have been already declared
                return;
            }
            case ASTNodeKind::NamespaceDecl:
            default:
                code_gen_declare_normal(gen);
                return;
        }
    } else {
        code_gen_declare_normal(gen);
    }
}

void FunctionDeclaration::code_gen_override(Codegen& gen, llvm::Function* llvm_func) {
    body_gen(gen, this, llvm_func);
}

void FunctionDeclaration::code_gen_external_declare(Codegen &gen) {
    llvm_data.clear();
    if(!is_generic()) {
        declare_non_gen_fn(gen, this, runtime_name_fast());
    } else {
        // declare functions for which bodies have been generated
        int16_t i = 0;
        while (i < bodies_gen_index) {
            set_active_iteration(i);
            declare_non_gen_fn(gen, this, runtime_name_fast());
            i++;
        }
        // create functions and generate bodies for only those functions that
        // do not have it, since create_fn and body_gen both use llvm_data.size() to start
        code_gen_declare(gen);
        code_gen_body(gen);
    }
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, StructDefinition* def) {
    if(is_comptime()) {
        return;
    }
    create_or_declare_fn(gen, this);
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, VariantDefinition* def) {
    if(is_comptime()) {
        return;
    }
    create_or_declare_fn(gen, this);
}

void FunctionDeclaration::code_gen_override_declare(Codegen &gen, FunctionDeclaration* decl) {
    set_llvm_data(decl->llvm_pointer(gen), decl->llvm_func_type(gen));
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, InterfaceDefinition* def) {
    create_fn(gen, this);
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, UnionDef* def) {
    if(is_comptime()) {
        return;
    }
    create_fn(gen, this);
}

void FunctionDeclaration::code_gen_body(Codegen &gen, StructDefinition* def) {
    if(is_comptime()) {
        return;
    }
    if(is_copy_fn()) {
        code_gen_copy_fn(gen, def);
        return;
    }
    if(is_move_fn()) {
        code_gen_move_fn(gen, def);
        return;
    }
    if(is_clear_fn()) {
        code_gen_clear_fn(gen, def);
        return;
    }
    if(is_delete_fn()) {
        code_gen_destructor(gen, def);
        return;
    }
    gen.current_function = nullptr;
    code_gen_body(gen);
}

void FunctionDeclaration::code_gen_body(Codegen &gen, InterfaceDefinition* def) {
    gen.current_function = nullptr;
    if(body.has_value()) {
        code_gen_body(gen);
    }
}

void FunctionDeclaration::code_gen_body(Codegen &gen, VariantDefinition* def) {
    if(is_comptime()) {
        return;
    }
    if(is_copy_fn()) {
        code_gen_copy_fn(gen, def);
        return;
    }
    if(is_clear_fn()) {
        code_gen_clear_fn(gen, def);
        return;
    }
    if(is_move_fn()) {
        code_gen_move_fn(gen, def);
        return;
    }
    if(is_delete_fn()) {
        code_gen_destructor(gen, def);
        return;
    }
    gen.current_function = nullptr;
    code_gen_body(gen);
}

void FunctionDeclaration::code_gen_body(Codegen &gen, UnionDef* def) {
    if(is_comptime()) {
        return;
    }
    gen.current_function = nullptr;
    code_gen_body(gen);
}

void FunctionDeclaration::setup_cleanup_block(Codegen &gen, llvm::Function* func) {
    if(body.has_value() && !body->nodes.empty()) {
        llvm::BasicBlock* cleanup_block = llvm::BasicBlock::Create(*gen.ctx, "", func);
        gen.redirect_return = cleanup_block;
        gen.current_function = nullptr;
        code_gen_body(gen);
        gen.CreateBr(cleanup_block); // ensure branch to cleanup block
        gen.SetInsertPoint(cleanup_block);
    } else {
        gen.SetInsertPoint(&func->getEntryBlock());
    }
}

void create_call_member_func(Codegen& gen, FunctionDeclaration* decl, StructDefinition* def, llvm::Function* func, unsigned index) {
    auto arg = func->getArg(0);
    std::vector<llvm::Value *> idxList{gen.builder->getInt32(0)};
    std::vector<llvm::Value*> args;
    auto element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), arg, idxList, index);
    if(decl->has_self_param()) {
        args.emplace_back(element_ptr);
    }
    gen.builder->CreateCall(decl->llvm_func_type(gen), decl->llvm_pointer(gen), args);
}

void code_gen_member_calls(
        Codegen& gen,
        StructDefinition* def,
        llvm::Function* func,
        FunctionDeclaration*(*choose_func)(MembersContainer*),
        void(*member_func_call)(Codegen& gen, FunctionDeclaration* decl, StructDefinition* def, llvm::Function* func, unsigned index)
) {
    unsigned index = 0;
    for(auto& inherited : def->inherited) {
        const auto base_def = inherited->type->get_direct_linked_struct();
        if(base_def) {
            auto chosen_func = choose_func(base_def);
            if(chosen_func) {
                member_func_call(gen, chosen_func, def, func, index);
            }
        }
        index++;
    }
    for(auto& var : def->variables) {
        if(var.second->value_type() == ValueType::Struct) {
            auto mem_type = var.second->get_value_type(gen.allocator);
            auto mem_def = mem_type->linked_node()->as_members_container();
            auto destructor = choose_func(mem_def);
            if(!destructor) {
                index++;
                continue;
            }
            member_func_call(gen, destructor, def, func, index);
        }
        index++;
    }
}

void code_gen_calling_member_functions(
        FunctionDeclaration& decl,
        Codegen& gen,
        StructDefinition* def,
        FunctionDeclaration*(*choose_func)(MembersContainer*),
        void(*member_func_call)(Codegen& gen, FunctionDeclaration* decl, StructDefinition* def, llvm::Function* func, unsigned index)
) {
    auto func = decl.llvm_func();
    decl.setup_cleanup_block(gen, func);
    code_gen_member_calls(gen, def, func, choose_func, member_func_call);
    gen.CreateRet(nullptr);
    gen.redirect_return = nullptr;
}

void FunctionDeclaration::code_gen_copy_fn(Codegen& gen, StructDefinition* def) {
    auto func = llvm_func();
    gen.SetInsertPoint(&func->getEntryBlock());
    code_gen_member_calls(gen, def, func, [](MembersContainer* mem_def)->FunctionDeclaration* {
        return mem_def->copy_func();
    }, [](Codegen& gen, FunctionDeclaration* decl, StructDefinition* def, llvm::Function* func, unsigned index) {
        auto selfArg = func->getArg(0);
        auto otherArg = func->getArg(1);
        std::vector<llvm::Value*> args;
        std::vector<llvm::Value *> idxList{gen.builder->getInt32(0)};
        auto element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), selfArg, idxList, index);
        args.emplace_back(element_ptr);
        idxList.pop_back();
        auto other_element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), otherArg, idxList, index);
        args.emplace_back(other_element_ptr);
        gen.builder->CreateCall(decl->llvm_func_type(gen), decl->llvm_pointer(gen), args);
    });
    code_gen_body(gen);
}

void FunctionDeclaration::code_gen_move_fn(Codegen& gen, StructDefinition* def) {
    auto func = llvm_func();
    gen.SetInsertPoint(&func->getEntryBlock());
    code_gen_member_calls(gen, def, func, [](MembersContainer* mem_def)->FunctionDeclaration* {
        return mem_def->pre_move_func();
    }, [](Codegen& gen, FunctionDeclaration* decl, StructDefinition* def, llvm::Function* func, unsigned index) {
        auto selfArg = func->getArg(0);
        auto otherArg = func->getArg(1);
        std::vector<llvm::Value*> args;
        std::vector<llvm::Value *> idxList{gen.builder->getInt32(0)};
        auto element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), selfArg, idxList, index);
        args.emplace_back(element_ptr);
        idxList.pop_back();
        auto other_element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), otherArg, idxList, index);
        args.emplace_back(other_element_ptr);
        gen.builder->CreateCall(decl->llvm_func_type(gen), decl->llvm_pointer(gen), args);
    });
    code_gen_body(gen);
}

void FunctionDeclaration::code_gen_clear_fn(Codegen& gen, StructDefinition* def) {
    auto func = llvm_func();
    gen.SetInsertPoint(&func->getEntryBlock());
    code_gen_member_calls(gen, def, func, [](MembersContainer* mem_def)->FunctionDeclaration* {
        return mem_def->clear_func();
    }, create_call_member_func);
    code_gen_body(gen);
}

llvm::Value* variant_struct_pointer(
        Codegen &gen,
        llvm::Value* variant_ptr,
        VariantDefinition* def
) {
    return gen.builder->CreateGEP(def->llvm_type(gen), variant_ptr, { gen.builder->getInt32(0), gen.builder->getInt32(1) }, "", gen.inbounds);
}

// process members of variant definition, calling functions for each member, in a switch statement
void process_members_calling_fns(
        Codegen& gen,
        VariantDefinition* def,
        llvm::Value* allocaInst,
        llvm::Function* func,
        bool(*should_process)(VariantMember* member),
        void(*process)(Codegen &gen, VariantMember* member, llvm::Value* struct_ptr, llvm::Function* func)
) {
    // get the type int
    auto gep = gen.builder->CreateGEP(def->llvm_type(gen), allocaInst, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    auto type_value = gen.builder->CreateLoad(gen.builder->getInt32Ty(), gep);

    // create an end block, for default case
    llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*gen.ctx, "", func);

    // figure out which members to process
    std::vector<std::pair<int, VariantMember*>> to_process;
    int index = 0;
    for(auto& mem : def->variables) {
        const auto variant_mem = mem.second->as_variant_member();
        if(should_process(variant_mem)) {
            to_process.emplace_back(index, variant_mem);
        }
        index++;
    }

    // get struct pointer
    auto struct_ptr = variant_struct_pointer(gen, allocaInst, def);

    // create switch block on type int
    auto switchInst = gen.builder->CreateSwitch(type_value, end_block, to_process.size());

    // create blocks for cases for which process exists
    for(auto& mem : to_process) {
        auto mem_block = llvm::BasicBlock::Create(*gen.ctx, "", func);
        gen.SetInsertPoint(mem_block);
        process(gen, mem.second, struct_ptr, func);
        gen.CreateBr(end_block);
        switchInst->addCase(gen.builder->getInt32(mem.first), mem_block);
    }

    gen.SetInsertPoint(end_block);
}

void FunctionDeclaration::code_gen_copy_fn(Codegen& gen, VariantDefinition* def) {
    auto func = llvm_func();
    gen.SetInsertPoint(&func->getEntryBlock());
    auto allocaInst = func->getArg(0);
    auto otherInst = func->getArg(1);
    // storing the type integer
    auto other_type = gen.builder->CreateGEP(def->llvm_type(gen), otherInst, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    auto this_type = gen.builder->CreateGEP(def->llvm_type(gen), allocaInst, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    auto loaded = gen.builder->CreateLoad(gen.builder->getInt32Ty(), other_type);
    gen.builder->CreateStore(loaded, this_type);
    // processing members to call copy functions on members
    process_members_calling_fns(gen, def, allocaInst, func, [](VariantMember* mem)-> bool {
        return mem->requires_copy_fn();
    }, [](Codegen& gen, VariantMember* mem, llvm::Value* struct_ptr, llvm::Function* func) {
        auto otherArg = func->getArg(1);
        auto other_struct_pointer = variant_struct_pointer(gen, otherArg, mem->parent_node);
        auto index = -1;
        for(auto& param : mem->values) {
            index++;
            auto linked = param.second->type->linked_node();
            if(linked) {
                auto container = linked->as_members_container();
                if(container) {
                    auto def = mem->parent_node;
                    auto decl = container->copy_func();
                    std::vector<llvm::Value*> args;
                    std::vector<llvm::Value *> idxList{gen.builder->getInt32(0)};
                    auto element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), struct_ptr, idxList, index);
                    args.emplace_back(element_ptr);
                    idxList.pop_back();
                    auto other_element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), other_struct_pointer, idxList, index);
                    args.emplace_back(other_element_ptr);
                    gen.builder->CreateCall(decl->llvm_func_type(gen), decl->llvm_pointer(gen), args);
                }
            }
        }
    });
    gen.builder->CreateRetVoid();
    if(body.has_value() && !body->nodes.empty()) {
        code_gen_body(gen);
    }
}

void FunctionDeclaration::code_gen_move_fn(Codegen& gen, VariantDefinition* def) {
    auto func = llvm_func();
    gen.SetInsertPoint(&func->getEntryBlock());
    auto allocaInst = func->getArg(0);
    auto otherInst = func->getArg(1);
    // storing the type integer
    auto other_type = gen.builder->CreateGEP(def->llvm_type(gen), otherInst, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    auto this_type = gen.builder->CreateGEP(def->llvm_type(gen), allocaInst, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    auto loaded = gen.builder->CreateLoad(gen.builder->getInt32Ty(), other_type);
    gen.builder->CreateStore(loaded, this_type);
    // processing members to call copy functions on members
    process_members_calling_fns(gen, def, allocaInst, func, [](VariantMember* mem)-> bool {
        return mem->requires_move_fn();
    }, [](Codegen& gen, VariantMember* mem, llvm::Value* struct_ptr, llvm::Function* func) {
        auto otherArg = func->getArg(1);
        auto other_struct_pointer = variant_struct_pointer(gen, otherArg, mem->parent_node);
        auto index = -1;
        for(auto& param : mem->values) {
            index++;
            auto linked = param.second->type->linked_node();
            if(linked) {
                auto container = linked->as_members_container();
                if(container) {
                    auto def = mem->parent_node;
                    auto decl = container->pre_move_func();
                    std::vector<llvm::Value*> args;
                    std::vector<llvm::Value *> idxList{gen.builder->getInt32(0)};
                    auto element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), struct_ptr, idxList, index);
                    args.emplace_back(element_ptr);
                    idxList.pop_back();
                    auto other_element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), other_struct_pointer, idxList, index);
                    args.emplace_back(other_element_ptr);
                    gen.builder->CreateCall(decl->llvm_func_type(gen), decl->llvm_pointer(gen), args);
                }
            }
        }
    });
    gen.builder->CreateRetVoid();
    if(body.has_value() && !body->nodes.empty()) {
        code_gen_body(gen);
    }
}

void FunctionDeclaration::code_gen_clear_fn(Codegen& gen, VariantDefinition* def) {
    auto func = llvm_func();
    gen.SetInsertPoint(&func->getEntryBlock());
    auto allocaInst = func->getArg(0);
    // processing members to call copy functions on members
    process_members_calling_fns(gen, def, allocaInst, func, [](VariantMember* mem)-> bool {
        return mem->requires_clear_fn();
    }, [](Codegen& gen, VariantMember* mem, llvm::Value* struct_ptr, llvm::Function* func) {
        llvm::FunctionType* dtr_func_type = nullptr;
        llvm::Value* dtr_func_callee = nullptr;
        int i = 0;
        for(auto& value : mem->values) {
            auto ref_node = value.second->type->get_direct_linked_node();
            if(ref_node) { // <-- the node is directly referenced
                auto clearFn = gen.determine_clear_fn_for(value.second->type, dtr_func_type, dtr_func_callee);
                if(clearFn) {
                    std::vector<llvm::Value*> args;
                    if(clearFn->has_self_param()) {
                        auto gep3 = gen.builder->CreateGEP(mem->llvm_type(gen), struct_ptr, { gen.builder->getInt32(0), gen.builder->getInt32(i) }, "", gen.inbounds);
                        args.emplace_back(gep3);
                    }
                    gen.builder->CreateCall(dtr_func_type, dtr_func_callee, args, "");
                }
            }
            i++;
        }
    });
    gen.builder->CreateRetVoid();
    if(body.has_value() && !body->nodes.empty()) {
        code_gen_body(gen);
    }
}

void FunctionDeclaration::code_gen_destructor(Codegen& gen, StructDefinition* def) {
    code_gen_calling_member_functions(*this, gen, def, [](MembersContainer* mem_def)->FunctionDeclaration* {
        return mem_def->destructor_func();
    }, create_call_member_func);
}

void FunctionDeclaration::code_gen_destructor(Codegen& gen, VariantDefinition* def) {
    auto func = llvm_func();
    setup_cleanup_block(gen, func);
    // every destructor has self at zero
    auto allocaInst = func->getArg(0);
    process_members_calling_fns(gen, def, allocaInst, func, [](VariantMember* mem) -> bool {
        return mem->requires_destructor();
    }, [](Codegen& gen, VariantMember* mem, llvm::Value* struct_ptr, llvm::Function* func) {
        llvm::FunctionType* dtr_func_type = nullptr;
        llvm::Value* dtr_func_callee = nullptr;
        int i = 0;
        for(auto& value : mem->values) {
            auto ref_node = value.second->type->get_direct_linked_node();
            if(ref_node) { // <-- the node is directly referenced
                auto destructorFunc = gen.determine_destructor_for(value.second->type, dtr_func_type, dtr_func_callee);
                if(destructorFunc) {
                    std::vector<llvm::Value*> args;
                    if(destructorFunc->has_self_param()) {
                        auto gep3 = gen.builder->CreateGEP(mem->llvm_type(gen), struct_ptr, { gen.builder->getInt32(0), gen.builder->getInt32(i) }, "", gen.inbounds);
                        args.emplace_back(gep3);
                    }
                    gen.builder->CreateCall(dtr_func_type, dtr_func_callee, args, "");
                }
            }
            i++;
        }
    });
    gen.CreateRet(nullptr);
    gen.redirect_return = nullptr;
}

std::vector<llvm::Type *> FunctionDeclaration::param_types(Codegen &gen) {
    return llvm_func_param_types(gen, params, returnType, false, isVariadic(), this);
}

llvm::Type *FunctionDeclaration::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *FunctionDeclaration::llvm_load(Codegen &gen) {
    return llvm_callee();
}

llvm::Value *FunctionDeclaration::llvm_pointer(Codegen &gen) {
    return llvm_callee();
}

llvm::Value *CapturedVariable::llvm_load(Codegen &gen) {
    return gen.builder->CreateLoad(llvm_type(gen), llvm_pointer(gen));
}

llvm::Value *CapturedVariable::llvm_pointer(Codegen &gen) {
    auto captured = gen.current_function->getArg(lambda->data_struct_index());
    return gen.builder->CreateStructGEP(lambda->capture_struct_type(gen), captured, index);
}

llvm::Type *CapturedVariable::llvm_type(Codegen &gen) {
    if(capture_by_ref) {
        return gen.builder->getPtrTy();
    } else {
        return linked->llvm_type(gen);
    }
}

bool BaseFunctionParam::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return type->linked_node()->add_child_index(gen, indexes, name);
}

#endif

unsigned FunctionParam::calculate_c_or_llvm_index() {
    const auto start = func_type->c_or_llvm_arg_start_index();
    return start + index;
}

void FunctionParam::accept(Visitor *visitor) {
    visitor->visit(this);
}

ValueType BaseFunctionParam::value_type() const {
    return type->value_type();
}

BaseTypeKind BaseFunctionParam::type_kind() const {
    return type->kind();
}

FunctionParam *FunctionParam::copy(ASTAllocator& allocator) const {
    Value* copied = nullptr;
    if (defValue) {
        copied = defValue->copy(allocator);
    }
    return new (allocator.allocate<FunctionParam>()) FunctionParam(name, type->copy(allocator), index, copied, is_implicit, func_type, location);
}

bool FunctionParam::link_param_type(SymbolResolver &linker) {
    if(is_implicit) {
        if(name == "self" || name == "other") { // name and other means pointers to parent node
            if(!func_type->parent_node) {
                linker.error("couldn't get implicit self / other type", this);
                return false;
            }
            const auto ptr_type = ((ReferenceType*) type);
            const auto linked_type = ((LinkedType*) ptr_type->type);
            const auto parent_node = func_type->parent_node;
            const auto parent_kind = parent_node->kind();
            ASTNode* parent;
            if(parent_kind == ASTNodeKind::StructMember) {
                parent = parent_node->as_struct_member_unsafe()->parent_node;
            } else if(parent_kind == ASTNodeKind::ImplDecl) {
                parent = parent_node->as_impl_def_unsafe()->struct_type->linked_node();
            } else {
                parent = func_type->parent_node;
            }
            if(!parent) {
                linker.error("couldn't get self / other implicit parameter type", this);
                return false;
            }
            linked_type->linked = parent;
            return true;
        } else {
            auto found = linker.find(name);
            if(found) {
                const auto ptr_type = ((ReferenceType*) type);
                const auto linked_type = ((LinkedType*) ptr_type->type);
                const auto found_kind = found->kind();
                if(found_kind == ASTNodeKind::TypealiasStmt) {
                    const auto retrieved = ((TypealiasStatement*) found)->actual_type;
                    type = retrieved;
                    const auto direct = retrieved->get_direct_linked_node();
                    if(direct && ASTNode::isStoredStructType(direct->kind())) {
                        linker.error("struct like types must be passed as references using implicit parameters with typealias, please add '&' to make it a reference", this);
                        return false;
                    }
                } else {
                    linked_type->linked = found;
                }
            } else {
                linker.error("couldn't get implicit parameter type", this);
                return false;
            }
            return true;
        }
    } else {
        return type->link(linker);
    }
}

void FunctionParam::declare_and_link(SymbolResolver &linker) {
    linker.declare(name, this);
    if(defValue) {
        defValue->link(linker, defValue, type);
    }
}

BaseType* BaseFunctionParam::create_value_type(ASTAllocator& allocator) {
    return type->copy(allocator);
}

void BaseFunctionParam::declare_and_link(SymbolResolver &linker) {
    if(!name.empty()) {
        linker.declare(name, this);
    }
    type->link(linker);
}

void BaseFunctionParam::redeclare_top_level(SymbolResolver &linker) {
    if(!name.empty()) {
        linker.declare(name, this);
    }
}

ASTNode *BaseFunctionParam::child(const chem::string_view &name) {
    const auto linked_node = type->linked_node();
    return linked_node ? linked_node->child(name) : nullptr;
}

void GenericTypeParameter::declare_only(SymbolResolver& linker) {
    linker.declare(identifier, this);
}

void GenericTypeParameter::declare_and_link(SymbolResolver &linker) {
    if(at_least_type) {
        at_least_type->link(linker);
    }
    declare_only(linker);
    if(def_type) {
        def_type->link(linker);
    }
}

void GenericTypeParameter::register_usage(ASTAllocator& allocator, BaseType* type) {
    if(type) {
        usage.emplace_back(type->copy(allocator));
    } else {
        if(def_type) {
            usage.emplace_back(def_type->copy(allocator));
        } else {
            std::cerr << "expected a generic type argument for parameter " << identifier << " in node " << parent_node->ns_node_identifier() << std::endl;
        }
    }
}

FunctionDeclaration::FunctionDeclaration(
        LocatedIdentifier identifier,
        std::vector<FunctionParam*> params,
        BaseType* returnType,
        bool isVariadic,
        ASTNode* parent_node,
        SourceLocation location,
        std::optional<LoopScope> body,
        AccessSpecifier specifier,
        bool signature_resolved
) : FunctionType(std::move(params), returnType, isVariadic, false, parent_node, location, signature_resolved),
    identifier(std::move(identifier)), body(std::move(body)), location(location),
    attrs(specifier, false, 0, false, false, false, false, false, false, false, false,
          false, false
          ) {
}

std::string FunctionDeclaration::runtime_name_no_parent_fast_str() {
    std::stringstream stream;
    runtime_name_no_parent_fast(stream);
    return stream.str();
}

void FunctionDeclaration::runtime_name(std::ostream &stream) {
    if(parent_node) {
        const auto k = parent_node->kind();
        switch(k) {
            case ASTNodeKind::InterfaceDecl: {
                const auto interface = parent_node->as_interface_def_unsafe();
                ExtendableMembersContainerNode* container = (interface->active_user && has_self_param()) ? (ExtendableMembersContainerNode*) interface->active_user : interface;
                container->runtime_name(stream);
                break;
            }
            case ASTNodeKind::StructDecl:{
                const auto def = parent_node->as_struct_def_unsafe();
                const auto interface = def->get_overriding_interface(this);
                ExtendableMembersContainerNode* container = has_self_param() ? def : (interface ? (ExtendableMembersContainerNode*) interface : def);
                container->runtime_name(stream);
                break;
            }
            case ASTNodeKind::ImplDecl: {
                const auto def = parent_node->as_impl_def_unsafe();
                if(has_self_param() && def->struct_type) {
                    const auto struct_def = def->struct_type->linked_struct_def();
                    struct_def->runtime_name(stream);
                } else {
                    const auto& interface = def->interface_type->linked_interface_def();
                    interface->runtime_name(stream);
                }
                break;
            }
            default:
                parent_node->runtime_name(stream);
                break;
        }
    }
    runtime_name_no_parent_fast(stream);
}

void FunctionDeclaration::runtime_name_no_parent_fast(std::ostream& stream) {
    stream << name_view();
    if(multi_func_index() != 0) {
        stream << "__cmf_";
        stream << std::to_string(multi_func_index());
    }
    if(active_iteration != 0) {
        stream << "__cgf_";
        stream << std::to_string(active_iteration);
    }
}

int16_t FunctionDeclaration::total_generic_iterations() {
    return ::total_generic_iterations(generic_params);
}

void FunctionDeclaration::make_destructor(ASTAllocator& allocator, ExtendableMembersContainerNode* def) {
    if(!has_self_param() || params.size() > 1 || params.empty()) {
        params.clear();
        params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", new (allocator.allocate<ReferenceType>()) ReferenceType(new (allocator.allocate<LinkedType>()) LinkedType(def->name_view(), def, ZERO_LOC), ZERO_LOC), 0, nullptr, true, this, ZERO_LOC));
    }
    returnType = new (allocator.allocate<VoidType>()) VoidType(ZERO_LOC);
}

void check_returns_void(SymbolResolver& resolver, FunctionDeclaration* decl) {
    if(decl->returnType->kind() != BaseTypeKind::Void) {
        resolver.error(decl->name_str() + " function return type should be void", (ASTNode*) decl);
    }
}

void check_self_param(SymbolResolver& resolver, FunctionDeclaration* decl, ASTNode* self) {
    if(decl->params.size() == 1) {
        const auto param = decl->params.front();
        if(param->is_implicit && param->name == "self") {
            return;
        }
    }
    resolver.error(decl->name_str() + " must have a single implicit self reference parameter", (ASTNode*) decl);
}

void check_self_other_params(SymbolResolver& resolver, FunctionDeclaration* decl, ASTNode* self) {
    if(decl->params.size() == 2) {
        const auto param = decl->params.front();
        const auto second = decl->params[1];
        if(
            param->is_implicit && param->name == "self" &&
            second->is_implicit && second->name == "other"
        ) {
            return;
        }
    }
    resolver.error(decl->name_str() + " function must have two implicit reference parameters", (ASTNode*) decl);
}

void FunctionDeclaration::ensure_constructor(SymbolResolver& resolver, StructDefinition* def) {
    returnType = new ((*resolver.ast_allocator).allocate<LinkedType>()) LinkedType(def->name_view(), def, ZERO_LOC);
}

void FunctionDeclaration::ensure_destructor(SymbolResolver& resolver, ExtendableMembersContainerNode* def) {
    check_returns_void(resolver, this);
    check_self_param(resolver, this, def);
}

void FunctionDeclaration::ensure_clear_fn(SymbolResolver& resolver, ExtendableMembersContainerNode* def) {
    check_returns_void(resolver, this);
    check_self_param(resolver, this, def);
}

void FunctionDeclaration::ensure_copy_fn(SymbolResolver& resolver, ExtendableMembersContainerNode* def) {
    returnType = new ((*resolver.ast_allocator).allocate<LinkedType>()) LinkedType(def->name_view(), def, ZERO_LOC);
    check_self_other_params(resolver, this, def);
}

void FunctionDeclaration::ensure_move_fn(SymbolResolver& resolver, ExtendableMembersContainerNode* def) {
    returnType = new ((*resolver.ast_allocator).allocate<LinkedType>()) LinkedType(def->name_view(), def, ZERO_LOC);
    check_self_other_params(resolver, this, def);
}

void FunctionDeclaration::set_active_iteration(int16_t iteration) {
#ifdef DEBUG
    if(iteration < -1) {
        throw std::runtime_error("please fix iteration, which is less than -1, generic iteration is always greater than or equal to -1");
    }
#endif
    if(iteration == -1) {
        active_iteration = 0;
    } else {
        active_iteration = iteration;
    }
    for (auto &param: generic_params) {
        param->active_iteration = iteration;
    }
    for(auto sub : subscribers) {
        sub->set_parent_iteration(iteration);
    }
}

int16_t FunctionDeclaration::register_call_with_existing(ASTDiagnoser& diagnoser, FunctionCall* call, BaseType* expected_type) {
    const auto total = generic_params.size();
    std::vector<BaseType*> generic_args(total);
    ::infer_generic_args(generic_args, generic_params, call, diagnoser, expected_type);
    // register and report to subscribers
    auto itr = get_iteration_for(generic_params, generic_args);
    if(itr == -1) {
        diagnoser.error("generic iteration doesn't exist for call", (ASTNode*) this);
        return -1;
    }
    return -1;
}

int16_t FunctionDeclaration::register_call(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, FunctionCall* call, BaseType* expected_type) {
    const auto total = generic_params.size();
    std::vector<BaseType*> generic_args(total);
    infer_generic_args(generic_args, generic_params, call, diagnoser, expected_type);
    const auto itr = register_generic_usage(astAllocator, generic_params, generic_args);
    if(itr.second) {
        for (auto sub: subscribers) {
            sub->report_parent_usage(astAllocator, diagnoser, itr.first);
        }
    }
    return itr.first;
}

BaseType* FunctionDeclaration::create_value_type(ASTAllocator& allocator) {
    std::vector<FunctionParam*> copied;
    for(const auto param : params) {
        copied.emplace_back(param->copy(allocator));
    }
    return new (allocator.allocate<FunctionType>()) FunctionType(std::move(copied), returnType->copy(allocator), isVariadic(), false, parent_node, ZERO_LOC, FunctionType::data.signature_resolved);
}

//hybrid_ptr<BaseType> FunctionDeclaration::get_value_type() {
//    return hybrid_ptr<BaseType> { create_value_type(), true };
//}

void FunctionDeclaration::accept(Visitor *visitor) {
    visitor->visit(this);
}

void FunctionDeclaration::redeclare_top_level(SymbolResolver &linker) {
    linker.declare_function(name_view(), this);
}

void FunctionDeclaration::declare_top_level(SymbolResolver &linker) {
    /**
     * when a user has a call to function which is declared below current function, that function
     * has a parameter of type ref struct, the struct has implicit constructor for the value we are passing
     * we need to know the struct, we require the function's parameters to be linked, however that happens declare_and_link which happens
     * when function's body is linked, then an error happens, so we must link the types of parameters of all functions before linking bodies
     * in a single scope
     *
     * TODO However this requires that all the types used for parameters of functions must be declared above the function, because it will link
     *  in declaration stage, If the need arises that types need to be declared below a function, we should refactor this code,
     *
     * Here we are not declaring parameters, just declaring generic ones, we are linking parameters
     */

    bool resolved = true;
    linker.scope_start();
    for(auto gen_param : generic_params) {
        gen_param->declare_and_link(linker);
    }
    for(auto param : params) {
        if(!param->link_param_type(linker)) {
            resolved = false;
        }
    }
    if(!returnType->link(linker)) {
        resolved = false;
    }
    if(resolved) {
        FunctionType::data.signature_resolved = true;
    }
    linker.scope_end();
    linker.declare_function(name_view(), this, specifier());
}

bool FunctionDeclaration::ensure_has_init_block() {
    if(!body.has_value() || body->nodes.empty()) return false;
    auto& first = body->nodes.front();
    return first->isInitBlock();
}

void FunctionDeclaration::ensure_has_init_block(ASTDiagnoser& diagnoser) {
    if(!ensure_has_init_block()) {
        diagnoser.error("init block must be the first member of the constructor", (ASTNode*) this);
    }
}

void FunctionDeclaration::declare_and_link(SymbolResolver &linker) {
    // if has body declare params
    linker.scope_start();
    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = this;
    for(auto gen_param : generic_params) {
        gen_param->declare_only(linker);
    }
    for (auto param: params) {
        param->declare_and_link(linker);
    }
    if (body.has_value() && FunctionType::data.signature_resolved) {
        body->link_sequentially(linker);
    }
    linker.scope_end();
    linker.current_func_type = prev_func_type;
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    ASTAllocator& func_allocator,
    FunctionCall* call_obj,
    Value* parent,
    bool evaluate_refs
) {
    const auto global = call_scope->global;
    global->call_stack.emplace_back(call_obj);
    InterpretScope fn_scope(global, func_allocator, global);
    const auto value = call(call_scope, call_obj->values, parent, &fn_scope, evaluate_refs, call_obj);
    global->call_stack.pop_back();
    return value;
}

// called by the return statement
void FunctionDeclaration::set_return(InterpretScope& func_scope, Value *value) {
    if(value) {
        // TODO this can be improved
        // currently every return is first initialized in the current scope
        // then every return is copied to the call scope
        interpretReturn = value->evaluated_value(func_scope)->copy(callScope->allocator);
    }
    body->stopInterpretOnce();
}

FunctionDeclaration *FunctionDeclaration::as_function() {
    return this;
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    std::vector<Value*> &call_args,
    Value* parent,
    InterpretScope *fn_scope,
    bool evaluate_refs,
    ASTAny* debug_value
) {
    callScope = call_scope;
    auto& allocator = fn_scope->allocator;
    auto self_param = get_self_param();
    auto params_given = call_args.size() + (self_param ? parent ? 1 : 0 : 0);
    if (params.size() != params_given) {
        fn_scope->error("function " + name_str() + " requires " + std::to_string(params.size()) + ", but given args are " +
                        std::to_string(call_args.size()), debug_value);
        return nullptr;
    }
    if(self_param) {
        fn_scope->declare(self_param->name, parent);
    }
    auto i = self_param ? 1 : 0;
    while (i < params.size()) {
        Value* param_val;
        if(evaluate_refs) {
            param_val = call_args[i]->scope_value(*call_scope);
        } else {
            if(call_args[i]->reference()) {
                param_val = call_args[i]->copy(allocator);
            } else {
                param_val = call_args[i]->scope_value(*call_scope);
            }
        }
        fn_scope->declare(params[i]->name, param_val);
        i++;
    }
    body.value().interpret(*fn_scope);
    return interpretReturn;
}

BaseType* CapturedVariable::create_value_type(ASTAllocator& allocator) {
    if(capture_by_ref) {
        return new (allocator.allocate<PointerType>()) PointerType(linked->create_value_type(allocator), ZERO_LOC);
    } else {
        return linked->create_value_type(allocator);
    }
}

BaseType* CapturedVariable::known_type() {
    auto val_type = linked->known_type();
    if(capture_by_ref) {
        ptrType.type = val_type;
        return &ptrType;
    } else {
        return val_type;
    }
}

void CapturedVariable::declare_and_link(SymbolResolver &linker) {
    linked = linker.find(name);
    linker.declare(name, this);
}

BaseTypeKind CapturedVariable::type_kind() const {
    if(capture_by_ref) {
        return BaseTypeKind::Pointer;
    } else {
        return linked->type_kind();
    }
}

ValueType CapturedVariable::value_type() const {
    if(capture_by_ref) {
        return ValueType::Pointer;
    } else {
        return linked->value_type();
    }
}
