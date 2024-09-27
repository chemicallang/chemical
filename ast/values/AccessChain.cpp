// Copyright (c) Qinetik 2024.

#include "AccessChain.h"
#include "Variableidentifier.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/base/BaseType.h"
#include "ast/utils/ASTUtils.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/VariantCall.h"
#include "ast/base/ASTAllocator.h"

uint64_t AccessChain::byte_size(bool is64Bit) {
    return values[values.size() - 1]->byte_size(is64Bit);
}

bool AccessChain::find_link_in_parent(ChainValue *parent, SymbolResolver &resolver) {
    throw std::runtime_error("AccessChain doesn't support find_link_in_parent, because it can't be embedded in itself");
}

void AccessChain::relink_parent() {
    if (values.size() > 1) {
        unsigned i = 1;
        while (i < values.size()) {
            values[i]->relink_parent(values[i - 1]);
            i++;
        }
    }
}

bool AccessChain::link(SymbolResolver &linker, BaseType *expected_type, Value** value_ptr, unsigned int end_offset) {

    if(!values[0]->link(linker, nullptr, values, 0, expected_type)) {
        return false;
    }
    values[0]->set_generic_iteration(linker.allocator);

    // auto prepend self identifier, if not present and linked with struct member, anon union or anon struct
    auto linked = values[0]->linked_node();
    if(linked && (linked->as_struct_member() || linked->as_unnamed_union() || linked->as_unnamed_struct())) {
        if (!linker.current_func_type) {
            linker.error("couldn't link identifier with struct member / function, with name '" + values[0]->representation() + '\'', values[0]);
            return false;
        }
        auto self_param = linker.current_func_type->get_self_param();
        if (!self_param) {
            auto decl = linker.current_func_type->as_function();
            if(!decl || !decl->has_annotation(AnnotationKind::Constructor) && !decl->has_annotation(AnnotationKind::CompTime)) {
                linker.error("couldn't link identifier '" + values[0]->representation() + "', because function doesn't take a self argument", values[0]);
                return false;
            }
        }
    }

    const auto values_size = values.size() - end_offset;
    if (values_size > 1) {

        // manually linking the second value
        if(!values[1]->link(linker, values[0], values, 1, expected_type)) {
            return false;
        }

        // if second value is linked with a variant member, we replace the access chain, with variant call
        if(value_ptr && values[1]->as_identifier()) {
            linked = values[1]->linked_node();
            if (linked && linked->as_variant_member()) {
                auto& chain = *value_ptr;
                const auto ac_chain = (AccessChain*) chain;
                chain = new (linker.ast_allocator->allocate<VariantCall>()) VariantCall(ac_chain, token);
                ((std::unique_ptr<VariantCall>&) chain)->link(linker, chain, expected_type);
                return true;
            }
        }

        unsigned i = 2;
        while (i < values_size) {
            if(!values[i]->link(linker, values[i - 1], values, i, expected_type)) {
                return false;
            }
            i++;
        }

    }

    return true;
}

void AccessChain::declare_and_link(SymbolResolver &linker) {
    link(linker, (BaseType*) nullptr, nullptr);
}

bool AccessChain::link(SymbolResolver &linker, Value*& value_ptr, BaseType *type) {
    return link(linker, type, &value_ptr);
}

AccessChain::AccessChain(ASTNode* parent_node, bool is_node, CSTToken* token) : parent_node(parent_node), is_node(is_node), token(token) {

}

AccessChain::AccessChain(std::vector<ChainValue*> values, ASTNode* parent_node, bool is_node, CSTToken* token) : values(std::move(values)), parent_node(parent_node), is_node(is_node), token(token) {

}

BaseType* AccessChain::create_type(ASTAllocator& allocator) {
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(allocator, active);
    auto type = values[values.size() - 1]->create_type(allocator);
    if(type) {
        const auto pure = type->pure_type();
        if (pure && type != pure) {
            type = pure;
        }
        restore_active_iterations(active);
    }
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

BaseType* AccessChain::create_value_type(ASTAllocator& allocator) {
    return create_type(allocator);
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

bool AccessChain::compile_time_computable() {
    for(auto& value : values) {
        if(!value->compile_time_computable()) {
            return false;
        }
    }
    return true;
}

AccessChain *AccessChain::copy(ASTAllocator& allocator) {
    auto chain = new (allocator.allocate<AccessChain>()) AccessChain(parent_node, is_node, token);
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

inline Value* AccessChain::parent_value(InterpretScope &scope) {
#ifdef DEBUG
    auto p = parent(scope);
    if (p == nullptr) {
        scope.error("parent is nullptr in access cain " + Value::representation());
    } else if (p->evaluated_value(scope) == nullptr) {
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
        values[values.size() - 1]->set_value_in(scope, parent, rawValue->scope_value(scope), op);
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

Value* AccessChain::evaluated_value(InterpretScope &scope) {
    if(values.size() == 1) return values[0]->evaluated_value(scope);
    Value* evaluated = values[0]->evaluated_value(scope);
    unsigned i = 1;
    while(i < values.size()) {
        auto next = values[i]->evaluated_chain_value(scope, evaluated);
        if(next == nullptr && evaluated && evaluated->as_chain_value() && i == 1) {
            auto c = (AccessChain*) copy(scope.allocator);
            c->values[0] = (ChainValue*) evaluated;
            c->relink_parent();
            return c;
        } else {
            evaluated = next;
        }
        i++;
    }
    return evaluated;
}

Value *AccessChain::scope_value(InterpretScope &scope) {
    return evaluated_value(scope);
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

ChainValue* get_grandpa_value(std::vector<ChainValue*> &chain_values, unsigned int index) {
    if(index - 2 < chain_values.size()) {
        return chain_values[index - 2];
    } else {
        return nullptr;
    }
}

std::pair<StructDefinition*, int16_t> get_grandpa_generic_struct(ASTAllocator& allocator, std::vector<ChainValue*>& chain_values, unsigned int index) {
    if(index - 2 < chain_values.size()) {
        const auto linked = chain_values[index - 1]->linked_node();
        const auto func_decl = linked ? linked->as_function() : nullptr;
        if (func_decl && func_decl->as_extension_func() == nullptr) {
            const auto gran = get_grandpa_value(chain_values, index);
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

void AccessChain::set_generic_iterations(ASTAllocator& allocator, std::unordered_map<uint16_t, int16_t>& active_iterations) {
    uint16_t i = 0;
    for(auto& value : values) {
        // namespace cannot create type
        const auto prev_itr = value->set_generic_iteration(allocator);
        if(prev_itr > -2) {
            active_iterations[i] = prev_itr;
        }
        i++;
    }
}

void AccessChain::restore_active_iterations(std::unordered_map<uint16_t, int16_t>& restore) {
    for(auto& pair : restore) {
        const auto& value = values[pair.first];
        const auto type = value->known_type();
        const auto members_container = type->linked_node()->as_members_container();
        members_container->set_active_iteration(pair.second);
    }
}