// Copyright (c) Chemical Language Foundation 2025.

#include "VariableIdentifier.h"
#include <memory>
#include "ast/values/AccessChain.h"
#include "ast/values/StructValue.h"
#include "ast/values/NullValue.h"
#include "ast/structures/EnumMember.h"
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

uint64_t VariableIdentifier::byte_size(TargetData& target) {
    return linked->byte_size(target);
}

void VariableIdentifier::process_linked() {
    if(linked->kind() == ASTNodeKind::FunctionDecl) {
        // if this is not set, function won't generate code (very important)
        // this doesn't account for recursion though, this identifier maybe present inside with linked function
        linked->as_function_unsafe()->set_has_usage(true);
    }
}

void VariableIdentifier::process_linked(ASTDiagnoser* diagnoser, FunctionTypeBody* curr_func) {
    switch(linked->kind()) {
        case ASTNodeKind::FunctionDecl:{
            // if this is not set, function won't generate code (very important)
            // this doesn't account for recursion though, this identifier maybe present inside with linked function
            const auto func = linked->as_function_unsafe();
            func->set_has_usage(true);
            if(!curr_func) {
                return;
            }
            switch(func->specifier()) {
                case AccessSpecifier::Public:
                    return;
                case AccessSpecifier::Private: {
                    // check both have same parents
                    const auto parent = func->parent();
                    const auto curr_parent = curr_func->get_parent();
                    if(parent != curr_parent) {
                        diagnoser->error(this) << "cannot access private function '" << value << '\'';
                    }
                    return;
                }
                case AccessSpecifier::Internal:{
                    // check in the same module
                    const auto funcMod = func->get_mod_scope();
                    const auto currMod = curr_func->get_parent()->get_mod_scope();
                    if(funcMod != currMod) {
                        diagnoser->error(this) << "cannot access internal function '" << value << '\'';
                    }
                    return;
                }
                case AccessSpecifier::Protected:
                    // TODO:
                    return;
            }
            break;
        }
        case ASTNodeKind::NamespaceDecl: {
            // TODO enable this, so we can check user uses :: for static access
            // linker.error("cannot link identifier with namespace " + value + "', Please use '::' to link with namespace", this);
            break;
        }
        case ASTNodeKind::StructMember: {
            const auto mem = linked->as_struct_member_unsafe();
            if(!curr_func) {
                return;
            }
            switch(mem->specifier()) {
                case AccessSpecifier::Public:
                    return;
                case AccessSpecifier::Private: {
                    // check have same parents
                    const auto parent = linked->parent();
                    const auto curr_parent = curr_func->get_parent();
                    if(parent != curr_parent) {
                        diagnoser->error(this) << "cannot access private member '" << value << "'";
                    }
                    return;
                }
                case AccessSpecifier::Internal:{
                    // check same modules
                    const auto module = curr_func->get_parent()->get_mod_scope();
                    const auto acc_scope = linked->get_mod_scope();
                    if(module != acc_scope) {
                        diagnoser->error(this) << "cannot access internal member '" << value << "' outside module";
                    }
                    return;
                }
                case AccessSpecifier::Protected:
                    // TODO:
                    return;
            }
            break;
        }
        default:
            return;
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

// will find value by this name in the parent
Value *VariableIdentifier::find_in(InterpretScope &scope, Value *parent) {
    return parent->child(scope, value);
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
    auto itr = scope.find_value_iterator(value);
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
    id->is_moved = is_moved;
    return id;
}

Value* VariableIdentifier::evaluated_value(InterpretScope &scope) {
    auto found = scope.find_value(value);
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
        } else if(linked_kind == ASTNodeKind::EnumMember) {
            const auto mem = linked->as_enum_member_unsafe();
            return mem->evaluate(scope.allocator, scope.global->typeBuilder, encoded_location());
        }
    }
    return this;
}