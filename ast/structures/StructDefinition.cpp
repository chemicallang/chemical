// Copyright (c) Qinetik 2024.

#include "StructMember.h"
#include "StructDefinition.h"
#include "FunctionDeclaration.h"
#include "ast/types/ReferencedType.h"

#ifdef COMPILER_BUILD

#include "ast/structures/InterfaceDefinition.h"

#include "compiler/llvmimpl.h"
#include "ast/values/IntValue.h"

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
    for (auto &function: functions) {
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
    return llvm::StructType::get(*gen.ctx, elements_type(gen));
}

#endif


StructMember::StructMember(
        std::string name,
        std::unique_ptr<BaseType> type,
        std::optional<std::unique_ptr<Value>> defValue
) : name(std::move(name)), type(std::move(type)), defValue(std::move(defValue)) {

}

void StructMember::accept(Visitor &visitor) {
    visitor.visit(this);
}

std::unique_ptr<BaseType> StructMember::create_value_type() {
    return std::unique_ptr<BaseType>(type->copy());
}

void StructMember::declare_and_link(SymbolResolver &linker) {
    type->link(linker);
    if (defValue.has_value()) {
        defValue.value()->link(linker);
    }
}

ASTNode *StructMember::child(const std::string &childName) {
    auto linked = type->linked_node();
    if (!linked) return nullptr;
    return linked->child(childName);
}

std::string StructMember::representation() const {
    std::string rep(name + " : " + type->representation());
    if (defValue.has_value()) {
        rep.append(defValue.value()->representation());
    }
    return rep;
}

StructDefinition::StructDefinition(
        std::string name,
        const std::optional<std::string> &overrides
) : name(std::move(name)) {
    if (overrides.has_value()) {
        this->overrides = std::make_unique<ReferencedType>(overrides.value());
    } else {
        this->overrides = std::nullopt;
    }
}

void StructDefinition::accept(Visitor &visitor) {
    visitor.visit(this);
}

void StructDefinition::declare_top_level(SymbolResolver &linker) {
    linker.declare(name, this);
}

void StructDefinition::declare_and_link(SymbolResolver &linker) {
    if (overrides.has_value()) {
        overrides.value()->link(linker);
    }
    MembersContainer::declare_and_link(linker);
}

StructDefinition *StructDefinition::as_struct_def() {
    return this;
}

void StructDefinition::interpret(InterpretScope &scope) {
    decl_scope = &scope;
}

void StructDefinition::interpret_scope_ends(InterpretScope &scope) {
    decl_scope = nullptr;
}

ASTNode *StructDefinition::child(const std::string &name) {
    auto node = MembersContainer::child(name);
    if (node) {
        return node;
    } else if (overrides.has_value()) {
        return overrides.value()->linked->child(name);
    };
    return nullptr;
}

#ifdef COMPILER_BUILD

std::vector<llvm::Type *> StructDefinition::elements_type(Codegen &gen) {
    auto vec = std::vector<llvm::Type *>();
    vec.reserve(variables.size());
    for (const auto &var: variables) {
        vec.push_back(var.second->llvm_type(gen));
    }
    return vec;
}

#endif

std::string StructDefinition::representation() const {
    std::string ret("struct " + name + " ");
    if (overrides.has_value()) {
        ret.append(": " + overrides.value()->representation() + " {\n");
    } else {
        ret.append("{\n");
    }
    ret.append(MembersContainer::representation());
    ret.append("\n}");
    return ret;
}