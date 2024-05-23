// Copyright (c) Qinetik 2024.

#include "AccessChain.h"
#include "ast/base/BaseType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"
#include "ast/types/PointerType.h"
#include "ast/values/FunctionCall.h"

void AccessChain::code_gen(Codegen &gen) {
    llvm_value(gen);
}

llvm::Type *AccessChain::llvm_type(Codegen &gen) {
    return values[values.size() - 1]->llvm_type(gen);
}

llvm::Value *AccessChain::llvm_value(Codegen &gen) {
    return values[values.size() - 1]->access_chain_value(gen, values);
}

llvm::Value *AccessChain::llvm_pointer(Codegen &gen) {
    return values[values.size() - 1]->access_chain_pointer(gen, values, values.size());
}

llvm::AllocaInst *AccessChain::llvm_allocate(Codegen &gen, const std::string &identifier) {
    return values[values.size() - 1]->access_chain_allocate(gen, identifier, this);
}

llvm::FunctionType *AccessChain::llvm_func_type(Codegen &gen) {
    return values[values.size() - 1]->llvm_func_type(gen);
}

bool AccessChain::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return values[values.size() - 1]->add_child_index(gen, indexes, name);
}

#endif

uint64_t AccessChain::byte_size(bool is64Bit) const {
    return values[values.size() - 1]->byte_size(is64Bit);
}

void AccessChain::declare_and_link(SymbolResolver &linker) {
    link(linker);
}

void AccessChain::link(SymbolResolver &linker) {
    values[0]->link(linker);
    if (values.size() > 1) {
        unsigned i = 1;
        while (i < values.size()) {
            values[i]->find_link_in_parent(values[i - 1].get(), linker);
            i++;
        }
    }
}

AccessChain::AccessChain(std::vector<std::unique_ptr<Value>> values) : values(std::move(values)) {

}

std::unique_ptr<BaseType> AccessChain::create_type() const {
    return values[values.size() - 1]->create_type();
}

std::unique_ptr<BaseType> AccessChain::create_value_type() {
    return create_type();
}

void AccessChain::accept(Visitor *visitor) {
    visitor->visit(this);
}

bool AccessChain::primitive() {
    return false;
}

bool AccessChain::reference() {
    return true;
}

void AccessChain::interpret(InterpretScope &scope) {
    auto v = evaluated_value(scope);
    if (v != nullptr && v->primitive()) {
        delete v;
    }
}

Value *AccessChain::parent(InterpretScope &scope) {
    Value *current = values[0].get();
    unsigned i = 1;
    while (i < (values.size() - 1)) {
        current = values[i]->find_in(scope, current);
        if (current == nullptr) {
            scope.error(
                    "(access chain) " + representation() + " child " + values[i]->representation() + " not found");
            return nullptr;
        }
        i++;
    }
    return current;
}

inline Value *AccessChain::parent_value(InterpretScope &scope) {
#ifdef DEBUG
    auto p = parent(scope);
    if (p == nullptr) {
        scope.error("parent is nullptr in access cain " + representation());
    } else if (p->evaluated_value(scope) == nullptr) {
        scope.error("evaluated value of parent is nullptr in access chain " + representation() + " pointer " +
                    p->representation());
    }
#endif
    return parent(scope)->evaluated_value(scope);
}

void AccessChain::set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) {
    if (values.size() <= 1) {
        values[0]->set_identifier_value(scope, rawValue, op);
    } else {
        values[values.size() - 1]->set_value_in(scope, parent_value(scope), rawValue->assignment_value(scope), op);
    }
}

std::string AccessChain::interpret_representation() const {
    return "[AccessChainInterpretRepresentation]";
}

Value *AccessChain::pointer(InterpretScope &scope) {
    if (values.size() <= 1) {
        return values[0].get();
    } else {
        return values[values.size() - 1]->find_in(scope, parent_value(scope));
    }
}

Value *AccessChain::evaluated_value(InterpretScope &scope) {
    return pointer(scope)->evaluated_value(scope);
}

Value *AccessChain::param_value(InterpretScope &scope) {
    return pointer(scope)->param_value(scope);
}

Value *AccessChain::initializer_value(InterpretScope &scope) {
    return pointer(scope)->initializer_value(scope);
}

Value *AccessChain::assignment_value(InterpretScope &scope) {
    return pointer(scope)->assignment_value(scope);
}

Value *AccessChain::return_value(InterpretScope &scope) {
    return pointer(scope)->return_value(scope);
}

ASTNode *AccessChain::linked_node() {
    return values[values.size() - 1]->linked_node();
}

ValueType AccessChain::value_type() const {
    return values[values.size() - 1]->value_type();
}

BaseTypeKind AccessChain::type_kind() const {
    return values[values.size() - 1]->type_kind();
}

std::string AccessChain::representation() const {
    std::string rep;
    int i = 0;
    while (i < values.size()) {
        rep.append(values[i]->representation());
        if (i != values.size() - 1) {
            rep.append(1, '.');
        }
        i++;
    }
    return rep;
}