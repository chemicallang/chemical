// Copyright (c) Qinetik 2024.

#include "AddrOfValue.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Type *AddrOfValue::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *AddrOfValue::llvm_value(Codegen &gen) {
    return value->llvm_pointer(gen);
}

#endif


AddrOfValue::AddrOfValue(
        std::unique_ptr<Value> value
) : value(std::move(value)) {

}

void AddrOfValue::link(SymbolResolver &linker) {
    value->link(linker);
}

Value *AddrOfValue::copy() {
    return new AddrOfValue(
            std::unique_ptr<Value>(value->copy())
    );
}

std::string AddrOfValue::representation() const {
    return '&' + value->representation();
}