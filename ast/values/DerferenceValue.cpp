// Copyright (c) Qinetik 2024.

#include "DereferenceValue.h"

DereferenceValue::DereferenceValue(
        std::unique_ptr<Value> value
) : value(std::move(value)) {

}

void DereferenceValue::link(SymbolResolver &linker) {
    value->link(linker);
}

std::unique_ptr<BaseType> DereferenceValue::create_type() const {
    auto addr = value->create_type();
    if(addr->kind() == BaseTypeKind::Pointer) {
        return std::unique_ptr<BaseType>(((PointerType*) (addr.get()))->type->copy());
    } else {
        // TODO cannot report error here, the type cannot be created because the linked type is not a pointer
        return nullptr;
    }
}

Value *DereferenceValue::copy() {
    return new DereferenceValue(
            std::unique_ptr<Value>(value->copy())
    );
}