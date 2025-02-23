// Copyright (c) Qinetik 2024.

#include "AccessChain.h"
#include "VariableIdentifier.h"
#include "FunctionCall.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/base/BaseType.h"
#include "ast/utils/ASTUtils.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/VariantCall.h"
#include "ast/values/IndexOperator.h"
#include "ast/base/ASTAllocator.h"

uint64_t AccessChain::byte_size(bool is64Bit) {
    return values[values.size() - 1]->byte_size(is64Bit);
}

bool AccessChain::find_link_in_parent(ChainValue *parent, SymbolResolver &resolver, BaseType *expected_type) {
    throw std::runtime_error("AccessChain doesn't support find_link_in_parent, because it can't be embedded in itself");
}

void AccessChain::fix_generic_iteration(ASTDiagnoser& diagnoser, BaseType* expected_type) {
    unsigned i = 0;
    const auto size = values.size();
    while(i < size) {
        const auto func_call = values[i]->as_func_call();
        if(func_call) {
            func_call->fix_generic_iteration(diagnoser, i == size - 1 ? expected_type : nullptr);
        }
        i++;
    }
}

void AccessChain::relink_parent() {
    // TODO remove this method, relinking parent is not required as we store the parent val nested in values
    if (values.size() > 1) {
        unsigned i = 1;
        while (i < values.size()) {
            values[i]->relink_parent(values[i - 1]);
            i++;
        }
    }
}

// for easier invocation
// type is only passed to the last value in the chain
bool link_at(std::vector<ChainValue*>& values, unsigned int index, SymbolResolver& linker, BaseType* expected_type) {
    return values[index]->link(linker, values, index, index == values.size() - 1 ? expected_type : nullptr);
}

bool AccessChain::link(SymbolResolver &linker, BaseType *expected_type, Value** value_ptr, unsigned int end_offset, bool check_validity, bool assign) {

    if(!values[0]->link(linker, values, 0, values.size() == 1 ? expected_type : nullptr)) {
        return false;
    }

    // auto prepend self identifier, if not present and linked with struct member, anon union or anon struct
    auto linked = values[0]->linked_node();
    if(linked) {
        const auto linked_kind = linked->kind();
        if(linked_kind == ASTNodeKind::StructMember || linked_kind == ASTNodeKind::UnnamedUnion || linked_kind == ASTNodeKind::UnnamedStruct) {
            if (!linker.current_func_type) {
                linker.error("couldn't link identifier with struct member / function, with name '" + values[0]->representation() + '\'', values[0]);
                return false;
            }
            auto self_param = linker.current_func_type->get_self_param();
            if (!self_param) {
                auto decl = linker.current_func_type->as_function();
                if (!decl || !decl->is_constructor_fn() && !decl->is_comptime()) {
                    linker.error("couldn't link identifier '" + values[0]->representation() + "', because function doesn't take a self argument", values[0]);
                    return false;
                }
            }
        }
    }

    if(values.size() == 1) {
        return true;
    }

    std::vector<int16_t> iterations;

    values[0]->set_generic_iteration(iterations, linker.allocator);

    const auto values_size = values.size() - end_offset;
    if (values_size > 1) {
        unsigned i = 1;
        while (i < values_size) {
            if(!link_at(values, i, linker, expected_type)) {
                return false;
            }
            i++;
        }
    }

    if(check_validity && linker.current_func_type) {
        // check chain for validity, if it's moved or members have been moved
        linker.current_func_type->check_chain(this, assign, linker);
    }

    return true;
}

BaseType* AccessChain::create_type(ASTAllocator& allocator) {
    const auto values_size = values.size();
    if(values_size == 1) {
        return values[0]->create_type(allocator);
    }
    std::vector<int16_t> active;
    set_generic_iteration(active, allocator);
    const auto type = values[values.size() - 1]->create_type(allocator);
    if(type) {
        const auto pure = type->pure_type();
        if(pure && type != pure) {
            restore_generic_iteration(active, allocator);
            return pure;
        }
    }
    restore_generic_iteration(active, allocator);
    return type;
}

//hybrid_ptr<BaseType> AccessChain::get_base_type() {
//    return values[values.size() - 1]->get_base_type();
//}

//hybrid_ptr<BaseType> AccessChain::get_value_type() {
//    return values[values.size() - 1]->get_base_type();
//}

BaseType* AccessChain::known_type() {
    return values[values.size() - 1]->known_type();
}

void AccessChain::accept(Visitor *visitor) {
    visitor->visit(this);
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
    auto chain = new (allocator.allocate<AccessChain>()) AccessChain(is_node(), encoded_location());
    for(auto& value : values) {
        chain->values.emplace_back((ChainValue*) value->copy(allocator));
    }
    chain->relink_parent();
    return chain;
}

void AccessChain::interpret(InterpretScope &scope) {
    evaluated_value(scope);
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

BaseTypeKind AccessChain::type_kind() const {
    return values[values.size() - 1]->type_kind();
}

std::pair<StructDefinition*, int16_t> get_grandpa_generic_struct(ASTAllocator& allocator, ChainValue* parent_val) {
    const auto linked = parent_val->linked_node();
    if(!linked) return { nullptr, -1 };
    const auto linked_kind = linked->kind();
    if(linked_kind == ASTNodeKind::FunctionDecl) {
        const auto func_decl = linked->as_function_unsafe();
        const auto gran = get_parent_from(parent_val);
        if(gran) {
            // grandpa value can refer to a namespace, which is unable to create_type
            const auto gran_type = gran->create_type(allocator);
            if (gran_type) {
                const auto generic_struct = gran_type->get_generic_struct();
                if (generic_struct) {
                    return {generic_struct, gran_type->get_generic_iteration()};
                }
            }
        }
    }
    return { nullptr, -1 };
}