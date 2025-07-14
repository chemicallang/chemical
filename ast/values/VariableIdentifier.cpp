// Copyright (c) Chemical Language Foundation 2025.

#include "VariableIdentifier.h"
#include <memory>
#include "ast/values/AccessChain.h"
#include "ast/values/StructValue.h"
#include "ast/values/NullValue.h"
#include "ast/types/VoidType.h"
#include "TypeInsideValue.h"

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

void VariableIdentifier::process_linked(ASTDiagnoser* linker) {
    const auto linkedKind = linked->kind();
    if(linkedKind == ASTNodeKind::FunctionDecl) {
        // if this is not set, function won't generate code (very important)
        // this doesn't account for recursion though, this identifier maybe present inside with linked function
        linked->as_function_unsafe()->set_has_usage(true);
    } else if(linkedKind == ASTNodeKind::NamespaceDecl && !is_ns){
        if(linker) {
            // TODO enable this, so we can check user uses :: for static access
//            linker.error("cannot link identifier with namespace " + value + "', Please use '::' to link with namespace", this);
        } else {
            // TODO complain that the diagnoser wasn't received
        }
    }
}

ASTNode* VariableIdentifier::linked_node() {
    return linked;
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
        const auto type = linked->known_type();
        return type ? type->copy(allocator) : new (allocator.allocate<VoidType>()) VoidType();;
    } else {
        return new (allocator.allocate<VoidType>()) VoidType();
    }
}

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
    auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(chem::string_view(view, value.size()), getType(), encoded_location(), is_ns);
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