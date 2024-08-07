// Copyright (c) Qinetik 2024.

#include "AccessChain.h"
#include "Variableidentifier.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/base/BaseType.h"
#include "ast/utils/ASTUtils.h"
#include "ast/structures/StructDefinition.h"

uint64_t AccessChain::byte_size(bool is64Bit) {
    return values[values.size() - 1]->byte_size(is64Bit);
}

void AccessChain::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    declare_and_link(linker);
}

void AccessChain::link_without_parent() {
    if (values.size() > 1) {
        unsigned i = 1;
        while (i < values.size()) {
            values[i]->find_link_in_parent(values[i - 1].get(), nullptr);
            i++;
        }
    }
}

void AccessChain::declare_and_link(SymbolResolver &linker) {

    values[0]->link(linker, nullptr, values, 0);

    // auto prepend self identifier, if not present and linked with struct member, anon union or anon struct
    auto linked = values[0]->linked_node();
    if(linked && (linked->as_struct_member() || linked->as_unnamed_union() || linked->as_unnamed_struct())) {
        if (!linker.current_func_type) {
            linker.error("couldn't link identifier with struct member / function, with name '" + values[0]->representation() + '\'');
            return;
        }
        auto self_param = linker.current_func_type->get_self_param();
        if (self_param) {
            auto self_id = new VariableIdentifier(self_param->name);
            self_id->linked = self_param;
            values.insert(values.begin(), std::unique_ptr<ChainValue>(self_id));
        } else {
            auto decl = linker.current_func_type->as_function();
            if(decl && decl->has_annotation(AnnotationKind::Constructor) && !decl->has_annotation(AnnotationKind::CompTime)) {
                auto found = linker.find("this");
                if(found) {
                    auto self_id = new VariableIdentifier("this");
                    self_id->linked = found;
                    values.insert(values.begin(), std::unique_ptr<ChainValue>(self_id));
                } else {
                    linker.error("couldn't find this in constructor for linking identifier '" + values[0]->representation() + "'");
                }
            } else {
                linker.error("couldn't link identifier '" + values[0]->representation() + "', because function doesn't take a self argument");
            }
        }
    }

    if (values.size() > 1) {
        unsigned i = 1;
        while (i < values.size()) {
            values[i]->link(linker, values[i - 1].get(), values, i);
            i++;
        }
    }
}

AccessChain::AccessChain(std::vector<std::unique_ptr<ChainValue>> values, ASTNode* parent_node, bool is_node) : values(std::move(values)), parent_node(parent_node), is_node(is_node) {

}

std::unique_ptr<BaseType> AccessChain::create_type() {
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(active);
    auto type = values[values.size() - 1]->create_type(values, values.size() - 1);
    restore_active_iterations(active);
    return type;
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

Value *AccessChain::copy() {
    auto chain = new AccessChain(parent_node, is_node);
    for(auto& value : values) {
        chain->values.emplace_back((ChainValue*) value->copy());
    }
    chain->link_without_parent();
    return chain;
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

void AccessChain::evaluate_children(InterpretScope &scope) {
    for(auto& value : values) {
        value->evaluate_children(scope);
    }
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
    if(values.size() == 1) return values[0]->evaluated_value(scope);
    hybrid_ptr<Value> evaluated = values[0]->evaluated_value(scope);
    unsigned i = 1;
    while(i < values.size()) {
        evaluated = values[i]->evaluated_chain_value(scope, evaluated.get());
        i++;
    }
    return evaluated;
}

std::unique_ptr<Value> AccessChain::create_evaluated_value(InterpretScope &scope) {
    if(values.size() == 1) return values[0]->create_evaluated_value(scope);
    auto evaluated = values[0]->evaluated_value(scope);
    unsigned i = 1;
    while(i < values.size()) {
        evaluated = values[i]->evaluated_chain_value(scope, evaluated.get());
        i++;
    }
    if(evaluated.get_will_free()) {
        return std::unique_ptr<Value>(evaluated.release());
    } else {
        return std::unique_ptr<Value>(evaluated->copy());
    }
}

Value *AccessChain::scope_value(InterpretScope &scope) {
    auto thing = evaluated_value(scope);
    if(thing.get_will_free()){
        return thing.release();
    } else {
        return thing->copy();
    }
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

Value* get_grandpa_value(std::vector<std::unique_ptr<ChainValue>> &chain_values, unsigned int index) {
    if(index - 2 < chain_values.size()) {
        return chain_values[index - 2].get();
    } else {
        return nullptr;
    }
}

std::pair<StructDefinition*, int16_t> get_grandpa_generic_struct(std::vector<std::unique_ptr<ChainValue>>& chain_values, unsigned int index) {
    if(index - 2 < chain_values.size()) {
        const auto linked = chain_values[index - 1]->linked_node();
        const auto func_decl = linked ? linked->as_function() : nullptr;
        if (func_decl && func_decl->as_extension_func() == nullptr) {
            const auto gran = get_grandpa_value(chain_values, index);
            // grandpa value can refer to a namespace, which is unable to create_type
            const auto gran_type = gran->create_type();
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

void AccessChain::set_generic_iterations(std::unordered_map<uint16_t, int16_t>& active_iterations) {
    uint16_t i = 0;
    for(auto& value : values) {
        // namespace cannot create type
        const auto type = value->create_type();
        if (type) {
            const auto type_itr = type->get_generic_iteration();
            if(type_itr != -1) {
                const auto generic_struct = type->get_generic_struct();
                if (generic_struct) {
                    active_iterations[i] = generic_struct->active_iteration;
                    generic_struct->set_active_iteration(type_itr);
                }
            }
        }
        i++;
    }
}

void AccessChain::restore_active_iterations(std::unordered_map<uint16_t, int16_t>& restore) {
    for(auto& pair : restore) {
        const auto& value = values[pair.first];
        const auto generic_struct = value->create_type()->get_generic_struct();
        generic_struct->set_active_iteration(pair.second);
    }
}