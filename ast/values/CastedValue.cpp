// Copyright (c) Qinetik 2024.

#include "CastedValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *CastedValue::llvm_type(Codegen &gen) {
    gen.error("CASTING NOT YET SUPPORTED");
    return value->llvm_type(gen);
}

llvm::Value *CastedValue::llvm_value(Codegen &gen) {
    gen.error("CASTING NOT YET SUPPORTED");
    return value->llvm_value(gen);
}

#endif


CastedValue::CastedValue(
        std::unique_ptr<Value> value,
        std::unique_ptr<BaseType> type
) : value(std::move(value)), type(std::move(type)) {

}

Value *CastedValue::copy(InterpretScope &scope) {
    return new CastedValue(
            std::unique_ptr<Value>(value->copy(scope)),
            std::unique_ptr<BaseType>(type->copy())
    );
}

std::string CastedValue::representation() const {
    return value->representation() + " as " + type->representation();
}