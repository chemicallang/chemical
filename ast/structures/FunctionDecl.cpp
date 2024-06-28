// Copyright (c) Qinetik 2024.

#include <memory>

#include "FunctionParam.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "compiler/SymbolResolver.h"
#include "CapturedVariable.h"
#include "ast/types/PointerType.h"
#include "ast/types/VoidType.h"
#include "ast/statements/Return.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/values/LambdaFunction.h"

llvm::Type *BaseFunctionParam::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::FunctionType *BaseFunctionParam::llvm_func_type(Codegen &gen) {
    return type->llvm_func_type(gen);
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
                gen.error("type is not a pointer type for parameter " + name);
            }
        } else {
            gen.error("type is not an array / pointer for parameter " + name);
        }
    } else {
        gen.error("parameter type is invalid " + name);
    }
    return nullptr;
}

llvm::Value *BaseFunctionParam::llvm_pointer(Codegen &gen) {
    auto arg = gen.current_function->getArg(calculate_c_or_llvm_index());
    if (arg) {
        return arg;
    } else {
        gen.error("couldn't get argument with name " + name);
        return nullptr;
    }
}

llvm::Value *BaseFunctionParam::llvm_load(Codegen &gen) {
    if (gen.current_function != nullptr) {
        unsigned actual_index = calculate_c_or_llvm_index();
        for (const auto &arg: gen.current_function->args()) {
            if (arg.getArgNo() == actual_index) {
                return (llvm::Value *) &arg;
            }
        }
        gen.error(
                "couldn't locate argument by name " + name + " at index " + std::to_string(actual_index) + " in the function");
    } else {
        gen.error("cannot provide pointer to a function parameter when not generating code for a function");
    }
    return nullptr;
}

llvm::FunctionType *FunctionDeclaration::llvm_func_type(Codegen &gen) {
    auto paramTypes = param_types(gen);
    if(paramTypes.empty()) {
        return llvm::FunctionType::get(llvm_func_return(gen, returnType.get()), isVariadic);
    } else {
        return llvm::FunctionType::get(llvm_func_return(gen, returnType.get()), paramTypes, isVariadic);
    }
}

llvm::Function* FunctionDeclaration::llvm_func() {
    if(body.has_value()) {
        return (llvm::Function*) funcCallee;
    } else {
        return nullptr;
    }
}

void body_gen(Codegen &gen, llvm::Function* funcCallee, std::optional<LoopScope>& body, BaseFunctionType* func_type) {
    if(body.has_value()) {
        auto prev_func_type = gen.current_func_type;
        gen.current_func_type = func_type;
        gen.current_function = funcCallee;
        gen.SetInsertPoint(&gen.current_function->getEntryBlock());
        body->code_gen(gen);
        gen.end_function_block();
        gen.current_function = nullptr;
        gen.current_func_type = prev_func_type;
    }
}

void FunctionDeclaration::code_gen(Codegen &gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    body_gen(gen, (llvm::Function*) funcCallee, body, this);
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

void create_fn(Codegen& gen, FunctionDeclaration *decl, const std::string& name) {
    auto func = gen.create_function(name, decl->llvm_func_type(gen), decl->specifier);
    llvm_func_def_attr(func);
    decl->traverse([func](Annotation* annotation){
        llvm_func_attr(func, annotation->kind);
    });
    decl->funcType = func->getFunctionType();
    decl->funcCallee = func;
}

inline void create_fn(Codegen& gen, FunctionDeclaration *decl) {
    create_fn(gen, decl, decl->name);
}

void declare_fn(Codegen& gen, FunctionDeclaration *decl) {
    auto callee = gen.declare_function(decl->name, decl->llvm_func_type(gen));
    decl->funcType = callee.getFunctionType();
    decl->funcCallee = callee.getCallee();
}

void FunctionDeclaration::code_gen_declare(Codegen &gen) {
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

void FunctionDeclaration::code_gen_interface(Codegen &gen, InterfaceDefinition* def) {
    create_fn(gen, this, def->name + "." + name);
    gen.current_function = nullptr;
    if(body.has_value()) {
        code_gen(gen);
    }
}

void FunctionDeclaration::code_gen_override(Codegen& gen, FunctionDeclaration* decl) {
    body_gen(gen, (llvm::Function*) funcCallee, decl->body, decl);
    decl->funcCallee = funcCallee;
    decl->funcType = funcType;
}

void FunctionDeclaration::code_gen_struct(Codegen &gen, StructDefinition* def) {
    if(has_annotation(AnnotationKind::CompTime) && !has_annotation(AnnotationKind::Constructor)) {
        gen.error("comptime annotation is only allowed on constructor methods");
        return;
    }
    create_fn(gen, this, def->name + "." + name);
    gen.current_function = nullptr;
    code_gen(gen);
}

void FunctionDeclaration::code_gen_union(Codegen &gen, UnionDef* def) {
    if(has_annotation(AnnotationKind::CompTime) && !has_annotation(AnnotationKind::Constructor)) {
        gen.error("comptime annotation is only allowed on constructor methods");
        return;
    }
    create_fn(gen, this, def->name + "." + name);
    gen.current_function = nullptr;
    code_gen(gen);
}

void FunctionDeclaration::code_gen_constructor(Codegen& gen, StructDefinition* def) {
    code_gen_struct(gen, def);
}

void FunctionDeclaration::code_gen_destructor(Codegen& gen, StructDefinition* def) {
    ensure_destructor(def);
    create_fn(gen, this, def->name + "." + name);
    auto func = (llvm::Function*) funcCallee;
    llvm::BasicBlock* cleanup_block = llvm::BasicBlock::Create(*gen.ctx, "", func);
    gen.redirect_return = cleanup_block;
    gen.current_function = nullptr;
    code_gen(gen);
    gen.CreateBr(cleanup_block); // ensure branch to cleanup block
    gen.SetInsertPoint(cleanup_block);
    unsigned index = 0;
    for(auto& var : def->variables) {
        if(var.second->value_type() == ValueType::Struct) {
            auto mem_type = var.second->get_value_type();
            auto mem_def = mem_type->linked_node()->as_struct_def();
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
            gen.builder->CreateCall(destructor->llvm_func_type(gen), destructor->funcCallee, args);
        }
        index++;
    }
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
    return llvm_pointer(gen);
}

llvm::Value *FunctionDeclaration::llvm_pointer(Codegen &gen) {
    return funcCallee;
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
        BaseFunctionType* func_type
) : name(std::move(name)), type(std::move(type)), func_type(func_type) {

};

FunctionParam::FunctionParam(
        std::string name,
        std::unique_ptr<BaseType> type,
        unsigned int index,
        std::optional<std::unique_ptr<Value>> defValue,
        BaseFunctionType* func_type
) : BaseFunctionParam(
        std::move(name),
        std::move(type),
        func_type
    ),
    index(index),
    defValue(std::move(defValue))
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
    std::optional<std::unique_ptr<Value>> copied = std::nullopt;
    if (defValue.has_value()) {
        copied.emplace(defValue.value()->copy());
    }
    return new FunctionParam(name, std::unique_ptr<BaseType>(type->copy()), index, std::move(copied), func_type);
}

std::unique_ptr<BaseType> BaseFunctionParam::create_value_type() {
    return std::unique_ptr<BaseType>(type->copy());
}

hybrid_ptr<BaseType> BaseFunctionParam::get_value_type() {
    return hybrid_ptr<BaseType> { type.get(), false };
}

void BaseFunctionParam::declare_and_link(SymbolResolver &linker) {
    if(!name.empty()) {
        linker.declare(name, this);
    }
    type->link(linker, type);
}

ASTNode *BaseFunctionParam::child(const std::string &name) {
    return type->linked_node()->child(name);
}

FunctionDeclaration::FunctionDeclaration(
        std::string name,
        func_params params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic,
        std::optional<LoopScope> body
) : BaseFunctionType(std::move(params), std::move(returnType), isVariadic),
    name(std::move(name)),
    body(std::move(body)) {

    params.shrink_to_fit();
    if(this->name == "main") {
        specifier = AccessSpecifier::Public;
    } else {
        specifier = AccessSpecifier::Private;
    }
}

void FunctionDeclaration::ensure_constructor(StructDefinition* def) {
    returnType = std::make_unique<ReferencedType>(def->name, def);
}

void FunctionDeclaration::ensure_destructor(StructDefinition* def) {
    if(!has_self_param() || params.size() > 1 || params.empty()) {
        params.clear();
        params.emplace_back(std::make_unique<FunctionParam>("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>(def->name, def)), 0, std::nullopt, this));
    }
    returnType = std::make_unique<VoidType>();
    if(!body.has_value()) {
        std::vector<std::unique_ptr<ASTNode>> nodes;
        nodes.emplace_back(new ReturnStatement(std::nullopt, this));
        body.emplace(std::move(nodes));
    }
}

std::unique_ptr<BaseType> FunctionDeclaration::create_value_type() {
    std::vector<std::unique_ptr<FunctionParam>> copied;
    for(const auto& param : params) {
        copied.emplace_back(param->copy());
    }
    return std::make_unique<FunctionType>(std::move(copied), std::unique_ptr<BaseType>(returnType->copy()), isVariadic, false);
}

hybrid_ptr<BaseType> FunctionDeclaration::get_value_type() {
    return hybrid_ptr<BaseType> { create_value_type().release() };
}

void FunctionDeclaration::accept(Visitor *visitor) {
    visitor->visit(this);
}

void FunctionDeclaration::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void FunctionDeclaration::declare_and_link(SymbolResolver &linker) {
    // if has body declare params
    linker.scope_start();
    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = this;
    for (auto &param: params) {
        param->declare_and_link(linker);
    }
    returnType->link(linker, returnType);
    if (body.has_value()) {
        body->declare_and_link(linker);
    }
    linker.scope_end();
    linker.current_func_type = prev_func_type;
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    std::vector<std::unique_ptr<Value>> &call_params,
    Value* parent
) {
    if (!body.has_value()) return nullptr;
    InterpretScope fn_scope(nullptr, call_scope->global);
    return call(call_scope, call_params, parent, &fn_scope);
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
    InterpretScope *fn_scope
) {
    if (!body.has_value()) return nullptr;
    auto prev_func_type = fn_scope->global->current_func_type;
    fn_scope->global->current_func_type = this;
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
        fn_scope->declare(params[i]->name, call_args[i]->param_value(*call_scope));
        i++;
    }
    auto previous = call_scope->global->curr_node_position;
    call_scope->global->curr_node_position = 0;
    body.value().interpret(*fn_scope);
    call_scope->global->curr_node_position = previous;
    if(self_param) {
        fn_scope->erase_value(self_param->name);
    }
    fn_scope->global->current_func_type = prev_func_type;
    return interpretReturn;
}

std::unique_ptr<BaseType> CapturedVariable::create_value_type() {
    if(capture_by_ref) {
        return std::make_unique<PointerType>(linked->create_value_type());
    } else {
        return linked->create_value_type();
    }
}

hybrid_ptr<BaseType> CapturedVariable::get_value_type() {
    if(capture_by_ref) {
        return hybrid_ptr<BaseType> { new PointerType(linked->create_value_type()), true };
    } else {
        return linked->get_value_type();
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
