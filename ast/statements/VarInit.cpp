// Copyright (c) Qinetik 2024.

#include "VarInit.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void VarInitStatement::code_gen(Codegen &gen) {
    if (value.has_value()) {
        allocaInst = value.value()->llvm_allocate(gen, identifier);
    } else {
        allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
    }
}

bool VarInitStatement::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    if(value.has_value()) {
        return value.value()->add_child_index(gen, indexes, name);
    } else if(type.has_value()) {
        return type.value()->linked_node()->add_child_index(gen, indexes, name);
    }
    return false;
}

bool VarInitStatement::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, unsigned int index) {
    if(value.has_value()) {
        return value.value()->add_child_index(gen, indexes, index);
    } else if(type.has_value()) {
        return type.value()->linked_node()->add_child_index(gen, indexes, index);
    }
    return false;
}

inline void VarInitStatement::check_has_type(Codegen &gen) {
    if (!type.has_value() && !value.has_value()) {
        gen.error("neither variable type no variable value were given");
        return;
    }
}

llvm::Value *VarInitStatement::llvm_pointer(Codegen &gen) {
    return allocaInst;
}

llvm::Type *VarInitStatement::llvm_elem_type(Codegen &gen) {
    return value.has_value() ? value.value()->llvm_elem_type(gen) : nullptr;
}

llvm::Type *VarInitStatement::llvm_type(Codegen &gen) {
    check_has_type(gen);
    return value.has_value() ? value.value()->llvm_type(gen) : type.value()->llvm_type(gen);
}

#endif

VarInitStatement::VarInitStatement(
        bool is_const,
        std::string identifier,
        std::optional<std::unique_ptr<BaseType>> type,
        std::optional<std::unique_ptr<Value>> value
) : is_const(is_const), identifier(std::move(identifier)), type(std::move(type)), value(std::move(value)) {}

void VarInitStatement::accept(Visitor &visitor) {
    visitor.visit(this);
}

VarInitStatement *VarInitStatement::as_var_init() {
    return this;
}

ASTNode *VarInitStatement::child(const std::string &name) {
    if(type.has_value()) {
        return type.value()->linked_node()->child(name);
    } else if(value.has_value()) {
        return value.value()->linked_node()->child(name);
    }
    return nullptr;
}

void VarInitStatement::declare_and_link(ASTLinker &linker) {
    linker.current[identifier] = this;
    if(type.has_value()) {
        type.value()->link(linker);
    }
    if (value.has_value()) {
        value.value()->link(linker);
    }
}

void VarInitStatement::undeclare_on_scope_end(ASTLinker &linker) {
    linker.current.erase(identifier);
}

void VarInitStatement::interpret(InterpretScope &scope) {
    if (value.has_value()) {
        auto initializer = value.value()->initializer_value(scope);
        scope.declare(identifier, initializer);
        is_reference = initializer == nullptr || !initializer->primitive();
    }
    decl_scope = &scope;
}

/**
 * called by assignment to assign a new value in the scope that this variable was declared
 */
void VarInitStatement::declare(Value *new_value) {
    decl_scope->declare(identifier, new_value);
}

/**
 * called when the value associated with this var init has been moved
 */
void VarInitStatement::moved() {
    has_moved = true;
}

void VarInitStatement::interpret_scope_ends(InterpretScope &scope) {
    auto found = scope.find_value_iterator(identifier);
    if (found.first != found.second.end()) {
        if (!is_reference) {
            delete found.first->second;
        }
        found.second.erase(found.first);
    } else if (!has_moved) {
        scope.error("cannot clear non existent variable on the value map " + identifier);
    }
}

std::string VarInitStatement::representation() const {
    std::string rep;
    if (is_const) {
        rep.append("const ");
    } else {
        rep.append("var ");
    }
    rep.append(identifier);
    if (type.has_value()) {
        rep.append(" : ");
        rep.append(type.value()->representation());
    }
    if (value.has_value()) {
        rep.append(" = ");
        rep.append(value.value()->representation());
    }
    return rep;
}