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
#include "std/except.h"

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
    const auto fKind = fEvl->val_kind();
    const auto sKind = sEvl->val_kind();
    if(fKind == ValueKind::Bool && sKind == ValueKind::Bool) {
        const auto result = operate(operation, fEvl->get_the_bool(), sEvl->get_the_bool());
        return pack_bool(scope, result, location);
    } else if(is_int_n(fKind) && is_int_n(sKind)) {
        // both values are int num values
        const auto first = (IntNumValue*) fEvl;
        const auto second = (IntNumValue*) sEvl;
        const auto answer = operate(operation, first->get_num_value(), second->get_num_value());
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
        // Skip the return value (it's been moved to the caller)
        if(val == returnValue) continue;

        // Only struct values can have destructors
        if(val->val_kind() == ValueKind::StructValue) {
            auto structVal = val->as_struct_value_unsafe();
            // Use linked_extendable() and verify it's actually a StructDecl
            // before casting to StructDefinition* (prevents UB on unions/variants)
            auto ext = structVal->linked_extendable();
            if(ext && ext->kind() == ASTNodeKind::StructDecl) {
                auto structDef = (StructDefinition*) ext;
                if(structDef->has_destructor()) {
                    auto destructor_fn = structDef->destructor_func();
                    if(destructor_fn && destructor_fn->body.has_value()) {
                        // Temporarily override the current function type so that
                        // return statements inside the destructor body work correctly
                        const auto prev_func = global->current_func_type;
                        global->current_func_type = destructor_fn;

                        // Create a temporary scope to interpret the destructor body
                        InterpretScope child_scope(global, allocator, global);
                        child_scope.declare("self", val);
                        child_scope.interpret(&destructor_fn->body.value());

                        // Remove self from child scope to prevent recursive destruction
                        auto self_it = child_scope.values.find("self");
                        if(self_it != child_scope.values.end()) {
                            child_scope.values.erase(self_it);
                        }

                        global->current_func_type = prev_func;
                    }
                }
            }
        }
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