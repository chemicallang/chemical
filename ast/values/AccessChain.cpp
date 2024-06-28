// Copyright (c) Qinetik 2024.

#include "AccessChain.h"
#include "Variableidentifier.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/BaseType.h"

uint64_t AccessChain::byte_size(bool is64Bit) {
    return values[values.size() - 1]->byte_size(is64Bit);
}

void AccessChain::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    declare_and_link(linker);
}

void AccessChain::declare_and_link(SymbolResolver &linker) {
    values[0]->link(linker, values[0]);
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

std::unique_ptr<BaseType> AccessChain::create_type() {
    return values[values.size() - 1]->create_type();
}

hybrid_ptr<BaseType> AccessChain::get_base_type() {
    return values[values.size() - 1]->get_base_type();
}

hybrid_ptr<BaseType> AccessChain::get_value_type() {
    return values[values.size() - 1]->get_base_type();
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
    evaluated_value(scope);
}

Value *AccessChain::parent(InterpretScope &scope) {
    Value *current = values[0].get();
    unsigned i = 1;
    while (i < (values.size() - 1)) {
        current = values[i]->find_in(scope, current);
        if (current == nullptr) {
            scope.error(
                    "(access chain) " + Value::representation() + " child " + values[i]->representation() + " not found");
            return nullptr;
        }
        i++;
    }
    return current;
}

inline hybrid_ptr<Value> AccessChain::parent_value(InterpretScope &scope) {
#ifdef DEBUG
    auto p = parent(scope);
    if (p == nullptr) {
        scope.error("parent is nullptr in access cain " + Value::representation());
    } else if (p->evaluated_value(scope).get() == nullptr) {
        scope.error("evaluated value of parent is nullptr in access chain " + Value::representation() + " pointer " +
                    p->representation());
    }
#endif
    return parent(scope)->evaluated_value(scope);
}

void AccessChain::set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) {
    if (values.size() <= 1) {
        values[0]->set_identifier_value(scope, rawValue, op);
    } else {
        auto parent = parent_value(scope);
        values[values.size() - 1]->set_value_in(scope, parent.get(), rawValue->assignment_value(scope), op);
    }
}

Value *AccessChain::pointer(InterpretScope &scope) {
    if (values.size() <= 1) {
        return values[0].get();
    } else {
        auto parent = parent_value(scope);
        return values[values.size() - 1]->find_in(scope, parent.get());
    }
}

hybrid_ptr<Value> AccessChain::evaluated_value(InterpretScope &scope) {
    return pointer(scope)->evaluated_value(scope);
}

Value *AccessChain::scope_value(InterpretScope &scope) {
    return pointer(scope)->scope_value(scope);
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