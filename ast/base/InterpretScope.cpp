// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "InterpretScope.h"
#include "GlobalInterpretScope.h"
#include "Value.h"
#include <iostream>
#include "ast/structures/Scope.h"
#include "ast/values/BoolValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/PointerValue.h"

#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_RESET   "\x1b[0m"

ASTAny* InterpretScope::allocate_released(std::size_t obj_size, std::size_t alignment) {
    const auto ptr = (ASTAny*) (void*) allocator.allocate_released_size(obj_size, alignment);
    allocated.emplace_back(ptr);
    return ptr;
}

void InterpretScope::declare(std::string &name, Value *value) {
    values[name] = value;
}

void InterpretScope::declare(const chem::string_view& name, Value* value) {
    values[name.str()] = value;
}

Value *InterpretScope::find_value(const std::string &name) {
    auto found = values.find(name);
    if (found == values.end()) {
        if(parent == nullptr) return nullptr;
        return parent->find_value(name);
    } else {
        return found->second;
    }
}

std::pair<value_iterator, InterpretScope&> InterpretScope::find_value_iterator(const std::string &name) {
    auto found = values.find(name);
    if (found == values.end()) {
        if(parent == nullptr) return { values.end(), *this };
        return parent->find_value_iterator(name);
    } else {
        return { found, *this };
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
            throw std::runtime_error("unknown operation between bool values");
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
            throw std::runtime_error("UNKNOWN INTERPRET OPERATION");
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
            throw std::runtime_error("UNKNOWN INTERPRET OPERATION");
#endif
            return 0;
    }
}

ValueKind determine_output(Operation op, ValueKind first, ValueKind second) {
    switch(op) {
        case Operation::IsEqual:
        case Operation::IsNotEqual:
        case Operation::GreaterThan:
        case Operation::GreaterThanOrEqual:
        case Operation::LessThan:
        case Operation::LessThanOrEqual:
            return ValueKind::Bool;
        default:
            return first > second ? first : second;
    }
}

inline bool is_int_n(ValueKind k) {
    return k >= ValueKind::IntNStart && k <= ValueKind::IntNEnd;
}

inline BoolValue* pack_bool(InterpretScope& scope, bool value, SourceLocation location) {
    return new (scope.allocate<BoolValue>()) BoolValue(value, location);
}

Value* InterpretScope::evaluate(Operation operation, Value* fEvl, Value* sEvl, SourceLocation location, Value* debugValue) {
    auto& scope = *this;
    const auto fKind = fEvl->val_kind();
    const auto sKind = sEvl->val_kind();
    if(fKind == ValueKind::Bool && sKind == ValueKind::Bool) {
        const auto result = operate(operation, fEvl->get_the_bool(), sEvl->get_the_bool());
        return new (scope.allocate<BoolValue>()) BoolValue(result, location);
    } else if(is_int_n(fKind) && is_int_n(sKind)) {
        // both values are int num values
        const auto first = (IntNumValue*) fEvl;
        const auto second = (IntNumValue*) sEvl;
        const auto answer = operate(operation, first->get_num_value(), second->get_num_value());
        return pack_by_kind(scope, determine_output(operation, fKind, sKind), answer, location);
    } else if(fKind == ValueKind::Double || fKind == ValueKind::Float || sKind == ValueKind::Double || sKind == ValueKind::Float) {
        const auto first = get_double_value(fEvl, fKind);
        const auto second = get_double_value(sEvl, sKind);
        const auto answer = operate(operation, first, second);
        return pack_by_kind(scope, determine_output(operation, fKind, sKind), answer, location);
    } else if(fKind == ValueKind::NullValue || sKind == ValueKind::NullValue) {
        // comparison with null, a == null or null == a
        switch (operation) {
            case Operation::IsEqual:
                return pack_bool(scope, fKind == ValueKind::NullValue && sKind == ValueKind::NullValue, location);
            case Operation::IsNotEqual:
                return pack_bool(scope, fKind != sKind, location);
                break;
            default:
                return new (scope.allocate<NullValue>()) NullValue(location);
        }
    } else if((fKind == ValueKind::String && is_int_n(sKind))) {
        const auto strVal = fEvl->as_string_unsafe();
        const auto numVal = (IntNumValue*) sEvl;
        const auto num = numVal->get_num_value();
        return new (scope.allocate<StringValue>()) StringValue(chem::string_view(strVal->value.data() + num, strVal->value.size() - num), location);
    } else if ((sKind == ValueKind::String && is_int_n(fKind))) {
        const auto strVal = sEvl->as_string_unsafe();
        const auto numVal = (IntNumValue*) fEvl;
        const auto num = numVal->get_num_value();
        return new (scope.allocate<StringValue>()) StringValue(chem::string_view(strVal->value.data() + num, strVal->value.size() - num), location);
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
    } else {
#ifdef DEBUG
        throw std::runtime_error("OPERATION BETWEEN VALUES OF UNKNOWN KIND");
#endif
        scope.error("Operation between values of unknown kind", debugValue);
        return nullptr;
    }
}

void InterpretScope::erase_value(const std::string &name) {
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

void InterpretScope::error(std::string& err, ASTAny* any) {
    switch(any->any_kind()) {
        case ASTAnyKind::Value:
            global->interpret_error(err, (Value*) any);
            break;
        case ASTAnyKind::Node:
            global->interpret_error(err, (ASTNode*) any);
            break;
        case ASTAnyKind::Type:
            global->interpret_error(err, (BaseType*) any);
            break;
    }
}

void InterpretScope::error(std::string_view err, ASTAny* any) {
    switch(any->any_kind()) {
        case ASTAnyKind::Value:
            global->interpret_error(err, (Value*) any);
            break;
        case ASTAnyKind::Node:
            global->interpret_error(err, (ASTNode*) any);
            break;
        case ASTAnyKind::Type:
            global->interpret_error(err, (BaseType*) any);
            break;
    }
}

bool InterpretScope::isTarget64Bit() {
    return global->target_data.is_64Bit;
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

// a dummy value, which will call a lambda upon destruction
class DestructValue : public Value {
public:

    void* data;
    void(*destruct)(void* data);

    DestructValue(
        void* data,
        void(*destruct)(void* data)
    ) : Value(ValueKind::DestructValue, ZERO_LOC), data(data), destruct(destruct) {

    }

    void accept(Visitor *visitor) override {
        // cannot be visited
    }
    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<DestructValue>()) DestructValue(data, destruct);
    }
    ~DestructValue() {
        destruct(data);
    }
};

void InterpretScope::add_destructor(void* data, void(*destruct)(void* data)) {
    new (allocate<DestructValue>()) DestructValue(data, destruct);
}

InterpretScope::~InterpretScope() {
    for(auto ptr : allocated) {
        ptr->~ASTAny();
    }
}