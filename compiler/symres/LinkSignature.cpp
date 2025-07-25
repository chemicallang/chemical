// Copyright (c) Chemical Language Foundation 2025.

#include "LinkSignature.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "ast/statements/UsingStmt.h"
#include "ast/statements/AliasStmt.h"
#include "ast/statements/Typealias.h"
#include "ast/base/TypeBuilder.h"
#include "ast/statements/VarInit.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericImplDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/If.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/Scope.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/VariantMemberParam.h"
#include "ast/types/LinkedValueType.h"
#include "LinkSignatureAPI.h"
#include "compiler/SymbolResolver.h"

void sym_res_signature(SymbolResolver& resolver, Scope* scope) {
    TopLevelLinkSignature visitor(resolver);
    visitor.visit(scope);
}

void TopLevelLinkSignature::VisitVariableIdentifier(VariableIdentifier* value) {
    const auto decl = linker.find(value->value);
    if(decl) {
        value->linked = decl;
        value->setType(decl->known_type());
        // to ensure function decl is informed about usage
        value->process_linked(&linker, nullptr);
    } else if(value->linked == nullptr) {
        linker.error(value) << "unresolved variable identifier, '" << value->value << "' not found";
        value->linked = (ASTNode*) linker.unresolved_decl;
    }
}

ASTNode* get_chain_linked(Value* value) {
    const auto id = value->get_last_id();
    return id ? id->linked : nullptr;
}

void TopLevelLinkSignature::VisitLinkedType(LinkedType* type) {
    // can assume that after parsing every linked type is named
    // because it is, we only create pure linked types after symbol resolution
    if(type->is_value()) {
        const auto linked_type = (LinkedValueType*) type;
        // only identifiers would be allowed in this value referencing type
        visit(linked_type->value);
        // now lets check the last item in the value (considering it an access chain)
        const auto linked = get_chain_linked(linked_type->value);
        if(linked) {
            type->linked = linked;
        } else {
            type->linked = (ASTNode*) linker.unresolved_decl;
            linker.error(type_location) << "unresolved type not found";
        }
    } else if(type->is_named()){
        const auto named = (NamedLinkedType*) type;
        const auto decl = linker.find(named->debug_link_name());
        if(decl) {
            type->linked = decl;
        } else if(type->linked == nullptr) {
            linker.error(type_location) << "unresolved type, '" << named->debug_link_name() << "' not found";
            type->linked = (ASTNode*) linker.unresolved_decl;
        }
    }
}

void TopLevelLinkSignature::VisitAccessChain(AccessChain* value) {
    // an access chain during link signature is being used
    // this must be for example in using statement using ns::something::some
    // or it could be top level function call in var statement var x = ns::some()

#ifdef DEBUG
    if(value->values.empty()) {
        linker.error(value) << "empty access chain detected";
        return;
    }
#endif

    // take the first value
    auto& first = value->values[0];

    // only supporting identifiers at the moment
    // so we don't have to call linked_node on first value
    if(first->kind() != ValueKind::Identifier) {
        linker.error(first) << "only identifiers are supported in top level access chains at the moment";
        return;
    }

    // visit the first element normally
    visit(first);

    // no need to traverse further, if only single element
    if(value->values.size() == 1) {
        value->setType(value->values[0]->getType());
        return;
    }

    // a variable for traversal
    const auto first_id = first->as_identifier_unsafe();
    auto parent = first_id->linked;
    if(parent == nullptr) {
        return;
    }

    // access chain contains multiple values
    // its guaranteed that values in access chain, after the first value are identifiers
    unsigned i = 1;
    const auto size = value->values.size();
    while(i < size) {
        const auto child = value->values[i]->as_identifier_unsafe();
        const auto child_linked = parent->child(child->value);
        if(child_linked) {
            child->linked = child_linked;
            child->setType(child_linked->known_type());
            parent = child_linked;
        } else {
            child->linked = (ASTNode*) linker.unresolved_decl;
            linker.error(child) << "unresolved identifier, '" << child->value << "' not found";
            break;
        }
        i++;
    }

    // the last item holds the type for this access chain
    value->setType(value->values[size - 1]->getType());

}

void TopLevelLinkSignature::VisitExpression(Expression* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitExpression(value);
    value->determine_type(linker.comptime_scope.typeBuilder);
    if(!linker.comptime_context) {
        linker.error("cannot evaluate expression at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::VisitAddrOfValue(AddrOfValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitAddrOfValue(value);
    value->determine_type();
    if(!linker.comptime_context) {
        linker.error("cannot take address of value at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::VisitArrayValue(ArrayValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitArrayValue(value);
    value->determine_type(*linker.ast_allocator);
}

void TopLevelLinkSignature::VisitComptimeValue(ComptimeValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitComptimeValue(value);
    // type determined during symbol resolution needs to be set
    value->setType(value->getValue()->getType());
}

void TopLevelLinkSignature::VisitDereferenceValue(DereferenceValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitDereferenceValue(value);
    if(linker.comptime_context) {
        // determining the type for this dereference value
        auto& typeBuilder = linker.comptime_scope.typeBuilder;
        if (!value->determine_type(typeBuilder)) {
            linker.error("couldn't determine type for de-referencing", value);
        }
    } else {
        linker.error("cannot dereference value at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::VisitIncDecValue(IncDecValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitIncDecValue(value);
    value->setType(value->determine_type());
    if(!linker.comptime_context) {
        linker.error("cannot increment or decrement value at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::VisitIndexOperator(IndexOperator* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitIndexOperator(value);
    // determining the type for this index operator
    auto& typeBuilder = linker.comptime_scope.typeBuilder;
    value->determine_type(typeBuilder);
    if(!linker.comptime_context) {
        linker.error("cannot index into a value at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::TopLevelLinkSignature::VisitIsValue(IsValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitIsValue(value);
    if(!linker.comptime_context) {
        linker.error("cannot determine at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::VisitLambdaFunction(LambdaFunction* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitLambdaFunction(value);
    if(!linker.comptime_context) {
        // NOTE: this requires resolving lambda type, for which code should be separate
        // also requires knowing expected type, however values are visited automatically
        // due to nature of recursive visitor, we cannot send expected types
        linker.error("lambda functions at top level outside function body aren't supported", value);
    }
}

void TopLevelLinkSignature::VisitNegativeValue(NegativeValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitNegativeValue(value);
    // determine type for negative value
    value->determine_type(linker.comptime_scope.typeBuilder);
}

void TopLevelLinkSignature::VisitUnsafeValue(UnsafeValue* value) {
    const auto prev = linker.safe_context;
    linker.safe_context = false;
    RecursiveVisitor<TopLevelLinkSignature>::VisitUnsafeValue(value);
    linker.safe_context = prev;
    value->setType(value->getValue()->getType());
}

const auto RUNTIME_EVAL_ERR = "cannot evaluate at runtime outside function body";

void TopLevelLinkSignature::VisitNewValue(NewValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitNewValue(value);
    // type determined at symbol resolution must be set
    value->ptr_type.type = value->value->getType();
    if(!linker.comptime_context) {
        linker.error(RUNTIME_EVAL_ERR, value);
    }
}

void TopLevelLinkSignature::VisitNewTypedValue(NewTypedValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitNewTypedValue(value);
    if(!linker.comptime_context) {
        linker.error(RUNTIME_EVAL_ERR, value);
    }
}

void TopLevelLinkSignature::VisitPlacementNewValue(PlacementNewValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitPlacementNewValue(value);
    // type of the value determined at symbol resolution must be set
    value->ptr_type.type = value->value->getType();
    if(!linker.comptime_context) {
        linker.error(RUNTIME_EVAL_ERR, value);
    }
}

void TopLevelLinkSignature::VisitNotValue(NotValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitNotValue(value);
    // determine the type of not value
    value->setType(value->getValue()->getType());
    if(!linker.comptime_context) {
        linker.error(RUNTIME_EVAL_ERR, value);
    }
}

void TopLevelLinkSignature::VisitPatternMatchExpr(PatternMatchExpr* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitPatternMatchExpr(value);
    // TODO: maybe pattern match expression should be a node
    // currently we emplace a void type
    // as expression is only used as a statement
    value->setType((BaseType*) linker.comptime_scope.typeBuilder.getVoidType());
}

void TopLevelLinkSignature::VisitBlockValue(BlockValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitBlockValue(value);
    if(!linker.comptime_context) {
        linker.error(RUNTIME_EVAL_ERR, value);
    }
}

bool sig_embedded_traverse(void* data, ASTAny* item) {
    const auto traverser = static_cast<TopLevelLinkSignature*>(data);
    switch(item->any_kind()) {
        case ASTAnyKind::Node:
            traverser->visit(((ASTNode*) item));
            break;
        case ASTAnyKind::Value:
            traverser->visit(((Value*) item));
            break;
        case ASTAnyKind::Type:
            traverser->VisitTypeNoNullCheck((BaseType*) item);
            break;
    }
    return true;
}

void TopLevelLinkSignature::VisitEmbeddedNode(EmbeddedNode* node) {
    auto traverser = linker.binder.findHook(node->name, CBIFunctionType::TraversalNode);
    if(traverser) {
        // traverse any nested values and set their types
        ((EmbeddedNodeTraversalFunc) traverser)(node, this, sig_embedded_traverse);
    } else {
        linker.error(node) << "couldn't find traversal method of embedded node with name '" << node->name << "' for signature resolution";
    }
    auto found = linker.binder.findHook(node->name, CBIFunctionType::SymResLinkSignatureNode);
    if(found) {
        ((EmbeddedNodeSymResLinkSignature) found)(&linker, node);
    } else {
        linker.error(node) << "couldn't find link signature method for embedded node with name '" << node->name << "'";
    }
}

void TopLevelLinkSignature::VisitEmbeddedValue(EmbeddedValue* value) {
    auto traverser = linker.binder.findHook(value->name, CBIFunctionType::TraversalValue);
    if(traverser) {
        // traverse any nested values and set their types
        ((EmbeddedValueTraversalFunc) traverser)(value, this, sig_embedded_traverse);
    } else {
        linker.error(value) << "couldn't find traversal method of embedded value with name '" << value->name << "' for signature resolution";
    }
    auto found = linker.binder.findHook(value->name, CBIFunctionType::SymResLinkSignatureValue);
    if(found) {
        ((EmbeddedValueSymResLinkSignature) found)(&linker, value);
    } else {
        linker.error(value) << "couldn't find link signature method for embedded value with name '" << value->name << "'";
    }
}

void TopLevelLinkSignature::VisitGenericType(GenericType* type) {
    // save the type into a temporary before visiting children
    auto loc = type_location;
    // must be visited first, so child generic types are instantiated and ready
    RecursiveVisitor<TopLevelLinkSignature>::VisitGenericType(type);
    // we must instantiate generic declarations and link with those
    if(type->referenced != nullptr) {
        type->instantiate(linker.genericInstantiator, loc);
    }
}

void TopLevelLinkSignature::VisitArrayType(ArrayType* type) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitArrayType(type);
    // array types require calculating array size from given expression
    const auto arr_size_val = type->array_size_value;
    if(arr_size_val == nullptr) return;
    const auto evaluated = arr_size_val->evaluated_value(linker.comptime_scope);
    if(evaluated == nullptr) return;
    const auto number = evaluated->get_the_number();
    if(number.has_value()) {
        type->set_array_size(number.value());
    }
}

void TopLevelLinkSignature::VisitUsingStmt(UsingStmt* node) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitUsingStmt(node);
    node->declare_symbols(linker);
}

void TopLevelLinkSignature::VisitAliasStmt(AliasStmt* stmt) {
    const auto value = stmt->value;

    // currently only identifier values are supported
    if (value->kind() == ValueKind::AccessChain) {
        const auto chain = value->as_access_chain_unsafe();
        if (chain->values.size() != 1 || chain->values.front()->kind() != ValueKind::Identifier) {
            linker.error(stmt) << "incompatible value given to alias";
            return;
        }
    }

    // TODO: this value can fail resolution, however we proceed as if it doesn't
    // we shouldn't use alias statement
    visit(stmt->value);

    // TODO: linked_node shouldn't be called in link_signature
    const auto node = value->linked_node();
    if (!node) {
        linker.error(stmt) << "cannot alias incompatible value";
        return;
    }
    if (stmt->specifier >= node->specifier()) {
        linker.error(stmt) << "cannot alias a node to a higher specifier";
        return;
    }

    // declares the node without runtime
    linker.declare_node(stmt->alias_name, node, stmt->specifier, false);

}

void TopLevelLinkSignature::VisitVarInitStmt(VarInitStatement* node) {
    if(node->is_comptime() && !linker.comptime_context) {
        linker.comptime_context = true;
        RecursiveVisitor<TopLevelLinkSignature>::VisitVarInitStmt(node);
        linker.comptime_context = false;
    } else {
        RecursiveVisitor<TopLevelLinkSignature>::VisitVarInitStmt(node);
    }
    const auto value = node->value;
    const auto type = node->type.getType();
    if(type && value) {
        const auto as_array = value->as_array_value();
        if(type->kind() == BaseTypeKind::Array && as_array) {
            const auto arr_type = ((ArrayType*) type);
            if(arr_type->has_no_array_size()) {
                arr_type->set_array_size(as_array->array_size());
            } else if(!as_array->has_explicit_size()) {
                as_array->set_array_size(arr_type->get_array_size());
            }
        }
    }
}

void TopLevelLinkSignature::VisitFunctionDecl(FunctionDeclaration* node) {
    linker.scope_start();

    // visiting the signature of the function
    for(auto param : node->params) {
        if(param->is_implicit()) {
            // implicit parameters are handled there
            param->link_implicit_param(linker);
        } else {
            visit(param->type);
        }
        if(param->defValue) {
            visit(param->defValue);
        }
    }
    visit(node->returnType);

    // TODO: we can't tell if signature resolved perfectly
    // TODO: eliminate this flag
    node->data.signature_resolved = true;

    if(node->isExtensionFn()) {
        node->put_as_extension_function(linker.allocator, linker);
    }
    linker.scope_end();
}

void TopLevelLinkSignature::LinkVariablesNoScope(VariablesContainer* container) {
    for (const auto var: container->variables()) {
        visit(var);
    }
}

void TopLevelLinkSignature::LinkMembersContainerNoScope(MembersContainer* container) {
    for(auto& inherits : container->inherited) {
        visit(inherits.type);
    }
    for (const auto var: container->variables()) {
        visit(var);
    }
    for(auto& func : container->functions()) {
        visit(func);
    }
}

void TopLevelLinkSignature::LinkVariables(VariablesContainer* container) {
    linker.scope_start();
    LinkVariablesNoScope(container);
    linker.scope_end();
}

void TopLevelLinkSignature::LinkMembersContainer(MembersContainer* container) {
    linker.scope_start();
    LinkMembersContainerNoScope(container);
    linker.scope_end();
}

void TopLevelLinkSignature::link_param(GenericTypeParameter* param) {
    if(param->at_least_type) {
        visit(param->at_least_type);
    }
    linker.declare(param->identifier, param);
    if(param->def_type) {
        visit(param->def_type);
    }
}

void TopLevelLinkSignature::VisitGenericTypeDecl(GenericTypeDecl* node) {
    auto& generic_params = node->generic_params;
    linker.scope_start();
    for(const auto param : generic_params) {
        link_param(param);
    }
    VisitTypealiasStmt(node->master_impl);
    linker.scope_end();
    node->signature_linked = true;
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
}

void TopLevelLinkSignature::VisitGenericFuncDecl(GenericFuncDecl* node) {
    auto& generic_params = node->generic_params;
    // symbol resolve the master declaration
    linker.scope_start();
    for(const auto param : generic_params) {
        link_param(param);
    }
    // we don't put the master implementation (into extendable container)
    // because the receiver could be generic
    visit(node->master_impl);
    // we set it has usage, so every shallow copy or instantiation has usage
    // since we create instantiation only when calls are detected, so no declaration will be created
    // when there's no usage
    node->master_impl->set_has_usage(true);
    linker.scope_end();
    node->signature_linked = true;
    // finalizing the signature of every function that was instantiated before link_signature
    auto& allocator = *linker.ast_allocator;;
    for(const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    auto resolved_sig = node->master_impl->data.signature_resolved;
    // since these instantiations were created before link signature
    // we must set the resolved_signature to true, which is false before link signature
    for(const auto inst : node->instantiations) {
        inst->data.signature_resolved = resolved_sig;
    }
    // finalize the signatures of all instantiations
    // this basically visits the instantiations signature and makes the types concrete
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // now that we have completely finalized the signature of instantiations
    // we will put all the functions if they are extension functions
    // they will go into their appropriate structs
//    if(master_impl->isExtensionFn()) {
//        for(const auto inst : instantiations) {
//            inst->put_as_extension_function(linker.allocator, linker);
//        }
//    }
}

void TopLevelLinkSignature::VisitGenericStructDecl(GenericStructDecl* node) {
    auto& generic_params = node->generic_params;
    linker.scope_start();
    for(const auto param : generic_params) {
        link_param(param);
    }
    LinkMembersContainerNoScope(node->master_impl);
    linker.scope_end();
    node->signature_linked = true;
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // since these instantiations were created before link_signature
    // the functions have signature_resolved set to false, we must fix that
    for(const auto inst : node->instantiations) {
        for(const auto func : inst->master_functions()) {
            // TODO set it to true, if its actually resolved
            func->FunctionType::data.signature_resolved = true;
        }
    }
    // since these instantiations were created before link_signature
    // they don't have any generated functions like default constructor / destructor
    for(const auto inst : node->instantiations) {
        // TODO we're passing the ast allocator, this generic could be at module level, in that case we should select the module alloctor
        inst->generate_functions(*linker.ast_allocator, linker);
    }
}

void TopLevelLinkSignature::VisitGenericUnionDecl(GenericUnionDecl* node) {
    auto& generic_params = node->generic_params;
    linker.scope_start();
    for(const auto param : generic_params) {
        link_param(param);
    }
    LinkMembersContainerNoScope(node->master_impl);
    linker.scope_end();
    node->signature_linked = true;
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // since these instantiations were created before link_signature
    // the functions have signature_resolved set to false, we must fix that
    for(const auto inst : node->instantiations) {
        for(const auto func : inst->master_functions()) {
            // TODO set it to true, if its actually resolved
            func->FunctionType::data.signature_resolved = true;
        }
    }
}

void TopLevelLinkSignature::VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
    auto& generic_params = node->generic_params;
    linker.scope_start();
    for(const auto param : generic_params) {
        link_param(param);
    }
    LinkMembersContainerNoScope(node->master_impl);
    linker.scope_end();
    node->signature_linked = true;
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // since these instantiations were created before link_signature
    // the functions have signature_resolved set to false, we must fix that
    for(const auto inst : node->instantiations) {
        for(const auto func : inst->master_functions()) {
            // TODO set it to true, if its actually resolved
            func->FunctionType::data.signature_resolved = true;
        }
    }
}

void TopLevelLinkSignature::VisitGenericVariantDecl(GenericVariantDecl* node) {
    auto& generic_params = node->generic_params;
    linker.scope_start();
    for(const auto param : generic_params) {
        link_param(param);
    }
    LinkMembersContainerNoScope(node->master_impl);
    linker.scope_end();
    node->signature_linked = true;
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    // finalize the siganture of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // since these instantiations were created before link_signature
    // the functions have signature_resolved set to false, we must fix that
    for(const auto inst : node->instantiations) {
        for(const auto func : inst->master_functions()) {
            // TODO set it to true, if its actually resolved
            func->FunctionType::data.signature_resolved = true;
        }
    }
    // since these instantiations were created before link_signature
    // they don't have any generated functions like default constructor / destructor
    for(const auto inst : node->instantiations) {
        // TODO we're passing the ast allocator, this generic could be at module level, in that case we should select the module alloctor
        inst->generate_functions(*linker.ast_allocator, linker);
    }
}

void TopLevelLinkSignature::VisitGenericImplDecl(GenericImplDecl* node) {
    auto& generic_params = node->generic_params;
    linker.scope_start();
    for(const auto param : generic_params) {
        link_param(param);
    }
    LinkMembersContainerNoScope(node->master_impl);
    linker.scope_end();
    node->signature_linked = true;
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        GenericImplDecl::finalize_signature(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // since these instantiations were created before link_signature
    // the functions have signature_resolved set to false, we must fix that
    for(const auto inst : node->instantiations) {
        for(const auto func : inst->master_functions()) {
            // TODO set it to true, if its actually resolved
            func->FunctionType::data.signature_resolved = true;
        }
    }
}

void TopLevelLinkSignature::VisitIfStmt(IfStatement* node) {
    if(node->is_top_level()) {
        auto scope = node->get_evaluated_scope_by_linking(linker);
        if(scope) {
            VisitByPtrTypeNoNullCheck(scope);
        }
    }
}

void TopLevelLinkSignature::VisitImplDecl(ImplDefinition* node) {
    linker.scope_start();
    // linking interface and struct type
    visit(node->interface_type);
    if (node->struct_type) {
        visit(node->struct_type);
    }
    // TODO: linked_node calling is not allowed in link_signature
    // this code should be moved to type checking pass
    const auto inter_linked = node->interface_type->linked_node();
    if (inter_linked) {
        const auto interface_def = inter_linked->as_interface_def();
        if (interface_def) {
            if (interface_def->is_static() && interface_def->has_implementation()) {
                linker.error("static interface must have only a single implementation", node->encoded_location());
            }
            interface_def->register_impl(node);
        } else {
            linker.error("expected type to be an interface", node->encoded_location());
        }
    }

    const auto linked_node = node->interface_type->get_direct_linked_node();
    if(!linked_node) {
        return;
    }
    const auto linked = linked_node->as_interface_def();
    if(!linked) {
        linker.error(node->interface_type.encoded_location()) << "couldn't find interface by this name for implementation";
        return;
    }
    for(auto& func : node->master_functions()) {
        if(!func->is_override()) {
            func->set_override(true);
        }
    }
    const auto struct_linked = node->struct_type ? node->struct_type->linked_struct_def() : nullptr;
    LinkMembersContainerNoScope(node);
    if(struct_linked) {
        // adding all methods of this implementation to struct
        struct_linked->adopt(node);
    }
    linker.scope_end();
}

void TopLevelLinkSignature::VisitInterfaceDecl(InterfaceDefinition* node) {
    LinkMembersContainer(node);
    node->ensure_inherited_visibility(linker, node->specifier());
}

void TopLevelLinkSignature::VisitNamespaceDecl(Namespace* node) {
    linker.scope_start();
    const auto root = node->root;
    if(root) {
        root->declare_extended_in_linker(linker);
    } else {
        node->declare_extended_in_linker(linker);
    }
    for(const auto child : node->nodes) {
        visit(child);
    }
    linker.scope_end();
}

void TopLevelLinkSignature::VisitScope(Scope* node) {
    for (const auto child : node->nodes) {
        visit(child);
    }
}

void TopLevelLinkSignature::VisitUnnamedStruct(UnnamedStruct* node) {
    node->take_variables_from_parsed_nodes(linker);
    LinkVariables(node);
}

void TopLevelLinkSignature::VisitStructDecl(StructDefinition* node) {
    auto& allocator = node->specifier() == AccessSpecifier::Public ? *linker.ast_allocator : *linker.mod_allocator;
    LinkMembersContainer(node);
    node->generate_functions(allocator, linker);
    node->ensure_inherited_visibility(linker, node->specifier());
}

void TopLevelLinkSignature::VisitUnionDecl(UnionDef* node) {
    LinkMembersContainer(node);
    node->ensure_inherited_visibility(linker, node->specifier());
}

void TopLevelLinkSignature::VisitVariantDecl(VariantDefinition* node) {
    auto& allocator = node->specifier() == AccessSpecifier::Public ? *linker.ast_allocator : *linker.mod_allocator;
    auto& diagnoser = linker;
    LinkMembersContainer(node);
    node->generate_functions(allocator, diagnoser);
    node->ensure_inherited_visibility(linker, node->specifier());
}

void TopLevelLinkSignature::VisitVariantMember(VariantMember* node) {
    for(auto& value : node->values) {
        VisitByPtrTypeNoNullCheck(value.second);
    }
}

void TopLevelLinkSignature::VisitUnnamedUnion(UnnamedUnion* node) {
    node->take_variables_from_parsed_nodes(linker);
    LinkVariables(node);
}