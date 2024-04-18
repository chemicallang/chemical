// Copyright (c) Qinetik 2024.

#include "AccessChain.h"
#include "ast/base/BaseType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

void AccessChain::code_gen(Codegen &gen) {
    llvm_value(gen);
}

llvm::Value *AccessChain::llvm_value(Codegen &gen) {
    if(values.size() == 1) {
        return values[0]->llvm_value(gen);
    }
    return gen.builder->CreateLoad(values[values.size() - 1]->llvm_type(gen), llvm_pointer(gen), "acc");
}

llvm::Value *AccessChain::llvm_pointer(Codegen &gen) {
    if(values.size() == 1) {
        return values[0]->llvm_pointer(gen);
    } else {
        auto last = values[values.size() - 1].get();
        if(last->as_func_call() != nullptr) {
            gen.error("[TODO:] Generate code for a member function call !");
        } else {
            std::vector<llvm::Value*> idxList;

            // add member index of first value
            // if this is a index operator, only the integer index will be added since parent is nullptr
            if(!values[0]->add_member_index(gen, nullptr, idxList)) {
                gen.error("couldn't add member index for fragment '" + values[0]->representation() + "' in access chain '" + representation() + "'");
            }

            auto parent = values[0]->linked_node();
            unsigned i = 1;
            while (i < values.size()) {
                if(!values[i]->add_member_index(gen, parent, idxList)) {
                    gen.error("couldn't add member index for fragment '" + values[i]->representation() + "' in access chain '" + representation() + "'");
                }
                parent = values[i]->find_link_in_parent(parent);
                i++;
            }
            return gen.builder->CreateGEP(values[0]->llvm_type(gen), values[0]->llvm_pointer(gen), idxList);
        }
    }
}

#endif

void AccessChain::declare_and_link(SymbolResolver &linker) {
    link(linker);
}

void AccessChain::link(SymbolResolver &linker) {
    values[0]->link(linker);
    if (values.size() > 1) {
        auto parent = values[0]->linked_node();
        if (!parent) {
            linker.error("couldn't find fragment '" + values[0]->representation() + "' in access chain '" +
                         representation() + "'");
            return;
        }
        unsigned i = 1;
        while (i < values.size()) {
            parent = values[i]->find_link_in_parent(parent);
            if (!parent) {
                linker.error("couldn't find fragment '" + values[i]->representation() + "' in access chain '" +
                             representation() + "'");
                break;
            }
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

void AccessChain::accept(Visitor &visitor) {
    visitor.visit(this);
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