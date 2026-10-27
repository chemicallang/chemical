// Copyright (c) Chemical Language Foundation 2025.

#include "VariableIdentifier.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/StructMember.h"
#include <memory>
#include "compiler/lab/LabBuildCompiler.h"
#include "ast/structures/EnumMember.h"
#include "ast/structures/Namespace.h"

VarInitStatement* declaration(Value* value) {
    if(Value::isIdentifier(value->val_kind())) {
        const auto linked = value->as_identifier_unsafe()->linked;
        return linked ? linked->as_var_init() : nullptr;
    } else {
        return nullptr;
    }
}

uint64_t VariableIdentifier::byte_size(const TargetData& target) {
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
                        diagnoser->info(func) << "private function being accessed";
                    }
                    return;
                }
                case AccessSpecifier::Internal:{
                    // check in the same module
                    const auto funcMod = func->get_mod_scope();
                    const auto currMod = curr_func->get_parent()->get_mod_scope();
                    if(funcMod != currMod) {
                        diagnoser->error(this) << "cannot access internal function '" << value << '\'';
                        diagnoser->info(func) << "internal function being accessed";
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
    // Handle namespace member lookup for qualified access (e.g., ns::thing)
    if(linked && linked->kind() == ASTNodeKind::NamespaceDecl) {
        auto ns = linked->as_namespace_unsafe();
        auto found = ns->extended.find(name);
        if(found != ns->extended.end()) {
            if(found->second->kind() == ASTNodeKind::VarInitStmt) {
                auto varInit = found->second->as_var_init_unsafe();
                if(varInit->value) {
                    return varInit->value->evaluated_value(scope);
                }
            }
        }
        // Also check in nodes directly if not found in extended
        for(auto node : ns->nodes) {
            if(node->kind() == ASTNodeKind::VarInitStmt) {
                auto varInit = node->as_var_init_unsafe();
                if(varInit->name_view() == name && varInit->value) {
                    return varInit->value->evaluated_value(scope);
                }
            }
        }
        return nullptr;
    }
    const auto eval = evaluated_value(scope);
    if(eval && eval != this) {
        return eval->child(scope, name);
    } else {
        return nullptr;
    }
}

VariableIdentifier* VariableIdentifier::copy(ASTAllocator& allocator) {
    const auto view = allocator.allocate_str(value.data(), value.size());
    auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(chem::string_view(view, value.size()), getType(), encoded_location(), is_ns);
    id->linked = linked;
    return id;
}

Value* VariableIdentifier::evaluated_value(InterpretScope &scope) {
    // Fast path: use the cached parent-hop depth recorded by a previous lookup.
    if(value_depth_cached.load(std::memory_order_acquire)) {
        if(auto* owner = scope.ancestor_at(cached_value_depth.load(std::memory_order_relaxed))) {
            auto cached = owner->values.find(value);
            if(cached != owner->values.end()) {
                return cached->second;
            }
        }
    }
    unsigned depth = 0;
    auto found = scope.find_value(value, &depth);
    if (found != nullptr) {
        cached_value_depth.store(depth, std::memory_order_relaxed);
        value_depth_cached.store(true, std::memory_order_release);
        return found;
    }
    auto linkedNode = linked_node();
    if(linkedNode == nullptr) {
        return nullptr;
    }
    const auto linked_kind = linkedNode->kind();
    switch (linked_kind) {
        case ASTNodeKind::NamespaceDecl:
            return this;
        case ASTNodeKind::StructMember:
        case ASTNodeKind::UnnamedUnion:
        case ASTNodeKind::UnnamedStruct: {
            const auto found_self = scope.find_value("self");
            if(found_self) {
                // Guard against self-referencing: when this same VariableIdentifier
                // is stored in the scope under "self", resolving would cause infinite
                // recursion. Return this to let child() handle the nullptr path.
                if(found_self == this) return this;
                // getting the child
                auto childResult = found_self->child(scope, value);
                if(childResult && Value::isIdentifier(childResult->val_kind()) && childResult != this) {
                    return static_cast<VariableIdentifier*>(childResult)->evaluated_value(scope);
                }
                return childResult;
            }
            return this;
        }
        case ASTNodeKind::VarInitStmt: {
            const auto init = linked->as_var_init_unsafe();
            if(init->is_const()) {
                return init->value;
            }
            return this;
        }
        case ASTNodeKind::EnumMember: {
            const auto mem = linked->as_enum_member_unsafe();
            return mem->evaluate(scope.allocator, scope.global->typeBuilder, encoded_location());
        }
        default:
            return this;
    }
}