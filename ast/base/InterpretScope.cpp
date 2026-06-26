// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "InterpretScope.h"
#include "GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "Value.h"
#include <iostream>
#include "ast/structures/Scope.h"
#include "ast/values/BoolValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/PointerValue.h"
#include "ast/values/StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/WrapValue.h"
#include "ast/values/CastedValue.h"
#include "std/except.h"
#include "compiler/lab/LabBuildCompiler.h"

#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_RESET   "\x1b[0m"

Value* InterpretScope::getNullValue() {
    return global->typeBuilder.getNullValue();
}

void InterpretScope::declare(const chem::string_view& name, Value* value) {
    values[name] = value;
}

Value* InterpretScope::find_value(const chem::string_view& name) {
    auto found = values.find(name);
    if (found == values.end()) {
        if(parent == nullptr) return nullptr;
        return parent->find_value(name);
    } else {
        return found->second;
    }
}

std::pair<value_iterator, InterpretScope&> InterpretScope::find_value_iterator(const chem::string_view& name) {
    auto found = values.find(name);
    if (found == values.end()) {
        if(parent == nullptr) return { values.end(), *this };
        return parent->find_value_iterator(name);
    } else {
        return { found, *this };
    }
}

void InterpretScope::erase_value(const chem::string_view& name) {
    auto iterator = find_value_iterator(name);
    if(iterator.first == iterator.second.values.end()) {
        std::cerr << ANSI_COLOR_RED << "couldn't locate value " << name << " for removal" << ANSI_COLOR_RESET
                  << std::endl;
#ifdef DEBUG
        print_values();
#endif
    } else {
        iterator.second.values.erase(iterator.first);
    }
}

bool operate(Operation op, bool first, bool second) {
    switch(op) {
        case Operation::LogicalAND:
            return first && second;
        case Operation::LogicalOR:
            return first || second;
        case Operation::IsEqual:
            return first == second;
        case Operation::IsNotEqual:
            return first != second;
        default:
#ifdef DEBUG
            CHEM_THROW_RUNTIME("unknown operation between bool values");
#endif
            return false;
    }
}

uint64_t operate(Operation op, uint64_t first, uint64_t second) {
    switch(op) {
        case Operation::Addition:
            return first + second;
        case Operation::Subtraction:
            return first - second;
        case Operation::Multiplication:
            return first * second;
        case Operation::Division:
            return first / second;
        case Operation::IsEqual:
            return first == second;
        case Operation::IsNotEqual:
            return first != second;
        case Operation::GreaterThan:
            return first > second;
        case Operation::LessThan:
            return first < second;
        case Operation::GreaterThanOrEqual:
            return first >= second;
        case Operation::LessThanOrEqual:
            return first <= second;
        case Operation::Modulus:
            return first % second;
        case Operation::LeftShift:
            return first << second;
        case Operation::RightShift:
            return first >> second;
        case Operation::BitwiseAND:
            return first & second;
        case Operation::BitwiseOR:
            return first | second;
        case Operation::BitwiseXOR:
            return first ^ second;
        default:
#ifdef DEBUG
            CHEM_THROW_RUNTIME("UNKNOWN INTERPRET OPERATION");
#endif
            return 0;
    }
}

double operate(Operation op, double first, double second) {
    switch(op) {
        case Operation::Addition:
            return first + second;
        case Operation::Subtraction:
            return first - second;
        case Operation::Multiplication:
            return first * second;
        case Operation::Division:
            return first / second;
        case Operation::IsEqual:
            return first == second;
        case Operation::IsNotEqual:
            return first != second;
        case Operation::GreaterThan:
            return first > second;
        case Operation::LessThan:
            return first < second;
        case Operation::GreaterThanOrEqual:
            return first >= second;
        case Operation::LessThanOrEqual:
            return first <= second;
        default:
#ifdef DEBUG
            CHEM_THROW_RUNTIME("UNKNOWN INTERPRET OPERATION");
#endif
            return 0;
    }
}

std::optional<bool> operate(Operation op, const chem::string_view& first, const chem::string_view& second) {
    switch(op) {
        case Operation::IsEqual:
            return first == second;
        case Operation::IsNotEqual:
            return first != second;
        case Operation::GreaterThan:
            return first > second;
        case Operation::LessThan:
            return first < second;
        case Operation::GreaterThanOrEqual:
            return first >= second;
        case Operation::LessThanOrEqual:
            return first <= second;
        default:
            return std::nullopt;
    }
}

bool is_bool_output(Operation op) {
    switch(op) {
        case Operation::IsEqual:
        case Operation::IsNotEqual:
        case Operation::GreaterThan:
        case Operation::GreaterThanOrEqual:
        case Operation::LessThan:
        case Operation::LessThanOrEqual:
            return true;
        default:
            return false;
    }
}

inline bool is_int_n(ValueKind k) {
    return k == ValueKind::IntN;
}

inline BoolValue* pack_bool(InterpretScope& scope, bool value, SourceLocation location) {
    return new (scope.allocate<BoolValue>()) BoolValue(value, scope.global->typeBuilder.getBoolType(), location);
}

Value* InterpretScope::evaluate(Operation operation, Value* fEvl, Value* sEvl, SourceLocation location, Value* debugValue) {
    auto& scope = *this;
    // Fully resolve WrapValue transparently: unwrap and evaluate the underlying
    if(fEvl->kind() == ValueKind::WrapValue) {
        const auto underlying = fEvl->as_wrap_value_unsafe()->underlying;
        fEvl = underlying->evaluated_value(scope);
        if(!fEvl) return nullptr;
    }
    if(sEvl->kind() == ValueKind::WrapValue) {
        const auto underlying = sEvl->as_wrap_value_unsafe()->underlying;
        sEvl = underlying->evaluated_value(scope);
        if(!sEvl) return nullptr;
    }
    const auto fKind = fEvl->val_kind();
    const auto sKind = sEvl->val_kind();
    if(fKind == ValueKind::Bool && sKind == ValueKind::Bool) {
        const auto result = operate(operation, fEvl->get_the_bool(), sEvl->get_the_bool());
        return pack_bool(scope, result, location);
    } else if(is_int_n(fKind) && is_int_n(sKind)) {
        // both values are int num values
        const auto first = (IntNumValue*) fEvl;
        const auto second = (IntNumValue*) sEvl;
        uint64_t answer;
        if(operation == Operation::RightShift && !first->getType()->as_intn_type_unsafe()->is_unsigned()) {
            // Arithmetic right shift for signed types: cast to int64_t to preserve sign
            answer = (uint64_t)((int64_t)first->get_num_value() >> second->get_num_value());
        } else {
            answer = operate(operation, first->get_num_value(), second->get_num_value());
        }
        if(is_bool_output(operation)) {
            return pack_bool(scope, answer, location);
        } else {
            return pack_by_kind(scope, determine_output(first->getType()->IntNKind(), second->getType()->IntNKind()), answer, location);
        }
    } else if(fKind == ValueKind::Double || fKind == ValueKind::Float || sKind == ValueKind::Double || sKind == ValueKind::Float) {
        const auto first = get_double_value(fEvl, fKind);
        const auto second = get_double_value(sEvl, sKind);
        const auto answer = operate(operation, first, second);
        if(is_bool_output(operation)) {
            return pack_bool(scope, (bool) answer, location);
        } else {
            const auto both_float = fKind == ValueKind::Float && sKind == ValueKind::Float;
            return pack_by_kind(scope, both_float ? ValueKind::Float : ValueKind::Double, answer, location);
        }
    } else if(fKind == ValueKind::NullValue || sKind == ValueKind::NullValue) {
        // comparison with null, a == null or null == a
        // Also handle PointerValue vs NullValue (null pointers)
        auto isFirstNull = (fKind == ValueKind::NullValue) || 
            (fKind == ValueKind::PointerValue && ((PointerValue*)fEvl)->data == nullptr);
        auto isSecondNull = (sKind == ValueKind::NullValue) || 
            (sKind == ValueKind::PointerValue && ((PointerValue*)sEvl)->data == nullptr);
        switch (operation) {
            case Operation::IsEqual:
                return pack_bool(scope, isFirstNull && isSecondNull, location);
            case Operation::IsNotEqual:
                return pack_bool(scope, isFirstNull != isSecondNull, location);
            default:
                return new (scope.allocate<NullValue>()) NullValue(global->typeBuilder.getNullPtrType(), location);
        }
    } else if(fKind == ValueKind::String && sKind == ValueKind::String) {
        const auto firstVal = fEvl->as_string_unsafe();
        const auto secondVal = sEvl->as_string_unsafe();
        auto bool_answer = operate(operation, firstVal->value, secondVal->value);
        if(bool_answer.has_value()) {
            return pack_bool(scope, bool_answer.value(), location);
        } else {
            if(operation == Operation::Addition) {
                const auto expected_size = firstVal->value.size() + secondVal->value.size();
                const auto new_ptr = scope.allocator.allocate_released_size(sizeof(char) * (expected_size), alignof(char));
                memcpy(new_ptr, firstVal->value.data(), firstVal->value.size());
                memcpy(new_ptr + firstVal->value.size(), secondVal->value.data(), secondVal->value.size());
                return new (scope.allocate<StringValue>()) StringValue(chem::string_view(new_ptr, expected_size), firstVal->getType(), location);
            } else {
                scope.error("unknown operation between strings", debugValue);
                return nullptr;
            }
        }
    } else if((fKind == ValueKind::String && is_int_n(sKind))) {
        const auto strVal = fEvl->as_string_unsafe();
        const auto numVal = (IntNumValue*) sEvl;
        const auto num = numVal->get_num_value();
        return new (scope.allocate<StringValue>()) StringValue(chem::string_view(strVal->value.data() + num, strVal->value.size() - num), scope.global->typeBuilder.getStringType(), location);
    } else if ((sKind == ValueKind::String && is_int_n(fKind))) {
        const auto strVal = sEvl->as_string_unsafe();
        const auto numVal = (IntNumValue*) fEvl;
        const auto num = numVal->get_num_value();
        return new (scope.allocate<StringValue>()) StringValue(chem::string_view(strVal->value.data() + num, strVal->value.size() - num), scope.global->typeBuilder.getStringType(), location);
    } else if((fKind == ValueKind::PointerValue && is_int_n(sKind))) {
        const auto ptrVal = (PointerValue*) fEvl;
        switch(operation) {
            case Operation::Addition:
                return ptrVal->increment(scope, sEvl->as_int_num_value_unsafe()->get_num_value(), location, debugValue);
            case Operation::Subtraction:
                return ptrVal->decrement(scope, sEvl->as_int_num_value_unsafe()->get_num_value(), location, debugValue);
            case Operation::IsEqual:
                return pack_bool(scope, (uintptr_t)ptrVal->data == sEvl->get_number().value_or(0), location);
            case Operation::IsNotEqual:
                return pack_bool(scope, (uintptr_t)ptrVal->data != sEvl->get_number().value_or(0), location);
            default:
                scope.error("unknown operation performed on a pointer value", debugValue);
                return nullptr;
        }
    } else if((sKind == ValueKind::PointerValue && is_int_n(fKind))) {
        const auto ptrVal = (PointerValue*) sEvl;
        switch(operation) {
            case Operation::Addition:
                return ptrVal->increment(scope, fEvl->as_int_num_value_unsafe()->get_num_value(), location, debugValue);
            case Operation::Subtraction:
                return ptrVal->decrement(scope, fEvl->as_int_num_value_unsafe()->get_num_value(), location, debugValue);
            case Operation::IsEqual:
                return pack_bool(scope, (uintptr_t)ptrVal->data == fEvl->get_number().value_or(0), location);
            case Operation::IsNotEqual:
                return pack_bool(scope, (uintptr_t)ptrVal->data != fEvl->get_number().value_or(0), location);
            default:
                scope.error("unknown operation performed on a pointer value", debugValue);
                return nullptr;
        }
    } else if(fKind == ValueKind::PointerValue && sKind == ValueKind::PointerValue) {
        // Pointer - Pointer subtraction (element count difference)
        auto firstPtr = (PointerValue*) fEvl;
        auto secondPtr = (PointerValue*) sEvl;
        const auto castedTypeSize = firstPtr->getType()->byte_size(scope.global->target_data);
        if(castedTypeSize == 0) {
            scope.error("cannot subtract pointers with zero-sized element type", debugValue);
            return nullptr;
        }
        const auto byteDiff = (char*) firstPtr->data - (char*) secondPtr->data;
        // Note: this is a signed difference (could be negative for subtraction)
        // For pointer subtraction, we return the element count
        switch(operation) {
            case Operation::Subtraction: {
                auto elemDiff = (int64_t)(byteDiff / (int64_t)castedTypeSize);
                // Return signed 64-bit integer (ptrdiff_t equivalent)
                return new (scope.allocate<IntNumValue>()) IntNumValue(
                    (uint64_t) elemDiff,
                    scope.global->typeBuilder.getIntNType(64, true),
                    location
                );
            }
            case Operation::IsEqual:
                return pack_bool(scope, firstPtr->data == secondPtr->data, location);
            case Operation::IsNotEqual:
                return pack_bool(scope, firstPtr->data != secondPtr->data, location);
            case Operation::LessThan:
                return pack_bool(scope, firstPtr->data < secondPtr->data, location);
            case Operation::GreaterThan:
                return pack_bool(scope, firstPtr->data > secondPtr->data, location);
            case Operation::LessThanOrEqual:
                return pack_bool(scope, firstPtr->data <= secondPtr->data, location);
            case Operation::GreaterThanOrEqual:
                return pack_bool(scope, firstPtr->data >= secondPtr->data, location);
            default:
                scope.error("unknown operation between pointer values", debugValue);
                return nullptr;
        }
    } else {
            // Handle CastedValue: unwrap to the underlying evaluated value
        if(fKind == ValueKind::CastedValue) {
            auto casted = (CastedValue*)fEvl;
            auto unwrapped = casted->evaluated_value(scope);
            if(!unwrapped) {
                scope.error("Operation between values of unknown kind (casted value resolved to null)", debugValue);
                return nullptr;
            }
            return scope.evaluate(operation, unwrapped, sEvl, location, debugValue);
        }
        if(sKind == ValueKind::CastedValue) {
            auto casted = (CastedValue*)sEvl;
            auto unwrapped = casted->evaluated_value(scope);
            if(!unwrapped) {
                scope.error("Operation between values of unknown kind (casted value resolved to null)", debugValue);
                return nullptr;
            }
            return scope.evaluate(operation, fEvl, unwrapped, location, debugValue);
        }
        // Utility: get MembersContainer from a value using type-based lookup (more reliable than linked_extendable)
        auto getContainerFromValue = [](Value* val) -> MembersContainer* {
            if(!val) return nullptr;
            auto type = val->getType();
            if(type) {
                auto canonical = type->canonical();
                if(canonical) {
                    auto node = canonical->get_linked_canonical_node(true, false);
                    if(node) return node->get_members_container();
                }
            }
            return nullptr;
        };
        // Handle compound assignment operators: convert AddTo→add_assign, SubtractFrom→sub_assign, etc.
        {
            Operation compoundToBinary = operation;
            switch(operation) {
                case Operation::AddTo: compoundToBinary = Operation::Addition; break;
                case Operation::SubtractFrom: compoundToBinary = Operation::Subtraction; break;
                case Operation::MultiplyBy: compoundToBinary = Operation::Multiplication; break;
                case Operation::DivideBy: compoundToBinary = Operation::Division; break;
                case Operation::ModuloBy: compoundToBinary = Operation::Modulus; break;
                case Operation::ShiftLeftBy: compoundToBinary = Operation::LeftShift; break;
                case Operation::ShiftRightBy: compoundToBinary = Operation::RightShift; break;
                case Operation::ANDWith: compoundToBinary = Operation::BitwiseAND; break;
                case Operation::ExclusiveORWith: compoundToBinary = Operation::BitwiseXOR; break;
                case Operation::InclusiveORWith: compoundToBinary = Operation::BitwiseOR; break;
                default: break;
            }
            if(compoundToBinary != operation) {
                auto glob = scope.global;
                if(glob && glob->build_compiler) {
                    auto& coreNodes = glob->build_compiler->coreNodes;
                    auto& implsIndex = glob->build_compiler->implsIndex;
                    MembersContainer* container = getContainerFromValue(fEvl);
                    if(!container) container = getContainerFromValue(sEvl);
                    if(container) {
                        auto impl = implsIndex.get_ass_op_impl(coreNodes, container, compoundToBinary);
                        if(!impl) {
                            impl = implsIndex.get_expr_op_impl(coreNodes, container, compoundToBinary);
                        }
                        if(impl) {
                            const auto prev_func = glob->current_func_type;
                            glob->current_func_type = impl;
                            glob->call_stack.emplace_back(nullptr);
                            InterpretScope fn_scope(glob, glob->allocator, glob);
                            std::vector<Value*> opArgs = { sEvl };
                            auto result = impl->call(&scope, opArgs, fEvl, &fn_scope, true, debugValue);
                            glob->call_stack.pop_back();
                            glob->current_func_type = prev_func;
                            return result;
                        }
                    }
                }
            }
        }
        // Fallback: try operator overload dispatch via build_compiler
        auto glob = scope.global;
        if(glob && glob->build_compiler) {
            auto& coreNodes = glob->build_compiler->coreNodes;
            auto& implsIndex = glob->build_compiler->implsIndex;
            // Convert compound assignment ops to binary for lookup in the regular fallback
            Operation lookupOp = operation;
            switch(operation) {
                case Operation::AddTo: lookupOp = Operation::Addition; break;
                case Operation::SubtractFrom: lookupOp = Operation::Subtraction; break;
                case Operation::MultiplyBy: lookupOp = Operation::Multiplication; break;
                case Operation::DivideBy: lookupOp = Operation::Division; break;
                case Operation::ModuloBy: lookupOp = Operation::Modulus; break;
                case Operation::ShiftLeftBy: lookupOp = Operation::LeftShift; break;
                case Operation::ShiftRightBy: lookupOp = Operation::RightShift; break;
                case Operation::ANDWith: lookupOp = Operation::BitwiseAND; break;
                case Operation::ExclusiveORWith: lookupOp = Operation::BitwiseXOR; break;
                case Operation::InclusiveORWith: lookupOp = Operation::BitwiseOR; break;
                default: break;
            }
            MembersContainer* container = getContainerFromValue(fEvl);
            if(!container) container = getContainerFromValue(sEvl);
            if(container) {
                // Try specific assign operator impl first (for compound assignments like +=)
                FunctionDeclaration* overloaded = nullptr;
                if(lookupOp != operation) {
                    overloaded = implsIndex.get_ass_op_impl(coreNodes, container, lookupOp);
                }
                if(!overloaded) {
                    overloaded = implsIndex.get_expr_op_impl(coreNodes, container, lookupOp);
                }
                if(!overloaded && fEvl != sEvl) {
                    MembersContainer* altContainer = getContainerFromValue(sEvl);
                    if(altContainer && altContainer != container) {
                        if(!overloaded && lookupOp != operation) {
                            overloaded = implsIndex.get_ass_op_impl(coreNodes, altContainer, lookupOp);
                        }
                        if(!overloaded) {
                            overloaded = implsIndex.get_expr_op_impl(coreNodes, altContainer, lookupOp);
                        }
                    }
                }
    
                if(overloaded) {
                    const auto prev_func = glob->current_func_type;
                    glob->current_func_type = overloaded;
                    glob->call_stack.emplace_back(nullptr);
                    InterpretScope fn_scope(glob, glob->allocator, glob);
                    std::vector<Value*> opArgs = { sEvl };
                    auto result = overloaded->call(&scope, opArgs, fEvl, &fn_scope, true, debugValue);
                    glob->call_stack.pop_back();
                    glob->current_func_type = prev_func;
                    return result;
                }
            }
        }
        std::fprintf(stderr, "[DBG_EVAL] unknown operation: fKind=%d sKind=%d op=%d\n",
            (int)fKind, (int)sKind, (int)operation);
        scope.error("Operation between values of unknown kind", debugValue);
        return nullptr;
    }
}

void InterpretScope::error(std::string& err, ASTNode* any) {
    global->interpret_error(err, any);
}

void InterpretScope::error(std::string& err, Value* any) {
    global->interpret_error(err, any);
}

void InterpretScope::error(std::string_view err, ASTNode* any) {
    global->interpret_error(err, any);
}

void InterpretScope::error(std::string_view err, Value* any) {
    global->interpret_error(err, any);
}

InterpretScope::~InterpretScope() {
    if(should_destruct_values) {
        destroy_values();
    }
}

void InterpretScope::destroy_values() {
    for(auto& [name, val] : values) {
        if (val == nullptr) continue;
        // Skip the return value (it's been moved to the caller)
        if(val == returnValue) continue;

        // Recursive helper to destruct a single value (struct, array, or nested)
        auto destruct_value = [this](Value* target_val, auto& self_ref) -> void {
            if (!target_val) return;
            if(target_val->val_kind() == ValueKind::StructValue) {
                auto structVal = target_val->as_struct_value_unsafe();
                auto ext = structVal->linked_extendable();
                if(ext && (ext->kind() == ASTNodeKind::StructDecl || ext->kind() == ASTNodeKind::VariantDecl)) {
                    ExtendableMembersContainerNode* container = ext;
                    if(container->has_destructor()) {
                        auto destructor_fn = container->destructor_func();
                        if(destructor_fn && destructor_fn->body.has_value()) {
                            const auto prev_func = global->current_func_type;
                            global->current_func_type = destructor_fn;

                            InterpretScope child_scope(global, allocator, global);
                            child_scope.declare("self", target_val);
                            child_scope.interpret(&destructor_fn->body.value());

                            auto self_it = child_scope.values.find("self");
                            if(self_it != child_scope.values.end()) {
                                child_scope.values.erase(self_it);
                            }

                            global->current_func_type = prev_func;
                        }
                    }
                }
                // Always recursively destruct member values, matching C backend behavior:
                // the C codegen calls the user destructor first, then generates member
                // destruction code. With proper move semantics in struct literal initialization,
                // the source variable is cleared before it's stored as a member, so there's
                // no double-destruction risk.
                for(auto& [member_name, member_init] : structVal->values) {
                    if(member_init.value) {
                        self_ref(member_init.value, self_ref);
                    }
                }
            } else if(target_val->val_kind() == ValueKind::ArrayValue) {
                auto arrVal = target_val->as_array_value_unsafe();
                // Destruct each element that is a struct value with a destructor.
                // Elements were populated by evaluated_value() or by set_value() and
                // AddrOfValue already creates PointerValues pointing directly into this
                // vector, so modifications through &arr[i] affect the actual elements.
                for(size_t ei = 0; ei < arrVal->values.size(); ei++) {
                    auto elemVal = arrVal->values[ei];
                    if(elemVal) {
                        self_ref(elemVal, self_ref);
                    }
                }
            }
        };

        destruct_value(val, destruct_value);
    }
}

void InterpretScope::move_clear_source(Value* initializer, const chem::string_view& new_name) {
    if(!initializer) return;
    
    bool shouldClear = false;
    
    if(initializer->val_kind() == ValueKind::StructValue) {
        auto structVal = initializer->as_struct_value_unsafe();
        auto ext = structVal->linked_extendable();
        if(ext && (ext->kind() == ASTNodeKind::StructDecl || ext->kind() == ASTNodeKind::VariantDecl)) {
            auto container = (ExtendableMembersContainerNode*)ext;
            if(container->has_destructor()) {
                shouldClear = true;
            }
        }
    } else if(initializer->val_kind() == ValueKind::ArrayValue) {
        auto arrVal = initializer->as_array_value_unsafe();
        for(auto elem : arrVal->values) {
            if(!elem) continue;
            Value* actualElem = elem;
            // Resolve Identifier elements to their actual value to check
            // if they contain destructible structs (e.g. [d] where d is a struct).
            if(elem->val_kind() == ValueKind::Identifier) {
                auto idVal = elem->as_identifier_unsafe();
                actualElem = find_value(idVal->value);
            }
            if(actualElem && actualElem->val_kind() == ValueKind::StructValue) {
                auto sv = actualElem->as_struct_value_unsafe();
                auto ext = sv->linked_extendable();
                if(ext && (ext->kind() == ASTNodeKind::StructDecl || ext->kind() == ASTNodeKind::VariantDecl)) {
                    auto container = (ExtendableMembersContainerNode*)ext;
                    if(container->has_destructor()) {
                        shouldClear = true;
                        break;
                    }
                }
            }
        }
    }
    
    if(!shouldClear) return;
    
    InterpretScope* scanScope = this;
    while(scanScope) {
        for(auto& [name, val] : scanScope->values) {
            if(name != new_name) {
                if(val == initializer) {
                    val = nullptr;
                    return;
                }
                // For ArrayValue initializers, also check if any scope variable
                // holds a struct that matches an element in the array.
                if(initializer->val_kind() == ValueKind::ArrayValue && val != nullptr) {
                    auto arrVal = initializer->as_array_value_unsafe();
                    for(size_t ei = 0; ei < arrVal->values.size(); ei++) {
                        auto elem = arrVal->values[ei];
                        if(!elem) continue;
                        if(val == elem) {
                            val = nullptr;
                            return;
                        }
                        // Identifier elements (from [d] syntax) need to be resolved
                        // to their actual value for comparison.
                        if(elem->val_kind() == ValueKind::Identifier) {
                            auto idVal = elem->as_identifier_unsafe();
                            auto actualVal = find_value(idVal->value);
                            if(actualVal && val == actualVal) {
                                val = nullptr;
                                // Replace the identifier in the array with the actual
                                // value so that array destruction properly cleans it up.
                                arrVal->values[ei] = actualVal;
                                return;
                            }
                        }
                    }
                }
            }
        }
        scanScope = scanScope->parent;
    }
}

void InterpretScope::print_values() {
    std::cout << "Values:" << std::endl;
    for (auto const &value: values) {
        std::cout << value.first << " : " << value.second->representation() << std::endl;
    }
    if(parent != nullptr) {
        std::cout << "Parent ";
        parent->print_values();
    }
}