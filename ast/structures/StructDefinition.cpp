// Copyright (c) Qinetik 2024.

#include "StructMember.h"
#include "StructDefinition.h"
#include "FunctionDeclaration.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/StructType.h"
#include "compiler/SymbolResolver.h"
#include "InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/types/VoidType.h"
#include "ast/types/PointerType.h"
#include "UnnamedStruct.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/values/IntValue.h"

llvm::StructType* StructDefinition::get_struct_type(Codegen& gen) {
    if(!llvm_struct_type) {
        if(has_annotation(AnnotationKind::Anonymous)) {
            return llvm::StructType::get(*gen.ctx, elements_type(gen));
        }
        llvm_struct_type = llvm::StructType::create(*gen.ctx, elements_type(gen), name);
    }
    return llvm_struct_type;
}

void StructDefinition::code_gen(Codegen &gen) {
    std::unordered_map<std::string, llvm::Function *> *ref = nullptr;
    InterfaceDefinition *interface = nullptr;
    if (overrides.has_value()) {
        auto overridden = gen.unimplemented_interfaces.find(overrides.value()->type);
        if (overridden == gen.unimplemented_interfaces.end()) {
            gen.error("Couldn't find overridden interface with name '" + overrides.value()->type +
                      "' for implementation");
            return;
        }
        ref = &overridden->second;
        interface = (InterfaceDefinition *) overrides.value()->linked_node();
    }
    bool has_destructor = false;
    for (auto &function: functions) {
        if(function.second->has_annotation(AnnotationKind::Destructor)) {
            function.second->code_gen_destructor(gen, this);
            has_destructor = true;
        }
        if(function.second->has_annotation(AnnotationKind::Constructor)) {
            function.second->code_gen_constructor(gen, this);
            continue;
        }
        if (overrides.has_value()) {
            auto overridden = interface->child(function.second->name);
            if (overridden) {
                auto found = ref->find(function.second->name);
                if (found == ref->end()) {
                    gen.error("Couldn't find function with name " + function.second->name + " in interface " +
                              overrides.value()->type + " for implementation");
                    continue;
                }
                if (found->second == nullptr) {
                    gen.error(
                            "Function with name " + function.second->name + " in interface " + overrides.value()->type +
                            " has already been implemented");
                    continue;
                }
                auto fn = overridden->as_function();
                if (fn) {
                    fn->code_gen_override(gen, function.second.get());
                    found->second = nullptr;
                    continue;
                }
            }
        }
        function.second->code_gen_struct(gen, this);
    }
    if(!has_destructor && requires_destructor()) {
        auto decl = create_destructor();
        decl->code_gen_destructor(gen, this);
    }
}

llvm::Type *StructMember::llvm_type(Codegen &gen) {
    return type->llvm_struct_member_type(gen);
}

llvm::FunctionType *StructMember::llvm_func_type(Codegen &gen) {
    return type->llvm_func_type(gen);
}

bool StructMember::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &childName) {
    auto linked = type->linked_node();
    if (!linked) return false;
    linked->add_child_index(gen, indexes, childName);
    return true;
}

llvm::Type *StructDefinition::llvm_type(Codegen &gen) {
    return get_struct_type(gen);
}

llvm::Type *UnnamedStruct::llvm_type(Codegen &gen) {
    return llvm::StructType::get(*gen.ctx, elements_type(gen));
}

void StructDefinition::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    auto func = destructor_func();
    if(func) {
        std::vector<llvm::Value*> args;
        if(func->has_self_param()) {
            args.emplace_back(allocaInst);
        }
        gen.builder->CreateCall(func->llvm_func_type(gen), func->funcCallee, args);
    }
}

#endif

BaseDefMember::BaseDefMember(std::string name) : name(std::move(name)) {

}

StructMember::StructMember(
        std::string name,
        std::unique_ptr<BaseType> type,
        std::optional<std::unique_ptr<Value>> defValue
) : BaseDefMember(std::move(name)), type(std::move(type)), defValue(std::move(defValue)) {

}

void StructMember::accept(Visitor *visitor) {
    visitor->visit(this);
}

std::unique_ptr<BaseType> StructMember::create_value_type() {
    return std::unique_ptr<BaseType>(type->copy());
}

hybrid_ptr<BaseType> StructMember::get_value_type() {
    return hybrid_ptr<BaseType> { type.get(), false };
}

void StructMember::declare_and_link(SymbolResolver &linker) {
    linker.declare(name, this);
    type->link(linker, type);
    if (defValue.has_value()) {
        defValue.value()->link(linker, defValue.value());
    }
}

ASTNode *StructMember::child(const std::string &childName) {
    auto linked = type->linked_node();
    if (!linked) return nullptr;
    return linked->child(childName);
}

ValueType StructMember::value_type() const {
    return type->value_type();
}

BaseTypeKind StructMember::type_kind() const {
    return type->kind();
}

UnnamedStruct::UnnamedStruct(
        std::string name
) : BaseDefMember(std::move(name)) {

}

StructDefinition::StructDefinition(
        std::string name,
        const std::optional<std::string> &overrides
) : ExtendableMembersContainerNode(std::move(name)) {
    if (overrides.has_value()) {
        this->overrides = std::make_unique<ReferencedType>(overrides.value());
    } else {
        this->overrides = std::nullopt;
    }
}

bool StructDefinition::requires_destructor() {
    auto destructor = destructor_func();
    if(destructor) return true;
    for(const auto& var : variables) {
        auto mem_type = var.second->get_value_type();
        if(mem_type->value_type() == ValueType::Struct && mem_type->linked_node()->as_struct_def()->requires_destructor()) {
            return true;
        }
    }
    return false;
}

FunctionDeclaration* StructDefinition::create_destructor() {
    auto decl = new FunctionDeclaration("delete", {}, std::make_unique<VoidType>(), false, std::nullopt);
    decl->annotations.emplace_back(AnnotationKind::Destructor);
    functions["delete"] = std::unique_ptr<FunctionDeclaration>(decl);
    return decl;
}

void StructDefinition::accept(Visitor *visitor) {
    visitor->visit(this);
}

void StructDefinition::declare_top_level(SymbolResolver &linker) {
    if (overrides.has_value()) {
        overrides.value()->link(linker, reinterpret_cast<std::unique_ptr<BaseType>&>(overrides.value()));
    }
    linker.declare(name, this);
}

void StructDefinition::declare_and_link(SymbolResolver &linker) {
    MembersContainer::declare_and_link(linker);
}

StructDefinition *StructDefinition::as_struct_def() {
    return this;
}

void StructDefinition::interpret(InterpretScope &scope) {
    decl_scope = &scope;
}

ASTNode *StructDefinition::child(const std::string &name) {
    auto node = ExtendableMembersContainerNode::child(name);
    if (node) {
        return node;
    } else if (overrides.has_value()) {
        return overrides.value()->linked->child(name);
    };
    return nullptr;
}

std::unique_ptr<BaseType> StructDefinition::create_value_type() {
    return std::make_unique<ReferencedType>(name, this);
}

hybrid_ptr<BaseType> StructDefinition::get_value_type() {
    return hybrid_ptr<BaseType> { new ReferencedType(name, this) };
}

hybrid_ptr<BaseType> UnnamedStruct::get_value_type() {
    return hybrid_ptr<BaseType> { new ReferencedType(name, this) };
}

ValueType StructDefinition::value_type() const {
    return ValueType::Struct;
}