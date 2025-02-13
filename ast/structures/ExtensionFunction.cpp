// Copyright (c) Qinetik 2024.

#include "ExtensionFunction.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/LinkedType.h"
#include "ast/base/ExtendableBase.h"
#include "ast/types/PointerType.h"
#include "ast/types/GenericType.h"
#include "ast/structures/InterfaceDefinition.h"

#ifdef COMPILER_BUILD

std::vector<llvm::Type *> ExtensionFunction::param_types(Codegen &gen) {
    std::vector<llvm::Type*> paramTypes;
    llvm_func_param_type(gen, paramTypes, receiver.type);
    llvm_func_param_types_into(gen, paramTypes, params, returnType, false, isVariadic(), this);
    return paramTypes;
}

#endif

unsigned int ExtensionFuncReceiver::calculate_c_or_llvm_index() {
    return 0;
}

void ExtensionFuncReceiver::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare(name, this);
}

ASTNode *ExtensionFuncReceiver::child(const chem::string_view &name) {
    const auto linked = type->linked_node();
    return linked ? linked->child(name) : nullptr;
}

//static std::string get_referenced(BaseType* type) {
//    const auto kind = type->kind();
//    if(kind == BaseTypeKind::Linked) {
//        return ((LinkedType*) type)->type;
//    } else if(kind == BaseTypeKind::Generic) {
//        return ((GenericType*) type)->referenced->type;
//    } else if(kind == BaseTypeKind::Pointer) {
//        return get_referenced(((PointerType*) type)->type);
//    } else {
//        return "";
//    }
//}

void ExtensionFunction::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    // do nothing here
}

void ExtensionFunction::link_signature(SymbolResolver &linker) {

    linker.scope_start();

    link_signature_no_scope(linker);

    if(!FunctionType::data.signature_resolved) {
        return;
    }

    if(!receiver.type->link(linker)) {
        FunctionType::data.signature_resolved = false;
        return;
    }

    linker.scope_end();

    const auto type = receiver.type;
    const auto pure_receiver = type->pure_type();
    const auto receiver_kind = pure_receiver->kind();
    if(receiver_kind != BaseTypeKind::Reference) {
        linker.error("receiver in extension function must always be a reference", type);
    }
    auto linked = type->linked_node();
    if(!linked) {
        linker.error("couldn't find container in extension function ith receiver type \"" + type->representation() + "\"", (AnnotableNode*) this);
        return;
    }
    const auto linked_kind = linked->kind();
    if(linked_kind == ASTNodeKind::InterfaceDecl) {
        const auto interface = linked->as_interface_def_unsafe();
        if(!interface->is_static()) {
            linker.error("extension functions are only supported on static interfaces, either make the interface static or move the function inside the interface", receiver.type);
            return;
        }
    }
    auto container = linked->as_extendable_members_container();
    if(!container) {
        linker.error("type doesn't support extension functions " + type->representation(), receiver.type);
        return;
    }

    container->extension_functions[name_view()] = this;
}

void ExtensionFuncReceiver::accept(Visitor *visitor) {
    visitor->visit(this);
}

void ExtensionFunction::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {

    auto linked = receiver.type->linked_node();
    if(linked) {
        const auto field_func = linked->child(name_view());
        if (field_func != this) {
            linker.error("couldn't declare extension function with name '" + name_str() + "' because type '" +
                         receiver.type->representation() + "' already has a field / function with same name \n",
                         receiver.type);
            return;
        }
    } else {
        linker.error("couldn't get the node of receiver in extension function", receiver.type);
        return;
    }

    // if has body declare params
    linker.scope_start();
    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = this;
    for(auto& gen_param : generic_params) {
        gen_param->declare_and_link(linker, (ASTNode*&) gen_param);
    }
    receiver.declare_and_link(linker, (ASTNode*&) receiver);
    for (auto &param: params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    if (body.has_value()) {
        body->link_sequentially(linker);
    }
    linker.scope_end();
    linker.current_func_type = prev_func_type;
}

BaseType* ExtensionFunction::create_value_type(ASTAllocator& allocator) {
    auto value_type = FunctionDeclaration::create_value_type(allocator);
    auto functionType = (FunctionType*) value_type;
    functionType->params.insert(functionType->params.begin(), new (allocator.allocate<FunctionParam>()) FunctionParam("self", receiver.type->copy(allocator), 0, nullptr, true, this, ZERO_LOC));
    return value_type;
}

//hybrid_ptr<BaseType> ExtensionFunction::get_value_type() {
//    return hybrid_ptr<BaseType> { create_value_type().release() };
//}