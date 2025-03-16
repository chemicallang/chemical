// Copyright (c) Chemical Language Foundation 2025.

#include <memory>

#include "FunctionParam.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/UnionDef.h"
#include "compiler/SymbolResolver.h"
#include "CapturedVariable.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/statements/VarInit.h"
#include "ast/values/CastedValue.h"
#include "ast/types/LinkedType.h"
#include "ast/structures/InitBlock.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/types/VoidType.h"
#include "ast/values/FunctionCall.h"
#include "ast/structures/GenericFuncDecl.h"
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

llvm::Type *FunctionParam::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Type *FunctionParam::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return type->llvm_chain_type(gen, values, index);
}

void FunctionParam::code_gen(Codegen &gen) {
    pointer = nullptr;
}

llvm::Value *FunctionParam::llvm_pointer(Codegen &gen) {
    if(pointer) {
        return pointer;
    }
    auto index = calculate_c_or_llvm_index(gen.current_func_type);
    if(index >= gen.current_function->arg_size()) {
        gen.error(this) << "couldn't get argument with name " << name << " since function has " << std::to_string(gen.current_function->arg_size()) << " arguments";
        return nullptr;
    }
    auto arg = gen.current_function->getArg(index);
    if (arg) {
        if(has_address_taken()) {
            const auto pure = type->pure_type(gen.allocator);
            if(pure->kind() == BaseTypeKind::IntN) {
                const auto allocaInstr = gen.builder->CreateAlloca(pure->llvm_type(gen));
                gen.di.instr(allocaInstr, this);
                pointer = allocaInstr;
                const auto storeInstr = gen.builder->CreateStore(arg, pointer);
                gen.di.instr(storeInstr, this);
                return pointer;
            }
        }
        return arg;
    } else {
        gen.error(this) << "couldn't get argument with name " << name;
        return nullptr;
    }
}

llvm::Value *FunctionParam::llvm_load(Codegen& gen, SourceLocation location) {
    if (gen.current_function != nullptr) {
        if(pointer) {
            const auto loadInstr = gen.builder->CreateLoad(type->llvm_type(gen), pointer);
            gen.di.instr(loadInstr, location);
            return loadInstr;
        } else {
            return llvm_pointer(gen);
        }
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

llvm::Function* FunctionDeclaration::known_func() {
    return llvm_data.empty() ? nullptr : llvm_data[0];
}

llvm::FunctionType *FunctionDeclaration::known_func_type() {
    const auto func = known_func();
    return func ? func->getFunctionType() : nullptr;
}

llvm::FunctionType *FunctionDeclaration::llvm_func_type(Codegen &gen) {
    const auto known = known_func_type();
    if(known) return known;
    return create_llvm_func_type(gen);
}

llvm::Function*& FunctionDeclaration::get_llvm_data() {
    if(llvm_data.empty() && is_override()) {
        const auto struct_def = parent()->as_struct_def();
        if(struct_def) {
            const auto overriding = struct_def->get_overriding_info(this);
            if(overriding.first) {
                const auto interface = overriding.first->as_interface_def();
                if(interface) {
                    auto& use = interface->users[struct_def];
                    const auto& found = use.find(overriding.second);
                    if(found != use.end()) {
                        llvm_data.emplace_back(found->second);
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
    return llvm_data[0];
}

void FunctionType::queue_destruct_params(Codegen& gen) {
    for(const auto param : params) {
        const auto directly_linked = param->type->get_direct_linked_container();
        if(directly_linked && directly_linked->destructor_func()) {
            llvm::Value* should_destruct = nullptr;
            if(param->get_has_move()) {

                // create a boolean flag
                const auto instr = gen.builder->CreateAlloca(gen.builder->getInt1Ty());
                gen.di.instr(instr, this);

                // store true in it, that this value should be destructed
                const auto storeIns = gen.builder->CreateStore(gen.builder->getInt1(true), instr);
                gen.di.instr(storeIns, this);

                should_destruct = instr;
            }
            gen.destruct_nodes.emplace_back(param, should_destruct);
        }
    }
}

void body_gen_no_scope(Codegen &gen, llvm::Function* funcCallee, Scope& body, FunctionDeclaration* func_type) {
    auto prev_func_type = gen.current_func_type;
    auto prev_func = gen.current_function;
    gen.current_func_type = func_type;
    gen.current_function = funcCallee;
    auto prev_destruct_nodes = std::move(gen.destruct_nodes);
    const auto destruct_begin = 0;
    gen.SetInsertPoint(&funcCallee->getEntryBlock());
    func_type->queue_destruct_params(gen);
    for(auto& param : func_type->params) {
        param->code_gen(gen);
    }
    // It's very important that we clear the evaluated function calls, before generating body
    // because different bodies of generic functions must reevaluate comptime functions
    gen.evaluated_func_calls.clear();
    body.code_gen(gen, destruct_begin);
    // TODO send the ending location of the body
    gen.end_function_block(func_type->body_location());
    gen.destruct_nodes = std::move(prev_destruct_nodes);
    gen.current_function = nullptr;
    gen.current_func_type = prev_func_type;
}

inline void body_gen_no_scope(Codegen &gen, FunctionDeclaration* decl, llvm::Function* funcCallee) {
    if(decl->body.has_value()) {
        body_gen_no_scope(gen, funcCallee, decl->body.value(), decl);
    }
}

void body_gen(Codegen &gen, FunctionDeclaration* decl, llvm::Function* funcCallee) {
    if(decl->body.has_value()) {
        // we start a di subprogram as current scope
        gen.di.start_function_scope(decl, funcCallee);
        // and then generate the actual body
        body_gen_no_scope(gen, funcCallee, decl->body.value(), decl);
        // we end the di subprogram as current scope, that we started above
        gen.di.end_function_scope();
    }
}

void func_body_gen_no_scope(FunctionDeclaration* decl, Codegen& gen) {
    if(!decl->exists_at_runtime()) {
        return;
    }
    body_gen_no_scope(gen, decl, decl->llvm_func());
}

void FunctionDeclaration::code_gen_body(Codegen &gen) {
    if(!exists_at_runtime()) {
        return;
    }
    body_gen(gen, this, llvm_func());
}

void FunctionDeclaration::code_gen(Codegen &gen) {
    if (!exists_at_runtime()) {
        return;
    }
    if (parent()) {
        auto k = parent()->kind();
        switch (k) {
            case ASTNodeKind::StructDecl: {
                const auto parent_decl = parent()->as_struct_def_unsafe();
                parent_decl->code_gen_function_body(gen, this);
                return;
            }
            case ASTNodeKind::VariantDecl: {
                const auto parent_decl = parent()->as_variant_def_unsafe();
                parent_decl->code_gen_function_body(gen, this);
                return;
            }
            case ASTNodeKind::UnionDecl: {
                const auto parent_decl = parent()->as_union_def_unsafe();
                parent_decl->code_gen_function_body(gen, this);
                return;
            }
            case ASTNodeKind::InterfaceDecl: {
                const auto parent_decl = parent()->as_interface_def_unsafe();
                parent_decl->code_gen_function_body(gen, this);
                return;
            }
            case ASTNodeKind::ImplDecl: {
                const auto parent_decl = parent()->as_impl_def_unsafe();
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

void FunctionDeclaration::set_llvm_data(llvm::Function* func) {
#ifdef DEBUG
    if(0 > (int) llvm_data.size()) {
        throw std::runtime_error("decl's generic active iteration is greater than total llvm_data size");
    }
#endif
    if(llvm_data.empty()) {
        llvm_data.emplace_back(func);
    } else {
        llvm_data[0] = func;
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
    if(existing_func && !existing_func->isDeclaration()) {
        gen.error((ASTNode*) decl) << "function with name '" << name << "' already exists in the module";
    }
#endif
    auto func_type = decl->create_llvm_func_type(gen);
    auto func = gen.create_function(name, func_type, decl, decl->specifier());
    decl->llvm_attributes(func);
    decl->set_llvm_data(func);
}

void create_fn(Codegen& gen, FunctionDeclaration *decl) {     // non generic functions always have generic iteration equal to zero
    create_non_generic_fn(gen, decl, decl->runtime_name_fast(gen));
}

void declare_non_gen_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name) {
    auto callee = gen.declare_function(name, decl->create_llvm_func_type(gen), decl, decl->specifier());
    decl->set_llvm_data(callee);
}

void declare_non_gen_weak_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name, bool is_exported) {
    auto callee = gen.declare_weak_function(name, decl->create_llvm_func_type(gen), decl, is_exported, decl->ASTNode::encoded_location());
    decl->set_llvm_data(callee);
}

void declare_fn_weak(Codegen& gen, FunctionDeclaration *decl, bool is_exported) {
    declare_non_gen_weak_fn(gen, decl, decl->runtime_name_fast(gen), is_exported);
}

void declare_fn(Codegen& gen, FunctionDeclaration *decl) {
    declare_non_gen_fn(gen, decl, decl->runtime_name_fast(gen));
}

void create_or_declare_fn(Codegen& gen, FunctionDeclaration* decl) {
    if (decl->body.has_value()) {
        create_fn(gen, decl);
    } else {
        declare_fn(gen, decl);
    }
}

void FunctionDeclaration::code_gen_declare_normal(Codegen& gen) {
    if(!exists_at_runtime()) {
        return;
    }
    create_or_declare_fn(gen, this);
    gen.current_function = nullptr;
}

void FunctionDeclaration::code_gen_declare(Codegen &gen) {
    if(!exists_at_runtime()) {
        return;
    }
    if(parent()) {
        auto k = parent()->kind();
        switch(k) {
            case ASTNodeKind::StructDecl:{
                const auto parent_decl = parent()->as_struct_def_unsafe();
                parent_decl->code_gen_function_declare(gen, this);
                return;
            }
            case ASTNodeKind::VariantDecl: {
                const auto parent_decl = parent()->as_variant_def_unsafe();
                parent_decl->code_gen_function_declare(gen, this);
                return;
            }
            case ASTNodeKind::UnionDecl: {
                const auto parent_decl = parent()->as_union_def_unsafe();
                parent_decl->code_gen_function_declare(gen, this);
                return;
            }
            case ASTNodeKind::InterfaceDecl: {
                const auto parent_decl = parent()->as_interface_def_unsafe();
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
    if(!is_exported()) {
        // TODO this should not happen, we should not even include nodes
        // we should remove the nodes if they are not public, so we never have to call code_gen_external_declare
        // function wasn't exported
        return;
    }
    declare_non_gen_fn(gen, this, runtime_name_fast());
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, StructDefinition* def) {
    if(!exists_at_runtime()) {
        return;
    }
    create_or_declare_fn(gen, this);
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, VariantDefinition* def) {
    if(!exists_at_runtime()) {
        return;
    }
    create_or_declare_fn(gen, this);
}

void FunctionDeclaration::code_gen_override_declare(Codegen &gen, FunctionDeclaration* decl) {
    set_llvm_data(decl->llvm_func());
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, InterfaceDefinition* def) {
    if(def->is_static()) {
        declare_fn_weak(gen, this, def->specifier() == AccessSpecifier::Public);
    } else {
        create_fn(gen, this);
    }
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, UnionDef* def) {
    if(!exists_at_runtime()) {
        return;
    }
    create_fn(gen, this);
}

void FunctionDeclaration::code_gen_body(Codegen &gen, StructDefinition* def) {
    if(!exists_at_runtime()) {
        return;
    }
    if(is_constructor_fn()) {
        code_gen_constructor(gen, def);
        return;
    }
    if(is_copy_fn()) {
        code_gen_copy_fn(gen, def);
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
    if(!exists_at_runtime()) {
        return;
    }
    if(is_copy_fn()) {
        code_gen_copy_fn(gen, def);
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
    if(!exists_at_runtime()) {
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
        func_body_gen_no_scope(this, gen);
        gen.CreateBr(cleanup_block, ASTNode::encoded_location()); // ensure branch to cleanup block
        gen.SetInsertPoint(cleanup_block);
    } else {
        gen.SetInsertPoint(&func->getEntryBlock());
    }
}

void create_call_member_func(
        Codegen& gen,
        FunctionDeclaration* decl,
        StructDefinition* def,
        llvm::Function* func,
        unsigned index,
        bool force_pass_self,
        SourceLocation location
) {
    auto arg = func->getArg(0);
    std::vector<llvm::Value *> idxList{gen.builder->getInt32(0)};
    std::vector<llvm::Value*> args;
    auto element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), arg, idxList, index);
    if(force_pass_self || decl->has_self_param()) {
        args.emplace_back(element_ptr);
    }
    const auto callInst = gen.builder->CreateCall(decl->llvm_func_type(gen), decl->llvm_pointer(gen), args);
    gen.di.instr(callInst, location);
}

void code_gen_process_members(
        Codegen& gen,
        StructDefinition* def,
        llvm::Function* func,
        SourceLocation location,
        void(*member_func_call)(Codegen& gen, MembersContainer* member_container, StructDefinition* def, llvm::Function* func, unsigned index, SourceLocation location)
) {
    unsigned index = 0;
    for(auto& inherited : def->inherited) {
        const auto base_def = inherited.type->get_direct_linked_struct();
        if(base_def) {
            member_func_call(gen, base_def, def, func, index, location);
        }
        index++;
    }
    for(const auto var : def->variables()) {
        auto mem_type = var->known_type();
        if(mem_type->isStructLikeType()) {
            auto mem_def = mem_type->linked_node()->as_members_container();
            if(!mem_def) {
                index++;
                continue;
            }
            member_func_call(gen, mem_def, def, func, index, location);
        }
        index++;
    }
}

void FunctionDeclaration::code_gen_copy_fn(Codegen& gen, StructDefinition* def) {
    auto func = llvm_func();
    gen.SetInsertPoint(&func->getEntryBlock());

    // start the function scope
    gen.di.start_function_scope(this, func);

    // copy calls to members
    code_gen_process_members(gen, def, func, body_location(), [](Codegen& gen, MembersContainer* mem_def, StructDefinition* def, llvm::Function* func, unsigned index, SourceLocation location) {
        const auto decl = mem_def->copy_func();
        if(!decl) {
            return;
        }
        auto selfArg = func->getArg(0);
        auto otherArg = func->getArg(1);
        std::vector<llvm::Value*> args;
        std::vector<llvm::Value *> idxList{gen.builder->getInt32(0)};
        auto element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), selfArg, idxList, index);
        args.emplace_back(element_ptr);
        idxList.pop_back();
        auto other_element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), otherArg, idxList, index);
        args.emplace_back(other_element_ptr);
        const auto callInstr = gen.builder->CreateCall(decl->llvm_func_type(gen), decl->llvm_pointer(gen), args);
        gen.di.instr(callInstr, location);
    });

    // generate user body
    func_body_gen_no_scope(this, gen);

    // end the function scope
    gen.di.end_function_scope();

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
        SourceLocation location,
        bool(*should_process)(VariantMember* member),
        void(*process)(Codegen &gen, VariantMember* member, llvm::Value* struct_ptr, llvm::Function* func, SourceLocation location)
) {
    // get the type int
    auto gep = gen.builder->CreateGEP(def->llvm_type(gen), allocaInst, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    const auto type_value = gen.builder->CreateLoad(gen.builder->getInt32Ty(), gep);
    gen.di.instr(type_value, location);

    // create an end block, for default case
    llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*gen.ctx, "", func);

    // figure out which members to process
    std::vector<std::pair<int, VariantMember*>> to_process;
    int index = 0;
    for(const auto mem : def->variables()) {
        const auto variant_mem = mem->as_variant_member();
        if(should_process(variant_mem)) {
            to_process.emplace_back(index, variant_mem);
        }
        index++;
    }

    // get struct pointer
    auto struct_ptr = variant_struct_pointer(gen, allocaInst, def);

    // create switch block on type int
    const auto switchInst = gen.builder->CreateSwitch(type_value, end_block, to_process.size());
    gen.di.instr(switchInst, location);

    // create blocks for cases for which process exists
    for(auto& mem : to_process) {
        auto mem_block = llvm::BasicBlock::Create(*gen.ctx, "", func);
        gen.SetInsertPoint(mem_block);
        process(gen, mem.second, struct_ptr, func, location);
        gen.CreateBr(end_block, location);
        switchInst->addCase(gen.builder->getInt32(mem.first), mem_block);
    }

    gen.SetInsertPoint(end_block);
}

void FunctionDeclaration::code_gen_copy_fn(Codegen& gen, VariantDefinition* def) {

    auto func = llvm_func();
    gen.SetInsertPoint(&func->getEntryBlock());

    // start the function scope
    gen.di.start_function_scope(this, func);

    // get args
    auto allocaInst = func->getArg(0);
    auto otherInst = func->getArg(1);

    // storing the type integer
    auto other_type = gen.builder->CreateGEP(def->llvm_type(gen), otherInst, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    auto this_type = gen.builder->CreateGEP(def->llvm_type(gen), allocaInst, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);

    const auto loaded = gen.builder->CreateLoad(gen.builder->getInt32Ty(), other_type);
    gen.di.instr(loaded, body_location());

    const auto storeInst = gen.builder->CreateStore(loaded, this_type);
    gen.di.instr(storeInst, body_location());

    // processing members to call copy functions on members
    process_members_calling_fns(gen, def, allocaInst, func, body_location(), [](VariantMember* mem)-> bool {
        return mem->requires_copy_fn();
    }, [](Codegen& gen, VariantMember* mem, llvm::Value* struct_ptr, llvm::Function* func, SourceLocation location) {
        auto otherArg = func->getArg(1);
        auto other_struct_pointer = variant_struct_pointer(gen, otherArg, mem->parent());
        auto index = -1;
        for(auto& param : mem->values) {
            index++;
            auto linked = param.second->type->linked_node();
            if(linked) {
                auto container = linked->as_members_container();
                if(container) {
                    auto def = mem->parent();
                    auto decl = container->copy_func();
                    std::vector<llvm::Value*> args;
                    std::vector<llvm::Value *> idxList{gen.builder->getInt32(0)};
                    auto element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), struct_ptr, idxList, index);
                    args.emplace_back(element_ptr);
                    idxList.pop_back();
                    auto other_element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), other_struct_pointer, idxList, index);
                    args.emplace_back(other_element_ptr);
                    const auto callInst = gen.builder->CreateCall(decl->llvm_func_type(gen), decl->llvm_pointer(gen), args);
                    gen.di.instr(callInst, location);
                }
            }
        }
    });

    const auto retInstr = gen.builder->CreateRetVoid();
    gen.di.instr(retInstr, body_location());

    // generate the body
    if(body.has_value() && !body->nodes.empty()) {
        func_body_gen_no_scope(this, gen);
    }

    // end the function scope
    gen.di.end_function_scope();

}

void initialize_def_struct_values(
    Codegen &gen,
    StructDefinition* struct_def,
    FunctionDeclaration* decl,
    llvm::Function* func
) {
    std::unordered_map<chem::string_view, InitBlockInitializerValue>* initializers = nullptr;
    if(decl->body.has_value() && !decl->body->nodes.empty()) {
        auto block = decl->body->nodes.front()->as_init_block();
        if(block) {
            initializers = &block->initializers;
        }
    }
    auto self_arg = func->getArg(0);
    auto parent_type = struct_def->llvm_type(gen);
    for(const auto var : struct_def->variables()) {
        const auto defValue = var->default_value();
        if(!defValue) continue;
        auto has_not_been_initialized = !initializers || initializers->find(var->name) == initializers->end();
        if(has_not_been_initialized) {
            // couldn't move struct
            auto variable = struct_def->variable_type_index(var->name, false);
            std::vector<llvm::Value*> idx { gen.builder->getInt32(0) };
            defValue->store_in_struct(gen, nullptr, self_arg, parent_type, idx, variable.first, variable.second);
        }
    }
}

void FunctionDeclaration::code_gen_constructor(Codegen& gen, StructDefinition* def) {

    auto func = llvm_func();
    gen.SetInsertPoint(&func->getEntryBlock());

    // start the function scope
    gen.di.start_function_scope(this, func);

    // initialize default struct values in the constructor
    initialize_def_struct_values(gen, def, this, func);

    // call constructors of members
    code_gen_process_members(gen, def, func, body_location(), [](Codegen& gen, MembersContainer* mem_def, StructDefinition* def, llvm::Function* func, unsigned index, SourceLocation location) {
        const auto decl = mem_def->default_constructor_func();
        if(!decl) {
            return;
        }
        create_call_member_func(gen, decl, def, func, index, true, location);
    });

    // generate function body
    if(body.has_value() && !body->nodes.empty()) {
        func_body_gen_no_scope(this, gen);
    }

    // TODO use the ending location
    gen.CreateRet(nullptr, body_location());

    // end the function scope
    gen.di.end_function_scope();

}

void FunctionDeclaration::code_gen_destructor(Codegen& gen, StructDefinition* def) {

    auto func = llvm_func();

    // we start a function scope
    gen.di.start_function_scope(this, func);

    // sets up the cleanup along with generating the body user provided
    setup_cleanup_block(gen, func);

    code_gen_process_members(gen, def, func, body_location(), [](Codegen& gen, MembersContainer* mem_def, StructDefinition* def, llvm::Function* func, unsigned index, SourceLocation location) {
        const auto decl = mem_def->destructor_func();
        if(!decl) {
            return;
        }
        create_call_member_func(gen, decl, def, func, index, false, location);
    });

    // TODO use the ending location
    gen.CreateRet(nullptr, body_location());

    // end the function scope we started
    gen.di.end_function_scope();

    gen.redirect_return = nullptr;
}

void FunctionDeclaration::code_gen_destructor(Codegen& gen, VariantDefinition* def) {
    auto func = llvm_func();

    // start the function scope
    gen.di.start_function_scope(this, func);

    // sets up the cleanup along with generating the body user provided
    setup_cleanup_block(gen, func);

    // every destructor has self at zero
    auto allocaInst = func->getArg(0);
    process_members_calling_fns(gen, def, allocaInst, func, body_location(), [](VariantMember* mem) -> bool {
        return mem->requires_destructor();
    }, [](Codegen& gen, VariantMember* mem, llvm::Value* struct_ptr, llvm::Function* func, SourceLocation location) {
        llvm::Function* dtr_func_data = nullptr;
        int i = 0;
        for(auto& value : mem->values) {
            auto ref_node = value.second->type->get_direct_linked_node();
            if(ref_node) { // <-- the node is directly referenced
                auto destructorFunc = gen.determine_destructor_for(value.second->type, dtr_func_data);
                if(destructorFunc) {
                    std::vector<llvm::Value*> args;
                    if(destructorFunc->has_self_param()) {
                        auto gep3 = gen.builder->CreateGEP(mem->llvm_type(gen), struct_ptr, { gen.builder->getInt32(0), gen.builder->getInt32(i) }, "", gen.inbounds);
                        args.emplace_back(gep3);
                    }
                    const auto callInst = gen.builder->CreateCall(dtr_func_data, args, "");
                    gen.di.instr(callInst, location);
                }
            }
            i++;
        }
    });

    // TODO use the ending location
    gen.CreateRet(nullptr, body_location());

    // end the function scope we started
    gen.di.end_function_scope();

    gen.redirect_return = nullptr;
}

std::vector<llvm::Type *> FunctionDeclaration::param_types(Codegen &gen) {
    return llvm_func_param_types(gen, params, returnType, false, isVariadic(), this);
}

llvm::Type *FunctionDeclaration::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *CapturedVariable::llvm_load(Codegen& gen, SourceLocation location) {
    const auto loadInst = gen.builder->CreateLoad(llvm_type(gen), llvm_pointer(gen));
    gen.di.instr(loadInst, location);
    return loadInst;
}

llvm::Value *CapturedVariable::llvm_pointer(Codegen &gen) {
    const auto lambda = gen.current_func_type->as_lambda();
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

bool FunctionParam::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return type->linked_node()->add_child_index(gen, indexes, name);
}

#endif

unsigned FunctionParam::calculate_c_or_llvm_index(FunctionType* func_type) {
    const auto start = func_type->c_or_llvm_arg_start_index() - (func_type->isExtensionFn() ? 1 : 0);
    return start + index;
}

FunctionParam *FunctionParam::copy(ASTAllocator& allocator) const {
    const auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(
            name,
            type->copy(allocator),
            index,
            defValue ? defValue->copy(allocator) : nullptr,
            is_implicit(),
            parent(),
            encoded_location()
    );
    param->attrs = attrs;
    return param;
}

bool FunctionParam::link_param_type(SymbolResolver &linker) {
    if(is_implicit()) {
        if(name == "self" || name == "other") { // name and other means pointers to parent node
            const auto ptr_type = ((ReferenceType*) type);
            auto& linked_type_ref = ptr_type->type;
            const auto linked_type = ((LinkedType*) linked_type_ref);
            auto parent_node = parent();
            auto parent_kind = parent_node->kind();
            ASTNode* parent;
            if(parent_kind == ASTNodeKind::FunctionDecl || parent_kind == ASTNodeKind::StructMember) {
                const auto p = parent_node->parent();
                if(p) {
                    parent_node = p;
                    parent_kind = p->kind();
                }
            }
            switch(parent_kind) {
                case ASTNodeKind::ImplDecl:
                    parent = parent_node->as_impl_def_unsafe()->struct_type->linked_node();
                    break;
                case ASTNodeKind::StructDecl: {
                    const auto def = parent_node->as_struct_def_unsafe();
                    parent = def->generic_parent ? (ASTNode*) def->generic_parent : parent_node;
                    break;
                }
                case ASTNodeKind::VariantDecl:
                case ASTNodeKind::UnionDecl:
                case ASTNodeKind::InterfaceDecl:
                    parent = parent_node;
                    break;
                default:
                    parent = nullptr;
                    break;
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

void FunctionParam::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare(name, this);
    if(defValue) {
        defValue->link(linker, defValue, type);
    }
}

ASTNode *FunctionParam::child(const chem::string_view &name) {
    const auto linked_node = type->linked_node();
    return linked_node ? linked_node->child(name) : nullptr;
}

void GenericTypeParameter::declare_only(SymbolResolver& linker) {
    linker.declare(identifier, this);
}

void GenericTypeParameter::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
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
            std::cerr << "expected a generic type argument for parameter " << identifier << " in node " << parent()->get_located_id()->identifier << std::endl;
        }
    }
}

std::string FunctionDeclaration::runtime_name_no_parent_fast_str() {
    std::stringstream stream;
    runtime_name_no_parent_fast(stream);
    return stream.str();
}

void FunctionDeclaration::runtime_name(std::ostream &stream) {
    if(parent()) {
        const auto k = parent()->kind();
        switch(k) {
            case ASTNodeKind::InterfaceDecl: {
                const auto interface = parent()->as_interface_def_unsafe();
                if(interface->is_static()) {
                    interface->runtime_name(stream);
                } else {
                    ExtendableMembersContainerNode* container = (interface->active_user && has_self_param()) ? (ExtendableMembersContainerNode*) interface->active_user : interface;
                    container->runtime_name(stream);
                };
                break;
            }
            case ASTNodeKind::StructDecl:{
                const auto def = parent()->as_struct_def_unsafe();
                const auto interface = def->get_overriding_interface(this);
                if(interface && interface->is_static()) {
                    interface->runtime_name(stream);
                } else {
                    ExtendableMembersContainerNode* container = has_self_param() ? def : (interface ? (ExtendableMembersContainerNode*) interface : def);
                    container->runtime_name(stream);
                }
                break;
            }
            case ASTNodeKind::ImplDecl: {
                const auto def = parent()->as_impl_def_unsafe();
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
                parent()->runtime_name(stream);
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
    if(generic_instantiation != -1) {
        stream << "__cfg_";
        stream << generic_instantiation;
    }
}

void FunctionDeclaration::make_destructor(ASTAllocator& allocator, ExtendableMembersContainerNode* def) {
    if(!has_self_param() || params.size() > 1 || params.empty()) {
        params.clear();
        params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", new (allocator.allocate<ReferenceType>()) ReferenceType(new (allocator.allocate<LinkedType>()) LinkedType(def->name_view(), def, ZERO_LOC), ZERO_LOC), 0, nullptr, true, this, ZERO_LOC));
    }
    returnType = new (allocator.allocate<VoidType>()) VoidType(ZERO_LOC);
}

void check_returns_void(ASTDiagnoser& diagnoser, FunctionDeclaration* decl) {
    if(decl->returnType->kind() != BaseTypeKind::Void) {
        diagnoser.error((ASTNode*) decl) << decl->name_view() << " function return type should be void";
    }
}

void check_self_param(ASTDiagnoser& diagnoser, FunctionDeclaration* decl, ASTNode* self) {
    if(decl->params.size() == 1) {
        const auto param = decl->params.front();
        if(param->is_implicit() && param->name == "self") {
            return;
        }
    }
    diagnoser.error((ASTNode*) decl) << decl->name_view() << " must have a single implicit self reference parameter";
}

void check_self_other_params(ASTDiagnoser& diagnoser, FunctionDeclaration* decl, ASTNode* self) {
    if(decl->params.size() == 2) {
        const auto param = decl->params.front();
        const auto second = decl->params[1];
        if(
            param->is_implicit() && param->name == "self" &&
            second->is_implicit() && second->name == "other"
        ) {
            return;
        }
    }
    diagnoser.error((ASTNode*) decl) << decl->name_view() << " function must have two implicit reference parameters";
}

void FunctionDeclaration::ensure_constructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser, StructDefinition* def) {
    returnType = new (allocator.allocate<LinkedType>()) LinkedType(def->name_view(), def, ZERO_LOC);
}

void FunctionDeclaration::ensure_destructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ExtendableMembersContainerNode* def) {
    check_returns_void(diagnoser, this);
    check_self_param(diagnoser, this, def);
}

void FunctionDeclaration::ensure_clear_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ExtendableMembersContainerNode* def) {
    check_returns_void(diagnoser, this);
    check_self_param(diagnoser, this, def);
}

void FunctionDeclaration::ensure_copy_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ExtendableMembersContainerNode* def) {
    returnType = new (allocator.allocate<LinkedType>()) LinkedType(def->name_view(), def, ZERO_LOC);
    check_self_other_params(diagnoser, this, def);
}

void FunctionDeclaration::ensure_move_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ExtendableMembersContainerNode* def) {
    returnType = new (allocator.allocate<LinkedType>()) LinkedType(def->name_view(), def, ZERO_LOC);
    check_self_other_params(diagnoser, this, def);
}

void FunctionDeclaration::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    // extension functions do not declare themselves
    if(!isExtensionFn()) {
        linker.declare_function(name_view(), this, specifier());
    }
}

bool FunctionDeclaration::put_as_extension_function(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {
    // extension functions declare themselves inside the container
    auto& receiver = *params[0];
    const auto type = receiver.type;
    const auto pure_receiver = type->pure_type(allocator);
    const auto receiver_kind = pure_receiver->kind();
    if (receiver_kind != BaseTypeKind::Reference) {
        diagnoser.error("receiver in extension function must always be a reference", type);
    }
    auto linked = type->linked_node();
    if (!linked) {
        diagnoser.error((AnnotableNode*) this) << "couldn't find container in extension function ith receiver type \"" << type->representation() << "\"";
        return false;
    }
    const auto linked_kind = linked->kind();
    auto container = linked->as_extendable_members_container();
    if (linked_kind == ASTNodeKind::InterfaceDecl) {
        const auto interface = linked->as_interface_def_unsafe();
        if (!interface->is_static() && generic_parent == nullptr) {
            diagnoser.error("extension functions are only supported on static interfaces, either make the interface static or move the function inside the interface", receiver.type);
            return false;
        }
    } else if(linked_kind == ASTNodeKind::GenericTypeParam) {
        const auto param = linked->as_generic_type_param_unsafe();
        if(param->at_least_type) {
            const auto at_least_linked = param->at_least_type->get_direct_linked_node();
            if(at_least_linked) {
                container = at_least_linked->as_extendable_members_container_node();
            }
        }
    } else if(linked_kind == ASTNodeKind::GenericStructDecl) {
        container = linked->as_gen_struct_def_unsafe()->master_impl;
    } else if(linked_kind == ASTNodeKind::GenericInterfaceDecl) {
        container = linked->as_gen_interface_decl_unsafe()->master_impl;
    } else if(linked_kind == ASTNodeKind::GenericUnionDecl) {
        container = linked->as_gen_union_decl_unsafe()->master_impl;
    } else if(linked_kind == ASTNodeKind::GenericVariantDecl) {
        container = linked->as_gen_variant_decl_unsafe()->master_impl;
    }
    if (!container) {
        diagnoser.error(receiver.type) << "type doesn't support extension functions " << type->representation();
        return false;
    }
    if(container->extension_functions.contains(name_view())) {
        diagnoser.error(receiver.type) << "container already contains an extension function by the same name";
    } else {
        const auto field_func = linked->child(name_view());
        if (field_func != nullptr && field_func != generic_parent) {
            diagnoser.error(receiver.type) << "couldn't declare extension function with name '" << name_view() << "' because type '" << receiver.type->representation() << "' already has a field / function with same name \n";
            return false;
        }
        if(generic_parent) {
            container->add_extension_func(name_view(), generic_parent);
        } else {
            container->add_extension_func(name_view(), this);
        }
        return true;
    }
}

void FunctionDeclaration::link_signature_no_ext_scope(SymbolResolver &linker) {
    bool resolved = true;
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
}

void FunctionDeclaration::link_signature_no_scope(SymbolResolver& linker) {
    link_signature_no_ext_scope(linker);
    if(isExtensionFn()) {
        put_as_extension_function(linker.allocator, linker);
    }
}

void FunctionDeclaration::link_signature(SymbolResolver &linker)  {
    linker.scope_start();
    link_signature_no_ext_scope(linker);
    if(isExtensionFn()) {
        put_as_extension_function(linker.allocator, linker);
    }
    linker.scope_end();
}

bool FunctionDeclaration::ensure_has_init_block() {
    if(!body.has_value() || body->nodes.empty()) return false;
    auto& first = body->nodes.front();
    return ASTNode::isInitBlock(first->kind());
}

void FunctionDeclaration::ensure_has_init_block(ASTDiagnoser& diagnoser) {
    if(!ensure_has_init_block()) {
        diagnoser.error("init block must be the first member of the constructor", (ASTNode*) this);
    }
}

void FunctionDeclaration::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    // if has body declare params
    linker.scope_start();
    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = this;
    for (auto& param : params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    if(body.has_value()) {
        if(FunctionType::data.signature_resolved) {
            if(is_comptime()) {
                linker.comptime_context = true;
            }
            body->link_sequentially(linker);
            linker.comptime_context = false;
        } else {
            linker.warn("couldn't resolve signature of function", (ASTNode*) this);
        }
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
    const auto prev_func = global->current_func_type;
    global->current_func_type = this;
    global->call_stack.emplace_back(call_obj);
    InterpretScope fn_scope(global, func_allocator, global);
    const auto value = call(call_scope, call_obj->values, parent, &fn_scope, evaluate_refs, call_obj);
    global->call_stack.pop_back();
    global->current_func_type = prev_func;
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

BaseType* CapturedVariable::known_type() {
    auto val_type = linked->known_type();
    if(capture_by_ref) {
        ptrType.type = val_type;
        return &ptrType;
    } else {
        return val_type;
    }
}

bool CapturedVariable::declare_and_link(SymbolResolver& linker) {
    const auto found = linker.find(name);
    const auto has = found != nullptr;
    if(has) {
        linked = found;
    }
    linker.declare(name, this);
    return has;
}
