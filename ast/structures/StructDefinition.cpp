// Copyright (c) Qinetik 2024.

#include "StructMember.h"
#include "StructDefinition.h"
#include "FunctionDeclaration.h"
#include "ast/types/ReferencedType.h"

#ifdef COMPILER_BUILD

#include "ast/structures/InterfaceDefinition.h"

#include "compiler/llvmimpl.h"

void StructDefinition::code_gen(Codegen &gen) {
    for(auto& function : functions) {
        if(overrides.has_value()) {
            auto interface = (InterfaceDefinition*) overrides.value()->linked_node();
            if(!interface->has_implemented(function.second->name)) {
                auto overridden = interface->child(function.second->name);
                if (overridden) {
                    auto fn = overridden->as_function();
                    if (fn) {
                        fn->code_gen_override(gen, function.second.get());
                        interface->set_implemented(function.second->name, true);
                        continue;
                    }
                }
            } else {
                gen.error("Function '" + function.second->name + "' in interface '" + interface->name + "' has already been implemented, Failure in Struct '" + name + "'", function.second.get());
                continue;
            }
        }
        function.second->code_gen_struct(gen);
    }
}

llvm::Type* StructMember::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

bool StructMember::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &childName) {
    auto linked = type->linked_node();
    if(!linked) return false;
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
    if(defValue.has_value()) {
        defValue.value()->link(linker);
    }
}

ASTNode *StructMember::child(const std::string &childName) {
    auto linked = type->linked_node();
    if(!linked) return nullptr;
    linked->child(childName);
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
        const std::optional<std::string>& overrides
) : name(std::move(name)) {
    if(overrides.has_value()) {
        this->overrides = std::make_unique<ReferencedType>(overrides.value());
    } else {
        this->overrides = std::nullopt;
    }
}

void StructDefinition::accept(Visitor &visitor) {
    visitor.visit(this);
}

void StructDefinition::declare_top_level(SymbolResolver &linker) {
    linker.current[name] = this;
}

void StructDefinition::declare_and_link(SymbolResolver &linker) {
    if(overrides.has_value()) {
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

ASTNode* StructDefinition::child(const std::string &name) {
    auto node = MembersContainer::child(name);
    if(node) {
        return node;
    } else if(overrides.has_value()) {
        return overrides.value()->linked->child(name);
    };
}

#ifdef COMPILER_BUILD

std::vector<llvm::Type *> StructDefinition::elements_type(Codegen &gen) {
    auto vec = std::vector<llvm::Type *>();
    vec.reserve(variables.size());
    // TODO this doesn't work, a crash happens at variables
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