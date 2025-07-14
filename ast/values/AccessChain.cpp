// Copyright (c) Chemical Language Foundation 2025.

#include "AccessChain.h"
#include "VariableIdentifier.h"
#include "FunctionCall.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/base/BaseType.h"
#include "ast/utils/ASTUtils.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/IndexOperator.h"
#include "ast/base/ASTAllocator.h"

uint64_t AccessChain::byte_size(bool is64Bit) {
    return values[values.size() - 1]->byte_size(is64Bit);
}

void AccessChain::relink_parent() {
    const auto values_size = values.size();
    if (values_size > 1) {
        unsigned i = 1;
        while (i < values_size) {
            if(!values[i]->as_identifier_unsafe()->find_link_in_parent(values[i - 1], nullptr)) {
                break;
            }
            i++;
        }
    }
}

BaseType* AccessChain::create_type(ASTAllocator& allocator) {
    return values[values.size() - 1]->create_type(allocator);
}

BaseType* AccessChain::known_type() {
    return values[values.size() - 1]->known_type();
}

bool AccessChain::primitive() {
    return false;
}

bool AccessChain::compile_time_computable() {
    if(values.size() == 1) {
        return values[0]->compile_time_computable();
    }
    // first value should always be compile time computable
    // a.b <--- a should be a compile time computable var init
    // 'b' here is a member of struct, where 'a' is the struct value
    // 'b' doesn't need to be compile time computable
    if(!values.front()->compile_time_computable()) {
        return false;
    }
    // nested functions should also be compile time computable
    // a.b() <--- b should be compile time computable
    // a.b().c() <--- c should also be compile time computable
    // one day we'd allow c to be not compile time computable, so if 'b' returns a struct
    // at compile time, 'c' can process it
    for(const auto value : values) {
        const auto val_kind = value->val_kind();
        if(val_kind == ValueKind::FunctionCall && !value->compile_time_computable()) {
            return false;
        }
    }
    return true;
}

AccessChain *AccessChain::copy(ASTAllocator& allocator) {
    auto chain = new (allocator.allocate<AccessChain>()) AccessChain(is_node(), getType(), encoded_location());
    for(auto& value : values) {
        chain->values.emplace_back((ChainValue*) value->copy(allocator));
    }
    return chain;
}

Value *AccessChain::parent(InterpretScope &scope) {
    Value *current = values[0];
    unsigned i = 1;
    while (i < (values.size() - 1)) {
        current = values[i]->find_in(scope, current);
        if (current == nullptr) {
            scope.error("(access chain) " + Value::representation() + " child " + values[i]->representation() + " not found", (ASTNode*) this);
            return nullptr;
        }
        i++;
    }
    return current;
}

inline Value* AccessChain::parent_value(InterpretScope &scope) {
#ifdef DEBUG
    auto p = parent(scope);
    if (p == nullptr) {
        scope.error("parent is nullptr in access cain " + Value::representation(), (ASTNode*) this);
    } else if (p->evaluated_value(scope) == nullptr) {
        scope.error("evaluated value of parent is nullptr in access chain " + Value::representation() + " pointer " +
                    p->representation(), (ASTNode*) this);
    }
#endif
    return parent(scope)->evaluated_value(scope);
}

void AccessChain::set_value(InterpretScope &scope, Value *rawValue, Operation op, SourceLocation passed_loc) {
    if (values.size() <= 1) {
        values[0]->set_value(scope, rawValue, op, passed_loc);
    } else {
        auto parent = parent_value(scope);
        values[values.size() - 1]->set_value_in(scope, parent, rawValue->scope_value(scope), op, passed_loc);
    }
}

Value *AccessChain::pointer(InterpretScope &scope) {
    if (values.size() <= 1) {
        return values[0];
    } else {
        auto parent = parent_value(scope);
        return values[values.size() - 1]->find_in(scope, parent);
    }
}

void copy_from(ASTAllocator& allocator, std::vector<ChainValue*>& destination, std::vector<ChainValue*>& source, unsigned from) {
    const auto size = source.size();
    while(from < size) {
        const auto value = source[from];
        destination.emplace_back((ChainValue*) value->copy(allocator));
        from++;
    }
}

Value* evaluate_it(ChainValue* value, InterpretScope& scope, Value* evaluated) {
    const auto kind = value->val_kind();
    if(kind == ValueKind::Identifier) {
        const auto id = value->as_identifier_unsafe();
        return evaluated ? evaluated->child(scope, id->value) : nullptr;
    } else {
        return value->evaluated_value(scope);
    }
}

// evaluate the chain partially if you have evaluated the chain till given index i
// or receive a copy of the chain with values that could be evaluated
Value* evaluate_from(std::vector<ChainValue*>& values, InterpretScope& scope, Value* evaluated, unsigned i) {
    while(i < values.size()) {
        const auto next = evaluate_it(values[i], scope, evaluated);
        // suppose we can't evaluate next value, in chain a.b.c we could evaluate a
        // but b.c we couldn't evaluate, what we do is we create a new chain a.b.c (a is evaluated) (b.c are copies)
        // we relink the parent of b.c so they know the parent has changed to a
        if(next == nullptr && evaluated && evaluated->as_chain_value()) {

            const auto duplicate = new (scope.allocate<AccessChain>()) AccessChain(false, evaluated->encoded_location());
            duplicate->values.emplace_back((ChainValue*) evaluated);
            copy_from(scope.allocator, duplicate->values, values, i);
            duplicate->relink_parent();
            return duplicate;

        } else {
            evaluated = next;
        }
        i++;
    }
    return evaluated;
}

Value* AccessChain::evaluated_value(InterpretScope &scope) {
    if(values.size() == 1) return values[0]->evaluated_value(scope);
    Value* evaluated = values[0]->evaluated_value(scope);
    return evaluate_from(values, scope, evaluated, 1);
}

ASTNode *AccessChain::linked_node() {
    return values[values.size() - 1]->linked_node();
}