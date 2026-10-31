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
#include "ast/statements/Return.h"
#include "ast/statements/ValueWrapperNode.h"
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
#include "utils/StackSpace.h"

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

void check_returns_void(ASTDiagnoser& diagnoser, FunctionDeclaration* decl) {
    if(decl->returnType->kind() != BaseTypeKind::Void) {
        diagnoser.error((ASTNode*) decl).with_code("SAFE-DROP-001")
            << decl->name_view() << " function return type should be void";
    }
}

void check_self_param(ASTDiagnoser& diagnoser, FunctionDeclaration* decl, ASTNode* self) {
    if(decl->params.size() == 1) {
        const auto param = decl->params.front();
        if(param->is_implicit() && param->name == "self") {
            return;
        }
    }
    diagnoser.error((ASTNode*) decl).with_code("SAFE-DROP-001")
        << decl->name_view() << " must have a single implicit self reference parameter";
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

/**
 * The AST statement interpreter is gone. What remains here is the expression
 * layer: a comptime function whose body is a single `return <expr>` (or a bare
 * expression) can still be folded at symbol-resolution time — this is how
 * `comptime if(has_atomic_builtins())` and `const` initializers are evaluated.
 * Any body that needs statements is reported: those calls are lowered and run
 * by the MIR comptime driver.
 */
static Value* fold_expression_body(InterpretScope& fn_scope, Scope& body) {
    if (body.nodes.size() != 1) return nullptr;
    ASTNode* node = body.nodes.front();
    if (!node) return nullptr;
    if (node->kind() == ASTNodeKind::ReturnStmt) {
        auto* ret = static_cast<ReturnStatement*>(node);
        if (!ret->value) return fn_scope.getNullValue();
        return ret->value->evaluated_value(fn_scope);
    }
    if (node->kind() == ASTNodeKind::ValueWrapper) {
        return static_cast<ValueWrapperNode*>(node)->value->evaluated_value(fn_scope);
    }
    return nullptr;
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    ASTAllocator& func_allocator,
    FunctionCall* call_obj,
    Value* parent,
    bool evaluate_refs
) {
    if (!call_scope || !call_obj) return nullptr;
    InterpretScope fn_scope(call_scope->global, func_allocator, call_scope->global);
    return call(call_scope, call_obj->values, parent, &fn_scope, evaluate_refs, call_obj);
}

Value *FunctionDeclaration::call(
    InterpretScope *call_scope,
    std::vector<Value*>& call_args,
    Value* parent_val,
    InterpretScope *fn_scope,
    bool evaluate_refs,
    Value* debug_value
) {
    if (!call_scope || !fn_scope) return nullptr;
    // Once a comptime evaluation in this thread has run the stack out, every
    // fold after it returns immediately: the root cause is reported once and
    // re-running would only repeat the deep recursion.
    if (chem::comptime_stack_failed()) {
        if (!chem::comptime_stack_reported()) {
            chem::mark_comptime_stack_reported();
            call_scope->error(chem::comptime_stack_error_message(),
                              debug_value ? (ASTNode*) debug_value : (ASTNode*) this);
        }
        return nullptr;
    }
    if (chem::comptime_stack_exhausted()) {
        // A comptime function which calls itself never returns; folding its body
        // recurses on the compiler's own stack. The space kept in reserve lets us
        // report the diagnostic and unwind the already nested calls.
        chem::mark_comptime_stack_failed();
        chem::mark_comptime_stack_reported();
        call_scope->error(chem::comptime_stack_error_message(),
                          debug_value ? (ASTNode*) debug_value : (ASTNode*) this);
        return nullptr;
    }
    auto self_param = get_self_param();
    if (self_param) fn_scope->declare(self_param->name, parent_val);

    size_t i = 0;
    while (i < call_args.size()) {
        FunctionParam* param = func_param_for_arg_at(static_cast<unsigned>(i));
        if (!param) break;
        Value* arg = call_args[i];
        fn_scope->declare(param->name, arg ? arg->scope_value(*call_scope) : nullptr);
        i++;
    }
    const size_t expected = expectedArgsSize();
    while (i < expected) {
        FunctionParam* param = func_param_for_arg_at(static_cast<unsigned>(i));
        if (param && param->defValue) {
            fn_scope->declare(param->name, param->defValue->scope_value(*call_scope));
        }
        i++;
    }

    if (body.has_value()) {
        Value* folded = fold_expression_body(*fn_scope, body.value());
        if (folded) return folded;
        // the stack guard (or a previous failure) already reported this
        if (chem::comptime_stack_failed()) return nullptr;
    }
    call_scope->error("comptime function has a statement body that cannot be folded at "
                      "symbol resolution (comptime calls are evaluated by the MIR driver)",
                      debug_value ? (ASTNode*) debug_value : (ASTNode*) this);
    return nullptr;
}

FunctionDeclaration *FunctionDeclaration::as_function() {
    return this;
}

BaseType* CapturedVariable::known_type() {
    // A nested closure captures the original variable, not the enclosing
    // closure's capture slot. Follow the capture chain so a by-reference
    // outer capture does not turn a nested `&mut x` into `&mut &mut x`.
    ASTNode* src = linked;
    while(src != nullptr && src->kind() == ASTNodeKind::CapturedVariable) {
        src = src->as_captured_var_unsafe()->linked;
    }
    auto val_type = src != nullptr ? src->known_type() : nullptr;
    if(capture_by_ref) {
        refType.type = val_type;
        return &refType;
    } else {
        return val_type;
    }
}
