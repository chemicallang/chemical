// Copyright (c) Qinetik 2024.

#include "VariableIdentifier.h"
#include <memory>
#include "compiler/SymbolResolver.h"
#include "ast/values/AccessChain.h"
#include "ast/values/StructValue.h"
#include "ast/values/NullValue.h"

VarInitStatement* declaration(Value* value) {
    if(value->val_kind() == ValueKind::Identifier) {
        const auto linked = value->as_identifier_unsafe()->linked;
        return linked ? linked->as_var_init() : nullptr;
    } else {
        return nullptr;
    }
}

uint64_t VariableIdentifier::byte_size(bool is64Bit) {
    return linked->byte_size(is64Bit);
}

bool VariableIdentifier::link(SymbolResolver &linker, bool check_access) {
    linked = linker.find(value);
    if(linked) {
        if(check_access && linker.current_func_type) {
            // check for validity if accessible or assignable (because moved)
            linker.current_func_type->check_id(this, linker);
        }
        if(linked->as_namespace() && !is_ns){
            // TODO enable this, so we can check user uses :: for static access
//            linker.error("cannot link identifier with namespace " + value + "', Please use '::' to link with namespace", this);
        }
        return true;
    } else {
        linker.error("unresolved variable identifier '" + value.str() + "' not found", this);
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
            diagnoser->error("unresolved child '" + value.str() + "' in parent '" + parent->representation() + "'", this);
        }
    } else if (diagnoser){
        diagnoser->error("unresolved child '" + value.str() + "' because parent '" + parent->representation() + "' couldn't be resolved.", this);
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
        case ASTNodeKind::ExtensionFunctionDecl:
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::NamespaceDecl:
        case ASTNodeKind::UnionDecl:
        case ASTNodeKind::VariantDecl:
            return true;
        case ASTNodeKind::VarInitStmt:
            return linked->as_var_init_unsafe()->is_comptime();
        default:
            return false;
    }
}

Value *VariableIdentifier::child(InterpretScope &scope, const chem::string_view &name) {
    const auto eval = evaluated_value(scope);
    if(eval && eval != this) {
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

void VariableIdentifier::set_value_in(InterpretScope &scope, Value *parent, Value *next_value, Operation op, SourceLocation location) {
#ifdef DEBUG
    if (parent == nullptr) {
        scope.error("set_value_in in variable identifier, received null pointer to parent", parent);
        return;
    }
#endif
    parent->set_child_value(scope, value, next_value, op);
}

void VariableIdentifier::set_value(InterpretScope &scope, Value *rawValue, Operation op, SourceLocation passed_loc) {

    // iterator for previous value
    auto itr = scope.find_value_iterator(value.str());
    if(itr.first == itr.second.values.end()) {
        scope.error("couldn't find identifier '" + value.str() + "' in current scope", this);
        return;
    }

    // first we resolve the value in the current scope
    const auto evalNewValue = rawValue->evaluated_value(scope);
    // now we copy the value onto the scope of the previous value
    const auto newValue = evalNewValue->scope_value(itr.second);
//    auto newValue = rawValue->scope_value(itr.second);
    if (newValue == nullptr) {
        scope.error("trying to assign null ptr to identifier " + value.str(), this);
        return;
    }

    auto nextValue = newValue;

    if (op != Operation::Assignment) {

        // get the previous value, perform operation on it
        auto prevValue = itr.first->second;
        // TODO debug value being passed as this, it should be taken as a parameter
        nextValue = itr.second.evaluate(op, prevValue, newValue, passed_loc, this);

    }

    itr.first->second = nextValue;

}

VariableIdentifier* VariableIdentifier::copy(ASTAllocator& allocator) {
    const auto view = allocator.allocate_str(value.data(), value.size());
    auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(chem::string_view(view, value.size()), location);
    id->linked = linked;
    return id;
}

Value* VariableIdentifier::evaluated_value(InterpretScope &scope) {
    auto found = scope.find_value(value.str());
    if (found != nullptr) {
        return found;
    }
    auto linkedNode = linked_node();
    if(linkedNode) {
        const auto linked_kind = linkedNode->kind();
        if(linked_kind == ASTNodeKind::StructMember) {
            const auto found_self = scope.find_value("self");
            if(found_self) {
                return found_self->child(scope, value);
            }
        } else if(linked_kind == ASTNodeKind::VarInitStmt) {
            const auto init = linked->as_var_init_unsafe();
            if(init->is_const()) {
                return init->value;
            }
        }
    }
    return this;
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

BaseTypeKind VariableIdentifier::type_kind() const {
    return linked->type_kind();
}