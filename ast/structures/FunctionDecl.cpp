// Copyright (c) Qinetik 2024.

#include "FunctionParam.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "compiler/SymbolResolver.h"
#include "CapturedVariable.h"
#include "ast/types/PointerType.h"
#include "ExtensionFunction.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/values/LambdaFunction.h"

llvm::Type *FunctionParam::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::FunctionType *FunctionParam::llvm_func_type(Codegen &gen) {
    return type->llvm_func_type(gen);
}

llvm::Type *FunctionParam::llvm_elem_type(Codegen &gen) {
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

llvm::Value *FunctionParam::llvm_pointer(Codegen &gen) {
    auto arg = gen.current_function->getArg(func_type->c_or_llvm_arg_start_index() + index);
    if (arg) {
        return arg;
    } else {
        gen.error("couldn't get argument with name " + name);
        return nullptr;
    }
}

llvm::Value *FunctionParam::llvm_load(Codegen &gen) {
    if (gen.current_function != nullptr) {
        unsigned actual_index = func_type->c_or_llvm_arg_start_index() + index;
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

llvm::Value *FunctionParam::llvm_ret_load(Codegen &gen, ReturnStatement *returnStmt) {
    return type->llvm_return_intercept(gen, llvm_load(gen), this);
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

void body_gen(Codegen &gen, llvm::Function* funcCallee, std::optional<LoopScope>& body) {
    if(body.has_value()) {
        gen.current_function = funcCallee;
        gen.SetInsertPoint(&gen.current_function->getEntryBlock());
        body->code_gen(gen);
        gen.end_function_block();
        gen.current_function = nullptr;
    }
}

void FunctionDeclaration::code_gen(Codegen &gen) {
    body_gen(gen, (llvm::Function*) funcCallee, body);
}

void llvm_func_attr(ASTDiagnoser* diagnoser, llvm::Function* func, AnnotationKind kind) {
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
            diagnoser->error("Unknown annotation of kind " + to_string(kind) + " on function won't be considered");
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
    decl->traverse([&gen, func](Annotation* annotation){
        llvm_func_attr(&gen, func, annotation->kind);
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
    body_gen(gen, (llvm::Function*) funcCallee, decl->body);
    decl->funcCallee = funcCallee;
    decl->funcType = funcType;
}

void FunctionDeclaration::code_gen_struct(Codegen &gen, StructDefinition* def) {
    create_fn(gen, this, def->name + "." + name);
    gen.current_function = nullptr;
    code_gen(gen);
}

std::vector<llvm::Type *> FunctionDeclaration::param_types(Codegen &gen) {
    return llvm_func_param_types(gen, params, returnType.get(), false, isVariadic);
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

bool FunctionParam::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return type->linked_node()->add_child_index(gen, indexes, name);
}

#endif

FunctionParam::FunctionParam(
        std::string name,
        std::unique_ptr<BaseType> type,
        unsigned int index,
        std::optional<std::unique_ptr<Value>> defValue,
        BaseFunctionType* func_type
) : name(std::move(name)),
    type(std::move(type)),
    index(index),
    defValue(std::move(defValue)),
    func_type(func_type)
{
    name.shrink_to_fit();
}

void FunctionParam::accept(Visitor *visitor) {
    visitor->visit(this);
}

ValueType FunctionParam::value_type() const {
    return type->value_type();
}

BaseTypeKind FunctionParam::type_kind() const {
    return type->kind();
}

FunctionParam *FunctionParam::copy() const {
    std::optional<std::unique_ptr<Value>> copied = std::nullopt;
    if (defValue.has_value()) {
        copied.emplace(defValue.value()->copy());
    }
    return new FunctionParam(name, std::unique_ptr<BaseType>(type->copy()), index, std::move(copied), func_type);
}

std::string FunctionParam::representation() const {
    return name + " : " + type->representation();
}

std::unique_ptr<BaseType> FunctionParam::create_value_type() {
    return std::unique_ptr<BaseType>(type->copy());
}

void FunctionParam::declare_and_link(SymbolResolver &linker) {
    if(!name.empty()) {
        linker.declare(name, this);
    }
    type->link(linker);
}

ASTNode *FunctionParam::child(const std::string &name) {
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

std::unique_ptr<BaseType> FunctionDeclaration::create_value_type() {
    std::vector<std::unique_ptr<FunctionParam>> copied;
    for(const auto& param : params) {
        copied.emplace_back(param->copy());
    }
    return std::make_unique<FunctionType>(std::move(copied), std::unique_ptr<BaseType>(returnType->copy()), isVariadic, false);
}

void FunctionDeclaration::accept(Visitor *visitor) {
    visitor->visit(this);
}

void FunctionDeclaration::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void ExtensionFunction::declare_top_level(SymbolResolver &linker) {
    ReferencedType* ref;
    if(type->kind() == BaseTypeKind::Referenced) {
        ref = (ReferencedType*) type.get();
    } else if(type->kind() == BaseTypeKind::Pointer) {
        auto ptr = (PointerType*) type.get();
        if(ptr->type->kind() == BaseTypeKind::Referenced) {
            ref = (ReferencedType*) type.get();
        } else {
            linker.error("Unsupported type in extension function" + type->representation());
            return;
        }
    } else {
        linker.error("Unsupported type in extension function " + type->representation());
        return;
    }
    if(!ref->linked) {
        linker.error("No linkage found for type mentioned in extension function " + type->representation());
        return;
    }
    auto container = ref->linked->as_extendable_members_container();
    if(!container) {
        linker.error("Type doesn't support extension functions " + type->representation());
        return;
    }
    if(ref->linked->child(name)) {
        linker.error("Type already has a field / function, Type " + type->representation() + " has member " + name);
        return;
    }
    container->extension_functions[name] = this;
}

void FunctionDeclaration::declare_and_link(SymbolResolver &linker) {
    // if has body declare params
    linker.scope_start();
    for (auto &param: params) {
        param->declare_and_link(linker);
    }
    returnType->link(linker);
    if (body.has_value()) {
        body->declare_and_link(linker);
    }
    linker.scope_end();
}

void FunctionDeclaration::interpret(InterpretScope &scope) {
    declarationScope = &scope;
}

Value *FunctionDeclaration::call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_params) {
    if (!body.has_value()) return nullptr;
    InterpretScope fn_scope(declarationScope, declarationScope->global, &body.value(), this);
    return call(call_scope, call_params, &fn_scope);
}

// called by the return statement
void FunctionDeclaration::set_return(Value *value) {
    interpretReturn = value;
    body->stopInterpretOnce();
}

FunctionDeclaration *FunctionDeclaration::as_function() {
    return this;
}

std::string FunctionDeclaration::representation() const {
    std::string ret;
    ret.append("func ");
    ret.append(name);
    ret.append(1, '(');
    int i = 0;
    while (i < params.size()) {
        const auto &param = params[i];
        ret.append(param->representation());
        if (i < params.size() - 1) {
            ret.append(", ");
        } else {
            if (isVariadic) {
                ret.append("...");
            }
        }
        i++;
    }
    ret.append(1, ')');
    ret.append(" : ");
    ret.append(returnType->representation());
    ret.append(1, ' ');
    if (body.has_value()) {
        ret.append("{\n");
        ret.append(body.value().representation());
        ret.append("\n}");
    }
    return ret;
}

Value *FunctionDeclaration::call(InterpretScope *call_scope, std::vector<std::unique_ptr<Value>> &call_params,
                                 InterpretScope *fn_scope) {
    if (!body.has_value()) return nullptr;
    if (params.size() != call_params.size()) {
        fn_scope->error("function " + name + " requires " + std::to_string(params.size()) + ", but given params are " +
                        std::to_string(call_params.size()));
        return nullptr;
    }
    auto i = 0;
    while (i < params.size()) {
        fn_scope->declare(params[i]->name, call_params[i]->param_value(*call_scope));
        i++;
    }
    auto previous = call_scope->global->curr_node_position;
    call_scope->global->curr_node_position = 0;
    body.value().interpret(*fn_scope);
    call_scope->global->curr_node_position = previous;
    // delete all the primitive values that were copied into the function
    i--;
    while (i > -1) {
        auto itr = fn_scope->find_value_iterator(params[i]->name);
        if (itr.first != itr.second.end()) {
            if (itr.first->second != nullptr && itr.first->second->primitive()) {
                delete itr.first->second;
            }
            itr.second.erase(itr.first);
        }
        i--;
    }
    return interpretReturn;
}

std::unique_ptr<BaseType> CapturedVariable::create_value_type() {
    if(capture_by_ref) {
        return std::make_unique<PointerType>(linked->create_value_type());
    } else {
        return linked->create_value_type();
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
