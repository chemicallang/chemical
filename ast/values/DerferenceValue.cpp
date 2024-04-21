// Copyright (c) Qinetik 2024.

#include "DereferenceValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *DereferenceValue::llvm_type(Codegen &gen) {
    auto addr = value->create_type();
    if(addr->kind() == BaseTypeKind::Pointer) {
        return ((PointerType*) (addr.get()))->type->llvm_type(gen);
    } else {
        gen.error("Derefencing a value that is not a pointer " + value->representation());
        return nullptr;
    }
}

llvm::Value *DereferenceValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateLoad(llvm_type(gen), value->llvm_value(gen), "deref");
}

#endif

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

std::string DereferenceValue::representation() const {
    return '*' + value->representation();
}