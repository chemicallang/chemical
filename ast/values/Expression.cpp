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
#include "ast/structures/ImplDefinition.h"
#include "compiler/symres/CoreNodes.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "ast/base/GlobalInterpretScope.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/values/StructValue.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/FileScope.h"

inline EnumDeclaration* getEnumDecl(BaseType* type) {
    return type->get_direct_linked_enum();
}

void Expression::replace_number_values(ASTAllocator& allocator, TypeBuilder& typeBuilder, BaseType* firstType, BaseType* secondType, Operation operation, TargetData& targetData) {
    if(firstType->kind() == BaseTypeKind::IntN && secondType->kind() == BaseTypeKind::IntN) {
        const auto is_shift = operation == Operation::LeftShift || operation == Operation::RightShift;
        if(is_shift) {
            if(secondValue->val_kind() == ValueKind::IntN) {
                auto valNType = (IntNType*) secondValue->getType()->canonical();
                auto targetNType = (IntNType*) firstType;
                if(valNType->num_bits(targetData) <= targetNType->num_bits(targetData)) {
                    auto value = ((IntNumValue*)secondValue)->get_num_value();
                    secondValue = ((IntNType*) firstType)->create(allocator, typeBuilder, value, secondValue->encoded_location());
                }
            }
        } else {
            if(firstValue->val_kind() == ValueKind::IntN) {
                auto valNType = (IntNType*) firstValue->getType()->canonical();
                auto targetNType = (IntNType*) secondType;
                if(valNType->num_bits(targetData) < targetNType->num_bits(targetData)) {
                    auto value = ((IntNumValue*)firstValue)->get_num_value();
                    firstValue = ((IntNType*) secondType)->create(allocator, typeBuilder, value, firstValue->encoded_location());
                }
            } else if(secondValue->val_kind() == ValueKind::IntN){
                auto valNType = (IntNType*) secondValue->getType()->canonical();
                auto targetNType = (IntNType*) firstType;
                if(valNType->num_bits(targetData) < targetNType->num_bits(targetData)) {
                    auto value = ((IntNumValue*)secondValue)->get_num_value();
                    secondValue = ((IntNType*) firstType)->create(allocator, typeBuilder, value, secondValue->encoded_location());
                }
            }
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

FunctionDeclaration* Expression::get_overloaded_func(CoreNodes& coreNodes, ImplementationsIndex& implsIndex) {
    const auto first_canonical = firstValue->getType()->canonical();
    const auto node = first_canonical->get_linked_canonical_node(true, false);
    if(node == nullptr) return nullptr;
    const auto container = node->get_members_container();
    if(container == nullptr) return nullptr;
    return implsIndex.get_expr_op_impl(coreNodes, container, operation);
}

bool isUntypedIntegerLiteral(Value* value, IntNTypeKind k) {
    if(k != IntNTypeKind::Int) return false;
    return value->kind() == ValueKind::IntN;
}

bool fits_into(IntNumValue* value, IntNType* type, TargetData& targetData) {
    const auto bits = type->num_bits(targetData);
    const auto is_unsigned = type->is_unsigned();
    const auto val = value->get_num_value();

    // if the value is unsigned, we just check if it fits in bits
    // but if the type is signed, we must check if it fits in bits - 1
    // because 1 bit is used for sign

    if(is_unsigned) {
        if(bits >= 64) return true;
        // check if value fits in bits
        return (val >> bits) == 0;
    } else {
        // if value is signed, we must check if it fits in bits - 1
        // however the value we have is uint64_t, so we must check if it fits in int64_t
        // and then check if it fits in bits - 1
        if(bits >= 64) return true; // assuming it fits in 64 bits (int64_t)
        // check if value fits in bits - 1
        const auto max = (1ULL << (bits - 1)) - 1;
        return val <= max;
    }
}

BaseType* Expression::get_determined_type(
    TypeBuilder& typeBuilder,
    CoreNodes& coreNodes,
    ImplementationsIndex& implsIndex,
    ASTDiagnoser& diagnoser,
    TargetData& targetData
) {
    const auto expr = this;
    auto firstType = expr->firstValue->getType();
    auto secondType = expr->secondValue->getType();
    if(expr->operation >= Operation::IndexBooleanReturningStart && expr->operation <= Operation::IndexBooleanReturningEnd) {
        // check first type is primitive
        if(!firstType->isPrimitive()) {
            // check if overloaded operator exists
            const auto overloaded = get_overloaded_func(coreNodes, implsIndex);
            if (overloaded == nullptr) {
                diagnoser.error("expected the value to have primitive type or have operator overloaded", expr->firstValue);
                return typeBuilder.getBoolType();
            }
            if(overloaded->params.size() != 2) {
                // since this expression has two values, we always expect two parameters
                diagnoser.error(expr) << "expected operator implementation function to have exactly two parameters";
                return typeBuilder.getBoolType();
            }
            // check the second type here that it matches the overloaded parameter
            if(!overloaded->params[1]->type->satisfies(expr->secondValue, false)) {
                diagnoser.error(expr->secondValue) << "value doesn't satisfy the overloaded operator parameter";
                return typeBuilder.getBoolType();
            }
            // return early second type has been checked
            return typeBuilder.getBoolType();
        }
        if(!secondType->isPrimitive()) {
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
            // check if overloaded operator exists
            const auto func = implsIndex.get_expr_op_impl(coreNodes, container, expr->operation);
            if (func == nullptr) {
                diagnoser.error("expected the value to have primitive type or have operator overloaded", expr->firstValue);
                return typeBuilder.getBoolType();
            }
            if(func->params.size() != 2) {
                // since this expression has two values, we always expect two parameters
                diagnoser.error(expr) << "expected operator implementation function to have exactly two parameters";
                return (BaseType*) typeBuilder.getVoidType();
            }
            // yes, its overloading an operator
            return func->returnType;
        }
    }

    // check first type is primitive
    if(!firstType->isPrimitive()) {
        diagnoser.error("expected the value to have primitive type", expr->firstValue);
    }
    // check second type is primitive
    if(!secondType->isPrimitive()) {
        diagnoser.error("expected the value to have primitive type", expr->secondValue);
    }

    const auto first = first_canonical->canonicalize_enum();
    const auto second = secondType->canonical()->canonicalize_enum();
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
        const auto firstIntNKind = first_intN->IntNKind();
        const auto secondIntNKind = second_intN->IntNKind();
        const auto first_literal = isUntypedIntegerLiteral(expr->firstValue, firstIntNKind);
        const auto second_literal = isUntypedIntegerLiteral(expr->secondValue, secondIntNKind);
        const auto is_shift = expr->operation == Operation::LeftShift || expr->operation == Operation::RightShift;
        // coercing literals to the other value's type
        // for shifts, the result type is the left operand's type, so never coerce the left literal
        if(first_literal && !second_literal) {
            if(!is_shift && fits_into(expr->firstValue->as_int_num_value_unsafe(), second_intN, targetData)) {
                expr->firstValue->setType(second);
                return second;
            }
        } else if(!first_literal && second_literal) {
            if(fits_into(expr->secondValue->as_int_num_value_unsafe(), first_intN, targetData)) {
                expr->secondValue->setType(first);
                return first;
            }
        }
        // selecting appropriate types based on first and second signed ness and bitwidth
        // we are matching c's behavior with this logic
        const auto is_first_greater_in_bits = first_intN->greater_than_in_bits(second_intN, targetData);
        const auto is_first_unsigned = first_intN->is_unsigned();
        const auto is_second_unsigned = second_intN->is_unsigned();
        if(is_first_unsigned && !is_second_unsigned) {
            if(is_first_greater_in_bits) {
                // first is greater in bits, but its unsigned
                // we want a signed type but equal to first's bitwidth
                return typeBuilder.getIntNType(IntNType::to_signed_kind(firstIntNKind));
            } else {
                return second;
            }
        } else if(!is_first_unsigned && is_second_unsigned) {
            if(is_first_greater_in_bits) {
                return first;
            } else {
                // second is greater in bits, but its unsigned
                // we want a signed type but equal to second's bitwidth
                return typeBuilder.getIntNType(IntNType::to_signed_kind(secondIntNKind));
            }
        } else {
            // both signed or unsigned
            return is_first_greater_in_bits ? first : second;
        }
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
    // Check for operator overloading using the AST-level type (before value evaluation)
    // Try first value's type first, then second value's type as fallback
    FunctionDeclaration* overloaded = nullptr;
    auto glob = scope.global;
    if(glob && glob->build_compiler) {
        overloaded = get_overloaded_func(
            glob->build_compiler->coreNodes,
            glob->build_compiler->implsIndex
        );
        // If not found via first value, try second value's type
        if(!overloaded) {
            // Use a temporary swap approach to check second value's type
            auto tempFirst = firstValue;
            firstValue = secondValue;
            auto tempSecond = secondValue;
            secondValue = tempFirst;
            overloaded = get_overloaded_func(
                glob->build_compiler->coreNodes,
                glob->build_compiler->implsIndex
            );
            // Restore original order — if overloaded is found, it's called
            // with self=sEvl and rhs=fEvl below
            firstValue = tempFirst;
            secondValue = tempSecond;
        }

    }
    
    auto fEvl = firstValue->evaluated_value(scope);
    if(!fEvl) return nullptr;
    
    // Short-circuit evaluation for logical operators
    if (operation == Operation::LogicalAND) {
        if (fEvl->val_kind() == ValueKind::Bool && !fEvl->get_the_bool()) return fEvl;
    } else if (operation == Operation::LogicalOR) {
        if (fEvl->val_kind() == ValueKind::Bool && fEvl->get_the_bool()) return fEvl;
    }
    
    auto sEvl = secondValue->evaluated_value(scope);
    if(!sEvl) return nullptr;
    
    // Try to find overloaded operator from evaluated values' runtime types if AST-level lookup failed
    if(!overloaded && glob && glob->build_compiler) {
        // Helper: find the container (ExtendableMembersContainerNode) from a value
        auto findStructDef = [](Value* val) -> ExtendableMembersContainerNode* {
            if(!val) return nullptr;
            if(val->val_kind() == ValueKind::StructValue) {
                auto sv = val->as_struct_value_unsafe();
                auto ext = sv->linked_extendable();
                if(ext && (ext->kind() == ASTNodeKind::StructDecl || ext->kind() == ASTNodeKind::VariantDecl)) {
                    return ext;
                }
            }
            return nullptr;
        };
        // Map operation to operator function name
        const char* opName = nullptr;
        switch(operation) {
            case Operation::Addition: opName = "add"; break;
            case Operation::Subtraction: opName = "sub"; break;
            case Operation::Multiplication: opName = "mul"; break;
            case Operation::Division: opName = "div"; break;
            case Operation::Modulus: opName = "rem"; break;
            case Operation::IsEqual: opName = "eq"; break;
            case Operation::IsNotEqual: opName = "ne"; break;
            case Operation::LessThan: opName = "lt"; break;
            case Operation::LessThanOrEqual: opName = "lte"; break;
            case Operation::GreaterThan: opName = "gt"; break;
            case Operation::GreaterThanOrEqual: opName = "gte"; break;
            case Operation::BitwiseAND: opName = "bitand"; break;
            case Operation::BitwiseOR: opName = "bitor"; break;
            case Operation::BitwiseXOR: opName = "bitxor"; break;
            case Operation::LeftShift: opName = "shl"; break;
            case Operation::RightShift: opName = "shr"; break;
            default: break;
        }
        ExtendableMembersContainerNode* structDef = findStructDef(fEvl);
        if(!structDef) structDef = findStructDef(sEvl);
        if(structDef && opName) {
            // Try implsIndex first
            auto container = structDef->get_members_container();
            if(container) {
                overloaded = glob->build_compiler->implsIndex.get_expr_op_impl(
                    glob->build_compiler->coreNodes, container, operation
                );
            }
            // Fallback: search parent scope for ImplDefinitions matching this struct
            if(!overloaded) {
                chem::string_view opNameSv(opName);
                // Search the struct's parent chain for ImplDefinitions
                auto searchParent = [&](ASTNode* parent) {
                    if(!parent) return;
                    std::vector<ASTNode*>* nodes = nullptr;
                    if(parent->kind() == ASTNodeKind::NamespaceDecl) {
                        nodes = &parent->as_namespace_unsafe()->nodes;
                    } else if(parent->kind() == ASTNodeKind::FileScope) {
                        nodes = &static_cast<FileScope*>(parent)->body.nodes;
                    } else if(parent->kind() == ASTNodeKind::Scope) {
                        nodes = &static_cast<Scope*>(parent)->nodes;
                    }
                    if(!nodes) return;
                    for(auto child : *nodes) {
                        if(!child || child->kind() != ASTNodeKind::ImplDecl) continue;
                        auto impl = child->as_impl_def_unsafe();
                        if(!impl || !impl->struct_type) continue;
                        // Check if impl's struct type matches our structDef
                        auto implStruct = impl->struct_type->get_direct_linked_canonical_node();
                        bool matches = (implStruct == structDef);
                        if(!matches && implStruct && structDef &&
                            implStruct->get_node_identifier() == structDef->get_node_identifier()) {
                            matches = true;
                        }
                        if(!matches) continue;
                        auto fn = impl->direct_child_function(opNameSv);
                        if(fn && fn->body.has_value()) {
                            overloaded = fn;
                            return;
                        }
                    }
                };
                searchParent(structDef->parent());
            }
        }
    }
    
    // Dispatch operator overloads: call the impl function with self=fEvl and rhs=sEvl
    if(overloaded) {
        const auto prev_func = glob->current_func_type;
        glob->current_func_type = overloaded;
        glob->call_stack.emplace_back(nullptr);
        InterpretScope fn_scope(glob, glob->allocator, glob);
        InterpretScope* prop = &scope;
        while(prop) {
            for(auto& [name, val] : prop->implicit_args) {
                if(fn_scope.implicit_args.find(name) == fn_scope.implicit_args.end()) {
                    fn_scope.implicit_args[name] = val;
                }
            }
            prop = prop->parent;
        }
        std::vector<Value*> opArgs = { sEvl };
        auto result = overloaded->call(&scope, opArgs, fEvl, &fn_scope, true, this);
        glob->call_stack.pop_back();
        glob->current_func_type = prev_func;
        return result;
    }
    
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