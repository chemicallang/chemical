// Copyright (c) Qinetik 2024.

#include "BaseFunctionType.h"
#include "ast/base/BaseType.h"
#include "FunctionParam.h"

BaseFunctionType::BaseFunctionType(
    std::vector<std::unique_ptr<FunctionParam>> params,
    std::unique_ptr<BaseType> returnType,
    bool isVariadic
) : params(std::move(params)), returnType(std::move(returnType)), isVariadic(isVariadic) {

}

void BaseFunctionType::assign_params() {
    for(auto& param : params) {
        param->func_type = this;
    }
}

BaseFunctionParam* BaseFunctionType::get_self_params() {
    if(!params.empty()) {
        auto& param = params[0];
        if(param->name == "this" || param->name == "self") {
            return param.get();
        }
    }
    return nullptr;
}

unsigned BaseFunctionType::c_or_llvm_arg_start_index() const {
    return (returnType->value_type() == ValueType::Struct ? 1 : 0); // + (has_self_param() ? 1 : 0);
}

bool BaseFunctionType::equal(BaseFunctionType *other) const {
    if (isVariadic != other->isVariadic) {
        return false;
    }
    if (!returnType->is_same(other->returnType.get())) {
        return false;
    }
    unsigned i = 0;
    while (i < params.size()) {
        if (!params[i]->type->is_same(other->params[i]->type.get())) {
            return false;
        }
        i++;
    }
    return true;
}