// Copyright (c) Qinetik 2024.

#include "ExtensionFunction.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ReferencedType.h"
#include "ast/base/ExtendableBase.h"
#include "ast/types/PointerType.h"
#include "ast/types/GenericType.h"

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
    ASTNode* parent_node,
    CSTToken* token
) : BaseFunctionParam(std::move(name), std::move(type)), parent_node(parent_node), token(token) {

}

unsigned int ExtensionFuncReceiver::calculate_c_or_llvm_index() {
    return 0;
}

void ExtensionFuncReceiver::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
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

void ExtensionFunction::declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {

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
    receiver.type->link(linker, receiver.type);
    for(auto& param : params) {
        param->type->link(linker, param->type);
    }
    linker.scope_end();

//    auto referenced = get_referenced(receiver.type.get());
    auto linked = receiver.type->linked_node();
    auto& type = receiver.type;
    if(!linked) {
        linker.error("couldn't find container in extension function ith receiver type \"" + type->representation() + "\"", (AnnotableNode*) this);
        return;
    }
    auto container = linked->as_extendable_members_container();
    if(!container) {
        linker.error("type doesn't support extension functions " + type->representation(), receiver.type.get());
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
        CSTToken* token,
        std::optional<LoopScope> body,
        std::optional<AccessSpecifier> specifier
) : FunctionDeclaration(
    std::move(name),
    std::move(params),
    std::move(returnType),
    std::move(isVariadic),
    parent_node,
    token,
    std::move(body),
    specifier
), receiver(std::move(receiver)) {

}

void ExtensionFunction::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {

    auto linked = receiver.type->linked_node();
    const auto field_func = linked->child(name);
    if(field_func != this) {
        linker.error("couldn't declare extension function with name '" + name + "' because type '" + receiver.type->representation() + "' already has a field / function with same name \n", receiver.type.get());
        return;
    }

    // if has body declare params
    linker.scope_start();
    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = this;
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker, (std::unique_ptr<ASTNode>&) gen_param);
    }
    receiver.declare_and_link(linker, (std::unique_ptr<ASTNode>&) receiver);
    for (auto &param: params) {
        param->declare_and_link(linker, (std::unique_ptr<ASTNode>&) param);
    }
    returnType->link(linker, returnType);
    if (body.has_value()) {
        body->link_sequentially(linker);
    }
    linker.scope_end();
    linker.current_func_type = prev_func_type;
}

std::unique_ptr<BaseType> ExtensionFunction::create_value_type() {
    auto value_type = FunctionDeclaration::create_value_type();
    auto functionType = (FunctionType*) value_type.get();
    functionType->params.insert(functionType->params.begin(), std::make_unique<FunctionParam>("self", std::unique_ptr<BaseType>(receiver.type->copy()), 0, nullptr, this, nullptr));
    return value_type;
}

hybrid_ptr<BaseType> ExtensionFunction::get_value_type() {
    return hybrid_ptr<BaseType> { create_value_type().release() };
}