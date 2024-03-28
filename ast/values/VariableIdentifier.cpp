// Copyright (c) Qinetik 2024.

#include "VariableIdentifier.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value *VariableIdentifier::arg_value(Codegen &gen, ASTNode *node) {
    auto param = node->as_parameter();
    if (param != nullptr && gen.current_function != nullptr) {
        for (const auto &arg: gen.current_function->args()) {
            if (arg.getArgNo() == param->index) {
                return (llvm::Value *) &arg;
            }
//                else {
//                    gen.error("no mismatch" + std::to_string(arg.getArgNo()) + " " + std::to_string(param->index));
//                }
        }
    }
//        else {
//            gen.error("param or function missing");
//        }
    return nullptr;
}

llvm::AllocaInst *VariableIdentifier::llvm_alloca(Codegen &gen) {
    auto found = gen.allocated.find(value);
    if (found == gen.allocated.end()) {
        gen.error("llvm_alloca called on variable identifier, couldn't locate the identifier : " + value);
        return nullptr;
    } else {
        return found->second;
    }
}

llvm::Value *VariableIdentifier::llvm_pointer(Codegen &gen) {
    return llvm_alloca(gen);
}

llvm::Value *VariableIdentifier::llvm_value(Codegen &gen) {
    auto resolved = resolve(gen);
    auto argVal = arg_value(gen, resolved);
    if (argVal != nullptr) {
        return argVal;
    }
    auto v = llvm_pointer(gen);
    return gen.builder->CreateLoad(resolved->llvm_type(gen), v, value);
}

ASTNode *VariableIdentifier::resolve(Codegen &gen) {
    auto found = gen.current.find(value);
    if (gen.current.end() == found) {
        gen.error("Couldn't find variable identifier in scope : " + value);
        throw std::runtime_error("couldn't find variable identifier in scope : " + value);
    } else {
        return found->second;
    }
}

#endif

Value *VariableIdentifier::child(InterpretScope &scope, const std::string &name) {
    return evaluated_value(scope)->child(scope, name);
}

// will find value by this name in the parent
Value *VariableIdentifier::find_in(InterpretScope &scope, Value *parent) {
    return parent->child(scope, value);
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
    auto var_init = declaration(scope);
    if (var_init == nullptr) {
        scope.error("couldn't find declaration for identifier " + value);
        return;
    }

    // making one statement a reference
    if (rawValue->reference() && !newValue->primitive()) {
        // var init statement of value that owns the value var x = y (<---- this one)
        auto value_var_init = rawValue->declaration(scope);
        if (value_var_init == nullptr) {
            scope.error("couldn't find declaration of the identifier " + rawValue->representation() +
                        " to assign to " + value);
            return;
        }
#ifdef DEBUG
        if (var_init->position == value_var_init->position) {
            scope.error("two init statements have the same position in AST, first : " + var_init->representation() +
                        " second : " + value_var_init->representation() +
                        ", triggered by assignment between their variables");
        }
#endif
        if (var_init->position > value_var_init->position) {
            // going down, declared above and now being assigned to declaration below (take reference / make decl below a reference)
            var_init->is_reference = true;
        } else {
            // going up, declared below and now being assigned to declaration above (move the value / make decl above a reference)
            value_var_init->is_reference = true;
            var_init->is_reference = false;
        }
    } else {
        var_init->is_reference = false;
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
        nextValue = scope.global->expr_evaluators[
                ExpressionEvaluator::index(prevValue->value_type(), prevValue->value_type(), op)
        ](prevValue, newValue);

    }

    // delete previous value if its a primitive / not a reference & exists
    if (itr.first != itr.second.end() && (!var_init->is_reference || itr.first->second->primitive())) {
        delete itr.first->second;
    }

    if (itr.first == itr.second.end()) {
        var_init->declare(nextValue);
    } else {
        itr.first->second = nextValue;
    }

}

VarInitStatement *VariableIdentifier::declaration(InterpretScope &scope) {
    auto node = scope.find_node(value);
    if (node != nullptr) {
        return node->as_var_init();
    }
    return nullptr;
}

Value *VariableIdentifier::evaluated_value(InterpretScope &scope) {
    auto found = scope.find_value(value);
    if (found != nullptr) {
        return found;
    } else {
        return nullptr;
    }
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
    // TODO this will only move a single identifier
    // TODO this won't move struct children !
    auto decl = declaration(scope);
    if (decl) {
        decl->moved();
    } else {
        scope.error("couldn't find var init for moving the value");
    }
    return store;
}

Value* VariableIdentifier::copy_prim_ref_other(InterpretScope& scope) {
    // evaluates the value, if its primitive copies it
    // otherwise, we pass another reference to the value, in the function calls
    auto val = scope.find_value_iterator(value);
    if (val.first == val.second.end() || val.first->second == nullptr) {
        return nullptr;
    }
    if (val.first->second->primitive()) {
        return val.first->second->copy(scope);
    } else {
        return val.first->second;
    }
}

Value *VariableIdentifier::param_value(InterpretScope &scope) {
    return copy_prim_ref_other(scope);
}

Value *VariableIdentifier::initializer_value(InterpretScope &scope) {
    return copy_prim_ref_other(scope);
}

Value *VariableIdentifier::assignment_value(InterpretScope &scope) {
    return copy_prim_ref_other(scope);
}

std::string VariableIdentifier::representation() const {
    return value;
}