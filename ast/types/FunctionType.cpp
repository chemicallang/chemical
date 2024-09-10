// Copyright (c) Qinetik 2024.

#include "FunctionType.h"
#include "ast/base/BaseType.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/FunctionParam.h"
#include "compiler/ASTDiagnoser.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Type* llvm_func_return(Codegen &gen, BaseType* type) {
    if(type->value_type() == ValueType::Struct) {
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
//    if(type->function_type() != nullptr) {
//        auto func_type = type->function_type();
//        // when a capturing lambda is a parameter, it is treated as two pointer parameters one for the lambda and another for it's data
//        if(func_type->isCapturing) {
//            paramTypes.emplace_back(gen.builder->getPtrTy());
//        }
//    }
}

void llvm_func_param_types_into(
        Codegen &gen,
        std::vector<llvm::Type*>& paramTypes,
        std::vector<std::unique_ptr<FunctionParam>>& params,
        BaseType* returnType,
        bool isCapturing,
        bool isVariadic
) {
    // functions that return struct take a pointer to struct and actually return void
    // so allocation takes place outside function
    if(returnType->value_type() == ValueType::Struct) {
        paramTypes.emplace_back(gen.builder->getPtrTy());
    }
    // capturing lambdas gets a struct passed to them which contain captured data
    if(isCapturing) {
        paramTypes.emplace_back(gen.builder->getPtrTy());
    }
    auto size = isVariadic ? (params.size() - 1) : params.size();
    unsigned i = 0;
    while (i < size) {
        llvm_func_param_type(gen, paramTypes, params[i]->type.get());
        i++;
    }
}

std::vector<llvm::Type *> FunctionType::param_types(Codegen &gen) {
    return llvm_func_param_types(gen, params, returnType.get(), isCapturing, isVariadic);
}

llvm::FunctionType *FunctionType::llvm_func_type(Codegen &gen) {
    return llvm::FunctionType::get(llvm_func_return(gen, returnType.get()), llvm_func_param_types(gen, params, returnType.get(), isCapturing, isVariadic), isVariadic);
}

llvm::Type *FunctionType::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
};

#endif

FunctionType::FunctionType(
    std::vector<std::unique_ptr<FunctionParam>> params,
    std::unique_ptr<BaseType> returnType,
    bool isVariadic,
    bool isCapturing,
    CSTToken* token
) : params(std::move(params)), returnType(std::move(returnType)), isVariadic(isVariadic), isCapturing(isCapturing), TokenizedBaseType(token) {

}

bool FunctionType::isInVarArgs(unsigned index) const {
    return isVariadic && index >= (params.size() - 1);
}

uint64_t FunctionType::byte_size(bool is64Bit) {
    if(is_capturing()) {
        return is64Bit ? 16 : 8;
    } else {
        return is64Bit ? 8 : 4;
    }
}

unsigned FunctionType::explicit_func_arg_offset() {
    return has_self_param() ? 1 : 0;
}

FunctionParam* FunctionType::func_param_for_arg_at(unsigned index) {
    if(params.empty()) return nullptr;
    const auto offset = explicit_func_arg_offset(); // first argument for implicit self
    if(isVariadic && index >= (params.size() - 1 - offset)) {
        return params.back().get();
    }
    return params[index + offset].get();
}

bool FunctionType::satisfy_args(std::vector<std::unique_ptr<Value>>& forArgs) {
    auto has_self = has_self_param();
    unsigned offset = has_self ? 1 : 0;
    auto required_args_len = params.size() - offset;
    if(forArgs.size() != required_args_len) {
        return false;
    }
    unsigned i = offset; // first argument for implicit self
    while(i < params.size()) {
        if(!params[i]->type->satisfies(forArgs[i - offset].get())) {
            return false;
        }
        i++;
    }
    return true;
}

bool FunctionType::do_param_types_match(std::vector<std::unique_ptr<FunctionParam>>& param_types, bool check_self) {
    if(params.size() != param_types.size()) return false;
    unsigned i = check_self ? 0 : (has_self_param() ? 1 : 0);
    const auto siz = params.size();
    while(i < siz) {
        if(!param_types[i]->type->is_same(params[i]->type.get())) {
            return false;
        }
        i++;
    }
    return true;
}

void FunctionType::assign_params() {
    for(auto& param : params) {
        param->func_type = this;
    }
}

BaseFunctionParam* FunctionType::get_self_param() {
    if(!params.empty()) {
        auto& param = params[0];
        if(param->name == "this" || param->name == "self") {
            return param.get();
        }
    }
    return nullptr;
}

unsigned FunctionType::c_or_llvm_arg_start_index() const {
    return (returnType->value_type() == ValueType::Struct ? 1 : 0); // + (has_self_param() ? 1 : 0);
}

bool FunctionType::equal(FunctionType *other) const {
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

bool FunctionType::satisfies(ValueType type) {
    return type == ValueType::Lambda;
}

FunctionType *FunctionType::copy() const {
    std::vector<std::unique_ptr<FunctionParam>> copied;
    for (auto &param: params) {
        copied.emplace_back(param->copy());
    }
    return new FunctionType(std::move(copied), std::unique_ptr<BaseType>(returnType->copy()), isVariadic, isCapturing, token);
}

void FunctionType::link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) {
    for (auto &param: params) {
        param->type->link(linker, param->type);
    }
    returnType->link(linker, returnType);
}

bool FunctionType::is_one_of_moved_chains(AccessChain* chain) {
    for(auto& moved : moved_chains) {
        if(moved == chain) {
            return true;
        }
    }
    return false;
}

bool FunctionType::is_one_of_moved_id(VariableIdentifier* id) {
    for(auto& moved : moved_identifiers) {
        if(moved == id) {
            return true;
        }
    }
    return false;
}

VariableIdentifier* FunctionType::find_moved_id(VariableIdentifier* id) {
    for(auto& moved : moved_identifiers) {
        if(moved->linked == id->linked) {
            return moved;
        }
    }
    return nullptr;
}

// given x.y and moved x.y.z match
// given x.y and moved x.a don't match
// given x.y.z and moved x match
// given x and moved x.y.z match
// given x and moved z.x don't
// given x.y.z and moved x.y.a don't
// when finding x.y, when has moved x.y.z, x.y then x.y is returned or x if present, the smallest chain
// that matches is returned
AccessChain* FunctionType::find_partially_matching_moved_chain(AccessChain& chain, ValueKind first_value_kind) {
    auto first_value = chain.values[0].get();
    AccessChain* smallest = nullptr;
    for(auto& moved_chain_ptr : moved_chains) {
        auto& moved_chain = *moved_chain_ptr;
        const auto moved_size = moved_chain.values.size();
        // since finding the smallest moved chain that matches with the given chain
        if(smallest && smallest->values.size() < moved_size) {
            continue;
        }
        auto& moved_chain_first = moved_chain.values[0];
        if(first_value->is_equal(moved_chain_first.get(), first_value_kind, moved_chain_first->val_kind())) {
            const auto given_size = chain.values.size();
            auto matching = true;
            const auto less_size = std::min(moved_size, given_size);
            unsigned i = 1; // zero has already been checked
            while(i < less_size) {
                if(!moved_chain.values[i]->is_equal(chain.values[i].get())) {
                    matching = false;
                    break;
                }
                i++;
            }
            if(matching) {
                smallest = moved_chain_ptr;
            }
        }
    }
    return smallest;
}

ChainValue* FunctionType::find_moved_chain_value(VariableIdentifier* id) {
    auto found = find_moved_id(id);
    if(found) return found;
    AccessChain* smallest = nullptr;
    for(auto& chain : moved_chains) {
        auto& moved_chain = *chain;
        if(smallest && smallest->values.size() < moved_chain.values.size()) {
            continue;
        }
        auto& other_first = *moved_chain.values[0];
        if(id->is_equal(&other_first, ValueKind::Identifier, other_first.val_kind())) {
            smallest = chain;
        }
    }
    return smallest;
}

ChainValue* FunctionType::find_moved_chain_value(AccessChain* chain_ptr) {
    auto& chain = *chain_ptr;
    auto& first_value = *chain.values[0];
    auto first_value_kind = first_value.val_kind();
    if(first_value_kind == ValueKind::Identifier) {
        if(chain.values.size() == 1) {
            return find_moved_chain_value(first_value.as_identifier());
        } else {
            auto found = find_moved_id(first_value.as_identifier());
            if(found) return found;
        }
    }
    return find_partially_matching_moved_chain(chain, first_value_kind);
}

void FunctionType::mark_moved_no_check(AccessChain* chain) {
    if(chain->values.size() == 1 && chain->values[0]->val_kind() == ValueKind::Identifier) {
        moved_identifiers.emplace_back(chain->values[0]->as_identifier());
    } else {
        moved_chains.emplace_back(chain);
    }
}

void FunctionType::mark_moved_no_check(VariableIdentifier* id) {
    moved_identifiers.emplace_back(id);
}

bool FunctionType::move_value(Value* value, ASTDiagnoser& diagnoser) {
    const auto chain = value->as_access_chain();
    if(chain) {
        const auto moved = find_moved_chain_value(chain);
        if(moved) {
            diagnoser.error("cannot move chain '" + chain->chain_representation() + "' as another one of it's chain '" + moved->representation() + "' has been moved" , (ASTNode*) chain);
            return false;
        }
        mark_moved_no_check(chain);
        return true;
    } else {
        const auto id = value->as_identifier();
        if(id) {
            const auto moved = find_moved_chain_value(id);
            if(moved) {
                diagnoser.error("cannot move id '" + id->representation() + "' as chain '" + moved->representation() + "' has been moved" , id);
                return false;
            }
            const auto linked = value->linked_node();
            const auto linked_kind = linked->kind();
            if (linked_kind == ASTNodeKind::VarInitStmt) {
                const auto init = linked->as_var_init_unsafe();
#ifdef DEBUG
                if(init->get_has_moved()) {
                    diagnoser.error("found var init that skipped move check, identifier '" + init->identifier + "' has already been moved", id);
                    return false;
                }
#endif
                init->moved();
            } else if(linked_kind == ASTNodeKind::FunctionParam) {
                const auto param = linked->as_func_param_unsafe();
#ifdef DEBUG
                if(param->get_has_moved()) {
                    diagnoser.error("found function param that skipped move check, identifier '" + param->name + "' has already been moved", id);
                    return false;
                }
#endif
                param->moved();
            }
            mark_moved_no_check(id);
            return true;
        }
    }
    return false;
}