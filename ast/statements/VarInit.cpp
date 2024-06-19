// Copyright (c) Qinetik 2024.

#include "VarInit.h"
#include "compiler/SymbolResolver.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void VarInitStatement::code_gen(Codegen &gen) {
    if (gen.current_function == nullptr) {
        if (value.has_value()) {
            llvm_ptr = value.value()->llvm_global_variable(gen, is_const, identifier);
        } else {
            llvm_ptr = new llvm::GlobalVariable(*gen.module, llvm_type(gen), is_const, llvm::GlobalValue::LinkageTypes::PrivateLinkage, nullptr, identifier);
        }
    } else {
        if (value.has_value()) {
            llvm_ptr = value.value()->llvm_allocate(gen, identifier);
        } else {
            llvm_ptr = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
        }
    }
    gen.destruct_nodes.emplace_back(this);
}

void VarInitStatement::code_gen_destruct(Codegen &gen, Value* returnValue) {
    if(returnValue && returnValue->linked_node() == this) return;
    value.value()->llvm_destruct(gen);
}

llvm::Value *VarInitStatement::llvm_load(Codegen &gen) {
    if(is_const && value.has_value() && value.value()->value_type() == ValueType::String) {
        // global strings do not require loading
        return llvm_pointer(gen);
    }
    auto v = llvm_pointer(gen);
    return gen.builder->CreateLoad(llvm_type(gen), v, identifier);
}

bool VarInitStatement::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    if (value.has_value()) {
        return value.value()->add_child_index(gen, indexes, name);
    } else if (type.has_value()) {
        return type.value()->linked_node()->add_child_index(gen, indexes, name);
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
    return llvm_ptr;
}

llvm::Type *VarInitStatement::llvm_elem_type(Codegen &gen) {
    return value.has_value() ? value.value()->llvm_elem_type(gen) : nullptr;
}

llvm::Type *VarInitStatement::llvm_type(Codegen &gen) {
    check_has_type(gen);
    return type.has_value() ? type.value()->llvm_type(gen) : value.value()->llvm_type(gen);
}

llvm::FunctionType *VarInitStatement::llvm_func_type(Codegen &gen) {
    return type.has_value() ? type.value()->llvm_func_type(gen) : value.value()->llvm_func_type(gen);
}

#endif

VarInitStatement::VarInitStatement(
        bool is_const,
        std::string identifier,
        std::optional<std::unique_ptr<BaseType>> type,
        std::optional<std::unique_ptr<Value>> value
) : is_const(is_const), identifier(std::move(identifier)), type(std::move(type)), value(std::move(value)) {}

std::unique_ptr<BaseType> VarInitStatement::create_value_type() {
    if(type.has_value()) {
        return std::unique_ptr<BaseType>(type.value()->copy());
    } else {
        return value.value()->create_type();
    }
}

void VarInitStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

VarInitStatement *VarInitStatement::as_var_init() {
    return this;
}

ASTNode *VarInitStatement::child(const std::string &name) {
    if (type.has_value()) {
        return type.value()->linked_node()->child(name);
    } else if (value.has_value()) {
        return value.value()->linked_node()->child(name);
    }
    return nullptr;
}

void VarInitStatement::declare_and_link(SymbolResolver &linker) {
    linker.declare(identifier, this);
    if (type.has_value()) {
        type.value()->link(linker);
    }
    if (value.has_value()) {
        value.value()->link(linker, this);
    }
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

ValueType VarInitStatement::value_type() const {
    if(type.has_value()) {
        return type.value()->value_type();
    } else {
        return value.value()->value_type();
    }
}

BaseTypeKind VarInitStatement::type_kind() const {
    if(type.has_value()) {
        return type.value()->kind();
    } else {
        return value.value()->type_kind();
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