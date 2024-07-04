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
#include "ast/types/ReferencedType.h"

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

    // returns the function that is being overridden by given function in the parameter
    auto get_overriding = [&](FunctionDeclaration* function) -> FunctionDeclaration* {
        if (overrides.has_value()) {
            auto overridden = interface->child(function->name);
            if(overridden) {
                return overridden->as_function();
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    };

    // tries to override the function present in interface
    // returns true if current function should be skipped because it has been overridden
    // or errored out
    auto override = [&](FunctionDeclaration* function) -> bool {
        if (overrides.has_value()) {
            auto overridden = interface->child(function->name);
            if (overridden) {
                auto found = ref->find(function->name);
                if (found == ref->end()) {
                    gen.error("Couldn't find function with name " + function->name + " in interface " +
                              overrides.value()->type + " for implementation");
                    return true;
                }
                if (found->second == nullptr) {
                    gen.error(
                            "Function with name " + function->name + " in interface " + overrides.value()->type +
                            " has already been implemented");
                    return true;
                }
                auto fn = overridden->as_function();
                if (fn) {
                    function->code_gen_override(gen, fn);
                    found->second = nullptr;
                    return true;
                } else {
                    gen.error("Function being overridden with name " + function->name + " in interface is not a function");
                }
            }
        }
        return false;
    };
    for(auto& function : functions) {
        auto overriding = get_overriding(function.second.get());
        if(overriding) {
            function.second->code_gen_override_declare(gen, overriding);
            continue;
        }
        function.second->code_gen_declare(gen, this);
    }
    for (auto &function: functions) {
        if(override(function.second.get())) {
            continue;
        }
        function.second->code_gen_body(gen, this);
    }
}

llvm::Type *StructMember::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Type *StructMember::llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, unsigned int index) {
    return type->llvm_chain_type(gen, values, index);
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
        std::optional<std::unique_ptr<Value>> defValue,
        ASTNode* parent_node
) : BaseDefMember(std::move(name)), type(std::move(type)), defValue(std::move(defValue)), parent_node(parent_node) {

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

BaseDefMember *StructMember::copy_member() {
    std::optional<std::unique_ptr<Value>> def_value = std::nullopt;
    if(defValue.has_value()) {
        def_value.emplace(defValue.value()->copy());
    }
    return new StructMember(name, std::unique_ptr<BaseType>(type->copy()), std::move(def_value), parent_node);
}

void StructMember::declare_and_link(SymbolResolver &linker) {
    linker.declare(name, this);
    type->link(linker, type);
    if (defValue.has_value()) {
        defValue.value()->link(linker, defValue.value());
    }
}

void UnnamedStruct::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    VariablesContainer::declare_and_link(linker);
    linker.scope_end();
    linker.declare(name, this);
}

BaseDefMember *UnnamedStruct::copy_member() {
    auto unnamed = new UnnamedStruct(name, parent_node);
    for(auto& variable : variables) {
        unnamed->variables[variable.first] = std::unique_ptr<BaseDefMember>(variable.second->copy_member());
    }
    return unnamed;
}

VariablesContainer *UnnamedStruct::copy_container() {
    return (VariablesContainer*) copy_member();
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
        std::string name,
        ASTNode* parent_node
) : BaseDefMember(std::move(name)), parent_node(parent_node) {

}

StructDefinition::StructDefinition(
        std::string name,
        const std::optional<std::string> &overrides,
        ASTNode* parent_node
) : ExtendableMembersContainerNode(std::move(name)), parent_node(parent_node) {
    if (overrides.has_value()) {
        this->overrides = std::make_unique<ReferencedType>(overrides.value());
    } else {
        this->overrides = std::nullopt;
    }
}

BaseType *StructDefinition::copy() const {
    return new ReferencedType(name, (ASTNode *) this);
}

BaseType *UnnamedStruct::copy() const {
    return new ReferencedType(name, (ASTNode *) this);
}

bool StructMember::requires_destructor() {
    return type->value_type() == ValueType::Struct && type->linked_node()->as_struct_def()->requires_destructor();
}

FunctionDeclaration* StructDefinition::create_destructor() {
    auto decl = new FunctionDeclaration("delete", {}, std::make_unique<VoidType>(), false, this, std::nullopt);
    decl->params.emplace_back(new FunctionParam("self", std::make_unique<PointerType>(std::make_unique<ReferencedType>(name, this)), 0, std::nullopt, decl));
    decl->body.emplace(LoopScope{nullptr});
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
    bool has_destructor = false;
    for(auto& func : functions) {
        if(func.second->has_annotation(AnnotationKind::Constructor)) {
            func.second->ensure_constructor(this);
        }
        if(func.second->has_annotation(AnnotationKind::Destructor)) {
            has_destructor = true;
        }
    }
    MembersContainer::declare_and_link(linker);
    if(!has_destructor && requires_destructor()) {
        auto found = functions.find("delete");
        if(found != functions.end()) {
            linker.error("default destructor is created by name 'delete' , a function by name 'delete' already exists in struct '" + name + "', please create a destructor by hand if you'd like to reserve 'delete' for your own usage");
            return;
        }
        create_destructor();
    }
}

StructDefinition *StructDefinition::as_struct_def() {
    return this;
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

VariablesContainer *StructDefinition::copy_container() {
    auto def = new StructDefinition(name, std::nullopt, parent_node);
    if(overrides.has_value()) {
        def->overrides = std::unique_ptr<ReferencedType>((ReferencedType *) overrides.value()->copy());
    }
    for(auto& variable : variables) {
        def->variables[variable.first] = std::unique_ptr<BaseDefMember>(variable.second->copy_member());
    }
    return def;
}

std::unique_ptr<BaseType> StructDefinition::create_value_type() {
    return std::make_unique<ReferencedType>(name, this);
}

hybrid_ptr<BaseType> StructDefinition::get_value_type() {
    return hybrid_ptr<BaseType> { this, false };
}

hybrid_ptr<BaseType> UnnamedStruct::get_value_type() {
    return hybrid_ptr<BaseType> { this, false };
}

ValueType StructDefinition::value_type() const {
    return ValueType::Struct;
}