// Copyright (c) Chemical Language Foundation 2025.

#include "Value.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/StructValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/UnsafeValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NullValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/Expression.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/structures/CapturedComptimeVariable.h"
#include "ast/statements/Return.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/IndexOperator.h"
#include "ast/types/ArrayType.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/values/DereferenceValue.h"
#include "ast/structures/VariantMemberParam.h"
#include "ast/values/VariantCase.h"
#include "ast/values/CastedValue.h"
#include "ast/values/GenericInstIdentifier.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/If.h"
#include <ranges>
#include "preprocess/RepresentationVisitor.h"
#include <sstream>
#include <iostream>
#include "ast/values/ValueNode.h"
#include "std/except.h"


bool isTypeRValue(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Bool:
        case BaseTypeKind::IntN:
        case BaseTypeKind::Float:
        case BaseTypeKind::Double:
        case BaseTypeKind::LongDouble:
        case BaseTypeKind::Float128:
            return true;
        case BaseTypeKind::Linked: {
            const auto linked = type->as_linked_type_unsafe()->linked;
            switch (linked->kind()) {
                case ASTNodeKind::TypealiasStmt:
                    return isTypeRValue(linked->as_typealias_unsafe()->actual_type);
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

bool Value::isValueIntegerLiteral() {
    switch(kind()) {
        case ValueKind::Bool:
        case ValueKind::IntN:
            return true;
        default:
            return false;
    }
}

bool Value::isValueLiteral() {
    switch(kind()) {
        case ValueKind::Bool:
        case ValueKind::IntN:
        case ValueKind::Double:
        case ValueKind::Float:
            return true;
        default:
            return false;
    }
}

bool Value::isValueRValueInBackend() {
    switch(kind()) {
        case ValueKind::Bool:
        case ValueKind::IntN:
        case ValueKind::Double:
        case ValueKind::Float:
        case ValueKind::NegativeValue:
        case ValueKind::BitwiseNot:
        case ValueKind::SizeOfValue:
        case ValueKind::AlignOfValue:
        case ValueKind::OffsetOfValue:
        case ValueKind::Expression:
        case ValueKind::NullValue:
        case ValueKind::InValue:
        case ValueKind::IsValue:
        case ValueKind::AwaitExpr:
            return true;
        case ValueKind::FunctionCall:
            return isTypeRValue(getType());
        case ValueKind::AccessChain: {
            const auto last = as_access_chain_unsafe()->values.back();
            if (last->kind() == ValueKind::FunctionCall) {
                return isTypeRValue(last->getType());
            } else {
                return false;
            }
        }
        case ValueKind::Identifier:{
            const auto linked = linked_node();
            switch(linked->kind()) {
                case ASTNodeKind::FunctionParam:
                    return isTypeRValue(linked->as_func_param_unsafe()->type);
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

bool isTypeRValueFrontend(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Bool:
        case BaseTypeKind::IntN:
        case BaseTypeKind::Float:
        case BaseTypeKind::Double:
        case BaseTypeKind::LongDouble:
        case BaseTypeKind::Float128:
            return true;
        case BaseTypeKind::Linked: {
            const auto linked = type->as_linked_type_unsafe()->linked;
            switch (linked->kind()) {
                case ASTNodeKind::TypealiasStmt:
                    return isTypeRValueFrontend(linked->as_typealias_unsafe()->actual_type);
                case ASTNodeKind::StructDecl:
                case ASTNodeKind::VariantDecl:
                case ASTNodeKind::VariantMember:
                case ASTNodeKind::UnionDecl:
                    return true;
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

bool Value::isValueRValueInFrontend() {
    switch(kind()) {
        case ValueKind::Bool:
        case ValueKind::IntN:
        case ValueKind::String:
        case ValueKind::Double:
        case ValueKind::Float:
        case ValueKind::NegativeValue:
        case ValueKind::NotValue:
        case ValueKind::BitwiseNot:
        case ValueKind::SizeOfValue:
        case ValueKind::AlignOfValue:
        case ValueKind::OffsetOfValue:
        case ValueKind::StructValue:
        case ValueKind::Expression:
        case ValueKind::AddrOfValue:
        case ValueKind::ReferenceOfValue:
        case ValueKind::NullValue:
        case ValueKind::InValue:
        case ValueKind::IsValue:
        case ValueKind::ArrayValue:
        case ValueKind::AwaitExpr:
            return true;
        case ValueKind::FunctionCall:
            return isTypeRValueFrontend(getType());
        case ValueKind::AccessChain: {
            const auto last = as_access_chain_unsafe()->values.back();
            if (last->kind() == ValueKind::FunctionCall) {
                return isTypeRValueFrontend(last->getType());
            } else {
                return false;
            }
        }
        default:
            return false;
    }
}

uint64_t Value::byte_size(const TargetData& target) {
#ifdef DEBUG
    CHEM_THROW_RUNTIME("byte_size called on base Value");
#else
    std::cerr << "Value::byte_size called on value " << representation() << std::endl;
#endif
    return 0;
}

BaseType* Value::get_stored_value_type(ASTAllocator& allocator) {
    auto linked = linked_node();
    return linked ? linked->get_stored_value_type(allocator, linked->kind()) : nullptr;
}

// TODO: convert this into a direct function on the value
// this should not use linked_ndoe
ASTNode* direct_linked_node(Value* value) {
    switch (value->kind()) {
    case ValueKind::DereferenceValue:
        return nullptr;
    default:
        return value->linked_node();
    }
}


// stored pointer into a variable, that must be loaded, before using
bool Value::is_stored_ptr_or_ref(ASTAllocator& allocator) {
    switch(kind()) {
        case ValueKind::CastedValue:
            return as_casted_value_unsafe()->value->is_stored_ptr_or_ref(allocator);
        default:
            break;
    }
    auto linked = direct_linked_node(this);
    return linked != nullptr && linked->is_stored_ptr_or_ref(allocator);
}

bool Value::is_stored_ref(ASTAllocator& allocator) {
    auto linked = linked_node();
    return linked != nullptr && linked->is_stored_ref(allocator);
}

bool Value::is_ptr_or_ref(ASTAllocator& allocator) {
    auto linked = direct_linked_node(this);
    return linked != nullptr && linked->is_ptr_or_ref(allocator, linked->kind());
}

bool Value::is_ref(ASTAllocator& allocator) {
    auto linked = linked_node();
    return linked != nullptr && linked->is_ref(allocator);
}

bool Value::is_ref_value() {
    switch(val_kind()) {
        case ValueKind::AccessChain:
            return true;
        case ValueKind::Identifier:
            return true;
        default:
            return false;
    }
}

bool is_node_assignable(ASTNode* node) {
    const auto linked_kind = node->kind();
    switch(linked_kind) {
        case ASTNodeKind::VarInitStmt:{
            return !node->as_var_init_unsafe()->is_const();
        }
        case ASTNodeKind::FunctionParam: {
            return true;
        }
        case ASTNodeKind::VariantCaseVariable: {
            const auto member_param = node->as_variant_case_var_unsafe()->member_param;
            return !member_param->is_const;
        }
        case ASTNodeKind::StructMember:
        case ASTNodeKind::UnnamedUnion:
        case ASTNodeKind::UnnamedStruct: {
            return !node->as_base_def_member_unsafe()->get_is_const();
        }
        case ASTNodeKind::CapturedVariable: {
            const auto cap = node->as_captured_var_unsafe();
            if(cap->capture_by_ref) {
                return cap->refType.is_mutable;
            } else {
                return true;
            }
        }
        case ASTNodeKind::PatternMatchId: {
            return true;
        }
        default:
            return false;
    }
}

bool is_node_mutable(ASTNode* node, FunctionType* func_type, SymbolResolver& resolver) {
    const auto linked_kind = node->kind();
    switch(linked_kind) {
        case ASTNodeKind::VarInitStmt:{
            const auto type = node->as_var_init_unsafe()->known_type();
            return type->is_mutable();
        }
        case ASTNodeKind::FunctionParam: {
            const auto type = node->as_func_param_unsafe()->type;
            return type->is_mutable();
        }
        case ASTNodeKind::VariantCaseVariable: {
            const auto member_param = node->as_variant_case_var_unsafe()->member_param;
            const auto type = member_param->type;
            return type->is_mutable();
        }
        case ASTNodeKind::StructMember:
        case ASTNodeKind::UnnamedUnion:
        case ASTNodeKind::UnnamedStruct: {
            const auto self_param = func_type->get_self_param();
            if (self_param) {
                return self_param->type->is_mutable();
            } else {
                const auto func = func_type->as_function();
                // constructor takes a mutable reference by default
                return func->is_constructor_fn();
            }
        }
        default:
            return false;
    }
}

bool Value::is_ref_l_value() {
    const auto kind = val_kind();
    switch(kind) {
        case ValueKind::StructValue:
            return true;
        case ValueKind::AwaitExpr:
            // `await e` yields an rvalue result, it is never an l-value
            return false;
        case ValueKind::AccessChain:
            if(as_access_chain_unsafe()->values.back()->kind() == ValueKind::IndexOperator) {
                return true;
            }
            // intentional fallthrough
        default: {
            const auto linked = linked_node();
            return linked && is_node_assignable(linked);
        }
    }
}

bool Value::check_is_mutable(bool assigning) {
    switch(val_kind()) {
        case ValueKind::Identifier: {
            const auto id = as_identifier_unsafe();
            if(assigning) {
                return is_node_assignable(id->linked);
            } else {
                const auto type = id->linked->known_type();
                return type->is_mutable();
            }
        }
        case ValueKind::FunctionCall:{
            const auto call = as_func_call_unsafe();
            const auto type = call->getType();
            return type->is_mutable();
        }
        case ValueKind::IndexOperator: {
            const auto index = as_index_op_unsafe();
            return index->parent_val->check_is_mutable(false);
        }
        case ValueKind::AccessChain: {
            const auto chain = as_access_chain_unsafe();
            auto& chain_values = chain->values;
            const auto chain_size = chain_values.size();
            const auto last_ind = chain_size - 1;
            const auto last = chain_values[last_ind];
            const auto last_kind = last->val_kind();
            if(last_kind == ValueKind::FunctionCall || last_kind == ValueKind::IndexOperator) {
                return last->check_is_mutable(assigning);
            }
            unsigned i = 0;
            while(i < chain_size) {
                const auto value = chain_values[i];
                if(i == last_ind) {
                    const auto is_last_id = last_kind == ValueKind::Identifier;
                    if(!value->check_is_mutable(assigning && is_last_id)) {
                        return false;
                    }
                } else {
                    // every value the chain is written *through* must itself be mutable,
                    // so a write like `s.field = value` through a `const` struct value is
                    // rejected. Writing through a pointer does not require the pointer
                    // binding to be mutable (like `T* const`), and writing through a
                    // reference requires the reference itself to be mutable.
                    const auto ty = value->getType();
                    if(ty != nullptr && ty->kind() == BaseTypeKind::Pointer) {
                        // allowed
                    } else if(ty != nullptr && ty->kind() == BaseTypeKind::Reference) {
                        if(!ty->as_reference_type_unsafe()->is_mutable) {
                            return false;
                        }
                    } else if(!value->check_is_mutable(assigning)) {
                        return false;
                    }
                }
                i++;
            }
            return true;
        }
        case ValueKind::DereferenceValue: {
            return as_dereference_value_unsafe()->getValue()->check_is_mutable(false);
        }
        case ValueKind::Expression: {
            const auto expr = as_expression_unsafe();
            return expr->getType()->is_mutable();
        }
        case ValueKind::CastedValue: {
            const auto val = as_casted_value_unsafe();
            return val->getType()->is_mutable();
        }
        default:
            return false;
    }
}

bool Value::is_chain_func_call() {
    auto chain = as_access_chain();
    if(chain && chain->values.back()->as_func_call() != nullptr) {
        return true;
    }
    return false;
}

bool Value::is_func_call() {
    switch(kind()) {
        case ValueKind::FunctionCall:
            return true;
        case ValueKind::AccessChain:
            return as_access_chain_unsafe()->values.back()->kind() == ValueKind::FunctionCall;
        default:
            return false;
    }
}

bool Value::is_ref_moved() {
    switch(val_kind()) {
        case ValueKind::UnsafeValue:
            return static_cast<UnsafeValue*>(this)->getValue()->is_ref_moved();
        case ValueKind::AccessChain:
            return as_access_chain_unsafe()->is_moved();
        case ValueKind::Identifier:
            return as_identifier_unsafe()->is_moved;
        default:
                return false;
    }
}

VariableIdentifier* Value::get_chain_id() {
    switch(kind()) {
        case ValueKind::AccessChain:
            // the leaf of a chain is not necessarily an identifier (it can be a
            // call or an index operation), so this one stays nullable
            return as_access_chain_unsafe()->values.size() == 1 ? as_access_chain_unsafe()->values.back()->as_identifier() : nullptr;
        case ValueKind::Identifier:
        case ValueKind::GenericInstIdentifier:
            // the kind is already established here
            return as_identifier_unsafe();
        default:
            return nullptr;
    }
}

bool Value::reference() {
    const auto kind = val_kind();
    switch(kind) {
        case ValueKind::AccessChain:
        case ValueKind::Identifier:
        case ValueKind::GenericInstIdentifier:
        case ValueKind::FunctionCall:
            return true;
        default:
            return false;
    }
}

bool Value::requires_memcpy_ref_struct(BaseType* known_type) {
    // is referencing another struct, that is non movable and must be mem copied into the pointer
    const auto kind = val_kind();
    if(kind == ValueKind::Identifier || (kind == ValueKind::AccessChain && as_access_chain_unsafe()->values.back()->as_func_call() == nullptr)) {
        auto linked = known_type->get_direct_linked_canonical_node();
        if (linked) {
            switch(linked->kind()) {
                case ASTNodeKind::UnnamedStruct:
                case ASTNodeKind::UnnamedUnion:
                case ASTNodeKind::StructDecl:
                case ASTNodeKind::VariantDecl:
                case ASTNodeKind::UnionDecl:
                    return true;
                default:
                    return false;
            }
        }
    }
    return false;
}

Value* Value::child(InterpretScope& scope, const chem::string_view& name) {
#ifdef DEBUG
    std::cerr << "Value::child called on base value " + representation();
#endif
    return nullptr;
}

Value* Value::call_member(InterpretScope& scope, const chem::string_view& name, std::vector<Value*>& values) {
#ifdef DEBUG
    std::cerr << "Value::call_member called on base value " + representation() + " with name " + name.str();
#endif
    return nullptr;
}

Value* Value::index(InterpretScope& scope, int i) {
#ifdef DEBUG
    std::cerr << "Value::index called on base value " + representation();
#endif
    return nullptr;
}

Value* Value::find_in(InterpretScope& scope, Value* parent) {
#ifdef DEBUG
    std::cerr << "Value::find_in called on base value " + representation();
#endif
    return nullptr;
}

ASTNode* Value::get_chain_last_linked() {
    switch(kind()) {
        case ValueKind::Identifier: {
            return as_identifier_unsafe()->linked;
        }
        case ValueKind::AccessChain: {
            auto& values = as_access_chain_unsafe()->values;
            return values.back()->get_chain_last_linked();
        }
        default:
            return nullptr;
    }
}

void Value::report_assignment_of_chain_id() {
    const auto l = get_chain_last_linked();
    if(l) {
        switch(l->kind()) {
            case ASTNodeKind::FunctionParam:
                l->as_func_param_unsafe()->set_has_assignment();
                return;
            case ASTNodeKind::VarInitStmt:
                l->as_var_init_unsafe()->set_has_assignment();
                return;
            default:
                return;
        }
    }
}

void Value::report_addr_taken_of_chain_id() {
    const auto l = get_chain_last_linked();
    if(l) {
        switch(l->kind()) {
            case ASTNodeKind::FunctionParam:
                l->as_func_param_unsafe()->set_has_address_taken(true);
                return;
            case ASTNodeKind::VarInitStmt:
                l->as_var_init_unsafe()->set_has_assignment();
                return;
            default:
                return;
        }
    }
}

bool Value::is_pointer() {
    const auto k = getType()->kind();
    return k == BaseTypeKind::Pointer || k == BaseTypeKind::String;
}

bool Value::is_pointer_or_ref() {
    const auto k = getType()->kind();
    return k == BaseTypeKind::Pointer || k == BaseTypeKind::String || k == BaseTypeKind::Reference;
}

Value* Value::copy(ASTAllocator& allocator) {
#ifdef DEBUG
    std::cerr << "copy called on base Value, representation : " << representation();
#endif
    return nullptr;
}

bool Value::get_the_bool() {
    return ((BoolValue*) this)->value;
}

const chem::string_view& Value::get_the_string() {
    return ((StringValue*) this)->value;
}

std::optional<uint64_t> Value::get_number() {
    if(is_value_int_n()) {
        return as_int_num_value_unsafe()->get_num_value();
    } else if(kind() == ValueKind::NegativeValue) {
        auto value = as_negative_value_unsafe()->getValue()->get_number();
        if(value.has_value()) {
            return -value.value();
        } else {
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }
}

float Value::get_the_float() {
    return ((FloatValue*) this)->value;
}

double Value::get_the_double() {
    return ((DoubleValue*) this)->value;
}

Value* Value::get_first_value_from_value_node(ASTNode* node) {
    const auto k = node->kind();
    switch(k) {
        case ASTNodeKind::ValueWrapper:
            return node->as_value_wrapper_unsafe()->value;
        case ASTNodeKind::ValueNode:
            return node->as_value_node_unsafe()->value;
        case ASTNodeKind::IfStmt:
            return get_first_value_from_value_node(((IfStatement*) node)->ifBody.nodes.back());
        case ASTNodeKind::SwitchStmt: {
            const auto switch_node = ((SwitchStatement*) node);
            auto& scopes = switch_node->scopes;
            if(!scopes.empty()) {
                auto& nodes = scopes.front().nodes;
                if(nodes.empty()) {
                    return nullptr;
                } else {
                    return get_first_value_from_value_node(nodes.back());
                }
            } else {
                return nullptr;
            }
        }
        default:
            return nullptr;
    }
}

std::string Value::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    visitor.visit(this);
    return ostring.str();
}

Value* Value::scope_value(InterpretScope& scope) {
    if(kind() == ValueKind::LambdaFunc) {
        // prevents copying of lambda function
        return this;
    }
    const auto eval = evaluated_value(scope);
    if(eval == this) {
        return copy(scope.allocator);
    } else {
        return eval;
    }
}

BaseType* Value::pure_type_ptr() {
    return getType()->canonical();
}

bool is_largest_member(VariablesContainerBase* container, std::vector<Value*>& chain, unsigned index) {
    if (index < chain.size()) {
        return container->largest_member() == chain[index]->linked_node();
    } else {
        return true;
    }
}

bool Value::should_build_chain_type(std::vector<Value*>& chain, unsigned index) {
    while(index < chain.size()) {
        const auto linked = chain[index]->linked_node();
        if (linked) {
            switch (linked->kind()) {
                case ASTNodeKind::UnionDecl: {
                    const auto container = linked->as_union_def_unsafe();
                    if (index + 1 < chain.size()) {
                        if (container->largest_member() != chain[index + 1]->linked_node()) {
                            // non-largest member of the union being accessed
                            // we need to build the chain type
                            return true;
                        }
                        // largest member of the union being accessed
                        // next member can be a union as well, lets check that
                        index++;
                        continue;
                    } else {
                        // user didn't access anything inside the union
                        // so we shouldn't build chain type (return false)
                        // this is the last value in the chain, so we return
                        return false;
                    }
                }
                case ASTNodeKind::UnnamedUnion: {
                    const auto container = linked->as_unnamed_union_unsafe();
                    if (index + 1 < chain.size()) {
                        if (container->largest_member() != chain[index + 1]->linked_node()) {
                            // non-largest member of the union being accessed
                            // we need to build the chain type
                            return true;
                        }
                        // largest member of the union being accessed
                        // next member can be a union as well, lets check that
                        index++;
                        continue;
                    } else {
                        // user didn't access anything inside the union
                        // so we shouldn't build chain type (return false)
                        // this is the last value in the chain, so we return
                        return false;
                    }
                }
                default:
                    break;
            }
        }
        index++;
    }
    return false;
}

bool Value::is_equal(Value* other, ValueKind kind, ValueKind other_kind) {
    if(kind == other_kind) {
        switch(kind) {
            case ValueKind::AccessChain: {
                auto this_chain = as_access_chain_unsafe();
                auto other_chain = other->as_access_chain_unsafe();
                const auto siz = this_chain->values.size();
                if(siz != other_chain->values.size()) {
                    return false;
                }
                unsigned i = 0;
                while(i < siz) {
                    if(!this_chain->values[i]->is_equal(other_chain->values[i])) {
                        return false;
                    }
                    i++;
                }
                return true;
            }
            case ValueKind::Identifier:
            case ValueKind::GenericInstIdentifier: {
                // this kind is established by the switch, but the other value is
                // not guaranteed to be an identifier of the same kind
                const auto other_id = other->as_identifier();
                return other_id != nullptr && as_identifier_unsafe()->linked == other_id->linked;
            }
            case ValueKind::IndexOperator: {
                const auto this_index_op = as_index_op();
                const auto other_index_op = other->as_index_op();
                if(this_index_op->idx->is_value_int_n() && other_index_op->idx->is_value_int_n()) {
                    auto this_value = ((IntNumValue*) this_index_op->idx)->get_num_value();
                    auto other_value = ((IntNumValue*) other_index_op->idx)->get_num_value();
                    if(this_value != other_value) {
                        return false;
                    }
                } else {
                    return false;
                }
                return true;
            }
            case ValueKind::FunctionCall:{
                return false;
            }
            default:
                return false;
        }
    }
    return false;
}

VariableIdentifier* Value::get_single_id() {
    switch(kind()) {
        case ValueKind::AccessChain: {
            auto& values = as_access_chain_unsafe()->values;
            return values.size() == 1 ? values.back()->as_identifier() : nullptr;
        }
        case ValueKind::Identifier:
        case ValueKind::GenericInstIdentifier:
            // the kind is already established here
            return as_identifier_unsafe();
        default:
            return nullptr;
    }
}

VariableIdentifier* Value::get_last_id() {
    switch(kind()) {
        case ValueKind::AccessChain:
            return as_access_chain_unsafe()->values.back()->as_identifier();
        case ValueKind::Identifier:
        case ValueKind::GenericInstIdentifier:
            // the kind is already established here
            return as_identifier_unsafe();
        default:
            return nullptr;
    }
}

void Value::set_child_value(InterpretScope& scope, const chem::string_view& name, Value* value, Operation op) {
    switch(val_kind()) {
        case ValueKind::StructValue:
            as_struct_value_unsafe()->set_child_value(scope, name, value, op);
            return;
        default: {
            // Try to evaluate to get the underlying value
            auto evaluated = evaluated_value(scope);
            if(evaluated && evaluated != this) {
                evaluated->set_child_value(scope, name, value, op);
                return;
            }
            scope.error(this) << "can't set child value with name" << name;
            return;
        }
    }
}

void Value::set_value(InterpretScope& scope, Value* value, Operation op, SourceLocation location) {
    switch(val_kind()) {
        case ValueKind::AccessChain: {
            const auto chain = as_access_chain_unsafe();
            chain->set_value(scope, value, op, location);
            return;
        }
        case ValueKind::Identifier:
            as_identifier_unsafe()->set_value(scope, value, op, location);
            return;
        case ValueKind::DereferenceValue:
            as_dereference_value_unsafe()->set_value(scope, value, op, location);
            return;
        default:
            scope.error("couldn't set the value of", this);
    }
}

void Value::set_value_in(InterpretScope& scope, Value* parent, Value* value, Operation op, SourceLocation location) {
    switch(val_kind()) {
        case ValueKind::AccessChain: {
            const auto chain = as_access_chain_unsafe();
            if(chain->values.size() == 1) {
                chain->values.back()->set_value_in(scope, parent, value, op, location);
            } else {
                scope.error("couldn't set the value in parent of", this);
            }
            return;
        }
        case ValueKind::Identifier:
            as_identifier_unsafe()->set_value_in(scope, parent, value, op, location);
            // TODO
//        case ValueKind::FunctionCall:
            // TODO
//        case ValueKind::DereferenceValue:
        default:
            scope.error("couldn't set the value in parent of", this);
    }
}

//BaseType* implicit_constructor_type(ASTAllocator& allocator, BaseType* return_type, Value* value) {
//    auto k = return_type->kind();
//    if(k == BaseTypeKind::Linked || k == BaseTypeKind::Generic) {
//        const auto linked = return_type->linked_node();
//        const auto struc = linked->as_struct_def();
//        if(struc) {
//            const auto constr = struc->implicit_constructor_for(allocator, value);
//            if(constr) {
//                return constr->func_param_for_arg_at(0)->type;
//            }
//        }
//    }
//    return return_type;
//}

Value::~Value() = default;