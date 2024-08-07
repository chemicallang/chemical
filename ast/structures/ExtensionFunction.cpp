// Copyright (c) Qinetik 2024.

#include "ExtensionFunction.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ReferencedType.h"
#include "ast/base/ExtendableBase.h"
#include "ast/types/PointerType.h"
#include "ast/types/GenericType.h"
#include "ast/types/FunctionType.h"

#ifdef COMPILER_BUILD

std::vector<llvm::Type *> ExtensionFunction::param_types(Codegen &gen) {
    std::vector<llvm::Type*> paramTypes;
    llvm_func_param_type(gen, paramTypes, receiver.type.get());
    llvm_func_param_types_into(gen, paramTypes, params, returnType.get(), false, isVariadic);
    return paramTypes;
}

#endif

ExtensionFuncReceiver::ExtensionFuncReceiver(
    std::string name,
    std::unique_ptr<BaseType> type,
    ASTNode* parent_node
) : BaseFunctionParam(std::move(name), std::move(type)), parent_node(parent_node) {

}

unsigned int ExtensionFuncReceiver::calculate_c_or_llvm_index() {
    return 0;
}

void ExtensionFuncReceiver::declare_and_link(SymbolResolver &linker) {
    linker.declare(name, this);
}

ASTNode *ExtensionFuncReceiver::child(const std::string &name) {
    return type->linked_node()->child(name);
}

static std::string get_referenced(BaseType* type) {
    const auto kind = type->kind();
    if(kind == BaseTypeKind::Referenced) {
        return ((ReferencedType*) type)->type;
    } else if(kind == BaseTypeKind::Generic) {
        return ((GenericType*) type)->referenced->type;
    } else if(kind == BaseTypeKind::Pointer) {
        return get_referenced(((PointerType*) type)->type.get());
    } else {
        return "";
    }
}

void ExtensionFunction::declare_top_level(SymbolResolver &linker) {
    auto referenced = get_referenced(receiver.type.get());
    auto linked = linker.find(referenced);
    auto& type = receiver.type;
    if(!linked) {
        linker.error("couldn't find container with name \"" + referenced + "\" in extension function receiver type \"" + type->representation() + "\"");
        return;
    }
    auto container = linked->as_extendable_members_container();
    if(!container) {
        linker.error("type doesn't support extension functions " + type->representation());
        return;
    }
    if(linked->child(name)) {
        linker.error("type already has a field / function, Type " + type->representation() + " has member " + name);
        return;
    }
    container->extension_functions[name] = this;
}

void ExtensionFuncReceiver::accept(Visitor *visitor) {
    visitor->visit(this);
}

ExtensionFunction::ExtensionFunction(
        std::string name,
        ExtensionFuncReceiver receiver,
        std::vector<std::unique_ptr<FunctionParam>> params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic,
        ASTNode* parent_node,
        std::optional<LoopScope> body
) : FunctionDeclaration(
    std::move(name),
    std::move(params),
    std::move(returnType),
    std::move(isVariadic),
    parent_node,
    std::move(body)
), receiver(std::move(receiver)) {

}

void ExtensionFunction::declare_and_link(SymbolResolver &linker) {
    receiver.type->link(linker, receiver.type);
    // if has body declare params
    linker.scope_start();
    receiver.declare_and_link(linker);
    for (auto &param: params) {
        param->declare_and_link(linker);
    }
    returnType->link(linker, returnType);
    if (body.has_value()) {
        body->declare_and_link(linker);
    }
    linker.scope_end();
}

std::unique_ptr<BaseType> ExtensionFunction::create_value_type() {
    auto value_type = FunctionDeclaration::create_value_type();
    auto functionType = (FunctionType*) value_type.get();
    functionType->params.insert(functionType->params.begin(), std::make_unique<FunctionParam>("self", std::unique_ptr<BaseType>(receiver.type->copy()), 0, std::nullopt, this));
    return value_type;
}

hybrid_ptr<BaseType> ExtensionFunction::get_value_type() {
    return hybrid_ptr<BaseType> { create_value_type().release() };
}