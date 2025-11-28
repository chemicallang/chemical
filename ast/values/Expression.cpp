// Copyright (c) Chemical Language Foundation 2025.

#include "Expression.h"
#include "IntNumValue.h"
#include "ast/base/TypeBuilder.h"
#include "ast/types/IntNType.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/base/ASTNode.h"
#include "ast/types/BoolType.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/NullValue.h"
#include "compiler/ASTDiagnoser.h"
#include "ast/structures/MembersContainer.h"
#include "ast/values/FloatValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/StringValue.h"
#include "ast/types/ReferenceType.h"
#include "ast/base/InterpretScope.h"

inline EnumDeclaration* getEnumDecl(BaseType* type) {
    return type->get_direct_linked_enum();
}

void Expression::replace_number_values(ASTAllocator& allocator, TypeBuilder& typeBuilder, BaseType* firstType, BaseType* secondType) {
    if(firstType->kind() == BaseTypeKind::IntN && secondType->kind() == BaseTypeKind::IntN) {
        if(firstValue->val_kind() == ValueKind::IntN) {
            auto value = ((IntNumValue*)firstValue)->get_num_value();
            firstValue = ((IntNType*) secondType)->create(allocator, typeBuilder, value, firstValue->encoded_location());
        } else if(secondValue->val_kind() == ValueKind::IntN){
            auto value = ((IntNumValue*)secondValue)->get_num_value();
            secondValue = ((IntNType*) firstType)->create(allocator, typeBuilder, value, secondValue->encoded_location());
        }
    }
    const auto first = getEnumDecl(firstType);
    if(first && secondValue->kind() == ValueKind::IntN) {
        const auto second = secondValue->as_int_num_value_unsafe();
        if(second) {
            secondValue = first->get_underlying_integer_type()->create(allocator, typeBuilder, second->value, secondValue->encoded_location());
        }
    } else {
        const auto second = getEnumDecl(secondType);
        if(second && firstValue->kind() == ValueKind::IntN) {
            const auto firstVal = firstValue->as_int_num_value_unsafe();
            if(firstVal) {
                firstValue = second->get_underlying_integer_type()->create(allocator, typeBuilder, firstVal->value, firstValue->encoded_location());
            }
        }
    }
}

BaseType* unwrap_reference(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Reference:
            return type->as_reference_type_unsafe()->type;
        default:
            return type;
    }
}

bool isPrimitive(BaseType* type);

FunctionDeclaration* get_overloaded_func(Expression* expr) {
    const auto first_canonical = expr->firstValue->getType()->canonical();
    const auto node = first_canonical->get_linked_canonical_node(true, false);
    if(node == nullptr) return nullptr;
    const auto container = node->get_members_container();
    if(container == nullptr) return nullptr;
    const auto op_info = operator_impl_info(expr->operation);
    if (op_info.name.empty()) return nullptr;
    const auto child_node = container->child(op_info.name);
    if(child_node == nullptr) return nullptr;
    if(child_node->kind() == ASTNodeKind::FunctionDecl) {
        return child_node->as_function_unsafe();
    } else if(child_node->kind() == ASTNodeKind::MultiFunctionNode) {
        const auto multi_node = child_node->as_multi_func_node_unsafe();
        std::vector<Value*> args { expr->firstValue, expr->secondValue };
        return multi_node->func_for_call(args);
    } else {
        return nullptr;
    }
}

BaseType* determine_type(Expression* expr, TypeBuilder& typeBuilder, ASTDiagnoser& diagnoser) {
    auto firstType = expr->firstValue->getType();
    auto secondType = expr->secondValue->getType();
    if(expr->operation >= Operation::IndexBooleanReturningStart && expr->operation <= Operation::IndexBooleanReturningEnd) {
        // check first type is primitive
        if(!isPrimitive(firstType)) {
            // check if overloaded operator exists
            const auto overloaded = get_overloaded_func(expr);
            if(overloaded != nullptr) {
                if(overloaded->params.size() != 2) {
                    // since this expression has two values, we always expect two parameters
                    diagnoser.error(expr) << "expected operator implementation function to have exactly two parameters";
                    return (BaseType*) typeBuilder.getVoidType();
                }
                // check the second type here that it matches the overloaded parameter
                if(!overloaded->params[1]->type->satisfies(expr->secondValue, false)) {
                    diagnoser.error(expr->secondValue) << "value doesn't satisfy the overloaded operator parameter";
                    return (BaseType*) typeBuilder.getVoidType();
                }
                // return early second type has been checked
                return typeBuilder.getBoolType();
            } else {
                diagnoser.error("expected the value to have primitive type or have operator overloaded", expr->firstValue);
            }
        }
        if(!isPrimitive(secondType)) {
            diagnoser.error("expected the value to have primitive type", expr->secondValue);
        }
        return typeBuilder.getBoolType();
    }
    // check if its overloading operator
    const auto first_canonical = firstType->canonical();
    const auto node = first_canonical->get_linked_canonical_node(true, false);
    if(node) {
        const auto container = node->get_members_container();
        if(container) {
            const auto op_info = operator_impl_info(expr->operation);
            if(op_info.name.empty()) {
                // this operator cannot be overloaded
                diagnoser.error("cannot override this operator", expr);
                return (BaseType*) typeBuilder.getVoidType();
            }
            const auto child_node = container->child(op_info.name);
            if(!child_node) {
                diagnoser.error(expr) << "expected function with name '" << op_info.name << "' to overload operator but found none";
                return (BaseType*) typeBuilder.getVoidType();
            }
            if(child_node->kind() == ASTNodeKind::FunctionDecl) {
                const auto func = child_node->as_function_unsafe();
                if(func->params.size() != 2) {
                    // since this expression has two values, we always expect two parameters
                    diagnoser.error(expr) << "expected operator implementation function to have exactly two parameters";
                    return (BaseType*) typeBuilder.getVoidType();
                }
                // yes, its overloading an operator
                return func->returnType;
            } else if(child_node->kind() == ASTNodeKind::MultiFunctionNode) {
                const auto multi_node = child_node->as_multi_func_node_unsafe();
                std::vector<Value*> args { expr->firstValue, expr->secondValue };
                const auto func = multi_node->func_for_call(args);
                if(!func) {
                    diagnoser.error(expr) << "expected function with name '" << op_info.name << "' to overload operator but found none";
                    return (BaseType*) typeBuilder.getVoidType();
                }
                if(func->params.size() != 2) {
                    // since this expression has two values, we always expect two parameters
                    diagnoser.error(expr) << "expected operator implementation function to have exactly two parameters";
                    return (BaseType*) typeBuilder.getVoidType();
                }
                // check the second type here that it matches the overloaded parameter
                if(!func->params[1]->type->satisfies(expr->secondValue, false)) {
                    diagnoser.error(expr->secondValue) << "value doesn't satisfy the overloaded operator parameter";
                    return (BaseType*) typeBuilder.getVoidType();
                }
                // yes, its overloading an operator
                return func->returnType;
            } else {
                diagnoser.error(expr) << "expected function with name '" << op_info.name << "' to overload operator but found none";
                return (BaseType*) typeBuilder.getVoidType();
            }

        }
    }

    // check second type is primitive
    if(!isPrimitive(secondType)) {
        diagnoser.error("expected the value to have primitive type", expr->secondValue);
    }

    const auto first = unwrap_reference(first_canonical->canonicalize_enum());
    const auto second = unwrap_reference(secondType->canonical()->canonicalize_enum());
    const auto first_kind = first->kind();
    const auto second_kind = second->kind();
    // operation between integer and float/double results in float/double
    if(first_kind == BaseTypeKind::IntN && (second_kind == BaseTypeKind::Float || second_kind == BaseTypeKind::Double)) {
        return second;
    } else if(second_kind == BaseTypeKind::IntN && (first_kind == BaseTypeKind::Float || first_kind == BaseTypeKind::Double)) {
        return first;
    }
    // operation between two integers of different int n types results in int n type of higher bit width
    if(first_kind == BaseTypeKind::IntN && second_kind == BaseTypeKind::IntN) {
        const auto first_intN = first->as_intn_type_unsafe();
        const auto second_intN = second->as_intn_type_unsafe();
        return first_intN->greater_than_in_bits(second_intN) ? first : second;
    }
    // addition or subtraction of integer value into a pointer
    if((expr->operation == Operation::Addition || expr->operation == Operation::Subtraction) && (first_kind == BaseTypeKind::Pointer && second_kind == BaseTypeKind::IntN) || (first_kind == BaseTypeKind::IntN && second_kind == BaseTypeKind::Pointer)) {
        return first;
    }
    // subtracting a pointer results in a ulong type
    if(expr->operation == Operation::Subtraction && first_kind == BaseTypeKind::Pointer && second_kind == BaseTypeKind::Pointer) {
        return typeBuilder.getULongType();
    }
    return first;
}

void Expression::determine_type(TypeBuilder& typeBuilder, ASTDiagnoser& diagnoser) {
    setType(::determine_type(this, typeBuilder, diagnoser));
}

uint64_t Expression::byte_size(TargetData& target) {
    return getType()->byte_size(target);
}

ASTNode* Expression::linked_node() {
    return getType() ? getType()->linked_node() : nullptr;
}

bool Expression::primitive() {
    return false;
}

/**
 * evaluates both values and returns the result as unique_tr to Value
 * @return
 */
Value *Expression::evaluate(InterpretScope &scope) {
    auto fEvl = firstValue->evaluated_value(scope);
    auto sEvl = secondValue->evaluated_value(scope);
    return scope.evaluate(operation, fEvl, sEvl, encoded_location(), this);
}

Expression *Expression::copy(ASTAllocator& allocator) {
    return new (allocator.allocate<Expression>()) Expression(
        firstValue->copy(allocator),
        secondValue->copy(allocator),
        operation,
        encoded_location(),
        getType()
    );
}

bool Expression::compile_time_computable() {
    return firstValue->compile_time_computable() && secondValue->compile_time_computable();
}