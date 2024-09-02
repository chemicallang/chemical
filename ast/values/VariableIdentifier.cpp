// Copyright (c) Qinetik 2024.

#include "NamespaceIdentifier.h"

#include <memory>
#include "compiler/SymbolResolver.h"
#include "ast/values/AccessChain.h"
#include "ast/values/StructValue.h"

uint64_t VariableIdentifier::byte_size(bool is64Bit) {
    return linked->byte_size(is64Bit);
}

void VariableIdentifier::prepend_self(SymbolResolver &linker, std::unique_ptr<ChainValue>& value_ptr, const std::string& name, ASTNode* linked) {
    // struct members / functions, don't need to be accessed like self.a or this.a
    // because we'll append self and this automatically
    auto self_id = new VariableIdentifier(name, token);
    self_id->linked = linked;
    std::vector<std::unique_ptr<ChainValue>> values;
    values.emplace_back(self_id);
    values.emplace_back(value_ptr.release());
    value_ptr = std::make_unique<AccessChain>(std::move(values), nullptr, false, token);
}

bool VariableIdentifier::link(SymbolResolver &linker, std::unique_ptr<ChainValue>& value_ptr, bool prepend) {
    linked = linker.find(value);
    if(linked) {
        if(prepend && linked->isAnyStructMember()) {
            if(!linker.current_func_type) {
                linker.error("couldn't link identifier with struct member / function, with name '" + value + '\'', this);
                return false;
            }
            auto self_param = linker.current_func_type->get_self_param();
            if(self_param) {
                prepend_self(linker, value_ptr, self_param->name, self_param);
                return true;
            } else {
                auto decl = linker.current_func_type->as_function();
                if(decl && decl->has_annotation(AnnotationKind::Constructor) && !decl->has_annotation(AnnotationKind::CompTime)) {
//                    auto found = linker.find("this");
//                    if(found) {
//                        prepend_self(linker, value_ptr, "this", found);
//                    } else {
//                        linker.error("couldn't find this in constructor for linking identifier '" + value + "'", this);
//                    }
                } else {
                    linker.error("couldn't link identifier '" + value + "', because function doesn't take a self argument", this);
                }
            }
//        } else if((linked->as_interface_def() || linked->as_namespace() || linked->as_impl_def() || linked->as_struct_def()) && !can_link_with_namespace()) {
//            linker.error("cannot link identifier with definition '" + value + "', Please use '::' to link with definition");
//        }
        } else if(linked->as_namespace() && !can_link_with_namespace()){
            linker.error("cannot link identifier with namespace " + value + "', Please use '::' to link with namespace", this);
        } else {
            return true;
        }
    } else {
        linker.error("variable identifier '" + value + "' not found", this);
    }
    return false;
}

bool VariableIdentifier::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    return link(linker, (std::unique_ptr<ChainValue> &) (value_ptr), true);
}

bool VariableIdentifier::link(
    SymbolResolver &linker,
    ChainValue *parent,
    std::vector<std::unique_ptr<ChainValue>> &values,
    unsigned int index,
    BaseType* expected_type
) {
    if(parent) {
        return find_link_in_parent(parent, linker);
    } else {
        return link(linker, values[index], false);
    }
}

ASTNode* VariableIdentifier::linked_node() {
    return linked;
}

bool VariableIdentifier::find_link_in_parent(ChainValue *parent, ASTDiagnoser *diagnoser) {
    parent_val = parent;
    auto linked_node = parent->linked_node();
    if(linked_node) {
        linked = linked_node->child(value);
        if(linked) {
            return true;
        } else if(diagnoser) {
            diagnoser->error("couldn't link child '" + value + "'", this);
        }
    } else if (diagnoser){
        diagnoser->error("couldn't link child '" + value + "' because parent '" + parent->representation() + "' couldn't be resolved.", this);
    }
    return false;
}

void VariableIdentifier::relink_parent(ChainValue *parent) {
    find_link_in_parent(parent, nullptr);
}

bool VariableIdentifier::find_link_in_parent(ChainValue *parent, SymbolResolver &resolver) {
    return find_link_in_parent(parent, &resolver);
}

Value *VariableIdentifier::child(InterpretScope &scope, const std::string &name) {
    const auto eval = evaluated_value(scope);
    if(eval.get()) {
        return eval->child(scope, name);
    } else {
        return nullptr;
    }
}

BaseType* VariableIdentifier::known_type() {
    return linked->known_type();
}

// will find value by this name in the parent
Value *VariableIdentifier::find_in(InterpretScope &scope, Value *parent) {
    return parent->child(scope, value);
}

std::unique_ptr<BaseType> VariableIdentifier::create_type() {
    if(linked) {
        return linked->create_value_type();
    } else {
        return nullptr;
    }
}

hybrid_ptr<BaseType> VariableIdentifier::get_base_type() {
    if(linked) {
        return linked->get_value_type();
    } else {
        return hybrid_ptr<BaseType> { nullptr, false };
    }
}

bool VariableIdentifier::reference() {
    return true;
}

void VariableIdentifier::set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op) {
#ifdef DEBUG
    if (parent == nullptr) {
        scope.error("set_value_in in variable identifier, received null pointer to parent");
    }
#endif
    parent->set_child_value(value, next_value, op);
}

void VariableIdentifier::set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) {

    auto newValue = rawValue->assignment_value(scope);
    if (newValue == nullptr) {
        scope.error("trying to assign null ptr to identifier " + value);
        return;
    }

    // var init statement of current value, being assigned var x (<--- this one) = y
    auto var_init = declaration();
    if (var_init == nullptr) {
        scope.error("couldn't find declaration for identifier " + value);
        return;
    }

    // making one statement a reference
    if (rawValue->reference() && !newValue->primitive()) {
        // var init statement of value that owns the value var x = y (<---- this one)
        auto value_var_init = rawValue->declaration();
        if (value_var_init == nullptr) {
            scope.error("couldn't find declaration of the identifier " + rawValue->representation() +
                        " to assign to " + value);
            return;
        }
    }

    // iterator for previous value
    auto itr = scope.find_value_iterator(value);

    auto nextValue = newValue;

    // previous value doesn't exist, so it must only be a declaration above that is being assigned
    // preventing x += 1 (requires previous value)
    if (op != Operation::Assignment) {

        // previous value iterator is required
        if (itr.first == itr.second.end()) {
            scope.error("couldn't set non-existent variable " + value + " with operation " + to_string(op));
            return;
        }

        // get the previous value, perform operation on it
        auto prevValue = itr.first->second;
        nextValue = ExpressionEvaluators::ExpressionEvaluatorsMap.at(
            ExpressionEvaluators::index(prevValue->value_type(), prevValue->value_type(), op)
        )(prevValue, newValue);

    }

    // delete previous value
    delete itr.first->second;

    if (itr.first == itr.second.end()) {
        var_init->declare(nextValue);
    } else {
        itr.first->second = nextValue;
    }

}

VarInitStatement *VariableIdentifier::declaration() {
    if(!linked) return nullptr;
    return linked->as_var_init();
}

VariableIdentifier* VariableIdentifier::copy() {
    auto id = new VariableIdentifier(value, token);
    id->linked = linked;
    return id;
}

NamespaceIdentifier *NamespaceIdentifier::copy() {
    auto id = new NamespaceIdentifier(value, token);
    id->linked = linked;
    return id;
}

hybrid_ptr<Value> VariableIdentifier::evaluated_value(InterpretScope &scope) {
    auto found = scope.find_value(value);
    if (found != nullptr) {
        return hybrid_ptr<Value> { found , false };
    } else {
        return hybrid_ptr<Value> { nullptr , false };
    }
}

std::unique_ptr<Value> VariableIdentifier::create_evaluated_value(InterpretScope &scope) {
    auto found = scope.find_value_iterator(value);
    if(found.first != found.second.end()) {
        auto take = found.first->second;
        found.second.erase(found.first);
        return std::unique_ptr<Value>(take);
    } else {
        return nullptr;
    }
}

hybrid_ptr<Value> VariableIdentifier::evaluated_chain_value(InterpretScope &scope, Value* parent) {
    if(parent) {
        return hybrid_ptr<Value> { parent->child(scope, value), false };
    }
    return hybrid_ptr<Value> { nullptr, false };
}

Value *VariableIdentifier::return_value(InterpretScope &scope) {
    // current identifier, holds the value, we find it
    auto val = scope.find_value_iterator(value);
    // check if not found
    if (val.first == val.second.end() || val.first->second == nullptr) {
        return nullptr;
    }
    auto store = val.first->second;
    val.second.erase(val.first);
    return store;
}

Value *VariableIdentifier::scope_value(InterpretScope &scope) {
    // evaluates the value, copies it
    auto val = scope.find_value_iterator(value);
    if (val.first == val.second.end() || val.first->second == nullptr) {
        return nullptr;
    }
    return val.first->second->copy();
}

BaseTypeKind VariableIdentifier::type_kind() const {
    return linked->type_kind();
}

ValueType VariableIdentifier::value_type() const {
    return linked->value_type();
}