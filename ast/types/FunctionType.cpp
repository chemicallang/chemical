// Copyright (c) Qinetik 2024.

#include "FunctionType.h"
#include "ast/base/BaseType.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
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
    if(returnType->value_type() == ValueType::Struct) {
        if(!decl || (!decl->has_annotation(AnnotationKind::Copy) && !decl->has_annotation(AnnotationKind::Move))) {
            paramTypes.emplace_back(gen.builder->getPtrTy());
        }
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

std::vector<llvm::Type *> FunctionType::param_types(Codegen &gen) {
    return llvm_func_param_types(gen, params, returnType, isCapturing, isVariadic, as_function());
}

llvm::FunctionType *FunctionType::llvm_func_type(Codegen &gen) {
    return llvm::FunctionType::get(llvm_func_return(gen, returnType), param_types(gen), isVariadic);
}

llvm::Type *FunctionType::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
};

#endif

FunctionType::FunctionType(
    std::vector<FunctionParam*> params,
    BaseType* returnType,
    bool isVariadic,
    bool isCapturing,
    ASTNode* parent_node,
    CSTToken* token
) : params(std::move(params)), returnType(returnType), isVariadic(isVariadic), isCapturing(isCapturing),
    parent_node(parent_node), TokenizedBaseType(token) {

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
unsigned FunctionType::total_implicit_params() {
    unsigned i = 0;
    for(auto& param : params) {
        if(param->is_implicit) {
            i++;
        } else {
            break;
        }
    }
    return i;
}

unsigned int FunctionType::expectedArgsSize() {
    return params.size() - total_implicit_params();
}

FunctionParam* FunctionType::func_param_for_arg_at(unsigned index) {
    if(params.empty()) return nullptr;
    const auto offset = explicit_func_arg_offset(); // first argument for implicit self
    if(isVariadic && index >= (params.size() - 1 - offset)) {
        return params.back();
    }
    return params[index + offset];
}

FunctionParam* FunctionType::implicit_param_for(const std::string& name) {
    for(auto param : params) {
        if(param->is_implicit && param->name == name) {
            return param;
        } else {
            break;
        }
    }
    return nullptr;
}

bool FunctionType::satisfy_args(ASTAllocator& allocator, std::vector<Value*>& forArgs) {
    auto has_self = has_self_param();
    unsigned offset = has_self ? 1 : 0;
    auto required_args_len = params.size() - offset;
    if(forArgs.size() != required_args_len) {
        return false;
    }
    unsigned i = offset; // first argument for implicit self
    while(i < params.size()) {
        if(!params[i]->type->satisfies(allocator, forArgs[i - offset])) {
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

void FunctionType::assign_params() {
    for(auto& param : params) {
        param->func_type = this;
    }
}

BaseFunctionParam* FunctionType::get_self_param() {
    if(!params.empty()) {
        auto& param = params[0];
        if(param->name == "this" || param->name == "self") {
            return param;
        }
    }
    return nullptr;
}

unsigned FunctionType::c_or_llvm_arg_start_index() {
    const auto is_struct_return = returnType->value_type() == ValueType::Struct;
    if(is_struct_return) {
        auto func = as_function();
        if(func && (func->has_annotation(AnnotationKind::Copy) || func->has_annotation(AnnotationKind::Move))) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return 0;
    }
}

bool FunctionType::equal(FunctionType *other) const {
    if (isVariadic != other->isVariadic) {
        return false;
    }
    if (!returnType->is_same(other->returnType)) {
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

bool FunctionType::satisfies(ValueType type) {
    return type == ValueType::Lambda;
}

FunctionType *FunctionType::copy(ASTAllocator& allocator) const {
    std::vector<FunctionParam*> copied;
    for (auto &param: params) {
        copied.emplace_back(param->copy(allocator));
    }
    return new (allocator.allocate<FunctionType>()) FunctionType(std::move(copied), returnType->copy(allocator), isVariadic, isCapturing, parent_node, token);
}

void FunctionType::link(SymbolResolver &linker) {
    for (auto &param: params) {
        param->link_param_type(linker);
    }
    returnType->link(linker);
}

// first chain is contained in other chain
// other chain's value size is bigger or equal to first chain
bool first_chain_is_contained_in(AccessChain& first, AccessChain& other_ptr) {
    unsigned j = 0;
    for(auto& value_ptr : first.values) {
        auto& value = *value_ptr;
        auto& other = *other_ptr.values[j];
        if(!value.is_equal(&other)) {
            return false;
        }
        j++;
    }
    return true;
}

bool FunctionType::un_move_chain(AccessChain* chain_ptr) {
    if(moved_chains.empty()) return false;
    auto& chain = *chain_ptr;
    if(chain.values.size() == 1) {
        auto id = chain.values[0]->as_identifier();
        if(id && un_move_exact_id(id)) {
            return true;
        }
    }
    for(auto it = moved_chains.begin(); it != moved_chains.end(); ++it) {
        auto& moved = **it;
        if(moved.values.size() >= chain.values.size() && first_chain_is_contained_in(chain, moved)) {
            // Erase the element at the iterator position
            it = moved_chains.erase(it);
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

bool FunctionType::un_move_exact_id(VariableIdentifier* id) {
    if(moved_identifiers.empty()) return false;
    unsigned i = 0;
    for(auto& moved : moved_identifiers) {
        if(moved->linked == id->linked) {
            moved_identifiers.erase(moved_identifiers.begin() + i);
            return true;
        }
        i++;
    }
    return false;
}

bool FunctionType::un_move_chain_with_first_id(VariableIdentifier* id) {
    unsigned i = 0;
    for(auto& moved : moved_chains) {
        auto& chain = *moved;
        auto& first_value = *chain.values[0];
        auto moved_id = first_value.as_identifier();
        if(moved_id && moved_id->linked == id->linked) {
            moved_chains.erase(moved_chains.begin() + i);
            return true;
        }
        i++;
    }
    return false;
}

bool FunctionType::un_move_id(VariableIdentifier* id) {
    return un_move_exact_id(id) || un_move_chain_with_first_id(id);
}

// for example when consider_nested_members and consider_last_member are true:
// for given 'm' if only 'm.x' has been moved, we return it (nested members considered)
// for given 'm.x' if only 'm' has been moved, we return it (parent member considered)
// for given 'm.x' if only 'm.y' has been moved, we return null (unrelated not considered)
// for given 'm.x' if only 'm.x' has been moved, we return it (last member considered)
//
// for example when consider_nested_members and consider_last_member are false:
// for given 'm.x' if only 'm.x.y' has been moved, we return null (nested members not considered)
// for given 'm.x' if only 'm' has been moved, we return it (parent member being considered)
// for given 'm.x' if only 'm.y' has been moved, we return null (unrelated nor considered)
// for given 'm.x' if only 'm.x' has been moved, we return null (last member not considered)
AccessChain* FunctionType::find_partially_matching_moved_chain(AccessChain& chain, bool consider_nested_members, bool consider_last_member) {
    auto first_value = chain.values[0];
    const auto first_value_kind = first_value->val_kind();
    AccessChain* smallest = nullptr;
    for(auto& moved_chain_ptr : moved_chains) {
        auto& moved_chain = *moved_chain_ptr;
        const auto moved_size = moved_chain.values.size();
        // since finding the smallest moved chain that matches with the given chain
        if(smallest && smallest->values.size() < moved_size) {
            continue;
        }
        auto& moved_chain_first = moved_chain.values[0];
        if(first_value->is_equal(moved_chain_first, first_value_kind, moved_chain_first->val_kind())) {
            const auto given_size = chain.values.size();
            // check for nested members or not ?
            if(!consider_nested_members && moved_size > given_size) continue;
            auto matching = true;
            const auto less_size = std::min(moved_size, consider_last_member ? given_size : given_size - 1);
            unsigned i = 1; // zero has already been checked
            while(i < less_size) {
                if(!moved_chain.values[i]->is_equal(chain.values[i])) {
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
    const auto first_id = first_value.as_identifier();
    if(first_id) {
        if(chain.values.size() == 1) {
            return find_moved_chain_value(first_id);
        } else {
            auto found = find_moved_id(first_id);
            if(found) return found;
        }
    }
    return find_partially_matching_moved_chain(chain, true, true);
}

void FunctionType::mark_moved_no_check(AccessChain* chain) {
    if(chain->values.size() == 1 && chain->values[0]->val_kind() == ValueKind::Identifier) {
        moved_identifiers.emplace_back(chain->values[0]->as_identifier());
    } else {
        moved_chains.emplace_back(chain);
    }
    chain->is_moved = true;
}

void FunctionType::mark_moved_no_check(VariableIdentifier* id) {
    moved_identifiers.emplace_back(id);
    id->is_moved = true;
}

bool FunctionType::check_chain(AccessChain* chain, bool assigning, ASTDiagnoser& diagnoser) {
    ChainValue* moved;
    if(assigning) {
        moved = find_partially_matching_moved_chain(*chain, false, false);
    } else {
        moved = find_moved_chain_value(chain);
    }
    if(moved) {
        std::string err("cannot ");
        if(assigning) {
            err += "assign";
        } else {
            err += "access";
        }
        err += ' ' + chain->chain_representation() + "' as '" + moved->representation() + "' has been moved";
        diagnoser.error(err, (ASTNode*) chain, (ASTNode*) moved);
        return false;
    }
    return true;
}

bool FunctionType::check_id(VariableIdentifier* id, ASTDiagnoser& diagnoser) {
    const auto moved = find_moved_chain_value(id);
    if(moved) {
        diagnoser.error("cannot access identifier '" + id->representation() + "' as '" + moved->representation() + "' has been moved" , id, moved);
        return false;
    }
    const auto linked = id->linked_node();
    const auto linked_kind = linked->kind();
    if (linked_kind == ASTNodeKind::VarInitStmt) {
        const auto init = linked->as_var_init_unsafe();
#ifdef DEBUG
        if(init->get_has_moved()) {
            diagnoser.error("found var init that skipped move check, identifier '" + init->identifier + "' has already been moved", id);
            return false;
        }
#endif
    } else if(linked_kind == ASTNodeKind::FunctionParam) {
        const auto param = linked->as_func_param_unsafe();
#ifdef DEBUG
        if(param->get_has_moved()) {
            diagnoser.error("found function param that skipped move check, identifier '" + param->name + "' has already been moved", id);
            return false;
        }
#endif
    }
    return true;
}

bool FunctionType::mark_moved_value(Value* value, ASTDiagnoser& diagnoser) {
    const auto chain = value->as_access_chain();
    if(chain) {
        const auto moved = find_moved_chain_value(chain);
        if(moved) {
            diagnoser.error("cannot move '" + chain->chain_representation() + "' as '" + moved->representation() + "' has been moved" , (ASTNode*) chain, (ASTNode*) moved);
            return false;
        }
        mark_moved_no_check(chain);
        return true;
    } else {
        const auto id = value->as_identifier();
        if(id) {
            const auto moved = find_moved_chain_value(id);
            if(moved) {
                diagnoser.error("cannot move '" + id->representation() + "' as '" + moved->representation() + "' has been moved" , id, moved);
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

bool FunctionType::is_value_movable(Value* value_ptr, BaseType* type) {
    auto& value = *value_ptr;
    const auto expected_type_kind = type->kind();
    if(expected_type_kind == BaseTypeKind::Reference) {
        return false;
    }
    auto chain = value.as_access_chain();
    if(chain && chain->values.back()->as_func_call()) {
        return false;
    }
    const auto linked_def = type->get_direct_linked_struct();
    if(linked_def && linked_def->MembersContainer::requires_moving()) {
        return true;
    }
    return false;
}

bool FunctionType::mark_moved_value(
        ASTAllocator& allocator,
        Value* value_ptr,
        BaseType* expected_type,
        ASTDiagnoser& diagnoser,
        bool check_implicit_constructors
) {
    auto& value = *value_ptr;
    auto chain = value.as_access_chain();
    if(chain && chain->values.back()->as_func_call()) {
        return false;
    }
    const auto expected_type_kind = expected_type ? expected_type->kind() : BaseTypeKind::Unknown;
    if (expected_type_kind == BaseTypeKind::Reference) {
        return false;
    }
    const auto type = value.create_type(allocator);
    if(!type) return false;
    const auto linked_node = type->get_direct_linked_node();
    if(!linked_node) {
        return false;
    }
    const auto linked_node_kind = linked_node->kind();
    // TODO this doesn't account for typealiases
    if(linked_node_kind == ASTNodeKind::GenericTypeParam) {
        // all generic types are moved (later we'll allow them to be moved twice (use after move)
        // then we'll indicate in their type to check for multiple moves, to provide diagnostics to function arguments
        return mark_moved_value(&value, diagnoser);
    }
    const auto linked_def = linked_node->as_members_container(linked_node_kind);
    if(!linked_def) {
        return false;
    }
    const bool has_destr = linked_def->destructor_func();
    const bool has_clear_fn = linked_def->clear_func();
    const bool has_move_fn = linked_def->pre_move_func();
    if (!has_destr && !has_clear_fn && !has_move_fn) {
        return false;
    }
    bool final = false;
    if(expected_type) {
        const auto pure_expected = expected_type->pure_type();
        const auto pure_expected_kind = pure_expected->kind();
        const auto expected_node = pure_expected->get_ref_or_linked_node(pure_expected_kind);
        if(!expected_node) {
            if(expected_type_kind != BaseTypeKind::Any) {
                diagnoser.error("cannot move a struct to a non struct type", &value);
            }
            return false;
        }
        if (expected_node == (ASTNode*) linked_def) {
            final = mark_moved_value(&value, diagnoser);
        } else {
            const auto implicit = pure_expected->implicit_constructor_for(allocator, &value);
            if(implicit && check_implicit_constructors) {
                auto& param_type = *implicit->params[0]->type;
                if(!param_type.is_reference()) { // not a reference type (requires moving)
                    final = mark_moved_value(&value, diagnoser);
                }
            } else {
                diagnoser.error("unknown value being moved, where the struct types don't match", &value);
                return false;
            }
        }
    } else {
        final = mark_moved_value(&value, diagnoser);
    }
    if(final) {
        if(has_destr) {
            if(!has_clear_fn && !has_move_fn) {
                diagnoser.error("struct has a delete function but has no clear / move / implicit copy function", &value);
                return false;
            }
        } else {
            if(has_clear_fn || has_move_fn) {
                diagnoser.error("struct has a clear / delete function but has no delete function", &value);
            }
            return false;
        }
        return true;
    }
    return false;
}

bool FunctionType::mark_un_moved_lhs_value(Value* value_ptr, BaseType* value_type) {
    if(!value_type || !is_value_movable(value_ptr, value_type)) {
        return false;
    }
    auto& value = *value_ptr;
    auto chain = value.as_access_chain();
    if(chain) {
        // we indicate if previous value should be destructed by setting lhs of assignment's is_moved to true or false
        if(un_move_chain(chain)) {
            // setting true, to indicate that value was moved before, and this should not be destructed
            chain->is_moved = true;
            return true;
        } else {
            // setting false, to indicate that value is not moved, and this should be destructed
            chain->is_moved = false;
        }
    } else {
        auto id = value.as_identifier();
        if(id) {
            if(un_move_id(id)) {
                // setting true, to indicate that value was moved before, and this should not be destructed
                id->is_moved = true;
                return true;
            } else {
                // setting false, to indicate that value is not moved, and this should be destructed
                id->is_moved = false;
            }
            // since we've moved a new value into the lhs value
            // if it's connected to a var init statement, we marked it move to prevent final destructor from being called on it
            // now we re mark it not moved, so destructor is called
            const auto linked = id->linked;
            const auto linked_kind = linked->kind();
            if(linked_kind == ASTNodeKind::VarInitStmt) {
                const auto init = linked->as_var_init_unsafe();
                init->unmove();
            } else if(linked_kind == ASTNodeKind::FunctionParam) {
                const auto param = linked->as_func_param_unsafe();
                param->unmove();
            }
        }
    }
    return false;
}