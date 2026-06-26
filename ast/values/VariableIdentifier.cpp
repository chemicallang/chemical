// Copyright (c) Chemical Language Foundation 2025.

#include "VariableIdentifier.h"
#include <memory>
#include "ast/values/AccessChain.h"
#include "ast/values/StructValue.h"
#include "ast/values/NullValue.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "ast/structures/EnumMember.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/FileScope.h"
#include "ast/structures/Scope.h"
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
        // Try looking up through self (for struct member access like `a *= 2` in a method)
        auto linkedNode = linked_node();
        if(linkedNode && linkedNode->kind() == ASTNodeKind::StructMember) {
            auto foundSelf = scope.find_value("self");
            if(foundSelf) {
                auto childVal = foundSelf->child(scope, value);
                if(childVal) {
                    // Evaluate the operation before setting on self
                    if (op != Operation::Assignment) {
                        auto evalNewValue = rawValue->evaluated_value(scope);
                        auto newValue = scope.evaluate(op, childVal, evalNewValue, passed_loc, this);
                        foundSelf->set_child_value(scope, value, newValue, Operation::Assignment);
                    } else {
                        foundSelf->set_child_value(scope, value, rawValue->evaluated_value(scope), op);
                    }
                    return;
                }
            }
        }
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

    if (op != Operation::Assignment) {

        // get the previous value, perform operation on it
        auto prevValue = itr.first->second;

        // Handle operator overloads for compound assignments on struct-like values
        // First, try to get the actual struct value by evaluating (resolves AccessChain, Identifier, etc.)
        auto resolvedPrev = prevValue->evaluated_value(scope);
        if(resolvedPrev && resolvedPrev->val_kind() == ValueKind::StructValue) {
            auto glob = scope.global;
            if(glob && glob->build_compiler) {
                auto structVal = resolvedPrev->as_struct_value_unsafe();
                auto ext = structVal->linked_extendable();
                if(ext && (ext->kind() == ASTNodeKind::StructDecl || ext->kind() == ASTNodeKind::VariantDecl)) {
                    auto& coreNodes = glob->build_compiler->coreNodes;
                    auto& implsIndex = glob->build_compiler->implsIndex;
                    // Convert compound assignment ops (AddTo→Addition) for lookup
                    Operation binaryOp = op;
                    switch(op) {
                        case Operation::AddTo: binaryOp = Operation::Addition; break;
                        case Operation::SubtractFrom: binaryOp = Operation::Subtraction; break;
                        case Operation::MultiplyBy: binaryOp = Operation::Multiplication; break;
                        case Operation::DivideBy: binaryOp = Operation::Division; break;
                        case Operation::ModuloBy: binaryOp = Operation::Modulus; break;
                        case Operation::ShiftLeftBy: binaryOp = Operation::LeftShift; break;
                        case Operation::ShiftRightBy: binaryOp = Operation::RightShift; break;
                        case Operation::ANDWith: binaryOp = Operation::BitwiseAND; break;
                        case Operation::ExclusiveORWith: binaryOp = Operation::BitwiseXOR; break;
                        case Operation::InclusiveORWith: binaryOp = Operation::BitwiseOR; break;
                        default: break;
                    }
                    FunctionDeclaration* overloaded = implsIndex.get_ass_op_impl(
                        coreNodes, ext->get_members_container(), binaryOp
                    );
                    // Fallback: search parent scope for ImplDefinitions matching this struct
                    if(!overloaded) {
                        const char* lookupName = nullptr;
                        switch(binaryOp) {
                            case Operation::Addition: lookupName = "add_assign"; break;
                            case Operation::Subtraction: lookupName = "sub_assign"; break;
                            case Operation::Multiplication: lookupName = "mul_assign"; break;
                            case Operation::Division: lookupName = "div_assign"; break;
                            case Operation::Modulus: lookupName = "rem_assign"; break;
                            case Operation::LeftShift: lookupName = "shl_assign"; break;
                            case Operation::RightShift: lookupName = "shr_assign"; break;
                            case Operation::BitwiseAND: lookupName = "bitand_assign"; break;
                            case Operation::BitwiseXOR: lookupName = "bitxor_assign"; break;
                            case Operation::BitwiseOR: lookupName = "bitor_assign"; break;
                            default: break;
                        }
                        if(lookupName) {
                            chem::string_view nameSv(lookupName);
                            ASTNode* searchNode = ext->parent();
                            while(searchNode) {
                                std::vector<ASTNode*>* nodes = nullptr;
                                if(searchNode->kind() == ASTNodeKind::NamespaceDecl) {
                                    nodes = &searchNode->as_namespace_unsafe()->nodes;
                                } else if(searchNode->kind() == ASTNodeKind::FileScope) {
                                    nodes = &static_cast<FileScope*>(searchNode)->body.nodes;
                                } else if(searchNode->kind() == ASTNodeKind::Scope) {
                                    nodes = &static_cast<Scope*>(searchNode)->nodes;
                                }
                                if(nodes) {
                                    for(auto child : *nodes) {
                                        if(!child || child->kind() != ASTNodeKind::ImplDecl) continue;
                                        auto impl = child->as_impl_def_unsafe();
                                        if(!impl || !impl->struct_type) continue;
                                        auto implStruct = impl->struct_type->get_direct_linked_canonical_node();
                                        bool matches = (implStruct == ext);
                                        if(!matches && implStruct && ext &&
                                            implStruct->get_node_identifier() == ext->get_node_identifier()) {
                                            matches = true;
                                        }
                                        if(!matches) continue;
                                        auto fn = impl->direct_child_function(nameSv);
                                        if(fn && fn->body.has_value()) {
                                            overloaded = fn;
                                            break;
                                        }
                                    }
                                    if(overloaded) break;
                                }
                                searchNode = searchNode->parent();
                            }
                        }
                    }
                    if(overloaded) {
                        const auto prev_func = glob->current_func_type;
                        glob->current_func_type = overloaded;
                        glob->call_stack.emplace_back(nullptr);
                        InterpretScope fn_scope(glob, glob->allocator, glob);
                        std::vector<Value*> opArgs = { newValue };
                        auto result = overloaded->call(&scope, opArgs, prevValue, &fn_scope, true, this);
                        glob->call_stack.pop_back();
                        glob->current_func_type = prev_func;
                        itr.first->second = result;
                        return;
                    }
                }
            }
        }

        // TODO debug value being passed as this, it should be taken as a parameter
        auto nextValue = itr.second.evaluate(op, prevValue, newValue, passed_loc, this);
        itr.first->second = nextValue;

    } else {
        itr.first->second = newValue;
    }

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
        // Resolve through chains of VariableIdentifiers to prevent
        // infinite recursion in child(). If the found value is itself
        // an identifier, resolve it further.
        if(found->val_kind() == ValueKind::Identifier && found != this) {
            return static_cast<VariableIdentifier*>(found)->evaluated_value(scope);
        }
        return found;
    }
    auto linkedNode = linked_node();
    if(linkedNode) {
        const auto linked_kind = linkedNode->kind();
        if(linked_kind == ASTNodeKind::StructMember) {
            const auto found_self = scope.find_value("self");
            if(found_self) {
                // Guard against self-referencing: when this same VariableIdentifier
                // is stored in the scope under "self", resolving would cause infinite
                // recursion. Return this to let child() handle the nullptr path.
                if(found_self == this) return this;
                auto childResult = found_self->child(scope, value);
                if(childResult && childResult->val_kind() == ValueKind::Identifier && childResult != this) {
                    return static_cast<VariableIdentifier*>(childResult)->evaluated_value(scope);
                }
                return childResult;
            }
        } else if(linked_kind == ASTNodeKind::VarInitStmt) {
            const auto init = linked->as_var_init_unsafe();
            if(init->is_const()) {
                auto constVal = init->value;
                if(constVal && constVal->val_kind() == ValueKind::Identifier && constVal != this) {
                    return static_cast<VariableIdentifier*>(constVal)->evaluated_value(scope);
                }
                return constVal;
            }
        } else if(linked_kind == ASTNodeKind::EnumMember) {
            const auto mem = linked->as_enum_member_unsafe();
            return mem->evaluate(scope.allocator, scope.global->typeBuilder, encoded_location());
        }
    }
    return this;
}