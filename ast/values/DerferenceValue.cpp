// Copyright (c) Qinetik 2024.

#include "DereferenceValue.h"

DereferenceValue::DereferenceValue(
        std::unique_ptr<Value> value
) : value(std::move(value)) {

}

void DereferenceValue::link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) {
    value->link(linker, value);
}

std::unique_ptr<BaseType> DereferenceValue::create_type() {
    auto addr = value->create_type();
    if(addr->kind() == BaseTypeKind::Pointer) {
        return std::unique_ptr<BaseType>(((PointerType*) (addr.get()))->type->copy());
    } else {
        // TODO cannot report error here, the type cannot be created because the linked type is not a pointer
        return nullptr;
    }
}

BaseType *DereferenceValue::get_base_type_ref() {
    auto addr = value->get_base_type();
    if(addr->kind() == BaseTypeKind::Pointer) {
        return ((PointerType*) (addr.get()))->type.get();
    } else {
        // TODO cannot report error here, the type cannot be created because the linked type is not a pointer
        return nullptr;
    }
}

hybrid_ptr<BaseType> DereferenceValue::get_base_type() {
    return hybrid_ptr<BaseType> { get_base_type_ref(), false };
}

Value *DereferenceValue::copy() {
    return new DereferenceValue(
            std::unique_ptr<Value>(value->copy())
    );
}