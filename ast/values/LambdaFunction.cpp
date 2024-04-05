// Copyright (c) Qinetik 2024.

#include "LambdaFunction.h"
#include "ast/structures/FunctionDeclaration.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *LambdaFunction::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *LambdaFunction::llvm_value(Codegen &gen) {
    gen.error("LAMBDA NOT DONE !");
    return nullptr;
}

#endif

LambdaFunction::LambdaFunction(std::unique_ptr<FunctionDeclaration> decl) : decl(std::move(decl)) {

}

std::string LambdaFunction::representation() const {
    return "() => {};";
}

ValueType LambdaFunction::value_type() const {
    return ValueType::Lambda;
}