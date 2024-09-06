// Copyright (c) Qinetik 2024.

#include <memory>

#include "FunctionParam.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "compiler/SymbolResolver.h"
#include "CapturedVariable.h"
#include "ast/types/PointerType.h"
#include "ast/statements/VarInit.h"
#include "ast/values/CastedValue.h"
#include "ast/types/ReferencedType.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/types/VoidType.h"
#include "ast/values/FunctionCall.h"
#include "ast/statements/Return.h"
#include "ast/utils/GenericUtils.h"
#include "ast/types/GenericType.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/values/LambdaFunction.h"

llvm::Type *BaseFunctionParam::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Type *BaseFunctionParam::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) {
    return type->llvm_chain_type(gen, values, index);
}

llvm::FunctionType *BaseFunctionParam::llvm_func_type(Codegen &gen) {
    return type->llvm_func_type(gen);
}

void BaseFunctionParam::code_gen_destruct(Codegen &gen, Value *returnValue) {
    if(!(returnValue && returnValue->as_identifier() && returnValue->linked_node() == this)) {
        type->linked_node()->llvm_destruct(gen, gen.current_function->getArg(calculate_c_or_llvm_index()));
    }
}

llvm::Type *BaseFunctionParam::llvm_elem_type(Codegen &gen) {
    auto lType = llvm_type(gen);
    if (lType) {
        if (lType->isArrayTy()) {
            return lType->getArrayElementType();
        } else if (lType->isPointerTy()) {
            auto ptr_type = type->pointer_type();
            if (ptr_type) {
                return ptr_type->type->llvm_type(gen);
            } else {
                gen.error("type is not a pointer type for parameter " + name, this);
            }
        } else {
            gen.error("type is not an array / pointer for parameter " + name, this);
        }
    } else {
        gen.error("parameter type is invalid " + name, this);
    }
    return nullptr;
}

llvm::Value *BaseFunctionParam::llvm_pointer(Codegen &gen) {
    auto index = calculate_c_or_llvm_index();
    if(index > gen.current_function->arg_size()) {
        gen.error("couldn't get argument with name " + name + " since function has " + std::to_string(gen.current_function->arg_size()) + " arguments", this);
        return nullptr;
    }
    auto arg = gen.current_function->getArg(index);
    if (arg) {
        return arg;
    } else {
        gen.error("couldn't get argument with name " + name, this);
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
        return llvm::FunctionType::get(llvm_func_return(gen, returnType.get()), isVariadic);
    } else {
        return llvm::FunctionType::get(llvm_func_return(gen, returnType.get()), paramTypes, isVariadic);
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

llvm::Value* FunctionDeclaration::llvm_callee() {
    if(active_iteration == llvm_data.size() && has_annotation(AnnotationKind::Override)) {
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
                        return found->second;
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
    return llvm_data[active_iteration].first;
}

llvm::Function* FunctionDeclaration::llvm_func() {
    return (llvm::Function*)  llvm_callee();
}

void FunctionType::queue_destruct_params(Codegen& gen) {
    for(auto& param : params) {
        const auto k = param->type->kind();
        if(k == BaseTypeKind::Referenced || k == BaseTypeKind::Generic) {
            const auto def = param->type->linked_node();
            if(def) {
                const auto members_container = def->as_members_container();
                if(members_container && members_container->destructor_func()) {
                    gen.destruct_nodes.emplace_back(param.get());
                }
            }
        }
    }
}

void body_gen(Codegen &gen, llvm::Function* funcCallee, std::optional<LoopScope>& body, FunctionDeclaration* func_type) {
    if(body.has_value()) {
        auto prev_func_type = gen.current_func_type;
        gen.current_func_type = func_type;
        gen.current_function = funcCallee;
        const auto destruct_begin = gen.destruct_nodes.size();
        func_type->queue_destruct_params(gen);
        gen.SetInsertPoint(&gen.current_function->getEntryBlock());
        body->code_gen(gen, destruct_begin);
        gen.end_function_block();
        gen.current_function = nullptr;
        gen.current_func_type = prev_func_type;
    }
}

void body_gen(Codegen &gen, FunctionDeclaration* decl, llvm::Function* funcCallee) {
    body_gen(gen, funcCallee, decl->body, decl);
}

void FunctionDeclaration::code_gen_body(Codegen &gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
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
    if(has_annotation(AnnotationKind::CompTime)) {
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
    code_gen_body(gen);
}

void llvm_func_attr(llvm::Function* func, AnnotationKind kind) {
    switch(kind) {
        case AnnotationKind::Inline:
            return;
        case AnnotationKind::AlwaysInline:
            func->addFnAttr(llvm::Attribute::AlwaysInline);
            break;
        case AnnotationKind::NoInline:
            func->addFnAttr(llvm::Attribute::NoInline);
            break;
        case AnnotationKind::InlineHint:
            func->addFnAttr(llvm::Attribute::InlineHint);
            break;
        case AnnotationKind::OptSize:
            func->addFnAttr(llvm::Attribute::OptimizeForSize);
            break;
        case AnnotationKind::MinSize:
            func->addFnAttr(llvm::Attribute::MinSize);
            break;
        default:
            return;
    }
}

void llvm_func_def_attr(llvm::Function* func) {
//    func->addFnAttr(llvm::Attribute::UWTable); // this causes error
    func->addFnAttr(llvm::Attribute::NoUnwind);
}

void FunctionDeclaration::set_llvm_data(llvm::Value* func_callee, llvm::FunctionType* func_type) {
#ifdef DEBUG
    if(active_iteration > llvm_data.size()) {
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

void create_non_generic_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name) {
    auto func_type = decl->create_llvm_func_type(gen);
    std::string func_name = decl->has_annotation(AnnotationKind::Cpp) ? gen.clang.mangled_name(decl) : name;
    auto func = gen.create_function(func_name, func_type, decl->specifier);
    llvm_func_def_attr(func);
    decl->traverse([func](Annotation* annotation){
        llvm_func_attr(func, annotation->kind);
    });
    decl->set_llvm_data(func, func->getFunctionType());
}

void create_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name) {
    if(decl->generic_params.empty()) {
        create_non_generic_fn(gen, decl, name);
    } else {
        const auto total_use = decl->total_generic_iterations();
        auto i = (int16_t) decl->llvm_data.size();
        while(i < total_use) {
            decl->set_active_iteration(i);
            create_non_generic_fn(gen, decl, name);
            i++;
        }
        // we set active iteration to -1, so all generics would fail without setting active_iteration
        decl->set_active_iteration(-1);
    }
}

inline void create_fn(Codegen& gen, FunctionDeclaration *decl) {
    create_fn(gen, decl, decl->parent_node ? decl->runtime_name() : decl->name);
}

void declare_non_gen_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name) {
    auto callee = gen.declare_function(name, decl->create_llvm_func_type(gen));
    decl->set_llvm_data(callee.getCallee(), callee.getFunctionType());
}

void declare_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name) {
    if(decl->generic_params.empty()) {
        declare_non_gen_fn(gen, decl, name);
    } else {
        const auto total_use = decl->total_generic_iterations();
        auto i = (int16_t) decl->llvm_data.size();
        while(i < total_use) {
            decl->set_active_iteration(i);
            declare_non_gen_fn(gen, decl, name);
            i++;
        }
        // we set active iteration to -1, so all generics would fail without setting active_iteration
        decl->set_active_iteration(-1);
    }
}

void declare_fn(Codegen& gen, FunctionDeclaration *decl) {
    declare_fn(gen, decl, decl->parent_node ? decl->runtime_name() : decl->name);
}

void FunctionDeclaration::code_gen_declare_normal(Codegen& gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if (body.has_value()) {
        create_fn(gen, this);
    } else {
        declare_fn(gen, this);
    }
    gen.current_function = nullptr;
}

void FunctionDeclaration::code_gen_declare(Codegen &gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
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
    // TODO generic functions that have already generated should be declared
    // however generic functions that haven't been generated should be generated
    declare_fn(gen, this);
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, StructDefinition* def) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if(has_annotation(AnnotationKind::Destructor)) {
        ensure_destructor(def);
    }
    create_fn(gen, this);
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, VariantDefinition* def) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if(has_annotation(AnnotationKind::Destructor)) {
        ensure_destructor(def);
    }
    create_fn(gen, this);
}

void FunctionDeclaration::code_gen_override_declare(Codegen &gen, FunctionDeclaration* decl) {
    set_llvm_data(decl->llvm_pointer(gen), decl->llvm_func_type(gen));
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, InterfaceDefinition* def) {
    create_fn(gen, this);
}

void FunctionDeclaration::code_gen_declare(Codegen &gen, UnionDef* def) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if(has_annotation(AnnotationKind::Destructor)) {
        ensure_destructor(def);
    }
    create_fn(gen, this);
}

void FunctionDeclaration::code_gen_body(Codegen &gen, StructDefinition* def) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if(has_annotation(AnnotationKind::Destructor)) {
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
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    if(has_annotation(AnnotationKind::Destructor)) {
        code_gen_destructor(gen, def);
        return;
    }
    gen.current_function = nullptr;
    code_gen_body(gen);
}

void FunctionDeclaration::code_gen_body(Codegen &gen, UnionDef* def) {
    if(has_annotation(AnnotationKind::CompTime)) {
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
        gen.SetInsertPoint(&gen.current_function->getEntryBlock());
    }
}

void FunctionDeclaration::code_gen_destructor(Codegen& gen, StructDefinition* def) {
    auto func = llvm_func();
    setup_cleanup_block(gen, func);
    unsigned index = 0;
    for(auto& var : def->variables) {
        if(var.second->value_type() == ValueType::Struct) {
            auto mem_type = var.second->get_value_type();
            auto mem_def = mem_type->linked_node()->as_members_container();
            auto destructor = mem_def->destructor_func();
            if(!destructor) {
                index++;
                continue;
            }
            auto arg = func->getArg(0);
            std::vector<llvm::Value *> idxList{gen.builder->getInt32(0)};
            std::vector<llvm::Value*> args;
            auto element_ptr = Value::get_element_pointer(gen, def->llvm_type(gen), arg, idxList, index);
            if(destructor->has_self_param()) {
                args.emplace_back(element_ptr);
            }
            gen.builder->CreateCall(destructor->llvm_func_type(gen), destructor->llvm_pointer(gen), args);
        }
        index++;
    }
    gen.CreateRet(nullptr);
    gen.redirect_return = nullptr;
}

void FunctionDeclaration::code_gen_destructor(Codegen& gen, VariantDefinition* def) {
    auto func = llvm_func();
    setup_cleanup_block(gen, func);
    // every destructor has self at zero
    auto allocaInst = func->getArg(0);
    // get the type int
    auto gep = gen.builder->CreateGEP(def->llvm_type(gen), allocaInst, { gen.builder->getInt32(0), gen.builder->getInt32(0) }, "", gen.inbounds);
    auto type_value = gen.builder->CreateLoad(gen.builder->getInt32Ty(), gep);

    // create an end block, for default case
    llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);

    // figure out which members to destruct
    std::vector<std::pair<int, VariantMember*>> to_destruct;
    int index = 0;
    for(auto& mem : def->variables) {
        if(mem.second->requires_destructor()) {
            to_destruct.emplace_back(index, mem.second->as_variant_member());
        }
        index++;
    }

    // get struct pointer
    auto struct_ptr = gen.builder->CreateGEP(def->llvm_type(gen), allocaInst, { gen.builder->getInt32(0), gen.builder->getInt32(1) }, "", gen.inbounds);

    // create switch block on type int
    auto switchInst = gen.builder->CreateSwitch(type_value, end_block, to_destruct.size());

    // create blocks for cases for which destructor exists
    for(auto& mem : to_destruct) {
        auto mem_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);

        // destructing member
        gen.SetInsertPoint(mem_block);
        llvm::FunctionType* dtr_func_type = nullptr;
        llvm::Value* dtr_func_callee = nullptr;
        int i = 0;
        for(auto& value : mem.second->values) {
            auto ref_node = value.second->type->get_direct_ref_node();
            if(ref_node) { // <-- the node is directly referenced
                auto destructorFunc = gen.determine_destructor_for(value.second->type.get(), dtr_func_type, dtr_func_callee);
                if(destructorFunc) {
                    std::vector<llvm::Value*> args;
                    if(destructorFunc->has_self_param()) {
                        auto gep3 = gen.builder->CreateGEP(mem.second->llvm_type(gen), struct_ptr, { gen.builder->getInt32(0), gen.builder->getInt32(i) }, "", gen.inbounds);
                        args.emplace_back(gep3);
                    }
                    gen.builder->CreateCall(dtr_func_type, dtr_func_callee, args, "");
                }
            }
            i++;
        }
        gen.CreateBr(end_block);
        switchInst->addCase(gen.builder->getInt32(mem.first), mem_block);
    }

    gen.SetInsertPoint(end_block);
    gen.CreateRet(nullptr);
    gen.redirect_return = nullptr;
}

std::vector<llvm::Type *> FunctionDeclaration::param_types(Codegen &gen) {
    return llvm_func_param_types(gen, params, returnType.get(), false, isVariadic);
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
    auto captured = gen.current_function->getArg(0);
    return gen.builder->CreateStructGEP(lambda->capture_struct_type(gen), captured, index);
}

llvm::Type *CapturedVariable::llvm_type(Codegen &gen) {
    if(capture_by_ref) {
        return gen.builder->getPtrTy();
    } else {
        return linked->llvm_type(gen);
    }
}

bool BaseFunctionParam::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return type->linked_node()->add_child_index(gen, indexes, name);
}

#endif

BaseFunctionParam::BaseFunctionParam(
        std::string name,
        std::unique_ptr<BaseType> type,
        FunctionType* func_type
) : name(std::move(name)), type(std::move(type)), func_type(func_type) {

};

FunctionParam::FunctionParam(
        std::string name,
        std::unique_ptr<BaseType> type,
        unsigned int index,
        std::unique_ptr<Value> defValue,
        FunctionType* func_type,
        CSTToken* token
) : BaseFunctionParam(
        std::move(name),
        std::move(type),
        func_type
    ),
    index(index),
    defValue(std::move(defValue)),
    token(token)
{
    name.shrink_to_fit();
}

unsigned FunctionParam::calculate_c_or_llvm_index() {
    return func_type->c_or_llvm_arg_start_index() + index;
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

FunctionParam *FunctionParam::copy() const {
    std::unique_ptr<Value> copied = nullptr;
    if (defValue) {
        copied.reset(defValue->copy());
    }
    return new FunctionParam(name, std::unique_ptr<BaseType>(type->copy()), index, std::move(copied), func_type, token);
}

std::unique_ptr<BaseType> BaseFunctionParam::create_value_type() {
    return std::unique_ptr<BaseType>(type->copy());
}

hybrid_ptr<BaseType> BaseFunctionParam::get_value_type() {
    return hybrid_ptr<BaseType> { type.get(), false };
}

void BaseFunctionParam::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    if(!name.empty()) {
        linker.declare(name, this);
    }
    type->link(linker, type);
}

void BaseFunctionParam::redeclare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode> &node_ptr) {
    if(!name.empty()) {
        linker.declare(name, this);
    }
}

ASTNode *BaseFunctionParam::child(const std::string &name) {
    return type->linked_node()->child(name);
}

GenericTypeParameter::GenericTypeParameter(
        std::string identifier,
        std::unique_ptr<BaseType> def_type,
        ASTNode* parent_node,
        unsigned param_index,
        CSTToken* token
) : identifier(std::move(identifier)),
def_type(std::move(def_type)), parent_node(parent_node), param_index(param_index), token(token) {

}

void GenericTypeParameter::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    linker.declare(identifier, this);
    if(def_type) {
        def_type->link(linker, def_type);
    }
}

void GenericTypeParameter::register_usage(BaseType* type) {
    if(type) {
        usage.emplace_back(type->copy());
    } else {
        if(def_type) {
            usage.emplace_back(def_type->copy());
        } else {
            std::cerr << "expected a generic type argument for parameter " << identifier << " in node " << parent_node->ns_node_identifier() << std::endl;
        }
    }
}

FunctionDeclaration::FunctionDeclaration(
        std::string name,
        std::vector<std::unique_ptr<FunctionParam>> params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic,
        ASTNode* parent_node,
        CSTToken* token,
        std::optional<LoopScope> body,
        AccessSpecifier specifier
) : FunctionType(std::move(params), std::move(returnType), isVariadic, false, token),
    name(std::move(name)),
    body(std::move(body)), parent_node(parent_node), token(token), specifier(specifier) {
}

int16_t FunctionDeclaration::total_generic_iterations() {
    return ::total_generic_iterations(generic_params);
}

void FunctionDeclaration::ensure_constructor(StructDefinition* def) {
    returnType = std::make_unique<ReferencedType>(def->name, def, nullptr);
}

void FunctionDeclaration::ensure_destructor(ExtendableMembersContainerNode* def) {
    if(!has_self_param() || params.size() > 1 || params.empty()) {
        params.clear();
        params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>(def->name, def, nullptr), nullptr), 0, nullptr, this, nullptr));
    }
    returnType = std::make_unique<VoidType>(nullptr);
    if(!body.has_value()) {
        body.emplace(std::vector<std::unique_ptr<ASTNode>> {}, this, nullptr);
        body.value().nodes.emplace_back(new ReturnStatement(nullptr, this, &body.value(), nullptr));
    }
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

int16_t FunctionDeclaration::register_call(SymbolResolver& resolver, FunctionCall* call, BaseType* expected_type) {

    const auto total = generic_params.size();
    std::vector<BaseType*> generic_args(total);

    // set all to default type (if default type is not present, it would automatically be nullptr)
    unsigned i = 0;
    while(i < total) {
        generic_args[i] = generic_params[i]->def_type.get();
        i++;
    }

    // set given generic args to generic parameters
    i = 0;
    for(auto& arg : call->generic_list) {
        generic_args[i] = arg.get();
        i++;
    }

    // infer args, if user gave less args than expected
    if(call->generic_list.size() != total) {
        call->infer_generic_args(resolver, generic_args);
    }
    if(expected_type) {
        call->infer_return_type(resolver, generic_args, expected_type);
    }

    // register and report to subscribers
    const auto itr = register_generic_usage(resolver, this, generic_params, generic_args);
    if(itr.second) {
        for (auto sub: subscribers) {
            sub->report_parent_usage(resolver, itr.first);
        }
    }

    return itr.first;
}

std::unique_ptr<BaseType> FunctionDeclaration::create_value_type() {
    std::vector<std::unique_ptr<FunctionParam>> copied;
    for(const auto& param : params) {
        copied.emplace_back(param->copy());
    }
    return std::make_unique<FunctionType>(std::move(copied), std::unique_ptr<BaseType>(returnType->copy()), isVariadic, false, nullptr);
}

hybrid_ptr<BaseType> FunctionDeclaration::get_value_type() {
    return hybrid_ptr<BaseType> { create_value_type().release() };
}

void FunctionDeclaration::accept(Visitor *visitor) {
    visitor->visit(this);
}

void FunctionDeclaration::redeclare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    linker.declare_function(name, this);
}

void FunctionDeclaration::declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
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
    linker.scope_start();
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker, (std::unique_ptr<ASTNode>&) gen_param);
    }
    for(auto& param : params) {
        param->type->link(linker, param->type);
    }
    linker.scope_end();
    linker.declare_function(name, this, specifier);
}

void FunctionDeclaration::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    // if has body declare params
    linker.scope_start();
    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = this;
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker, (std::unique_ptr<ASTNode>&) gen_param);
    }
    for (auto &param: params) {
        linker.declare(param->name, param.get());
        if(param->defValue) {
            param->defValue->link(linker, param->defValue);
        }
    }
    returnType->link(linker, returnType);
    if (body.has_value()) {
        body->link_sequentially(linker);
    }
    linker.scope_end();
    linker.current_func_type = prev_func_type;
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    FunctionCall* call_obj,
    Value* parent,
    bool evaluate_refs
) {
    InterpretScope fn_scope(nullptr, call_scope->global);
    return call(call_scope, call_obj->values, parent, &fn_scope, evaluate_refs);
}

// called by the return statement
void FunctionDeclaration::set_return(Value *value) {
    interpretReturn = value;
    body->stopInterpretOnce();
}

FunctionDeclaration *FunctionDeclaration::as_function() {
    return this;
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    std::vector<std::unique_ptr<Value>> &call_args,
    Value* parent,
    InterpretScope *fn_scope,
    bool evaluate_refs
) {
    auto self_param = get_self_param();
    auto params_given = call_args.size() + (self_param ? parent ? 1 : 0 : 0);
    if (params.size() != params_given) {
        fn_scope->error("function " + name + " requires " + std::to_string(params.size()) + ", but given params are " +
                        std::to_string(call_args.size()));
        return nullptr;
    }
    if(self_param) {
        fn_scope->declare(self_param->name, parent);
    }
    auto i = self_param ? 1 : 0;
    while (i < params.size()) {
        Value* param_val;
        if(evaluate_refs) {
            param_val = call_args[i]->param_value(*call_scope);
        } else {
            if(call_args[i]->reference()) {
                param_val = call_args[i]->copy();
            } else {
                param_val = call_args[i]->param_value(*call_scope);
            }
        }
        fn_scope->declare(params[i]->name, param_val);
        i++;
    }
    body.value().interpret(*fn_scope);
    if(self_param) {
        fn_scope->erase_value(self_param->name);
    }
    return interpretReturn;
}

std::unique_ptr<BaseType> CapturedVariable::create_value_type() {
    if(capture_by_ref) {
        return std::make_unique<PointerType>(linked->create_value_type(), nullptr);
    } else {
        return linked->create_value_type();
    }
}

hybrid_ptr<BaseType> CapturedVariable::get_value_type() {
    if(capture_by_ref) {
        return hybrid_ptr<BaseType> { new PointerType(linked->create_value_type(), nullptr), true };
    } else {
        return linked->get_value_type();
    }
}

void CapturedVariable::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
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
