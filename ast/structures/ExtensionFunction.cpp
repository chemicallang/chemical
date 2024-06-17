// Copyright (c) Qinetik 2024.

#include "ExtensionFunction.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/ReferencedType.h"
#include "ast/base/ExtendableBase.h"
#include "ast/types/PointerType.h"

ExtensionFuncReceiver::ExtensionFuncReceiver(
    std::string name,
    std::unique_ptr<BaseType> type
) : BaseFunctionParam(std::move(name), std::move(type)) {

}

unsigned int ExtensionFuncReceiver::calculate_c_or_llvm_index() {
    return 0;
}

void ExtensionFuncReceiver::declare_and_link(SymbolResolver &linker) {
    linker.declare(name, this);
    type->link(linker);
}

void ExtensionFunction::declare_top_level(SymbolResolver &linker) {
    ReferencedType* ref;
    auto& type = receiver.type;
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

void ExtensionFuncReceiver::accept(Visitor *visitor) {
    visitor->visit(this);
}

std::string ExtensionFuncReceiver::representation() const {
    return name + " : " + type->representation();
}

ExtensionFunction::ExtensionFunction(
        std::string name,
        ExtensionFuncReceiver receiver,
        std::vector<std::unique_ptr<FunctionParam>> params,
        std::unique_ptr<BaseType> returnType,
        bool isVariadic,
        std::optional<LoopScope> body
) : FunctionDeclaration(
    std::move(name),
    std::move(params),
    std::move(returnType),
    std::move(isVariadic),
    std::move(body)
), receiver(std::move(receiver)) {

}

void ExtensionFunction::declare_and_link(SymbolResolver &linker) {
    // if has body declare params
    linker.scope_start();
    receiver.declare_and_link(linker);
    for (auto &param: params) {
        param->declare_and_link(linker);
    }
    returnType->link(linker);
    if (body.has_value()) {
        body->declare_and_link(linker);
    }
    linker.scope_end();
}