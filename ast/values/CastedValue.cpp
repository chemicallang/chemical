// Copyright (c) Qinetik 2024.

#include "CastedValue.h"


CastedValue::CastedValue(
        std::unique_ptr<Value> value,
        std::unique_ptr<BaseType> type
) : value(std::move(value)), type(std::move(type)) {

}

Value *CastedValue::copy() {
    return new CastedValue(
            std::unique_ptr<Value>(value->copy()),
            std::unique_ptr<BaseType>(type->copy())
    );
}

std::string CastedValue::representation() const {
    return value->representation() + " as " + type->representation();
}