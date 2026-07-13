// Copyright (c) Chemical Language Foundation 2025.

#include "LinkSignature.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "ast/statements/UsingStmt.h"
#include "ast/statements/AliasStmt.h"
#include "ast/statements/Typealias.h"
#include "ast/base/TypeBuilder.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Export.h"
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
#include "ast/types/IfType.h"
#include "ast/types/LinkedValueType.h"
#include "LinkSignatureAPI.h"
#include "ast/statements/UnresolvedDecl.h"
#include "compiler/SymbolResolver.h"
#include "compiler/frontend/AnnotationController.h"

/**
 * Visit the where clause of a function to link constraint types
 */
static void link_where_clause(TopLevelLinkSignature& signatureLinker, FunctionDeclaration* decl) {
    if (!decl->where_clause) return;
    for (auto& constraint : decl->where_clause->constraints) {
        // resolve the generic type parameter by name
        const auto found = signatureLinker.tld_find(constraint.param_name);
        if (found && found->kind() == ASTNodeKind::GenericTypeParam) {
            constraint.param = found->as_generic_type_param_unsafe();
        }
        // visit the trait types to link them
        for (auto& trait_type : constraint.constraints) {
            signatureLinker.visit(trait_type);
        }
    }
}

SymbolResolver* SymResLinkSignaturegetSymbolResolver(TopLevelLinkSignature* visitor) {
    return &visitor->linker;
}

void SymResLinkSignaturevisitNode(TopLevelLinkSignature* visitor, ASTNode* node) {
    visitor->visit(node);
}

void SymResLinkSignaturevisitValue(TopLevelLinkSignature* visitor, Value* value) {
    visitor->visit(value);
}

void SymResLinkSignaturevisitEmbeddedNode(TopLevelLinkSignature* visitor, EmbeddedNode* node) {
    auto& table = visitor->table;
    for(const auto child_node : node->chemical_nodes) {
        table.scope_start();
        visitor->visit(child_node);
        table.scope_end();
    }
    for(const auto child_val : node->chemical_values) {
        visitor->visit(child_val);
    }
}

void SymResLinkSignaturevisitEmbeddedValue(TopLevelLinkSignature* visitor, EmbeddedValue* value) {
    auto& table = visitor->table;
    for(const auto child_node : value->chemical_nodes) {
        table.scope_start();
        visitor->visit(child_node);
        table.scope_end();
    }
    for(const auto child_val : value->chemical_values) {
        visitor->visit(child_val);
    }
}

void sym_res_signature(SymbolResolver& resolver, Scope* scope) {
    TopLevelLinkSignature visitor(resolver);
    visitor.visit(scope);
}

void TopLevelLinkSignature::VisitVariableIdentifier(VariableIdentifier* value) {
    const auto decl = tld_find(value->value);
    if(decl) {
        value->linked = decl;
        const auto k = decl->known_type();
        value->setType(k);
        // special case check, when accessing a var decl
        // it may have not been linked yet, because of global variable inter-dependence
        // we don't want to force the user to give types, because mostly type can be inferred
        // when type can't be inferred, we generate this error
        if (k == nullptr && decl->kind() == ASTNodeKind::VarInitStmt) {
            value->linked = (ASTNode*) get_unresolved_decl();
            value->setType(value->linked->known_type());
            diagnoser.error(value) << "couldn't infer type of global variable, please specify type of global variable being accessed";
            diagnoser.info(decl) << "explicit type not given, however accessed in current module";
        } else {
            // to ensure function decl is informed about usage
            value->process_linked(&diagnoser, nullptr);
        }
    } else if(value->linked == nullptr) {
        diagnoser.error(value) << "unresolved variable identifier '" << value->value << "' not found";
        value->linked = (ASTNode*) get_unresolved_decl();
        value->setType(value->linked->known_type());
    }
}

ASTNode* get_chain_linked(Value* value) {
    const auto id = value->get_last_id();
    return id ? id->linked : nullptr;
}

inline void check_type_exported(ASTDiagnoser& diagnoser, ASTNode* linked, SourceLocation location) {
    const auto spec = linked->specifier(AccessSpecifier::Public);
    if(!is_linkage_public(spec)) {
        diagnoser.error("non exported type being used in a public type, please use 'public' or 'protected' to expose it", location);
    }
}

void TopLevelLinkSignature::VisitLinkedType(LinkedType* type) {
    if(type->is_value()) {
        const auto linked_type = (LinkedValueType*) type;
        // only identifiers would be allowed in this value referencing type
        visit(linked_type->value);
        // now lets check the last item in the value (considering it an access chain)
        const auto linked = get_chain_linked(linked_type->value);
        if(linked) {
            type->linked = linked;
            if(require_exported) check_type_exported(linker, linked, type_location);
        } else {
            type->linked = (ASTNode*) get_unresolved_decl();
            diagnoser.error(type_location) << "unresolved type not found";
        }
    } else if(type->is_named()){
        const auto named = (NamedLinkedType*) type;
        const auto decl = tld_find(named->debug_link_name());
        if(decl) {
            type->linked = decl;
            if(require_exported) check_type_exported(linker, decl, type_location);
        } else if(type->linked == nullptr) {
            diagnoser.error(type_location) << "unresolved type, '" << named->debug_link_name() << "' not found";
            type->linked = (ASTNode*) get_unresolved_decl();
        }
    }
}

ASTNode* get_chain_item_parent(Value* value) {
    switch(value->kind()) {
        case ValueKind::Identifier:
            return value->as_identifier_unsafe()->linked;
        case ValueKind::IndexOperator:
        case ValueKind::FunctionCall:
        case ValueKind::AccessChain:
            return value->getType()->get_linked_node();
        default:
            return nullptr;
    }
}

void TopLevelLinkSignature::VisitAccessChain(AccessChain* value) {
    // an access chain during link signature is being used
    // this must be for example in using statement using ns::something::some
    // or it could be top level function call in var statement var x = ns::some()

#ifdef DEBUG
    if(value->values.empty()) {
        diagnoser.error(value) << "empty access chain detected";
        return;
    }
#endif

    // take the first value
    auto& first = value->values[0];

    // visit the first element normally
    visit(first);

    // no need to traverse further, if only single element
    if(value->values.size() == 1) {
        value->setType(value->values[0]->getType());
        return;
    }

    // get the first chain item parent
    auto parent = get_chain_item_parent(first);
    if(parent == nullptr) {
        return;
    }

    // access chain contains multiple values
    // its guaranteed that values in access chain, after the first value are identifiers
    unsigned i = 1;
    const auto size = value->values.size();
    while(i < size) {
#ifdef DEBUG
      if (value->values[i]->kind() != ValueKind::Identifier) {
          CHEM_THROW_RUNTIME("value should be an identifier, but isn't");
      }
#endif
        const auto child = value->values[i]->as_identifier_unsafe();
        // special case check, when accessing a var decl
        // it may have not been linked yet, because of global variable inter-dependence
        // we don't want to force the user to give types, because mostly type can be inferred
        // when type can't be inferred, we generate this error
        if (parent->kind() == ASTNodeKind::VarInitStmt) {
            const auto stmt = parent->as_var_init_unsafe();
            const auto stmtType = stmt->known_type();
            if (stmtType == nullptr) {
                child->linked = (ASTNode*) get_unresolved_decl();
                child->setType(child->linked->known_type());
                diagnoser.error(value->values[i - 1]) << "couldn't infer type of global variable, please specify type of global variable being accessed";
                diagnoser.info(parent) << "explicit type not given, however accessed in current module";
                i++;
                continue;
            }
        }
        const auto child_linked = parent->child(getChildResolver(), child->value);
        if(child_linked) {
            child->linked = child_linked;
            child->setType(child_linked->known_type());
            parent = child_linked;
        } else {
            child->linked = (ASTNode*) get_unresolved_decl();
            child->setType(child->linked->known_type());
            diagnoser.error(child) << "unresolved identifier, '" << child->value << "' not found";
            diagnoser.info(parent) << "declaration doesn't contain child by name '" << child->value << "'";
            parent = child->linked;
        }
        i++;
    }

    // the last item holds the type for this access chain
    value->setType(value->values[size - 1]->getType());

}

void TopLevelLinkSignature::VisitFunctionCall(FunctionCall* value) {
    visit(value->parent_val);
    const auto last_linked = value->parent_val->get_chain_last_linked();
    if (last_linked == nullptr) {
        value->setType(get_unresolved_decl()->known_type());
        diagnoser.error(value) << "call to unsupported parent value at top level";
        return;
    }
    switch(last_linked->kind()) {
        case ASTNodeKind::FunctionDecl: {
            const auto decl = last_linked->as_function_unsafe();
            value->setType(decl->returnType);
            if(decl->is_comptime()) {

            } else {
                diagnoser.error(value) << "cannot call a runtime function at top level";
            }
            return;
        }
        case ASTNodeKind::VariantMember: {
            const auto mem = last_linked->as_variant_member_unsafe();
            value->setType(mem->parent()->known_type());
            return;
        }
        default: {
            value->setType(get_unresolved_decl()->known_type());
            diagnoser.error(value) << "call to unsupported parent value at top level";
            return;
        }
    }
}

void TopLevelLinkSignature::VisitComptimeBlock(ComptimeBlock* node) {
    if(comptime_context) {
        RecursiveVisitor<TopLevelLinkSignature>::VisitComptimeBlock(node);
    } else {
        comptime_context = true;
        RecursiveVisitor<TopLevelLinkSignature>::VisitComptimeBlock(node);
        comptime_context = false;
    }
}

void TopLevelLinkSignature::VisitExpression(Expression* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitExpression(value);
    value->determine_type(
        getTypeBuilder(),
        getCoreNodes(),
        getImplsIndex(),
        linker,
        getTargetData()
    );
    if(!comptime_context) {
        diagnoser.error("cannot evaluate expression at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::VisitAddrOfValue(AddrOfValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitAddrOfValue(value);
    value->determine_type();
    if(!comptime_context) {
        diagnoser.error("cannot take address of value at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::VisitReferenceOfValue(ReferenceOfValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitReferenceOfValue(value);
    value->determine_type();
}

void TopLevelLinkSignature::VisitArrayValue(ArrayValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitArrayValue(value);
    value->determine_type(getAstAllocator());
}

void TopLevelLinkSignature::VisitComptimeValue(ComptimeValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitComptimeValue(value);
    // type determined during symbol resolution needs to be set
    value->setType(value->getValue()->getType());
}

void TopLevelLinkSignature::VisitDereferenceValue(DereferenceValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitDereferenceValue(value);
    // determining the type for this dereference value
    auto& typeBuilder = getTypeBuilder();
    if (!value->determine_type(typeBuilder)) {
        diagnoser.error("couldn't determine type for de-referencing", value);
    }
    if(!comptime_context) {
        diagnoser.error("cannot dereference value at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::VisitIncDecValue(IncDecValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitIncDecValue(value);
    value->setType(value->determine_type(linker, getCoreNodes(), getImplsIndex()));
    if(!comptime_context) {
        diagnoser.error("cannot increment or decrement value at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::VisitIndexOperator(IndexOperator* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitIndexOperator(value);
    // determining the type for this index operator
    auto& typeBuilder = getTypeBuilder();
    value->determine_type(typeBuilder, getCoreNodes(), getImplsIndex(), linker);
    if(!comptime_context) {
        diagnoser.error("cannot index into a value at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::TopLevelLinkSignature::VisitIsValue(IsValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitIsValue(value);
    if(!comptime_context) {
        diagnoser.error("cannot determine at runtime outside function body", value);
    }
}

void TopLevelLinkSignature::VisitLambdaFunction(LambdaFunction* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitLambdaFunction(value);
    if(!comptime_context) {
        // NOTE: this requires resolving lambda type, for which code should be separate
        // also requires knowing expected type, however values are visited automatically
        // due to nature of recursive visitor, we cannot send expected types
        diagnoser.error("lambda functions at top level outside function body aren't supported", value);
    }
}

void TopLevelLinkSignature::VisitNegativeValue(NegativeValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitNegativeValue(value);
    // determine type for negative value
    value->determine_type(getTypeBuilder(), getCoreNodes(), getImplsIndex(), linker);
}

void TopLevelLinkSignature::VisitUnsafeValue(UnsafeValue* value) {
    const auto prev = safe_context;
    safe_context = false;
    RecursiveVisitor<TopLevelLinkSignature>::VisitUnsafeValue(value);
    safe_context = prev;
    value->setType(value->getValue()->getType());
}

const auto RUNTIME_EVAL_ERR = "cannot evaluate at runtime outside function body";

void TopLevelLinkSignature::VisitNewValue(NewValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitNewValue(value);
    // type determined at symbol resolution must be set
    value->ptr_type.type = value->value->getType();
    if(!comptime_context) {
        diagnoser.error(RUNTIME_EVAL_ERR, value);
    }
}

void TopLevelLinkSignature::VisitNewTypedValue(NewTypedValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitNewTypedValue(value);
    if(!comptime_context) {
        diagnoser.error(RUNTIME_EVAL_ERR, value);
    }
}

void TopLevelLinkSignature::VisitPlacementNewValue(PlacementNewValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitPlacementNewValue(value);
    // type of the value determined at symbol resolution must be set
    value->ptr_type.type = value->value->getType();
    if(!comptime_context) {
        diagnoser.error(RUNTIME_EVAL_ERR, value);
    }
}

void TopLevelLinkSignature::VisitNotValue(NotValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitNotValue(value);
    // determine the type of not value
    value->determine_type(linker, getCoreNodes(), getImplsIndex());
    if(!comptime_context) {
        diagnoser.error(RUNTIME_EVAL_ERR, value);
    }
}

void TopLevelLinkSignature::VisitBitwiseNot(BitwiseNot* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitBitwiseNot(value);
    // determine the type of bitwise not value
    value->determine_type(linker, getCoreNodes(), getImplsIndex());
    if(!comptime_context) {
        diagnoser.error(RUNTIME_EVAL_ERR, value);
    }
}

void TopLevelLinkSignature::VisitPatternMatchExpr(PatternMatchExpr* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitPatternMatchExpr(value);
    // TODO: maybe pattern match expression should be a node
    // currently we emplace a void type
    // as expression is only used as a statement
    value->setType((BaseType*) getTypeBuilder().getVoidType());
}

void TopLevelLinkSignature::VisitBlockValue(BlockValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitBlockValue(value);
    if(!comptime_context) {
        diagnoser.error(RUNTIME_EVAL_ERR, value);
    }
}

void TopLevelLinkSignature::VisitStructValue(StructValue* value) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitStructValue(value);
    const auto structValue = value;
    const auto refType = structValue->getRefType();
    if(refType) {
        TypeLoc temp_loc {refType, value->encoded_location()};
        visit(temp_loc);
        if(temp_loc.getType() != refType) {
            value->setRefType(temp_loc);
        }
    } else {
        diagnoser.error("unnamed struct value cannot link without a type", structValue);
        structValue->setType(new (getAstAllocator().allocate<StructType>()) StructType("", nullptr, structValue->encoded_location()));
        return;
    }
    if(!structValue->resolve_container(getGenericInstantiatorAPI(), !generic_context)) {
        return;
    }
    structValue->diagnose_missing_members_for_init(linker);
    if(!structValue->allows_direct_init()) {
        diagnoser.error(structValue) << "struct with name '" << structValue->linked_extendable()->name_view() << "' has a constructor, use @direct_init to allow direct initialization";
    }
    auto refTypeKind = structValue->getRefType()->kind();
    if(refTypeKind == BaseTypeKind::Generic) {
        for (auto& arg: structValue->generic_list()) {
            visit(arg);
        }
    }
}

void TopLevelLinkSignature::VisitEmbeddedNode(EmbeddedNode* node) {
    auto found = getCompilerBinder().findHook(node->name, CBIFunctionType::SymResLinkSignatureNode);
    if(found) {
        ((EmbeddedNodeSymResLinkSignature) found)(this, node);
    } else {
        diagnoser.error(node) << "couldn't find link signature method for embedded node with name '" << node->name << "'";
    }
}

void TopLevelLinkSignature::VisitEmbeddedValue(EmbeddedValue* value) {
    auto found = getCompilerBinder().findHook(value->name, CBIFunctionType::SymResLinkSignatureValue);
    if(found) {
        ((EmbeddedValueSymResLinkSignature) found)(this, value);
    } else {
        diagnoser.error(value) << "couldn't find link signature method for embedded value with name '" << value->name << "'";
    }
}

void TopLevelLinkSignature::VisitGenericType(GenericType* type) {
    // save the type into a temporary before visiting children
    auto loc = type_location;
    // must be visited first, so child generic types are instantiated and ready
    RecursiveVisitor<TopLevelLinkSignature>::VisitGenericType(type);
    // we must instantiate generic declarations and link with those
    // only if we are not present in generic context
    type->instantiate_inline(getGenericInstantiatorAPI(), loc);

}

void TopLevelLinkSignature::VisitArrayType(ArrayType* type) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitArrayType(type);
    // array types require calculating array size from given expression
    const auto arr_size_val = type->array_size_value;
    if(arr_size_val == nullptr) return;
    const auto evaluated = arr_size_val->evaluated_value(linker.comptime_scope);
    if(evaluated == nullptr) return;
    const auto number = evaluated->get_number();
    if(number.has_value()) {
        type->set_array_size(number.value());
    }
}

void TopLevelLinkSignature::VisitUsingStmt(UsingStmt* node) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitUsingStmt(node);
    node->declare_symbols(table, linker);
}

void TopLevelLinkSignature::VisitAliasStmt(AliasStmt* stmt) {
    const auto value = stmt->value;

    // currently only identifier values are supported
    if (value->kind() == ValueKind::AccessChain) {
        const auto chain = value->as_access_chain_unsafe();
        if (chain->values.size() != 1 || chain->values.front()->kind() != ValueKind::Identifier) {
            diagnoser.error(stmt) << "incompatible value given to alias";
            return;
        }
    }

    // TODO: this value can fail resolution, however we proceed as if it doesn't
    // we shouldn't use alias statement
    visit(stmt->value);

    const auto node = value->get_chain_last_linked();
    if (!node) {
        diagnoser.error(stmt) << "cannot alias incompatible value";
        return;
    }
    if (stmt->specifier >= node->specifier()) {
        diagnoser.error(stmt) << "cannot alias a node to a higher specifier";
        return;
    }

    // declares the node without runtime
    table.declare(stmt->alias_name, node);

}

void TopLevelLinkSignature::VisitVarInitStmt(VarInitStatement* node) {
    if (node->type == nullptr) {
        if (!node->value) {
            diagnoser.error("a type of a value must be given for global variable", node);
            return;
        }
        if(node->is_comptime() && !comptime_context) {
            comptime_context = true;
            visit(node->value);
            comptime_context = false;
        } else {
            visit(node->value);
        }
        const auto maybe_type = node->value->getType();
        if (maybe_type != nullptr && maybe_type->isPrimitive(false)) {
            node->type = {maybe_type, node->value->encoded_location()};
        } else {
            return;
        }
    } else {
        if(node->is_comptime() && !comptime_context) {
            comptime_context = true;
            visit(node->type);
            comptime_context = false;
        } else {
            visit(node->type);
        }
        if (node->value) {
            if(node->is_comptime() && !comptime_context) {
                comptime_context = true;
                visit(node->value);
                comptime_context = false;
            } else {
                visit(node->value);
            }
        }
    }
    // array type size determination from array value
    const auto value = node->value;
    const auto type = node->type.getType();
    if(type && value) {
        if(type->kind() == BaseTypeKind::Array && value->kind() == ValueKind::ArrayValue) {
            const auto as_array = value->as_array_value_unsafe();
            const auto arr_type = ((ArrayType*) type);
            if(arr_type->has_no_array_size()) {
                arr_type->set_array_size(as_array->array_size());
            } else if(!as_array->has_explicit_size()) {
                as_array->set_array_size(arr_type->get_array_size());
            }
        }
    }
}

void TopLevelLinkSignature::VisitTypealiasStmt(TypealiasStatement* stmt) {
    if(comptime_context) {
        RecursiveVisitor<TopLevelLinkSignature>::VisitTypealiasStmt(stmt);
    } else {
        comptime_context = true;
        RecursiveVisitor<TopLevelLinkSignature>::VisitTypealiasStmt(stmt);
        comptime_context = false;
    }
    if(stmt->actual_type->kind() == BaseTypeKind::IfType) {
        const auto if_type = stmt->actual_type->as_if_type_unsafe();
        auto evaluated = if_type->evaluate(linker.comptime_scope);
        if(evaluated) {
            stmt->actual_type = evaluated;
        } else {
            diagnoser.error("couldn't evaluate the if type", stmt->actual_type.encoded_location());
            stmt->actual_type = if_type->thenType;
        }
    }
}

void visit_func_decl(TopLevelLinkSignature& sig, FunctionDeclaration* node) {
    auto& table = sig.table;
    table.scope_start();

    // visiting the signature of the function
    for(auto param : node->params) {
        if(param->is_implicit()) {
            // implicit parameters are handled there
            // TODO: this method exists in sym res link body
            // we link parameters with 'self' params in link signature, so it should be moved here
            // second, in link body, no self parameters should exist, and we shouldn't be providing any support for them
            param->link_implicit_param(sig.linker);
        } else {
            sig.visit(param->type);
        }
        if(param->defValue) {
            sig.visit(param->defValue);
        }
    }
    sig.visit(node->returnType);

    // visit the where clause to link constraint types
    link_where_clause(sig, node);

    // TODO: we can't tell if signature resolved perfectly
    // TODO: eliminate this flag
    node->data.signature_resolved = true;

    if(node->isExtensionFn()) {
        node->put_as_extension_function(sig.diagnoser);
    }
    table.scope_end();
}

void TopLevelLinkSignature::VisitFunctionDecl(FunctionDeclaration* node) {
    if(!node->is_top_level()) {
        visit_func_decl(*this, node);
    } else {
        const auto spec = node->specifier();
        if(spec == AccessSpecifier::Public || spec == AccessSpecifier::Protected) {
            if(require_exported) {
                visit_func_decl(*this, node);
            } else {
                require_exported = true;
                visit_func_decl(*this, node);
                require_exported = false;
            }
        } else {
            visit_func_decl(*this, node);
        }
    }
}

void configure_members_by_inheritance(EnumDeclaration* current, int start) {
    // build sorted list of enum members (sorted by the index specified by the user)
    std::vector<EnumMember*> sorted_members;
    sorted_members.reserve(current->members.size());
    for(auto& member : current->members) {
        sorted_members.emplace_back(member.second);
    }
    std::stable_sort(std::begin(sorted_members), std::end(sorted_members), [](EnumMember* a, EnumMember* b) {
        return a->get_index_dirty() < b->get_index_dirty();
    });
    // now that we have a sorted list of members for current enum
    // we should modify its member index as long as we can predict it
    auto counter = 0;
    for(const auto mem : sorted_members) {
        if(mem->get_index_dirty() == counter) {
            // this means user didn't modify the index
            // we should modify it
            mem->set_index_dirty(start);
            start++;
            counter++;
        } else {
            return;
        }
    }
}

void TopLevelLinkSignature::VisitEnumDecl(EnumDeclaration* node) {
    RecursiveVisitor<TopLevelLinkSignature>::VisitEnumDecl(node);
    auto& underlying_type = node->underlying_type;
    auto& underlying_integer_type = node->underlying_integer_type;
    const auto pure_underlying = underlying_type->canonical();
    const auto k = pure_underlying->kind();
    if(k == BaseTypeKind::IntN) {
        underlying_integer_type = pure_underlying->as_intn_type_unsafe();
    } else {
        const auto linked = pure_underlying->get_direct_linked_node();
        if(linked->kind() == ASTNodeKind::EnumDecl) {
            const auto inherited = linked->as_enum_decl_unsafe();
            configure_members_by_inheritance(node, inherited->next_start);
            underlying_integer_type = inherited->underlying_integer_type;
        } else {
            diagnoser.error("given type is not an enum or integer type", node->encoded_location());
            underlying_integer_type = getTypeBuilder().getIntType();
        }
    }
}

void TopLevelLinkSignature::LinkVariablesNoScope(VariablesContainerBase* container) {
    for (const auto var: container->variables()) {
        visit(var);
    }
}

void TopLevelLinkSignature::LinkMembersContainerNoScope(MembersContainer* container) {
    // linking inherited types
    auto& inherited = container->inherited;
    for(auto& inherits : inherited) {
        visit(inherits.type);
    }
    // linking variables and default values
    for (const auto var: container->variables()) {
        visit(var);
        // verify the type of the default value
        const auto defValue = var->default_value();
        if(defValue != nullptr) {
            const auto type = var->known_type();
            const auto imp_constructor = type->implicit_constructor_for(defValue);
            if (imp_constructor == nullptr && !type->satisfies(defValue, false)) {
                diagnoser.unsatisfied_type_error(defValue, type);
            }
        }
    }
    // linking functions
    for(auto& func : container->evaluated_nodes()) {
        visit(func);
    }
}

void TopLevelLinkSignature::LinkMembersContainerNoScopeExposed(MembersContainer* container) {
    if(require_exported) {
        LinkMembersContainerNoScope(container);
    } else {
        require_exported = true;
        LinkMembersContainerNoScope(container);
        require_exported = false;
    }
}

void TopLevelLinkSignature::LinkVariables(VariablesContainerBase* container) {
    table.scope_start();
    LinkVariablesNoScope(container);
    table.scope_end();
}

void TopLevelLinkSignature::LinkMembersContainer(MembersContainer* container) {
    table.scope_start();
    LinkMembersContainerNoScope(container);
    table.scope_end();
}

void TopLevelLinkSignature::LinkMembersContainerExposed(MembersContainer* container) {
    table.scope_start();
    LinkMembersContainerNoScopeExposed(container);
    table.scope_end();
}

void TopLevelLinkSignature::LinkMembersContainer(MembersContainer* container, AccessSpecifier specifier) {
    switch(specifier) {
        case AccessSpecifier::Public:
        case AccessSpecifier::Protected:
            LinkMembersContainerExposed(container);
            return;
        default:
            LinkMembersContainer(container);
            return;
    }
}

void TopLevelLinkSignature::link_param(GenericTypeParameter* param) {
    // visit trait and default types to resolve their symbol references
    for(auto& t : param->traits) {
        visit(t);
    }
    if(param->def_type) {
        visit(param->def_type);
    }
    // declare the parameter later, so trait types and default types can't link with it
    // preventing self references
    table.declare(param->identifier, param);
}

void TopLevelLinkSignature::VisitGenericTypeDecl(GenericTypeDecl* node) {
    auto& generic_params = node->generic_params;
    table.scope_start();
    const auto prev_gen_context = generic_context;
    generic_context = true;
    for(const auto param : generic_params) {
        link_param(param);
    }
    VisitTypealiasStmt(node->master_impl);
    generic_context = prev_gen_context;
    table.scope_end();
    node->signature_linked = true;
}

void TopLevelLinkSignature::VisitGenericFuncDecl(GenericFuncDecl* node) {
    auto& generic_params = node->generic_params;
    // symbol resolve the master declaration
    table.scope_start();
    const auto prev_gen_context = generic_context;
    generic_context = true;
    for(const auto param : generic_params) {
        link_param(param);
    }
    // we don't put the master implementation (into extendable container)
    // because the receiver could be generic
    visit(node->master_impl);
    generic_context = prev_gen_context;
    // we set it has usage, so every shallow copy or instantiation has usage
    // since we create instantiation only when calls are detected, so no declaration will be created
    // when there's no usage
    node->master_impl->set_has_usage(true);
    table.scope_end();
}

void TopLevelLinkSignature::VisitGenericStructDecl(GenericStructDecl* node) {
    auto& generic_params = node->generic_params;
    table.scope_start();
    const auto prev_gen_context = generic_context;
    generic_context = true;
    for(const auto param : generic_params) {
        link_param(param);
    }
    LinkMembersContainerNoScope(node->master_impl);
    generic_context = prev_gen_context;
    table.scope_end();
    // we must generate functions for master as well
    // because user can call the constructor of master implementation, which should be available
    // if this creates a destructor, then it would be copied in instantiations and instantiations won't generate another destructor
    // similarly for default constructor
    node->master_impl->generate_functions(getAstAllocator(), linker, node);
}

void TopLevelLinkSignature::VisitGenericUnionDecl(GenericUnionDecl* node) {
    auto& generic_params = node->generic_params;
    table.scope_start();
    const auto prev_gen_context = generic_context;
    generic_context = true;
    for(const auto param : generic_params) {
        link_param(param);
    }
    LinkMembersContainerNoScope(node->master_impl);
    generic_context = prev_gen_context;
    table.scope_end();
}

void TopLevelLinkSignature::VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
    auto& generic_params = node->generic_params;
    table.scope_start();
    const auto prev_gen_context = generic_context;
    generic_context = true;
    for(const auto param : generic_params) {
        link_param(param);
    }
    LinkMembersContainerNoScope(node->master_impl);
    generic_context = prev_gen_context;
    table.scope_end();
}

void TopLevelLinkSignature::VisitGenericVariantDecl(GenericVariantDecl* node) {
    auto& generic_params = node->generic_params;
    table.scope_start();
    const auto prev_gen_context = generic_context;
    generic_context = true;
    for(const auto param : generic_params) {
        link_param(param);
    }
    LinkMembersContainerNoScope(node->master_impl);
    generic_context = prev_gen_context;
    table.scope_end();
    // we must generate functions for master as well
    // because user can call the constructor of master implementation, which should be available
    // if this creates a destructor, then it would be copied in instantiations and instantiations won't generate another destructor
    // similarly for default constructor
    node->master_impl->generate_functions(getAstAllocator(), linker, node);
}

void TopLevelLinkSignature::VisitGenericImplDecl(GenericImplDecl* node) {
    auto& generic_params = node->generic_params;
    table.scope_start();
    const auto prev_gen_context = generic_context;
    generic_context = true;
    for(const auto param : generic_params) {
        link_param(param);
    }
    LinkMembersContainerNoScope(node->master_impl);
    generic_context = prev_gen_context;
    table.scope_end();
}

class TopLevelIfStmtConditionChecker : public RecursiveVisitor<TopLevelIfStmtConditionChecker> {
public:

    ASTDiagnoser& diagnoser;

    ModuleScope* currModScope;

    SourceLocation type_location;

    bool has_error = false;

    TopLevelIfStmtConditionChecker(
        ASTDiagnoser& diagnoser,
        ModuleScope* currModScope,
        SourceLocation location
    ) : diagnoser(diagnoser), currModScope(currModScope), type_location(location) {

    }

    template<typename T>
    inline void visit(T* ptr) {
        VisitByPtrTypeNoNullCheck(ptr);
    }
    inline void visit(ASTNode* node) {
        VisitNodeNoNullCheck(node);
    }
    inline void visit(BaseDefMember* node) {
        VisitNodeNoNullCheck(node);
    }
    inline void visit(Value* value) {
        VisitValueNoNullCheck(value);
    }
    inline void visit(BaseType*& type_ref) {
        VisitTypeNoNullCheck(type_ref);
    }
    inline void visit(TypeLoc& type) {
        type_location = type.getLocation();
        auto changed = const_cast<BaseType*>(type.getType());
        VisitTypeNoNullCheck(changed);
    }
    inline void visit(LinkedType*& type_ref) {
        visit((BaseType*&) type_ref);
    }
    inline void visit(Scope& scope) {
        VisitScope(&scope);
    }

    void VisitVariableIdentifier(VariableIdentifier* value) {
        const auto parent = value->linked->get_mod_scope();
        if (parent == currModScope) {
            diagnoser.error("top level comptime if statement condition must not reference a symbol from current module", value);
            has_error = true;
        }
    }

    void VisitLinkedType(LinkedType* type) {
        const auto parent = type->linked->get_mod_scope();
        if (parent == currModScope) {
            diagnoser.error("top level comptime if statement condition must not reference a symbol from current module", type_location);
            has_error = true;
        }
    }

};

bool CheckTopLevelComptimeIfStmtCondition(
    ASTDiagnoser& diagnoser,
    Value* condition,
    ModuleScope* currModScope
) {
    if (currModScope == nullptr) {
        return false;
    }
    auto checker = TopLevelIfStmtConditionChecker(diagnoser, currModScope, condition->encoded_location());
    checker.visit_it(condition);
    return !checker.has_error;
}

void IfStatement::link_conditions(SymbolResolver &linker) {
    TopLevelLinkSignature symRes(linker);
    auto prev_comptime = symRes.comptime_context;
    symRes.comptime_context = true;
    symRes.visit(condition);
    for (auto& cond: elseIfs) {
        symRes.visit(cond.first);
    }
    symRes.comptime_context = prev_comptime;
}

void TopLevelLinkSignature::VisitIfStmt(IfStatement* node) {
    if (node->computed_scope.has_value()) {
        const auto scope = node->computed_scope.value();
        if (scope) {
            VisitByPtrTypeNoNullCheck(scope);
        }
    }
}
//
// enum class IndexingDefaultImplsKind {
//     Primitive,
//     Pointer,
//     Reference
// };
//
// void index_default_implementations(SymbolResolver& resolver, ImplDefinition* implDef, InterfaceDefinition* def, IndexingDefaultImplsKind kind) {
//     switch (kind) {
//         case IndexingDefaultImplsKind::Primitive:
//             for (const auto func : def->evaluated_nodes()) {
//                 if (func->kind() == ASTNodeKind::FunctionDecl && func->as_function_unsafe()->body.has_value()) {
//                     resolver.child_resolver.index_primitive_child_try(implDef->struct_type, func->as_function_unsafe()->name_view(), func);
//                 }
//             }
//             break;
//         case IndexingDefaultImplsKind::Pointer:
//             for (const auto func : def->evaluated_nodes()) {
//                 if (func->kind() == ASTNodeKind::FunctionDecl && func->as_function_unsafe()->body.has_value()) {
//                     if(!resolver.child_resolver.index_ptr_child(implDef->struct_type->as_pointer_type_unsafe(), func->as_function_unsafe()->name_view(), func)) {
//                         resolver.error("implementation for type is not allowed", implDef->struct_type.encoded_location());
//                         break;
//                     }
//                 }
//             }
//             break;
//         case IndexingDefaultImplsKind::Reference:
//             for (const auto func : def->evaluated_nodes()) {
//                 if (func->kind() == ASTNodeKind::FunctionDecl && func->as_function_unsafe()->body.has_value()) {
//                     if(!resolver.child_resolver.index_ref_child(implDef->struct_type->as_reference_type_unsafe(), func->as_function_unsafe()->name_view(), func)) {
//                         resolver.error("implementation for type is not allowed", implDef->struct_type.encoded_location());
//                         break;
//                     }
//                 }
//             }
//             break;
//     }
//     // run on inherited functions
//     for (auto& inh : def->inherited) {
//         const auto canonical = inh.type->get_direct_linked_canonical_node();
//         if (canonical && canonical->kind() == ASTNodeKind::InterfaceDecl) {
//             index_default_implementations(resolver, implDef, canonical->as_interface_def_unsafe(), kind);
//         }
//     }
// }
//
// // this generates shallow functions for the impl
// // the default implementations present in the interface
// void create_default_implementations(SymbolResolver& resolver, ImplDefinition* implDef, InterfaceDefinition* def) {
//     auto& allocator = *resolver.ast_allocator;
//     for (const auto func : def->evaluated_nodes()) {
//         if (func->kind() == ASTNodeKind::FunctionDecl) {
//             const auto default_func = func->as_function_unsafe();
//             if (!default_func->body.has_value()) continue;
//             // TODO: two functions with same name can exist in multiple interfaces
//             if (implDef->direct_child_function(default_func->name_view()) != nullptr) continue;
//             const auto copied = default_func->shallow_copy(allocator);
//             copied->FunctionType::data.signature_resolved = true;
//             copied->set_parent(implDef);
//             implDef->insert_func(copied);
//         }
//     }
//     // run on inherited functions
//     for (auto& inh : def->inherited) {
//         const auto canonical = inh.type->get_direct_linked_canonical_node();
//         if (canonical && canonical->kind() == ASTNodeKind::InterfaceDecl) {
//             create_default_implementations(resolver, implDef, canonical->as_interface_def_unsafe());
//         }
//     }
// }

void TopLevelLinkSignature::VisitImplDecl(ImplDefinition* node) {
    table.scope_start();
    // linking interface and struct type
    visit(node->interface_type);
    if (node->struct_type) {
        visit(node->struct_type);
    }
    LinkMembersContainerNoScope(node);
    table.scope_end();
}

bool is_object_safe(InterfaceDefinition* node) {
    // determining if the interface is object safe
    for(const auto func_node : node->evaluated_nodes()) {
        switch(func_node->kind()) {
            case ASTNodeKind::FunctionDecl:{
                const auto func = func_node->as_function_unsafe();
                if(func->returnType->get_direct_linked_node() == node) {
                    return false;
                }
                for(const auto param : func->params) {
                    if(param->type->get_direct_linked_node() == node) {
                        return false;
                    }
                }
                continue;
            }
            case ASTNodeKind::GenericFuncDecl:
                // if it contains generics, we consider the interface
                return false;
            default:
                continue;
        }
    }
    return true;
}

void TopLevelLinkSignature::VisitInterfaceDecl(InterfaceDefinition* node) {
    LinkMembersContainer(node, node->specifier());
    // user may have overridden it using an annotation in that case we must not check
    // by default every interface is by default object safe, that's the only time we check
    if(!node->is_non_dynamic() && !is_object_safe(node)) {
        node->set_object_safe(false);
    }
}

void TopLevelLinkSignature::VisitNamespaceDecl(Namespace* node) {
    table.scope_start();
    const auto root = node->root;
    if(root) {
        root->declare_extended_in_table(table);
    } else {
        node->declare_extended_in_table(table);
    }
    for(const auto child : node->nodes) {
        visit(child);
    }
    table.scope_end();
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
    auto& allocator = node->specifier() == AccessSpecifier::Public ? getAstAllocator() : getModAllocator();
    LinkMembersContainer(node, node->specifier());
    node->generate_functions(allocator, linker, node);
}

void TopLevelLinkSignature::VisitUnionDecl(UnionDef* node) {
    LinkMembersContainer(node, node->specifier());
}

void TopLevelLinkSignature::VisitVariantDecl(VariantDefinition* node) {
    auto& allocator = node->specifier() == AccessSpecifier::Public ? getAstAllocator() : getModAllocator();
    auto& diagnoser = linker;
    LinkMembersContainer(node, node->specifier());
    node->generate_functions(allocator, diagnoser, node);
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

void buildContainerIndexes(MembersContainer* container) {
    bool is_inherited_sizeof_zero = true;
    for(auto& inh : container->inherited) {
        const auto sub_container = inh.type->get_members_container();
        if(sub_container) {
            if (!sub_container->built_indexes) {
                // first build indexes on inherited containers
                // since we want to do multi-level inheritance
                buildContainerIndexes(sub_container);
            }
            // check if inherited size of zero
            // we also calculate this flag when building indexes
            if (!sub_container->is_sizeof_zero) {
                is_inherited_sizeof_zero = false;
            }
            // putting all index of this container
            // except we will never override, respecting already present indexes
            // because we want to support function hiding, function in container will hide function in inherited container (with same name)
            auto& container_indexes = container->indexes;
            auto& sub_container_indexes = sub_container->indexes;
            container_indexes.reserve(container_indexes.size() + sub_container_indexes.size());
            for(auto& index : sub_container_indexes) {
                container_indexes.try_emplace(index.first, index.second);
            }
        }
    }
    // set indexes to built
    // so we can skip building it again when other containers inherit this container
    container->built_indexes = true;
    // set is_sizeof_zero to true
    if (is_inherited_sizeof_zero && container->variables().empty()) {
        container->is_sizeof_zero = true;
    }
}

void buildInterfaceIndexes(InterfaceDefinition* container) {
    for(auto& inh : container->inherited) {
        const auto inherited_node = inh.type->get_direct_linked_node();
        // after link signature, the node can only be interface (because monomorphized if it was generic)
        if (inherited_node == nullptr || inherited_node->kind() != ASTNodeKind::InterfaceDecl) continue;
        const auto sub_container = inherited_node->as_interface_def_unsafe();
        if (!sub_container->built_indexes) {
            // first build indexes on inherited containers
            // since we want to do multi-level inheritance
            buildInterfaceIndexes(sub_container);
        }
        // basically or the bits of the inherited interface with the bits of the inheriter
        // for_example: if an interface inherits Copy, copy bit is turned on
        container->interface_bits |= sub_container->interface_bits;
        // putting all indexes of this container
        // except we will never override, respecting already present indexes
        // because we want to support function hiding, function in container will hide function in inherited container (with same name)
        auto& container_indexes = container->indexes;
        auto& sub_container_indexes = sub_container->indexes;
        container_indexes.reserve(container_indexes.size() + sub_container_indexes.size());
        for(auto& index : sub_container_indexes) {
            container_indexes.try_emplace(index.first, index.second);
        }
    }
    // set indexes to built
    // so we can skip building it again when other containers inherit this container
    container->built_indexes = true;
}

void index_implementation(SymbolResolver& linker, ImplDefinition* node) {
    // this code should be moved to type checking pass
    const auto linked_node = node->interface_type->get_direct_linked_node();
    if (linked_node->kind() == ASTNodeKind::InterfaceDecl) {
        const auto linked = linked_node->as_interface_def_unsafe();
        if (linked->is_static() && linked->has_implementation()) {
            linker.error("static interface must have only a single implementation", node->encoded_location());
        }
        linked->register_impl(node);
    } else if (linked_node->kind() == ASTNodeKind::GenericInterfaceDecl) {
#ifdef DEBUG
        if (!linker.generic_context) {
            linker.error("compiler bug: cannot implement a generic interface outside generic context, type not specialized", node->interface_type.encoded_location());
            return;
        }
#endif
    } else {
        linker.error("expected type to be an interface", node->encoded_location());
        return;
    }
    // we must NOT add indexes if this implementation is inside a generic container
    // because this can reference generic parameters (in interface/struct type, function return types)
    // we must let the instantiation occur which will handle this
    // TODO: replace is_generic with generic_context from linker
    bool is_generic = false;
    switch (node->parent()->kind()) {
        case ASTNodeKind::GenericStructDecl:
        case ASTNodeKind::GenericUnionDecl:
        case ASTNodeKind::GenericVariantDecl:
            is_generic = true;
            // intentional fall through
        default:
            break;
    }
    if(!is_generic && node->struct_type) {
        switch(node->struct_type->kind()) {
            case BaseTypeKind::IntN:
            case BaseTypeKind::String:
            case BaseTypeKind::ExpressiveString:
            case BaseTypeKind::Double:
            case BaseTypeKind::Float:
            case BaseTypeKind::Float128:
            case BaseTypeKind::LongDouble:
            case BaseTypeKind::Any:
            case BaseTypeKind::Void:
            case BaseTypeKind::NullPtr:
            case BaseTypeKind::Bool:
                // we create shallow clones of default implemented functions
                // create_default_implementations(linker, node, linked);
                // index all functions (on primitive type)
                for(const auto func : node->instantiated_functions()) {
                    linker.child_resolver.index_primitive_child(node->struct_type, func->name_view(), func);
                }
                // store it in index, so it can be retrieved
                linker.implsIndex.add_interface(linked_node, node->struct_type, node);
                break;
            case BaseTypeKind::Pointer:
                // we create shallow clones of default implemented functions
                // create_default_implementations(linker, node, linked);
                // index all functions (on pointer type)
                for(const auto func : node->instantiated_functions()) {
                    if(!linker.child_resolver.index_ptr_child(node->struct_type->as_pointer_type_unsafe(), func->name_view(), func)) {
                        linker.error("implementation for type is not allowed", node->struct_type.encoded_location());
                        break;
                    }
                }
                // store it in index, so it can be retrieved
                // TODO: currently we cannot index pointer types
                // because pointer types change (reallocated on every occurrence)
                break;
            case BaseTypeKind::Reference:
                // we create shallow clones of default implemented functions
                // create_default_implementations(linker, node, linked);
                // index all functions (on reference type)
                for(const auto func : node->instantiated_functions()) {
                    if(!linker.child_resolver.index_ref_child(node->struct_type->as_reference_type_unsafe(), func->name_view(), func)) {
                        linker.error("implementation for type is not allowed", node->struct_type.encoded_location());
                        break;
                    }
                }
                // store it in index, so it can be retrieved
                // TODO: currently we cannot index reference types
                // because reference types change (reallocated on every occurrence)
                break;
            case BaseTypeKind::Linked: {
                const auto member_node = node->struct_type->as_linked_type_unsafe()->linked;
                switch(member_node->kind()) {
                    case ASTNodeKind::StructDecl:
                    case ASTNodeKind::VariantDecl:
                    case ASTNodeKind::UnionDecl: {
                        const auto container = member_node->as_members_container_unsafe();
                        container->adopt(node);
                        // store it in index, so it can be retrieved
                        // we must store actual container, so it can be used to lookup
                        linker.implsIndex.add_interface(linked_node, container, node);
                        break;
                    }
                    default:
                        linker.error("cannot implement unsupported declaration", node->struct_type.encoded_location());
                        break;
                }
                break;
            }
            case BaseTypeKind::Generic: {
                const auto member_node = node->struct_type->as_generic_type_unsafe()->referenced->linked;
                switch (member_node->kind()) {
                    case ASTNodeKind::StructDecl:
                    case ASTNodeKind::VariantDecl:
                    case ASTNodeKind::UnionDecl: {
                        const auto container = member_node->as_members_container_unsafe();
                        container->adopt(node);
                        // store it in index, so it can be retrieved
                        // we must store actual container, so it can be used to lookup
                        linker.implsIndex.add_interface(linked_node, container, node);
                        break;
                    }
                    case ASTNodeKind::GenericStructDecl: {
                        const auto container = member_node->as_gen_struct_def_unsafe();
                        container->master_impl->adopt(node);
                        // store it in index, so it can be retrieved
                        // we must store actual container, so it can be used to lookup
                        linker.implsIndex.add_interface(linked_node, container, node);
                        break;
                    }
                    case ASTNodeKind::GenericVariantDecl:{
                        const auto container = member_node->as_gen_variant_decl_unsafe();
                        container->master_impl->adopt(node);
                        // store it in index, so it can be retrieved
                        // we must store actual container, so it can be used to lookup
                        linker.implsIndex.add_interface(linked_node, container, node);
                        break;
                    }
                    case ASTNodeKind::GenericUnionDecl:{
                        const auto container = member_node->as_gen_union_decl_unsafe();
                        container->master_impl->adopt(node);
                        // store it in index, so it can be retrieved
                        // we must store actual container, so it can be used to lookup
                        linker.implsIndex.add_interface(linked_node, container, node);
                        break;
                    }
                    default:
                        linker.error("cannot implement unsupported declaration", node->struct_type.encoded_location());
                        break;
                }
                break;
            }
            default:
                linker.error("cannot implement unsupported type", node->struct_type.encoded_location());
                break;
        }
    }
}

void build_indexes_of_impl(SymbolResolver& linker, ImplDefinition* decl) {
    const auto interfaceNode = decl->interface_type->get_direct_linked_canonical_node();
    if (interfaceNode && interfaceNode->kind() == ASTNodeKind::InterfaceDecl) {
        const auto interface = interfaceNode->as_interface_def_unsafe();
        const auto container = decl->struct_type->get_members_container();
        if (container) {
            // for impl decl, impl Interface for Struct <-- Interface extension methods
            // must be added into the struct so they can be invoked
            // if method already exists, we must not do that
            for (const auto extension : interface->extension_functions) {
                switch (extension->kind()) {
                    case ASTNodeKind::FunctionDecl:
                        container->indexes.try_emplace(extension->as_function_unsafe()->name_view(), extension);
                        continue;
                    case ASTNodeKind::GenericFuncDecl:
                        container->indexes.try_emplace(extension->as_gen_func_decl_unsafe()->master_impl->name_view(), extension);
                        continue;
                    default:
                        continue;
                }
            }
        }
        // putting indexes of interface into impl
        auto& container_indexes = decl->indexes;
        auto& sub_container_indexes = interface->indexes;
        container_indexes.reserve(container_indexes.size() + sub_container_indexes.size());
        for(auto& index : sub_container_indexes) {
            container_indexes.try_emplace(index.first, index.second);
        }
    }
}

void BuildIndexes(SymbolResolver& linker, std::vector<ASTNode*>& nodes) {
    for(const auto node : nodes) {
        switch(node->kind()) {
            case ASTNodeKind::StructDecl:
            case ASTNodeKind::UnionDecl:
            case ASTNodeKind::VariantDecl:
                buildContainerIndexes(node->as_members_container_unsafe());
                continue;
            case ASTNodeKind::InterfaceDecl:
                buildInterfaceIndexes(node->as_interface_def_unsafe());
                continue;
            case ASTNodeKind::ImplDecl:
                index_implementation(linker, node->as_impl_def_unsafe());
                build_indexes_of_impl(linker, node->as_impl_def_unsafe());
                continue;
            // for generic containers, we only need to build indexes of the master container
            // because that's where children are resolved from
            case ASTNodeKind::GenericStructDecl:
                buildContainerIndexes(node->as_gen_struct_def_unsafe()->master_impl);
                continue;
            case ASTNodeKind::GenericUnionDecl:
                buildContainerIndexes(node->as_gen_union_decl_unsafe()->master_impl);
                continue;
            case ASTNodeKind::GenericVariantDecl:
                buildContainerIndexes(node->as_gen_variant_decl_unsafe()->master_impl);
                continue;
            case ASTNodeKind::GenericInterfaceDecl:
                buildInterfaceIndexes(node->as_gen_interface_decl_unsafe()->master_impl);
                continue;
            case ASTNodeKind::GenericImplDecl:
                build_indexes_of_impl(linker, node->as_gen_impl_decl_unsafe()->master_impl);
                continue;
            case ASTNodeKind::NamespaceDecl:
                BuildIndexes(linker, node->as_namespace_unsafe()->nodes);
                continue;
            case ASTNodeKind::IfStmt:{
                const auto stmt = node->as_if_stmt_unsafe();
                if(stmt->computed_scope.has_value()) {
                    const auto scope = stmt->computed_scope.value();
                    if(scope) {
                        BuildIndexes(linker, scope->nodes);
                    }
                }
                continue;
            }
            default:
                continue;
        }
    }
}

static bool isValidExportParent(ASTNode* parent) {
    switch(parent->kind()) {
    case ASTNodeKind::FileScope:
        return true;
    case ASTNodeKind::IfStmt:
        return isValidExportParent(parent->as_if_stmt_unsafe()->parent());
    default:
        return false;
    }
}

void LinkExportStatement(SymbolResolver& linker, ExportStmt* node) {
    if (node->parent() && !isValidExportParent(node->parent())) {
        linker.error("Export statement can only be used as a top level statement", node);
        return;
    }

    // resolution of chain
    auto resolvedNode = linker.find(node->ids[0]);
    if(resolvedNode == nullptr) {
        linker.error(node->encoded_location()) << "unresolved symbol '" << node->ids[0] << "' in export statement";
        return;
    }
    auto start = node->ids.data() + 1;
    const auto end = node->ids.data() + node->ids.size();
    while(start != end) {
        resolvedNode = resolvedNode->child(*start);
        if(resolvedNode == nullptr) {
            linker.error(node->encoded_location()) << "unresolved symbol '" << *start << "' in parent";
            return;
        }
        start++;
    }

    if (resolvedNode->get_mod_scope() == linker.current_mod_scope) {
        linker.error("cannot export a symbol from the current module", node);
        return;
    }

    switch (resolvedNode->kind()) {
    case ASTNodeKind::NamespaceDecl:
    case ASTNodeKind::StructDecl:
    case ASTNodeKind::VariantDecl:
    case ASTNodeKind::UnionDecl:
    case ASTNodeKind::FunctionDecl:
    case ASTNodeKind::GenericFuncDecl:
    case ASTNodeKind::GenericStructDecl:
    case ASTNodeKind::GenericUnionDecl:
    case ASTNodeKind::GenericInterfaceDecl:
    case ASTNodeKind::GenericVariantDecl:
    case ASTNodeKind::TypealiasStmt:
        break;
    default:
        linker.error("unsupported declaration being used with export statement", node);
        return;
    }

    node->linked_node = resolvedNode;
}

void calculate_gen_param_bits(const std::vector<GenericTypeParameter*>& params) {
    for (const auto param : params) {
        for (auto& trait : param->traits) {
            const auto node = trait->get_direct_linked_node();
            if (node && node->kind() == ASTNodeKind::InterfaceDecl) {
                param->current_bits |= node->as_interface_def_unsafe()->interface_bits;
            }
        }
    }
}

/**
 * Calculates interface bits from where clause constraints.
 * These bits are applied temporarily during function body visiting.
 */
static void calculate_where_clause_bits(const WhereClause* where_clause) {
    if (!where_clause) return;
    for (auto& c : where_clause->constraints) {
        if (!c.param) continue;
        for (auto& trait_type : c.constraints) {
            const auto node = trait_type->get_direct_linked_node();
            if (node && node->kind() == ASTNodeKind::InterfaceDecl) {
                c.param->current_bits |= node->as_interface_def_unsafe()->interface_bits;
            }
        }
    }
}

// does two things (currently)
// 1 - links export statements
// 2 - calculates interface bits of generic type parameters
void AfterBuildIndexesPass(SymbolResolver& resolver, std::vector<ASTNode*>& nodes) {
    for (const auto node : nodes) {
        switch(node->kind()) {
            case ASTNodeKind::ExportStmt:
                LinkExportStatement(resolver, node->as_export_stmt_unsafe());
                continue;
            case ASTNodeKind::NamespaceDecl:
                AfterBuildIndexesPass(resolver, node->as_namespace_unsafe()->nodes);
                continue;
            case ASTNodeKind::GenericFuncDecl: {
                calculate_gen_param_bits(node->as_gen_func_decl_unsafe()->generic_params);
                // also calculate bits from where clause on the master function
                calculate_where_clause_bits(node->as_gen_func_decl_unsafe()->master_impl->where_clause);
                continue;
            }
            case ASTNodeKind::GenericStructDecl: {
                calculate_gen_param_bits(node->as_gen_struct_def_unsafe()->generic_params);
                // also apply where clause bits from member functions
                for(const auto func : node->as_gen_struct_def_unsafe()->master_impl->evaluated_nodes()) {
                    if (func->kind() == ASTNodeKind::FunctionDecl) {
                        calculate_where_clause_bits(func->as_function_unsafe()->where_clause);
                    }
                }
                continue;
            }
            case ASTNodeKind::GenericUnionDecl:
                calculate_gen_param_bits(node->as_gen_union_decl_unsafe()->generic_params);
                continue;
            case ASTNodeKind::GenericVariantDecl: {
                calculate_gen_param_bits(node->as_gen_variant_decl_unsafe()->generic_params);
                for(const auto func : node->as_gen_variant_decl_unsafe()->master_impl->evaluated_nodes()) {
                    if (func->kind() == ASTNodeKind::FunctionDecl) {
                        calculate_where_clause_bits(func->as_function_unsafe()->where_clause);
                    }
                }
                continue;
            }
            case ASTNodeKind::GenericInterfaceDecl: {
                calculate_gen_param_bits(node->as_gen_interface_decl_unsafe()->generic_params);
                for(const auto func : node->as_gen_interface_decl_unsafe()->master_impl->evaluated_nodes()) {
                    if (func->kind() == ASTNodeKind::FunctionDecl) {
                        calculate_where_clause_bits(func->as_function_unsafe()->where_clause);
                    }
                }
                continue;
            }
            case ASTNodeKind::GenericImplDecl: {
                calculate_gen_param_bits(node->as_gen_impl_decl_unsafe()->generic_params);
                for(const auto func : node->as_gen_impl_decl_unsafe()->master_impl->evaluated_nodes()) {
                    if (func->kind() == ASTNodeKind::FunctionDecl) {
                        calculate_where_clause_bits(func->as_function_unsafe()->where_clause);
                    }
                }
                continue;
            }
            case ASTNodeKind::GenericTypeDecl:
                calculate_gen_param_bits(node->as_gen_type_decl_unsafe()->generic_params);
                continue;
            case ASTNodeKind::IfStmt:{
                const auto stmt = node->as_if_stmt_unsafe();
                if(stmt->computed_scope.has_value()) {
                    const auto scope = stmt->computed_scope.value();
                    if(scope) {
                        AfterBuildIndexesPass(resolver, scope->nodes);
                    }
                }
                continue;
            }
            default:
                continue;
        }
    }
}

void sym_res_after_signature(SymbolResolver& resolver, Scope* scope) {
    auto& nodes = scope->nodes;
    // builds indexes of containers to children inside them can be looked up
    BuildIndexes(resolver, nodes);
    // links export statements, these can't be linked during link signature
    // calculate interface bits of generic type parameters
    AfterBuildIndexesPass(resolver, nodes);
}
