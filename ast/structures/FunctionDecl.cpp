// Copyright (c) Qinetik 2024.

#include "FunctionParam.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/types/FunctionType.h"

#ifdef COMPILER_BUILD

#include "ast/types/PointerType.h"
#include "compiler/llvmimpl.h"

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
    auto arg = gen.current_function->getArg(index);
    if (arg) {
        return arg;
    } else {
        gen.error("couldn't get argument with name " + name);
        return nullptr;
    }
}

llvm::Value *FunctionParam::llvm_load(Codegen &gen) {
    if (gen.current_function != nullptr) {
        for (const auto &arg: gen.current_function->args()) {
            if (arg.getArgNo() == index) {
                return (llvm::Value *) &arg;
            }
//                else { // for debugging
//                    gen.error("no mismatch" + std::to_string(arg.getArgNo()) + " " + std::to_string(param->index));
//                }
        }
        gen.error(
                "couldn't locate argument by name " + name + " at index " + std::to_string(index) + " in the function");
    } else {
        gen.error("cannot provide pointer to a function parameter when not generating code for a function");
    }
    return nullptr;
}

llvm::FunctionType *FunctionDeclaration::llvm_func_type(Codegen &gen) {
    if (params.empty() || (params.size() == 1 && isVariadic)) {
        return llvm::FunctionType::get(returnType->llvm_type(gen), isVariadic);
    } else {
        return llvm::FunctionType::get(returnType->llvm_type(gen), param_types(gen), isVariadic);
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

void create_fn(Codegen& gen, FunctionDeclaration *decl) {
    auto func = gen.create_function(decl->name, decl->llvm_func_type(gen), decl->specifier);
    decl->funcType = func->getFunctionType();
    decl->funcCallee = func;
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

void FunctionDeclaration::code_gen_interface(Codegen &gen) {
    create_fn(gen, this);
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

void FunctionDeclaration::code_gen_struct(Codegen &gen) {
    code_gen_declare(gen);
    code_gen(gen);
}

std::vector<llvm::Type *> FunctionDeclaration::param_types(Codegen &gen) {
    auto size = isVariadic ? (params.size() - 1) : params.size();
    std::vector<llvm::Type *> array(size);
    unsigned i = 0;
    while (i < size) {
        array[i] = params[i]->type->llvm_param_type(gen);
        i++;
    }
    return array;
}

llvm::Value *FunctionDeclaration::llvm_load(Codegen &gen) {
    return llvm_pointer(gen);
}

llvm::Value *FunctionDeclaration::llvm_pointer(Codegen &gen) {
    return funcCallee;
}

#endif

FunctionParam::FunctionParam(
        std::string name,
        std::unique_ptr<BaseType> type,
        unsigned int index,
        bool isVariadic,
        std::optional<std::unique_ptr<Value>> defValue
) : name(std::move(name)),
    type(std::move(type)),
    index(index), isVariadic(isVariadic), defValue(std::move(defValue)) {
    name.shrink_to_fit();
}

void FunctionParam::accept(Visitor &visitor) {
    visitor.visit(this);
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
    return new FunctionParam(name, std::unique_ptr<BaseType>(type->copy()), index, isVariadic, std::move(copied));
}

FunctionParam *FunctionParam::as_parameter() {
    return this;
}

std::string FunctionParam::representation() const {
    return name + " : " + type->representation();
}

std::unique_ptr<BaseType> FunctionParam::create_value_type() {
    return std::unique_ptr<BaseType>(type->copy());
}

void FunctionParam::declare_and_link(SymbolResolver &linker) {
    linker.declare(name, this);
    type->link(linker);
}

ASTNode *FunctionParam::child(const std::string &name) {
    return type->linked_node()->child(name);
}

bool FunctionParam::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return type->linked_node()->add_child_index(gen, indexes, name);
}

bool FunctionParam::add_child_indexes(Codegen &gen, std::vector<llvm::Value *> &indexes, std::vector<std::unique_ptr<Value>> &u_inds) {
    return type->linked_node()->add_child_indexes(gen, indexes, u_inds);
}

FunctionDeclaration::FunctionDeclaration(
        std::string name,
        func_params params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic,
        std::optional<LoopScope> body
) : name(std::move(name)), params(std::move(params)), returnType(std::move(returnType)), body(std::move(body)),
    isVariadic(isVariadic) {
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

void FunctionDeclaration::accept(Visitor &visitor) {
    visitor.visit(this);
}

void FunctionDeclaration::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void FunctionDeclaration::declare_and_link(SymbolResolver &linker) {
    // if has body declare params
    linker.scope_start();
    for (auto &param: params) {
        param->declare_and_link(linker);
    }
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