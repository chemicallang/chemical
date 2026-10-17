// Copyright (c) Chemical Language Foundation 2025.

#include <memory>

#include "ast/types/GenericType.h"
#include "FunctionParam.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/UnionDef.h"
#include "compiler/mangler/NameMangler.h"
#include "CapturedVariable.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/statements/VarInit.h"
#include "ast/types/LinkedType.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/types/VoidType.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/StructValue.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "compiler/ASTDiagnoser.h"
#include "preprocess/2c/BufferedWriter.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/PointerValue.h"
#include "ast/values/NullValue.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/IntNType.h"
#include "ast/base/TypeBuilder.h"

BaseType* FunctionDeclaration::inner_return_type() {
    auto rt = const_cast<BaseType*>(returnType.getType());
    // A lowered async function has its declared return type rewritten to
    // `FutureHandle<T>` by TopLevelLinkSignature, while its body still produces the
    // inner `T`. Only in that case must the type be unwrapped. Checking `is_async`
    // alone is not sufficient: when the active backend does not lower async (the
    // interpreter, or a transformer analyzing the module for documentation),
    // `returnType` stays the source type, which may itself be an ordinary generic
    // such as `Result<std::vector<u8>, FsError>`. Peeling its first type argument
    // would corrupt move/type matching ("unknown value being moved").
    if(!attrs.is_async || rt == nullptr || rt->kind() != BaseTypeKind::Generic) {
        return rt;
    }
    const auto gen = rt->as_generic_type_unsafe();
    const auto linked = gen->referenced != nullptr ? gen->referenced->linked : nullptr;
    if(linked == nullptr || linked->kind() != ASTNodeKind::StructDecl
       || linked->as_extendable_members_container_unsafe()->name_view() != chem::string_view("FutureHandle")) {
        return rt;
    }
    if(!gen->types.empty()) {
        return const_cast<BaseType*>(gen->types[0].getType());
    }
    return rt;
}


FunctionParam *FunctionParam::copy(ASTAllocator& allocator) const {
    const auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(
            name,
            type.copy(allocator),
            index,
            defValue ? defValue->copy(allocator) : nullptr,
            is_implicit(),
            parent(),
            encoded_location()
    );
    param->attrs = attrs;
    return param;
}

void FunctionDeclaration::make_destructor(ASTAllocator& allocator, ExtendableMembersContainerNode* def) {
    if(!has_self_param() || params.size() > 1 || params.empty()) {
        params.clear();
        params.emplace_back(new (allocator.allocate<FunctionParam>()) FunctionParam("self", TypeLoc( new (allocator.allocate<ReferenceType>()) ReferenceType(new (allocator.allocate<LinkedType>()) LinkedType(def)), def->encoded_location() ), 0, nullptr, true, this, ZERO_LOC));
    }
    returnType = {new(allocator.allocate<VoidType>()) VoidType(), returnType.getLocation()};
}

void check_returns_void(ASTDiagnoser& diagnoser, FunctionDeclaration* decl) {
    if(decl->returnType->kind() != BaseTypeKind::Void) {
        diagnoser.error((ASTNode*) decl) << decl->name_view() << " function return type should be void";
    }
}

void check_self_param(ASTDiagnoser& diagnoser, FunctionDeclaration* decl, ASTNode* self) {
    if(decl->params.size() == 1) {
        const auto param = decl->params.front();
        if(param->is_implicit() && param->name == "self") {
            return;
        }
    }
    diagnoser.error((ASTNode*) decl) << decl->name_view() << " must have a single implicit self reference parameter";
}

void check_self_other_params(ASTDiagnoser& diagnoser, FunctionDeclaration* decl, ASTNode* self) {
    if(decl->params.size() == 2) {
        const auto param = decl->params.front();
        const auto second = decl->params[1];
        if(param->is_implicit() && param->name == "self") {
            // automatically make second parameter implicit
            if(!second->is_implicit()) {
                second->set_is_implicit(true);
            }
            return;
        }
    }
    diagnoser.error((ASTNode*) decl) << decl->name_view() << " function must have implicit self reference parameter";
}

void FunctionDeclaration::ensure_constructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* def) {
    returnType = {new(allocator.allocate<LinkedType>()) LinkedType(def), returnType.getLocation()};
}

void FunctionDeclaration::ensure_destructor(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* def) {
    check_returns_void(diagnoser, this);
    check_self_param(diagnoser, this, def);
}

void FunctionDeclaration::ensure_clear_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* def) {
    check_returns_void(diagnoser, this);
    check_self_param(diagnoser, this, def);
}

void FunctionDeclaration::ensure_move_fn(ASTAllocator& allocator, ASTDiagnoser& diagnoser, ASTNode* def) {
    returnType = {new(allocator.allocate<LinkedType>()) LinkedType(def), returnType.getLocation()};
    check_self_other_params(diagnoser, this, def);
}

bool FunctionDeclaration::put_as_extension_function(ASTDiagnoser& diagnoser) {
    // extension functions declare themselves inside the container
    auto& receiver = *params[0];
    const auto type = receiver.type;
    const auto pure_receiver = type->canonical();
    const auto receiver_kind = pure_receiver->kind();
    if (receiver_kind != BaseTypeKind::Reference) {
        diagnoser.error("receiver in extension function must always be a reference", receiver.type.encoded_location());
    }
    auto linked = type->linked_node();
    if (!linked) {
        diagnoser.error((AnnotableNode*) this) << "couldn't find container in extension function ith receiver type \"" << type->representation() << "\"";
        return false;
    }
    const auto linked_kind = linked->kind();
    auto container = linked->as_extendable_member_container();
    if (linked_kind == ASTNodeKind::InterfaceDecl) {
        const auto interface = linked->as_interface_def_unsafe();
        if (!interface->is_static() && generic_parent == nullptr) {
            diagnoser.error("extension functions are only supported on static interfaces, either make the interface static or move the function inside the interface", receiver.type.encoded_location());
            return false;
        }
    } else if(linked_kind == ASTNodeKind::GenericTypeParam) {
        const auto param = linked->as_generic_type_param_unsafe();
        const auto tSize = param->traits.size();
        if(tSize == 1) {
            auto& at_least = param->traits.back();
            const auto at_least_linked = at_least->get_direct_linked_node();
            if(at_least_linked) {
                container = at_least_linked->as_extendable_member_container();
            }
        } else if(tSize > 1) {
            diagnoser.error("extension functions on generic parameters support only a single trait bound", receiver.type.encoded_location());
        }
    } else if(linked_kind == ASTNodeKind::GenericStructDecl) {
        container = linked->as_gen_struct_def_unsafe()->master_impl;
    } else if(linked_kind == ASTNodeKind::GenericInterfaceDecl) {
        container = linked->as_gen_interface_decl_unsafe()->master_impl;
    } else if(linked_kind == ASTNodeKind::GenericUnionDecl) {
        container = linked->as_gen_union_decl_unsafe()->master_impl;
    } else if(linked_kind == ASTNodeKind::GenericVariantDecl) {
        container = linked->as_gen_variant_decl_unsafe()->master_impl;
    }
    if (!container) {
        diagnoser.error(receiver.type.encoded_location()) << "type doesn't support extension functions " << type->representation();
        return false;
    }
    const auto field_func = linked->child(name_view());
    if (field_func != nullptr && field_func != generic_parent) {
        diagnoser.error(receiver.type.encoded_location()) << "couldn't declare extension function with name '" << name_view() << "' because type '" << receiver.type->representation() << "' already has a field / function with same name \n";
        return false;
    }
    if(generic_parent) {
        container->add_extension_func(name_view(), generic_parent);
    } else {
        container->add_extension_func(name_view(), this);
    }
    return true;
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    ASTAllocator& func_allocator,
    FunctionCall* call_obj,
    Value* parent,
    bool evaluate_refs
) {
    const auto global = call_scope->global;
    // interpreted calls recurse on the C++ stack, so a function which never returns
    // (like a comptime function calling itself) must be stopped before the stack of
    // the thread is exhausted, which would crash the compiler
    if(call_scope->stack_space_exhausted(call_obj != nullptr ? (ASTNode*) call_obj : (ASTNode*) this)) {
        return call_scope->getNullValue();
    }
    const auto prev_func = global->current_func_type;
    global->current_func_type = this;
    global->call_stack.emplace_back(call_obj);
    InterpretScope fn_scope(global, func_allocator, global);
    // Propagate implicit args from the lexical scope chain into this function scope.
    // When a provide block stores implicit args in a scope, inner function calls create
    // new scopes with parent=global — they won't find the provide's scope via parent chain.
    // By copying args here, inner nested calls can find them through call_scope->implicit_args.
    {
        InterpretScope* propagate = call_scope;
        while(propagate) {
            if(auto* map = propagate->implicit_args_if_any()) {
                auto& dst = fn_scope.implicit_args_ref();
                for(auto& [name, val] : *map) {
                    if(dst.find(name) == dst.end()) {
                        dst[name] = val;
                    }
                }
            }
            propagate = propagate->parent;
        }
    }
    const auto value = call(call_scope, call_obj->values, parent, &fn_scope, evaluate_refs, call_obj);
    global->call_stack.pop_back();
    global->current_func_type = prev_func;
    return value;
}

// called by the return statement
void FunctionDeclaration::set_return(InterpretScope& func_scope, Value *value) {
    if(value) {
        // Evaluate the return value expression in the current scope (func_scope),
        // not in the function-level scope (*target). Variables like variant case
        // variables are declared in nested scopes inside the function body and
        // would not be found when evaluating from the function-level scope.
        auto evaluated = value->evaluated_value(func_scope);
        // Normalize integer return values to the declared return type (e.g.
        // returning 300 from a function returning `u8` must yield 44), matching
        // the compiled backends.
        if(returnType) {
            evaluated = func_scope.coerce_to_type(evaluated, returnType);
        }
        // Walk up the scope chain to find the function-level scope
        // (created by call() with the global as parent).
        // set_return may be called with a nested scope (inside if/block/etc.),
        // but call() reads returnValue from the top-level fn_scope.
        InterpretScope* target = &func_scope;
        while(target->parent != nullptr && target->parent != func_scope.global) {
            target = target->parent;
        }
        target->returnValue = evaluated;
    }
    // Propagate the stop signal up the InterpretScope chain so that
    // non-loop scopes (e.g. if-block bodies) also stop iterating.
    // Without this, nested scopes continue interpreting sibling nodes
    // after a return, potentially overwriting the return value.
    InterpretScope* stop_target = &func_scope;
    while(stop_target != nullptr && stop_target != func_scope.global) {
        stop_target->stopInterpretation = true;
        stop_target = stop_target->parent;
    }
}

FunctionDeclaration *FunctionDeclaration::as_function() {
    return this;
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    std::vector<Value*> &call_args,
    Value* parent,
    InterpretScope *fn_scope,
    bool evaluate_refs,
    Value* debug_value
) {
    auto self_param = get_self_param();
    if(self_param) {
        fn_scope->declare(self_param->name, parent);
    }
    auto i = 0;
    while (i < call_args.size()) {
        const auto param = func_param_for_arg_at(i);
        if(!param) break;
        Value* param_val;
        const auto arg = call_args[i];
        param_val = arg->scope_value(*call_scope);
        // Pass-by-value semantics for non-destructor structs passed to non-reference parameters:
        // create a copy so modifications inside the function don't affect the caller's value.
        if(param_val && param_val->val_kind() == ValueKind::StructValue && param && param->type &&
           !param->type->is_reference() && !param->type->is_pointer()) {
            auto sv = param_val->as_struct_value_unsafe();
            auto ext = sv->linked_extendable();
            if(ext && ext->kind() == ASTNodeKind::StructDecl) {
                auto sd = (StructDefinition*)ext;
                if(!sd->has_destructor()) {
                    param_val = sv->copy(fn_scope->allocator);
                }
            }
        }
        // Normalize integer arguments to the parameter's declared type (e.g.
        // passing 300 to a `u8` parameter must truncate to 44), matching the
        // compiled backends.
        if(param && param->type && param_val) {
            param_val = call_scope->coerce_to_type(param_val, param->type);
        }
        // Move semantics: if the argument references an existing destructible struct
        // variable in the caller's scope, clear the source so it's not destructed
        // again when the caller's scope ends. Uses pointer-matching instead of AST
        // node type checks, because the compiler may have replaced identifiers.
        call_scope->move_clear_source(param_val, chem::string_view());
        // Check if this argument needs implicit constructor conversion
        if(param && param->type && param_val) {
            auto imp_cons = param->type->implicit_constructor_for(param_val);
            if(imp_cons && imp_cons != this) {
                InterpretScope imp_scope(call_scope->global, fn_scope->allocator, call_scope->global);
                std::vector<Value*> imp_args = { param_val->evaluated_value(*call_scope) };
                Value* newVal = imp_cons->call(
                    call_scope, imp_args, (Value*)nullptr, &imp_scope, true, debug_value
                );
                if(newVal) {
                    param_val = newVal;
                }
            }
        }
        fn_scope->declare(param->name, param_val);
        i++;
    }
    auto expected_args = expectedArgsSize();
    while(i < expected_args) {
        auto param = func_param_for_arg_at(i);
        if (param) {
            if(param->defValue) {
                Value* param_val = param->defValue->scope_value(*call_scope);
                // Apply implicit constructor conversion for default values
                if(param->type && param_val) {
                    auto imp_cons = param->type->implicit_constructor_for(param_val);
                    if(imp_cons && imp_cons != this) {
                        InterpretScope imp_scope(call_scope->global, fn_scope->allocator, call_scope->global);
                        std::vector<Value*> imp_args = { param_val->evaluated_value(*call_scope) };
                        Value* newVal = imp_cons->call(
                            call_scope, imp_args, (Value*)nullptr, &imp_scope, true, debug_value
                        );
                        if(newVal) {
                            param_val = newVal;
                        }
                    }
                }
                fn_scope->declare(param->name, param_val);
            } else if(!isInVarArgs(i)) {
                call_scope->error("function parameter doesn't have default value, no argument exists for it", debug_value);
            }
        }
        i++;
    }
    // Handle implicit parameters (those before explicit params in the params list).
    // These are NOT reachable via func_param_for_arg_at() because it uses
    // explicit_func_arg_offset() = total_implicit_params(), which skips them.
    // Instead, iterate params directly to find undeclared implicit params.
    for(const auto& param : params) {
        if(!param->is_implicit()) break;
        if(param->name == "self") continue; // handled above
        // Skip if already declared (e.g. from call_args for mixed implicit+explicit)
        if(fn_scope->values.find(param->name) != fn_scope->values.end()) {
            continue;
        }
        // Look up implicit args: first check the lexical scope chain (call_scope),
        // then fall back to fn_scope which has args propagated from the outer call().
        // This handles nested function calls where the provide block's scope is not
        // in the parent chain of the nested function's call_scope.
        Value* implicit_val = nullptr;
        InterpretScope* lookup_scope = call_scope;
        while(lookup_scope) {
            if(auto* map = lookup_scope->implicit_args_if_any()) {
                auto found = map->find(param->name);
                if(found != map->end()) {
                    implicit_val = found->second;
                    break;
                }
            }
            lookup_scope = lookup_scope->parent;
        }
        if(!implicit_val && fn_scope) {
            if(auto* map = fn_scope->implicit_args_if_any()) {
                auto found = map->find(param->name);
                if(found != map->end()) {
                    implicit_val = found->second;
                }
            }
        }
        if(implicit_val) {
            fn_scope->declare(param->name, implicit_val);
        } else if(param->defValue) {
            fn_scope->declare(param->name, param->defValue->scope_value(*call_scope));
        } else if(!isInVarArgs(param->index)) {
            call_scope->error(debug_value) << "couldn't find implicit argument '" << param->name << "' in current scope";
        }
    }
    if(!body.has_value()) {
        // Function with no body (extern declaration).
        // Return a zero value of the appropriate return type instead of nullptr.
        // This allows tests to call extern functions and get predictable results.
        if(returnType) {
            auto retCanon = returnType->canonical();
            if(retCanon) {
                switch(retCanon->kind()) {
                    case BaseTypeKind::IntN: {
                        auto intType = retCanon->as_intn_type_unsafe();
                        return new (fn_scope->allocate<IntNumValue>()) IntNumValue(0, intType, encoded_location());
                    }
                    case BaseTypeKind::Bool: {
                        return new (fn_scope->allocate<BoolValue>()) BoolValue(false, call_scope->global->typeBuilder.getBoolType(), encoded_location());
                    }
                    case BaseTypeKind::Float: {
                        return new (fn_scope->allocate<FloatValue>()) FloatValue(0.0f, (FloatType*)retCanon, encoded_location());
                    }
                    case BaseTypeKind::Double: {
                        return new (fn_scope->allocate<DoubleValue>()) DoubleValue(0.0, (DoubleType*)retCanon, encoded_location());
                    }
                    case BaseTypeKind::Pointer: {
                        auto ptrType = retCanon->as_pointer_type_unsafe();
                        return new (fn_scope->allocate<PointerValue>()) PointerValue(nullptr, ptrType->type, 0, 0, encoded_location());
                    }
                    default:
                        break;
                }
            }
        }
        return call_scope->getNullValue();
    }
    {
        InterpretScope body_scope(fn_scope, fn_scope->allocator, fn_scope->global);
        body_scope.interpret(&body.value());
        if(fn_scope->returnValue) {
            body_scope.returnValue = fn_scope->returnValue;
        }
    }
    if(!fn_scope->returnValue && is_constructor_fn() && is_generated_fn()) {
        const auto linkedNode = returnType->linked_node();
        if(linkedNode && linkedNode->kind() == ASTNodeKind::StructDecl) {
            auto structDef = static_cast<StructDefinition*>(linkedNode);
            auto retType = returnType.copy(fn_scope->allocator);
            auto structVal = new (fn_scope->allocate<StructValue>()) StructValue(
                retType,
                structDef,
                static_cast<VariablesContainerBase*>(structDef),
                encoded_location()
            );
            // Step 1: Initialize inherited structs by calling their default constructors
            for(auto& inherited : structDef->inherited) {
                const auto node = inherited.type->get_direct_linked_canonical_node();
                if(node && node->kind() == ASTNodeKind::StructDecl) {
                    const auto parentDef = node->as_struct_def_unsafe();
                    const auto parentCons = parentDef->default_constructor_func();
                    if(parentCons) {
                        InterpretScope nested_scope(fn_scope->global, fn_scope->allocator, fn_scope->global);
                        std::vector<Value*> nested_call_args;
                        Value* parentVal = parentCons->call(
                            call_scope, nested_call_args, (Value*) nullptr, &nested_scope, true, (Value*) nullptr
                        );
                        if(parentVal && parentVal->kind() == ValueKind::StructValue) {
                            auto parentStructVal = parentVal->as_struct_value_unsafe();
                            for(auto& [pname, pmember] : parentStructVal->values) {
                                if(structVal->values.find(pname) == structVal->values.end()) {
                                    structVal->values.emplace(pname, pmember);
                                }
                            }
                        }
                    }
                }
            }
            // Step 2: Initialize member variables (default values + struct default constructors)
            for(const auto var : structDef->variables()) {
                // Skip inherited members that were already initialized above
                if(structVal->values.find(var->name) != structVal->values.end()) {
                    continue;
                }
                const auto defVal = var->default_value();
                if(defVal) {
                    auto val = defVal->scope_value(*fn_scope);
                    const StructMemberInitializer init(var->name, val);
                    structVal->values.emplace(var->name, init);
                } else if(var->kind() == ASTNodeKind::StructMember) {
                    const auto member = static_cast<StructMember*>(var);
                    const auto kt = member->known_type();
                    if(kt) {
                        const auto linked = kt->get_direct_linked_canonical_node();
                        if(linked && linked->kind() == ASTNodeKind::StructDecl) {
                            const auto defCons = linked->as_struct_def_unsafe()->default_constructor_func();
                            if(defCons) {
                                InterpretScope nested_scope(fn_scope->global, fn_scope->allocator, fn_scope->global);
                                std::vector<Value*> nested_call_args;
                                Value* nestedVal = defCons->call(
                                    call_scope, nested_call_args, (Value*) nullptr, &nested_scope, true, (Value*) nullptr
                                );
                                if(nestedVal) {
                                    const StructMemberInitializer memberInit(var->name, nestedVal);
                                    structVal->values.emplace(var->name, memberInit);
                                }
                            }
                        }
                    }
                }
            }
            fn_scope->returnValue = structVal;
        }
    }
    // Step 3: If the return value needs implicit constructor conversion, call the implicit constructor
    if(fn_scope->returnValue && returnType) {
        auto imp_cons = returnType->implicit_constructor_for(fn_scope->returnValue);
        if(imp_cons && imp_cons != this) {
            InterpretScope imp_scope(call_scope->global, fn_scope->allocator, call_scope->global);
            std::vector<Value*> imp_args = { fn_scope->returnValue };
            Value* newVal = imp_cons->call(
                call_scope, imp_args, (Value*) nullptr, &imp_scope, true, debug_value
            );
            if(newVal) {
                fn_scope->returnValue = newVal;
            }
        }
    }
    // Self reference parameter (&self) should not be destructed when fn_scope ends.
    // The struct value is owned by the caller's scope; destructing it here would
    // prematurely call the destructor on the caller's struct.
    if(self_param && self_param->type && self_param->type->is_reference()) {
        auto self_it = fn_scope->values.find(self_param->name);
        if(self_it != fn_scope->values.end()) {
            self_it->second = nullptr;
        }
    }
    return fn_scope->returnValue;
}

BaseType* CapturedVariable::known_type() {
    auto val_type = linked->known_type();
    if(capture_by_ref) {
        refType.type = val_type;
        return &refType;
    } else {
        return val_type;
    }
}
