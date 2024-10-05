// Copyright (c) Qinetik 2024.

#include "VariableIdentifier.h"

#include <memory>
#include "compiler/SymbolResolver.h"
#include "ast/values/AccessChain.h"
#include "ast/values/StructValue.h"

uint64_t VariableIdentifier::byte_size(bool is64Bit) {
    return linked->byte_size(is64Bit);
}

bool VariableIdentifier::link(SymbolResolver &linker, ChainValue*& value_ptr, bool prepend, bool check_access) {
    linked = linker.find(value);
    if(linked) {
        if(check_access && linker.current_func_type) {
            // check for validity if accessible or assignable (because moved)
            linker.current_func_type->check_id(this, linker);
        }
        if(prepend && linked->isAnyStructMember()) {
            if(!linker.current_func_type) {
                linker.error("couldn't link identifier with struct member / function, with name '" + value + '\'', this);
                return false;
            }
            auto self_param = linker.current_func_type->get_self_param();
            if(self_param) {
                return true;
            } else {
                auto decl = linker.current_func_type->as_function();
                if(!decl || !decl->has_annotation(AnnotationKind::Constructor) && !decl->has_annotation(AnnotationKind::CompTime)) {
                    linker.error("couldn't link identifier '" + value + "', because function doesn't take a self argument", this);
                }
            }
//        } else if((linked->as_interface_def() || linked->as_namespace() || linked->as_impl_def() || linked->as_struct_def()) && !can_link_with_namespace()) {
//            linker.error("cannot link identifier with definition '" + value + "', Please use '::' to link with definition");
//        }
        } else if(linked->as_namespace() && !is_ns){
            linker.error("cannot link identifier with namespace " + value + "', Please use '::' to link with namespace", this);
        } else {
            return true;
        }
    } else {
        linker.error("variable identifier '" + value + "' not found", this);
    }
    return false;
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
            diagnoser->error("couldn't find child '" + value + "' in parent '" + parent->representation() + "'", this);
        }
    } else if (diagnoser){
        diagnoser->error("couldn't find child '" + value + "' because parent '" + parent->representation() + "' couldn't be resolved.", this);
    }
    return false;
}

void VariableIdentifier::relink_parent(ChainValue *parent) {
    find_link_in_parent(parent, nullptr);
}

bool VariableIdentifier::find_link_in_parent(ChainValue *parent, SymbolResolver &resolver, BaseType *expected_type) {
    return find_link_in_parent(parent, &resolver);
}

bool VariableIdentifier::compile_time_computable() {
    if(!linked) return false;
    switch(linked->kind()) {
        case ASTNodeKind::FunctionDecl:
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::NamespaceDecl:
        case ASTNodeKind::UnionDecl:
        case ASTNodeKind::VariantDecl:
            return true;
        default:
            return false;
    }
}

Value *VariableIdentifier::child(InterpretScope &scope, const std::string &name) {
    const auto eval = evaluated_value(scope);
    if(eval) {
        return eval->child(scope, name);
    } else {
        return nullptr;
    }
}

BaseType* VariableIdentifier::known_type() {
    return linked ? linked->known_type() : nullptr;
}

// will find value by this name in the parent
Value *VariableIdentifier::find_in(InterpretScope &scope, Value *parent) {
    return parent->child(scope, value);
}

BaseType* VariableIdentifier::create_type(ASTAllocator& allocator) {
    if(linked) {
        return linked->create_value_type(allocator);
    } else {
        return nullptr;
    }
}

//hybrid_ptr<BaseType> VariableIdentifier::get_base_type() {
//    if(linked) {
//        return linked->get_value_type();
//    } else {
//        return hybrid_ptr<BaseType> { nullptr, false };
//    }
//}

bool VariableIdentifier::reference() {
    return true;
}

void VariableIdentifier::set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op) {
#ifdef DEBUG
    if (parent == nullptr) {
        scope.error("set_value_in in variable identifier, received null pointer to parent", parent);
    }
#endif
    parent->set_child_value(value, next_value, op);
}

void VariableIdentifier::set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) {

    // iterator for previous value
    auto itr = scope.find_value_iterator(value);
    if(itr.first == itr.second.values.end()) {
        scope.error("couldn't find identifier '" + value + "' in current scope", this);
        return;
    }

    // using the scope of the found value, so that it's initialized in the same scope
    auto newValue = rawValue->scope_value(itr.second);
    if (newValue == nullptr) {
        scope.error("trying to assign null ptr to identifier " + value, this);
        return;
    }

    // var init statement of current value, being assigned var x (<--- this one) = y
    auto var_init = declaration();
    if (var_init == nullptr) {
        scope.error("couldn't find declaration for identifier " + value, this);
        return;
    }

    // making one statement a reference
    if (rawValue->reference() && !newValue->primitive()) {
        // var init statement of value that owns the value var x = y (<---- this one)
        auto value_var_init = rawValue->declaration();
        if (value_var_init == nullptr) {
            scope.error("couldn't find declaration of the identifier " + rawValue->representation() +
                        " to assign to " + value, this);
            return;
        }
    }

    auto nextValue = newValue;

    // previous value doesn't exist, so it must only be a declaration above that is being assigned
    // preventing x += 1 (requires previous value)
    if (op != Operation::Assignment) {

        // get the previous value, perform operation on it
        auto prevValue = itr.first->second;
        nextValue = ExpressionEvaluators::ExpressionEvaluatorsMap.at(
            ExpressionEvaluators::index(prevValue->value_type(), prevValue->value_type(), op)
        )(itr.second, prevValue, newValue);

    }

    itr.first->second = nextValue;

}

VarInitStatement *VariableIdentifier::declaration() {
    if(!linked) return nullptr;
    return linked->as_var_init();
}

VariableIdentifier* VariableIdentifier::copy(ASTAllocator& allocator) {
    auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(value, token);
    id->linked = linked;
    return id;
}

Value* VariableIdentifier::evaluated_value(InterpretScope &scope) {
    auto linkedNode = linked_node();
    if(linkedNode) {
        const auto linked_kind = linkedNode->kind();
        if(linked_kind == ASTNodeKind::StructMember) {
            const auto found = scope.find_value("self");
            if(found) {
                return found->child(scope, value);
            }
            return nullptr;
        }
    }
    auto found = scope.find_value(value);
    if (found != nullptr) {
        return found;
    } else {
        return nullptr;
    }
}

//std::unique_ptr<Value> VariableIdentifier::create_evaluated_value(InterpretScope &scope) {
//    auto found = scope.find_value_iterator(value);
//    if(found.first != found.second.end()) {
//        auto take = found.first->second;
//        found.second.erase(found.first);
//        return std::unique_ptr<Value>(take);
//    } else {
//        return nullptr;
//    }
//}

Value* VariableIdentifier::evaluated_chain_value(InterpretScope &scope, Value* parent) {
    return parent ? parent->child(scope, value) : nullptr;
}

Value *VariableIdentifier::scope_value(InterpretScope &scope) {
    // evaluates the value, copies it
    auto val = scope.find_value_iterator(value);
    if (val.first == val.second.values.end() || val.first->second == nullptr) {
        return nullptr;
    }
    return val.first->second;
}

BaseTypeKind VariableIdentifier::type_kind() const {
    return linked->type_kind();
}

ValueType VariableIdentifier::value_type() const {
    return linked->value_type();
}