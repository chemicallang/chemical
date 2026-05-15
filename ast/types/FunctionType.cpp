// Copyright (c) Chemical Language Foundation 2025.

#include "FunctionType.h"
#include "ast/base/BaseType.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/types/VoidType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "compiler/ASTDiagnoser.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Type* llvm_func_return(Codegen &gen, BaseType* type) {
    if(type->isStructLikeType()) {
        return gen.builder->getVoidTy();
    } else{
        return type->llvm_type(gen);
    }
}

void llvm_func_param_type(
        Codegen &gen,
        std::vector<llvm::Type*>& paramTypes,
        BaseType* type
) {
    paramTypes.emplace_back(type->llvm_param_type(gen));
}

void llvm_func_param_types_into(
        Codegen &gen,
        std::vector<llvm::Type*>& paramTypes,
        std::vector<FunctionParam*>& params,
        BaseType* returnType,
        bool isCapturing,
        bool isVariadic,
        FunctionDeclaration* decl
) {
    // functions that return struct take a pointer to struct and actually return void
    // so allocation takes place outside function
    if(returnType->isStructLikeType()) {
        paramTypes.emplace_back(gen.builder->getPtrTy());
    }
    // capturing lambdas gets a struct passed to them which contain captured data
    if(isCapturing) {
        paramTypes.emplace_back(gen.builder->getPtrTy());
    }
    auto size = isVariadic ? (params.size() - 1) : params.size();
    unsigned i = 0;
    while (i < size) {
        llvm_func_param_type(gen, paramTypes, params[i]->type);
        i++;
    }
}

std::vector<llvm::Type *> FunctionType::param_types(Codegen &gen, bool capturing) {
    return llvm_func_param_types(gen, params, returnType, capturing, isVariadic(), as_function());
}

llvm::FunctionType *FunctionType::llvm_func_type(Codegen &gen, bool capturing) {
    return llvm::FunctionType::get(llvm_func_return(gen, returnType), param_types(gen, capturing), isVariadic());
}

llvm::Type *FunctionType::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
};

#endif

bool CapturingFunctionType::satisfies(BaseType *type) {
    switch(type->kind()) {
        case BaseTypeKind::Function:
            return func_type->satisfies(type->as_function_type_unsafe());
        case BaseTypeKind::CapturingFunction:
            return func_type->satisfies(type->as_capturing_func_type_unsafe()->func_type);
        default:
            return false;
    }
}

bool FunctionType::satisfies(BaseType *type) {
    switch(type->kind()) {
        case BaseTypeKind::NullPtr:
            return true;
        case BaseTypeKind::Function: {
            const auto func = type->as_function_type_unsafe();
            return equal_type(func);
        }
        default:
            return false;
    }
}

bool FunctionType::isInVarArgs(unsigned index) const {
    return isVariadic() && index >= (params.size() - 1);
}

uint64_t FunctionType::byte_size(TargetData& target) {
    if(isCapturing()) {
        return target.is64Bit ? 16 : 8;
    } else {
        return target.is64Bit ? 8 : 4;
    }
}
unsigned FunctionType::total_implicit_params() {
    unsigned i = 0;
    for(auto& param : params) {
        if(param->is_implicit()) {
            i++;
        } else {
            break;
        }
    }
    return i;
}

unsigned int FunctionType::expectedArgsSize() {
    return params.size() - total_implicit_params() - (isExtensionFn() ? 1 : 0) - (isVariadic() ? 1 : 0);
}

FunctionParam* FunctionType::func_param_for_arg_at(unsigned index) {
    if(params.empty()) return nullptr;
    const auto offset = explicit_func_arg_offset(); // first argument for implicit self
    if(isVariadic() && index >= (params.size() - 1 - offset)) {
        return params.back();
    }
    const auto expected = index + offset;
    if(expected < params.size()) {
        return params[expected];
    } else {
        return nullptr;
    }
}

FunctionParam* FunctionType::implicit_param_for(const chem::string_view& name) {
    for(auto param : params) {
        if(param->is_implicit() && param->name == name) {
            return param;
        } else {
            break;
        }
    }
    return nullptr;
}

bool FunctionType::satisfy_args(std::vector<Value*>& forArgs) {
    auto has_self = has_self_param();
    unsigned offset = has_self ? 1 : 0;
    auto required_args_len = params.size() - offset;
    if(forArgs.size() != required_args_len) {
        return false;
    }
    unsigned i = offset; // first argument for implicit self
    while(i < params.size()) {
        if(!params[i]->type->satisfies(forArgs[i - offset], false)) {
            return false;
        }
        i++;
    }
    return true;
}

bool FunctionType::do_param_types_match(std::vector<FunctionParam*>& param_types, bool check_self) {
    if(params.size() != param_types.size()) return false;
    unsigned i = check_self ? 0 : (has_self_param() ? 1 : 0);
    const auto siz = params.size();
    while(i < siz) {
        if(!param_types[i]->type->is_same(params[i]->type)) {
            return false;
        }
        i++;
    }
    return true;
}

FunctionParam* FunctionType::get_self_param() {
    if(!params.empty()) {
        auto& param = params[0];
        if(isExtensionFn()) {
            // in extension functions, the first parameter is the receiver
            return param;
        }
        if(param->is_implicit() && (param->name == "self" || param->name == "this")) {
            return param;
        }
    }
    return nullptr;
}

bool FunctionType::has_explicit_params() {
    for(auto& param : params) {
        if(!param->is_implicit()) return true;
    }
    return false;
}

unsigned FunctionType::c_or_llvm_arg_start_index() {
    const auto offset = isExtensionFn() ? 1 : 0;
    if(returnType->isStructLikeType()) {
        return 1 + offset;
    } else {
        return 0 + offset;
    }
}

bool FunctionType::equal(FunctionType *other) const {
    if (isVariadic() != other->isVariadic()) {
        return false;
    }
    // return type can be nullptr
    // because sometimes user doesn't give return type for lambda
    // we can't set it to void (because we have to determine it by traversing ast)
    if (returnType == nullptr) {
        if (other->returnType == nullptr || other->returnType->canonical()->kind() != BaseTypeKind::Void) {
            return false;
        }
    } else {
        if (other->returnType == nullptr) {
            if (returnType->canonical()->kind() != BaseTypeKind::Void) {
                return false;
            }
        } else {
            if (!returnType->is_same(other->returnType)) {
                return false;
            }
        }
    }
    if(params.size() != other->params.size()) {
        return false;
    }
    unsigned i = 0;
    while (i < params.size()) {
        if (!params[i]->type->is_same(other->params[i]->type)) {
            return false;
        }
        i++;
    }
    return true;
}

FunctionType *FunctionType::copy(ASTAllocator& allocator) {
    const auto func_type = new (allocator.allocate<FunctionType>()) FunctionType(returnType.copy(allocator), isExtensionFn(), isVariadic(), isCapturing(), data.signature_resolved);
    for (auto &param: params) {
        func_type->params.emplace_back(param->copy(allocator));
    }
    return func_type;
}