// Copyright (c) Chemical Language Foundation 2025.

#include "ChainValue.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/StructValue.h"
#include "ast/statements/Assignment.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/IntValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/BigIntValue.h"
#include "ast/values/UBigIntValue.h"
#include "ast/values/Int128Value.h"
#include "ast/values/Negative.h"
#include "ast/values/UInt128Value.h"
#include "ast/values/ShortValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/UShortValue.h"
#include "ast/values/Expression.h"
#include "ast/values/UCharValue.h"
#include "ast/values/NumberValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/statements/Return.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/values/UIntValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/IndexOperator.h"
#include "ast/types/ArrayType.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/values/DereferenceValue.h"
#include "ast/structures/VariantMemberParam.h"
#include "ast/values/VariantCase.h"
#include "ast/values/CastedValue.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/If.h"
#include <ranges>
#include "preprocess/RepresentationVisitor.h"
#include <sstream>
#include <iostream>

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include "ast/structures/StructMember.h"

llvm::AllocaInst *Value::llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) {
    const auto value = llvm_value(gen, expected_type);
    const auto type = expected_type ? expected_type->llvm_type(gen) : llvm_type(gen);
    auto alloc = gen.builder->CreateAlloca(type, nullptr);
    gen.di.instr(alloc, this);
    const auto store = gen.builder->CreateStore(expected_type ? gen.implicit_cast(value, expected_type, type) : value, alloc);
    gen.di.instr(store, this);
    return alloc;
}

llvm::AllocaInst* ChainValue::access_chain_allocate(Codegen& gen, std::vector<ChainValue*>& values, unsigned int until, BaseType* expected_type) {
    const auto val = values[until];
    const auto value = val->access_chain_value(gen, values, until, expected_type);
    const auto alloc = gen.builder->CreateAlloca(val->llvm_type(gen), nullptr);
    gen.di.instr(alloc, val);
    const auto store = gen.builder->CreateStore(value, alloc);
    gen.di.instr(store, val);
    return alloc;
}

llvm::Value* Value::get_element_pointer(
        Codegen& gen,
        llvm::Type* in_type,
        llvm::Value* ptr,
        std::vector<llvm::Value *>& idxList,
        unsigned int index
) {
    idxList.emplace_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    return gen.builder->CreateGEP(in_type, ptr, idxList, "", gen.inbounds);
}

unsigned int Value::store_in_struct(
        Codegen& gen,
        Value* parent,
        llvm::Value* allocated,
        llvm::Type* allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType* expected_type
) {

    const auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);

    // mutating capturing function into instance type
    const auto mutated = gen.mutate_capturing_function(expected_type, this, elementPtr);
    if(mutated) {
        return index + 1;
    }

    // if it's a struct type that can be mem copied directly into the element pointer
    if(requires_memcpy_ref_struct(expected_type)) {
        if(!gen.copy_or_move_struct(expected_type, this, elementPtr)) {
            gen.warn("couldn't copy or move the struct to location", this);
        }
        return index + 1;
    }

    const auto value = llvm_value(gen, expected_type);
    if(!gen.assign_dyn_obj(this, expected_type, elementPtr, value, encoded_location())) {

        const auto value_pure = getType()->canonical();
        const auto derefType = value_pure->getAutoDerefType(expected_type);

        llvm::Value* Val = value;
        if(derefType) {
            const auto loadInstr = gen.builder->CreateLoad(derefType->llvm_type(gen), value);
            gen.di.instr(loadInstr, this);
            Val = loadInstr;
        } else if(value->getType()->isIntegerTy()) {
            const auto ll_type = expected_type->pure_type(gen.allocator)->llvm_type(gen);
            Val = gen.implicit_cast(Val, expected_type, ll_type);
        }

        const auto storeInstr = gen.builder->CreateStore(Val, elementPtr);
        gen.di.instr(storeInstr, this);

    }
    return index + 1;
}

unsigned int Value::store_in_array(
        Codegen& gen,
        Value* parent,
        llvm::Value* allocated,
        llvm::Type* allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType* expected_type
) {
    auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);

    // mutating capturing function into instance type
    const auto mutated = gen.mutate_capturing_function(expected_type, this, elementPtr);
    if(mutated) {
        return index + 1;
    }

    // if it's a struct type that can be mem copied directly into the element pointer
    if(requires_memcpy_ref_struct(expected_type)) {
        if(!gen.copy_or_move_struct(expected_type, this, elementPtr)) {
            gen.warn("couldn't copy or move the struct to location", this);
        }
        return index + 1;
    }

    const auto value = llvm_value(gen, expected_type);
    if(!gen.assign_dyn_obj(this, expected_type, elementPtr, value, encoded_location())) {

        const auto value_pure = getType()->canonical();
        const auto derefType = value_pure->getAutoDerefType(expected_type);

        llvm::Value* Val = value;
        if(derefType) {
            const auto loadInstr = gen.builder->CreateLoad(derefType->llvm_type(gen), value);
            gen.di.instr(loadInstr, this);
            Val = loadInstr;
        } else if(value->getType()->isIntegerTy()) {
            const auto ll_type = expected_type->pure_type(gen.allocator)->llvm_type(gen);
            Val = gen.implicit_cast(Val, expected_type, ll_type);
        }

        const auto storeInstr = gen.builder->CreateStore(Val, elementPtr);
        gen.di.instr(storeInstr, this);

    }
    return index + 1;
}

void Value::destruct(Codegen& gen, std::vector<std::pair<Value*, llvm::Value*>>& destructibles) {
    for(auto& val : std::ranges::reverse_view(destructibles)) {
        val.first->llvm_destruct(gen, val.second);
    }
}

llvm::Value* Value::load_value(Codegen& gen, BaseType* known_t, llvm::Type* type, llvm::Value* ptr, SourceLocation location) {
    if(known_t->isStructLikeType()) {
        return ptr;
    }
    const auto loadInstr = gen.builder->CreateLoad(type, ptr);
    gen.di.instr(loadInstr, location);
    return loadInstr;
}

llvm::Value* ChainValue::access_chain_value(Codegen &gen, std::vector<ChainValue*>& values, unsigned int until, std::vector<std::pair<Value*, llvm::Value*>>& destructibles, BaseType* expected_type) {
    if(until == 0) {
        return values[0]->llvm_value(gen, expected_type);
    };
    const auto last = values[until];
    const auto kind = last->val_kind();
    if(kind == ValueKind::Identifier) {
        const auto id = last->as_identifier_unsafe();
        if(id->linked->kind() == ASTNodeKind::EnumMember) {
            return id->linked->llvm_load(gen, id->encoded_location());
        }
    }
    return Value::load_value(gen, last, access_chain_pointer(gen, values, destructibles, until));
}

/**
 * should llvm_chain_type be used to get the llvm_type
 * llvm_chain_type checks each member in the access chain
 * for example, in a.b.c, 'b' could be a unnamed union, or anonymous union declaration
 * now 'c' is accessing member of a union, to be able to access 'c' we emit a get element ptr instruction
 * which must have correct parent type b, now if llvm_type were to be called on 'b' it would consider it's largest
 * member, because that's what unions do, their storage type is the largest member
 * now that largest member may or may not contain 'c' which is the next member, and we are giving llvm
 * the largest member type, so this method checks if the chain contains a union or unnamed union, and
 * one of largest member is not being accessed in the union, then we call llvm_chain_type
 */
bool should_use_chain_type(ASTAllocator& allocator, std::vector<ChainValue*>& values, unsigned index) {
    unsigned i = index;
    // why -1, last not checked, because last maybe a union but there's no member access into it
    const auto total = values.size() - 1;
    while(i < total - 1) {
        auto& value = values[i];
        auto base_type = value->getType();
        auto node = base_type->get_direct_linked_node();
        if(node) {
            auto node_kind = node->kind();
            if(node_kind == ASTNodeKind::UnionDecl) {
                auto union_def = node->as_union_def_unsafe();
                auto& next = values[i + 1];
                auto next_node = next->linked_node();
                auto largest = union_def->largest_member();
                if(next_node != largest) {
                    return true;
                }
            } else if(node_kind == ASTNodeKind::UnnamedUnion) {
                auto unnamed_def = node->as_unnamed_union_unsafe();
                auto& next = values[i + 1];
                auto next_node = next->linked_node();
                auto largest = unnamed_def->largest_member();
                if(next_node != largest) {
                    return true;
                }
            }
        }
        i++;
    }
    return false;
}

// this method could be put into access chain's llvm_type, when the need arises
llvm::Type* access_chain_llvm_type(Codegen& gen, BaseType* type, std::vector<ChainValue*>& values, unsigned index) {
    if(should_use_chain_type(gen.allocator, values, index)) {
        return type->llvm_chain_type(gen, values, index);
    } else {
        return type->llvm_type(gen);
    };
}

llvm::Value* create_gep(Codegen &gen, std::vector<ChainValue*>& values, unsigned index, llvm::Value* pointer, std::vector<llvm::Value*>& idxList) {
    const auto parent = values[index];
    const auto linked = parent->linked_node();
    auto type = parent->getType()->canonical();
    if(type) {
        auto type_kind = type->kind();
        if (type_kind == BaseTypeKind::Array && linked && linked->as_func_param()) {
            auto arr_type = (ArrayType*) type;
            return gen.builder->CreateGEP(access_chain_llvm_type(gen, arr_type->elem_type, values, index), pointer, idxList, "", gen.inbounds);
        } else if (type_kind == BaseTypeKind::Pointer) {
            return gen.builder->CreateGEP(access_chain_llvm_type(gen, ((PointerType*) (type))->type, values, index),
                                          pointer, idxList, "", gen.inbounds);
        } else if (type_kind == BaseTypeKind::Reference) {
            return gen.builder->CreateGEP(access_chain_llvm_type(gen, ((ReferenceType*) (type))->type, values, index),
                                          pointer, idxList, "", gen.inbounds);
        }
    }
    return gen.builder->CreateGEP(parent->llvm_chain_type(gen, values, index), pointer, idxList, "", gen.inbounds);
}

std::pair<unsigned int, llvm::Value*> ChainValue::access_chain_parent_pointer(
        Codegen &gen,
        std::vector<ChainValue*>& values,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
        unsigned int until,
        std::vector<llvm::Value*>& idxList
) {

#ifdef DEBUG
    if(until == 0) {
        throw std::runtime_error("index can't be zero, because it takes a parent pointer, parent exists at location zero");
    }
#endif

    unsigned parent_index = 0;
    Value* parent = values[0];
    llvm::Value* pointer = parent->llvm_pointer(gen);

    // queue return of first function call return for destruction (at the end of this chain loading)
    if(parent->val_kind() == ValueKind::FunctionCall) {
        destructibles.emplace_back(parent, pointer);
    }

    unsigned i = 1;

    const auto is_stored = parent->is_stored_ptr_or_ref(gen.allocator);
    if(is_stored && i <= until) {
        const auto loadInst = gen.builder->CreateLoad(parent->llvm_type(gen), pointer);
        gen.di.instr(loadInst, parent);
        pointer = loadInst;
    }

    while (i <= until) {
        if(i + 1 <= until && values[i]->is_stored_ptr_or_ref(gen.allocator)) {
            llvm::Value* gep;
            if(idxList.empty()) {
                gep = pointer;
            } else {
                gep = create_gep(gen, values, parent_index, pointer, idxList);
            }
            const auto current = values[i];
            const auto loadInst = gen.builder->CreateLoad(current->llvm_type(gen), gep);
            gen.di.instr(loadInst, current);
            pointer = loadInst;
            parent = current;
            parent_index = i;
            idxList.clear();
        } else {
            if (!values[i]->add_member_index(gen, values[i - 1], idxList)) {
                auto& diag = gen.error(values[i]);
                diag << "couldn't add member index for fragment '";
                diag << values[i]->representation();
                diag << "' in access chain ";
                bool is_first = true;
                for(auto& val : values) {
                    if(!is_first) {
                        diag << ',';
                    }
                    diag << '\'';
                    diag << val->representation();
                    diag << '\'';
                    is_first = false;
                }
            }
        }
        i++;
    }

    return { parent_index, pointer };

}

llvm::Value* ChainValue::access_chain_pointer(
        Codegen &gen,
        std::vector<ChainValue*>& values,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
        unsigned int until
) {
    // a single value, we just return pointer to it
    if(until == 0) {
        return values[0]->llvm_pointer(gen);
    }
    // evaluate last comptime function call
    const auto last = values[until];
    const auto last_func_call = last->as_func_call();
    if(last_func_call) {
        const auto func_decl = last_func_call->safe_linked_func();
        if(func_decl && func_decl->is_comptime()) {
            auto& ret_value = gen.eval_comptime(last_func_call, func_decl);
            if(ret_value) {
                return ret_value->llvm_pointer(gen);
            } else {
                return nullptr;
            }
        }
    }
    std::vector<llvm::Value*> idxList;
    auto parent_pointer = access_chain_parent_pointer(gen, values, destructibles, until, idxList);
    return create_gep(gen, values, parent_pointer.first, parent_pointer.second, idxList);
}

void Value::llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block) {
    auto value = llvm_value(gen);
    if(value->getType()->isPointerTy()) {
        value = gen.builder->CreateICmpNE(value, NullValue::null_llvm_value(gen));
    }
    gen.CreateCondBr(value, then_block, otherwise_block, encoded_location());
}

llvm::Value* Value::llvm_pointer(Codegen& gen) {
    throw std::runtime_error("llvm_pointer called on bare Value");
}

llvm::Value* Value::llvm_value(Codegen& gen, BaseType* type) {
    throw std::runtime_error("Value::llvm_value called on bare Value " + representation());
}

bool Value::add_member_index(Codegen& gen, Value* parent, std::vector<llvm::Value*>& indexes) {
#ifdef DEBUG
    throw std::runtime_error("Value::add_member_index called on a value");
#else
    std::cerr << "add_member_index called on base value " << representation();
#endif
}

bool Value::add_child_index(Codegen& gen, std::vector<llvm::Value*>& indexes, const chem::string_view& name) {
#ifdef DEBUG
    throw std::runtime_error("Value::add_child_index called on a Value");
#else
    std::cerr << "add_child_index called on base Value " << representation();
#endif
}

llvm::Value* Value::llvm_arg_value(Codegen& gen, BaseType* expected_type) {
    return llvm_value(gen, expected_type);
}

/**
 * this method is called by return statement to get the return value for this Value
 * if this class defines specific behavior for return, it should override this method
 */
llvm::Value* Value::llvm_ret_value(Codegen& gen, Value* returnValue) {
    return llvm_value(gen, nullptr);
}

/**
 * called by assignment, to assign the current value to left hand side
 */
void Value::llvm_assign_value(Codegen& gen, llvm::Value* lhsPtr, Value* lhs) {
    const auto rhsValue = llvm_value(gen, lhs ? lhs->known_type() : nullptr);
    gen.assign_store(lhs, lhsPtr, this, rhsValue, encoded_location());
}

void ChainValue::access_chain_assign_value(
    Codegen& gen,
    AccessChain* chain,
    unsigned int until,
    std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
    llvm::Value* lhsPtr,
    Value* lhs,
    BaseType* expected_type
) {
    const auto value = access_chain_value(gen, chain->values, until, destructibles, expected_type);
    gen.assign_store(lhs, lhsPtr, chain, value, encoded_location());
}

#endif

bool Value::isValueKindRValue(ValueKind kind) {
    switch(kind) {
        case ValueKind::Bool:
        case ValueKind::NumberValue:
        case ValueKind::Char:
        case ValueKind::UChar:
        case ValueKind::Short:
        case ValueKind::UShort:
        case ValueKind::Int:
        case ValueKind::UInt:
        case ValueKind::Long:
        case ValueKind::ULong:
        case ValueKind::BigInt:
        case ValueKind::UBigInt:
        case ValueKind::Double:
        case ValueKind::Float:
        case ValueKind::NegativeValue:
        case ValueKind::SizeOfValue:
        case ValueKind::AlignOfValue:
            return true;
        default:
            return false;
    }
}

bool isTypeRValue(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Bool:
        case BaseTypeKind::IntN:
        case BaseTypeKind::Float:
        case BaseTypeKind::Double:
        case BaseTypeKind::LongDouble:
        case BaseTypeKind::Float128:
            return true;
        case BaseTypeKind::Linked: {
            const auto linked = type->as_linked_type_unsafe()->linked;
            switch (linked->kind()) {
                case ASTNodeKind::TypealiasStmt:
                    return isTypeRValue(linked->as_typealias_unsafe()->actual_type);
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

bool Value::isValueIntegerLiteral() {
    switch(kind()) {
        case ValueKind::Bool:
        case ValueKind::NumberValue:
        case ValueKind::Char:
        case ValueKind::UChar:
        case ValueKind::Short:
        case ValueKind::UShort:
        case ValueKind::Int:
        case ValueKind::UInt:
        case ValueKind::Long:
        case ValueKind::ULong:
        case ValueKind::BigInt:
        case ValueKind::UBigInt:
            return true;
        default:
            return false;
    }
}

bool Value::isValueLiteral() {
    switch(kind()) {
        case ValueKind::Bool:
        case ValueKind::NumberValue:
        case ValueKind::Char:
        case ValueKind::UChar:
        case ValueKind::Short:
        case ValueKind::UShort:
        case ValueKind::Int:
        case ValueKind::UInt:
        case ValueKind::Long:
        case ValueKind::ULong:
        case ValueKind::BigInt:
        case ValueKind::UBigInt:
        case ValueKind::Double:
        case ValueKind::Float:
            return true;
        default:
            return false;
    }
}

bool Value::isValueRValue(ASTAllocator& allocator) {
    switch(kind()) {
        case ValueKind::Bool:
        case ValueKind::NumberValue:
        case ValueKind::Char:
        case ValueKind::UChar:
        case ValueKind::Short:
        case ValueKind::UShort:
        case ValueKind::Int:
        case ValueKind::UInt:
        case ValueKind::Long:
        case ValueKind::ULong:
        case ValueKind::BigInt:
        case ValueKind::UBigInt:
        case ValueKind::Double:
        case ValueKind::Float:
        case ValueKind::NegativeValue:
        case ValueKind::SizeOfValue:
        case ValueKind::AlignOfValue:
            return true;
        case ValueKind::FunctionCall:
            return isTypeRValue(getType());
        case ValueKind::AccessChain:
            return as_access_chain_unsafe()->values.back()->isValueRValue(allocator);
        case ValueKind::Identifier:{
            const auto linked = linked_node();
            switch(linked->kind()) {
                case ASTNodeKind::FunctionParam:
                    return isTypeRValue(linked->as_func_param_unsafe()->type);
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

uint64_t Value::byte_size(bool is64Bit) {
#ifdef DEBUG
    throw std::runtime_error("byte_size called on base Value " + representation());
#else
    std::cerr << "Value::byte_size called on value " << representation() << std::endl;
    return 0;
#endif
}

BaseType* Value::get_stored_value_type(ASTAllocator& allocator) {
    auto linked = linked_node();
    return linked ? linked->get_stored_value_type(allocator, linked->kind()) : nullptr;
}

// stored pointer into a variable, that must be loaded, before using
bool Value::is_stored_ptr_or_ref(ASTAllocator& allocator) {
    auto linked = linked_node();
    return linked != nullptr && linked->is_stored_ptr_or_ref(allocator);
}

bool Value::is_stored_ref(ASTAllocator& allocator) {
    auto linked = linked_node();
    return linked != nullptr && linked->is_stored_ref(allocator);
}

bool Value::is_ptr_or_ref(ASTAllocator& allocator) {
    auto linked = linked_node();
    return linked != nullptr && linked->is_ptr_or_ref(allocator, linked->kind());
}

bool Value::is_ref(ASTAllocator& allocator) {
    auto linked = linked_node();
    return linked != nullptr && linked->is_ref(allocator);
}

bool Value::is_ref_value() {
    switch(val_kind()) {
        case ValueKind::AccessChain:
            return true;
        case ValueKind::Identifier:
            return true;
        default:
            return false;
    }
}

bool is_node_assignable(ASTNode* node) {
    const auto linked_kind = node->kind();
    switch(linked_kind) {
        case ASTNodeKind::VarInitStmt:{
            return !node->as_var_init_unsafe()->is_const();
        }
        case ASTNodeKind::FunctionParam: {
            return true;
        }
        case ASTNodeKind::VariantCaseVariable: {
            const auto member_param = node->as_variant_case_var_unsafe()->member_param;
            return !member_param->is_const;
        }
        case ASTNodeKind::StructMember:
        case ASTNodeKind::UnnamedUnion:
        case ASTNodeKind::UnnamedStruct: {
            return !node->as_base_def_member_unsafe()->get_is_const();
        }
        case ASTNodeKind::CapturedVariable: {
            const auto cap = node->as_captured_var_unsafe();
            if(cap->capture_by_ref) {
                return cap->refType.is_mutable;
            } else {
                return true;
            }
        }
        case ASTNodeKind::PatternMatchId: {
            return true;
        }
        default:
            return false;
    }
}

bool is_node_mutable(ASTNode* node, FunctionType* func_type, SymbolResolver& resolver) {
    const auto linked_kind = node->kind();
    switch(linked_kind) {
        case ASTNodeKind::VarInitStmt:{
            const auto type = node->as_var_init_unsafe()->known_type();
            return type->is_mutable();
        }
        case ASTNodeKind::FunctionParam: {
            const auto type = node->as_func_param_unsafe()->type;
            return type->is_mutable();
        }
        case ASTNodeKind::VariantCaseVariable: {
            const auto member_param = node->as_variant_case_var_unsafe()->member_param;
            const auto type = member_param->type;
            return type->is_mutable();
        }
        case ASTNodeKind::StructMember:
        case ASTNodeKind::UnnamedUnion:
        case ASTNodeKind::UnnamedStruct: {
            const auto self_param = func_type->get_self_param();
            if (self_param) {
                return self_param->type->is_mutable();
            } else {
                const auto func = func_type->as_function();
                // constructor takes a mutable reference by default
                return func->is_constructor_fn();
            }
        }
        default:
            return false;
    }
}

bool Value::is_ref_l_value() {
    const auto kind = val_kind();
    switch(kind) {
        case ValueKind::StructValue:
            return true;
        default: {
            const auto linked = linked_node();
            return linked && is_node_assignable(linked);
        }
    }
}

bool Value::check_is_mutable(ASTAllocator& allocator, bool assigning) {
    switch(val_kind()) {
        case ValueKind::Identifier: {
            const auto id = as_identifier_unsafe();
            if(assigning) {
                return is_node_assignable(id->linked);
            } else {
                const auto type = id->linked->known_type_SymRes(allocator);
                return type->is_mutable();
            }
        }
        case ValueKind::FunctionCall:{
            const auto call = as_func_call_unsafe();
            const auto type = call->getType();
            return type->is_mutable();
        }
        case ValueKind::IndexOperator: {
            const auto index = as_index_op_unsafe();
            return index->parent_val->check_is_mutable(allocator, false);
        }
        case ValueKind::AccessChain: {
            const auto chain = as_access_chain_unsafe();
            auto& chain_values = chain->values;
            const auto chain_size = chain_values.size();
            const auto last_ind = chain_size - 1;
            const auto last = chain_values[last_ind];
            const auto last_kind = last->val_kind();
            if(last_kind == ValueKind::FunctionCall || last_kind == ValueKind::IndexOperator) {
                return last->check_is_mutable(allocator, assigning);
            }
            unsigned i = 0;
            while(i < chain_size) {
                const auto value = chain_values[i];
                if(i == last_ind) {
                    const auto is_last_id = last_kind == ValueKind::Identifier;
                    if(!value->check_is_mutable(allocator, assigning && is_last_id)) {
                        return false;
                    }
                } else {
                    if(!value->check_is_mutable(allocator, false)) {
                        return false;
                    }
                }
                i++;
            }
            return true;
        }
        case ValueKind::DereferenceValue: {
            return as_dereference_value_unsafe()->getValue()->check_is_mutable(allocator, false);
        }
        case ValueKind::Expression: {
            const auto expr = as_expression_unsafe();
            return expr->getType()->is_mutable();
        }
        default:
            return false;
    }
}

bool Value::is_chain_func_call() {
    auto chain = as_access_chain();
    if(chain && chain->values.back()->as_func_call() != nullptr) {
        return true;
    }
    return false;
}

bool Value::is_func_call() {
    switch(kind()) {
        case ValueKind::FunctionCall:
            return true;
        case ValueKind::AccessChain:
            return as_access_chain_unsafe()->values.back()->kind() == ValueKind::FunctionCall;
        default:
            return false;
    }
}

bool Value::is_ref_moved() {
    switch(val_kind()) {
        case ValueKind::AccessChain:
            return as_access_chain_unsafe()->is_moved();
        case ValueKind::Identifier:
            return as_identifier_unsafe()->is_moved;
        default:
                return false;
    }
}

VariableIdentifier* Value::get_chain_id() {
    switch(kind()) {
        case ValueKind::AccessChain:
            return as_access_chain_unsafe()->values.size() == 1 ? as_access_chain_unsafe()->values.back()->as_identifier() : nullptr;
        case ValueKind::Identifier:
            return as_identifier_unsafe();
        default:
            return nullptr;
    }
}

bool Value::reference() {
    const auto kind = val_kind();
    switch(kind) {
        case ValueKind::AccessChain:
        case ValueKind::Identifier:
        case ValueKind::FunctionCall:
            return true;
        default:
            return false;
    }
}

bool Value::requires_memcpy_ref_struct(BaseType* known_type) {
    // is referencing another struct, that is non movable and must be mem copied into the pointer
    const auto kind = val_kind();
    if(kind == ValueKind::Identifier || (kind == ValueKind::AccessChain && as_access_chain_unsafe()->values.back()->as_func_call() == nullptr)) {
        auto linked = known_type->get_direct_linked_canonical_node();
        if (linked) {
            switch(linked->kind()) {
                case ASTNodeKind::UnnamedStruct:
                case ASTNodeKind::UnnamedUnion:
                case ASTNodeKind::StructDecl:
                case ASTNodeKind::VariantDecl:
                case ASTNodeKind::UnionDecl:
                    return true;
                default:
                    return false;
            }
        }
    }
    return false;
}

Value* Value::child(InterpretScope& scope, const chem::string_view& name) {
#ifdef DEBUG
    std::cerr << "Value::child called on base value " + representation();
#endif
    return nullptr;
}

Value* Value::call_member(InterpretScope& scope, const chem::string_view& name, std::vector<Value*>& values) {
#ifdef DEBUG
    std::cerr << "Value::call_member called on base value " + representation() + " with name " + name.str();
#endif
    return nullptr;
}

Value* Value::index(InterpretScope& scope, int i) {
#ifdef DEBUG
    std::cerr << "Value::index called on base value " + representation();
#endif
    return nullptr;
}

Value* Value::find_in(InterpretScope& scope, Value* parent) {
#ifdef DEBUG
    std::cerr << "Value::find_in called on base value " + representation();
#endif
    return nullptr;
}

IntNumValue* IntNumValue::create_number(ASTAllocator& alloc, TypeBuilder& typeBuilder, unsigned int bitWidth, bool is_signed, uint64_t value, SourceLocation location) {
    switch(bitWidth) {
        case 8:
            if(is_signed) {
                return new (alloc.allocate<CharValue>()) CharValue((char) value, typeBuilder.getCharType(), location);
            } else {
                return new (alloc.allocate<UCharValue>()) UCharValue((unsigned char) value, typeBuilder.getUCharType(), location);
            }
        case 16:
            if(is_signed) {
                return new (alloc.allocate<ShortValue>()) ShortValue((short) value, typeBuilder.getShortType(), location);
            } else {
                return new (alloc.allocate<UShortValue>()) UShortValue((unsigned short) value, typeBuilder.getUShortType(), location);
            }
        case 32:
            if(is_signed) {
                return new (alloc.allocate<IntValue>()) IntValue((int) value, typeBuilder.getIntType(), location);
            } else {
                return new (alloc.allocate<UIntValue>()) UIntValue((unsigned int) value, typeBuilder.getUIntType(), location);
            }
        case 64:
            if(is_signed) {
                return new (alloc.allocate<BigIntValue>()) BigIntValue((long long) value, typeBuilder.getBigIntType(), location);
            } else {
                return new (alloc.allocate<UBigIntValue>()) UBigIntValue((unsigned long long) value, typeBuilder.getUBigIntType(), location);
            }
        case 128:
            if(is_signed) {
                return new (alloc.allocate<Int128Value>()) Int128Value(value, false, typeBuilder.getInt128Type(), location);
            } else {
                return new (alloc.allocate<UInt128Value>()) UInt128Value(value, false, typeBuilder.getUInt128Type(), location);
            }
        default:
#ifdef DEBUG
            throw std::runtime_error("value couldn't be created");
#endif
            return nullptr;
    }
}

StructDefinition* Value::get_param_linked_struct() {
    const auto linked = linked_node();
    if(!linked) return nullptr;
    const auto linked_kind = linked->kind();
    if(linked_kind == ASTNodeKind::FunctionParam) {
        return linked->as_func_param_unsafe()->type->get_direct_linked_struct();
    }
    return nullptr;
}

BaseType* Value::create_type(ASTAllocator& allocator) {
#ifdef DEBUG
    throw std::runtime_error("create_type called on base value");
#else
    return nullptr;
#endif
}

bool Value::is_pointer() {
    const auto k = known_type()->kind();
    return k == BaseTypeKind::Pointer || k == BaseTypeKind::String;
}

bool Value::is_pointer_or_ref() {
    const auto k = known_type()->kind();
    return k == BaseTypeKind::Pointer || k == BaseTypeKind::String || k == BaseTypeKind::Reference;
}

Value* Value::copy(ASTAllocator& allocator) {
#ifdef DEBUG
    std::cerr << "copy called on base Value, representation : " << representation();
#endif
    return nullptr;
}

unsigned Value::get_the_uint() {
    return ((UIntValue*) this)->value;
}

char Value::get_the_char() {
    return ((CharValue*) this)->value;
}

bool Value::get_the_bool() {
    return ((BoolValue*) this)->value;
}

const chem::string_view& Value::get_the_string() {
    return ((StringValue*) this)->value;
}

std::optional<uint64_t> Value::get_the_number() {
    switch(val_kind()) {
        case ValueKind::Short:
        case ValueKind::UShort:
        case ValueKind::Int:
        case ValueKind::UInt:
        case ValueKind::Long:
        case ValueKind::ULong:
        case ValueKind::BigInt:
        case ValueKind::UBigInt:
        case ValueKind::NumberValue:
            return ((IntNumValue*) this)->get_num_value();
        default:
            return std::nullopt;
    }
}

int Value::get_the_int() {
    switch(val_kind()) {
        case ValueKind::Int:
            return ((IntValue*) this)->value;
        case ValueKind::NumberValue:
            return (int) ((NumberValue*) this)->value;
        default:
#ifdef DEBUG
            throw std::runtime_error("unknown value for as_int");
#endif
            std::cerr << "unknown value for as_int" << std::endl;
            return -1;
    }
}

std::optional<uint64_t> Value::get_number() {
    if(is_value_int_n()) {
        return as_int_num_value_unsafe()->get_num_value();
    } else if(kind() == ValueKind::NegativeValue) {
        auto value = as_negative_value_unsafe()->getValue()->get_number();
        if(value.has_value()) {
            return -value.value();
        } else {
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }
}

float Value::get_the_float() {
    return ((FloatValue*) this)->value;
}

double Value::get_the_double() {
    return ((DoubleValue*) this)->value;
}

Value* Value::get_first_value_from_value_node(ASTNode* node) {
    const auto k = node->kind();
    switch(k) {
        case ASTNodeKind::ValueWrapper:
            return node->as_value_wrapper_unsafe()->value;
        case ASTNodeKind::ValueNode:
            return node->holding_value();
        case ASTNodeKind::IfStmt:
            return get_first_value_from_value_node(((IfStatement*) node)->ifBody.nodes.back());
        case ASTNodeKind::SwitchStmt: {
            const auto switch_node = ((SwitchStatement*) node);
            auto& scopes = switch_node->scopes;
            if(!scopes.empty()) {
                auto& nodes = scopes.front().nodes;
                if(nodes.empty()) {
                    return nullptr;
                } else {
                    return get_first_value_from_value_node(nodes.back());
                }
            } else {
                return nullptr;
            }
        }
        default:
            return nullptr;
    }
}

std::string Value::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    visitor.visit(this);
    return ostring.str();
}

Value* Value::scope_value(InterpretScope& scope) {
    const auto eval = evaluated_value(scope);
    if(eval == this) {
        return copy(scope.allocator);
    } else {
        return eval;
    }
}

BaseType* Value::pure_type_ptr() {
    return known_type()->canonical();
}

bool Value::should_build_chain_type(std::vector<Value*>& chain, unsigned index) {
    ASTNode* linked;
    VariablesContainer* union_container = nullptr;
    while(index < chain.size()) {
        linked = chain[index]->linked_node();
        if(linked && (linked->as_union_def() || linked->as_unnamed_union())) {
            union_container = linked->as_variables_container();
        } else if(union_container && linked != union_container->largest_member()) {
            return true;
        }
        index++;
    }
    return false;
}

bool ChainValue::is_equal(ChainValue* other, ValueKind kind, ValueKind other_kind) {
    if(kind == other_kind) {
        switch(kind) {
            case ValueKind::AccessChain: {
                auto this_chain = as_access_chain_unsafe();
                auto other_chain = other->as_access_chain_unsafe();
                const auto siz = this_chain->values.size();
                if(siz != other_chain->values.size()) {
                    return false;
                }
                unsigned i = 0;
                while(i < siz) {
                    if(!this_chain->values[i]->is_equal(other_chain->values[i])) {
                        return false;
                    }
                    i++;
                }
                return true;
            }
            case ValueKind::Identifier:
                return as_identifier()->linked == other->as_identifier()->linked;
            case ValueKind::IndexOperator: {
                const auto this_index_op = as_index_op();
                const auto other_index_op = other->as_index_op();
                const auto siz = this_index_op->values.size();
                if(siz != other_index_op->values.size()) {
                    return false;
                }
                unsigned i = 0;
                while(i < siz) {
                    if(this_index_op->values[i]->is_value_int_n() && other_index_op->values[i]->is_value_int_n()) {
                        auto this_value = ((IntNumValue*) this_index_op->values[i])->get_num_value();
                        auto other_value = ((IntNumValue*) other_index_op->values[i])->get_num_value();
                        if(this_value != other_value) {
                            return false;
                        }
                    } else {
                        return false;
                    }
                    i++;
                }
                return true;
            }
            case ValueKind::FunctionCall:{
                return false;
            }
            default:
                return false;
        }
    }
    return false;
}

VariableIdentifier* Value::get_last_id() {
    switch(kind()) {
        case ValueKind::AccessChain:
            return as_access_chain_unsafe()->values.back()->as_identifier();
        case ValueKind::Identifier:
            return as_identifier_unsafe();
        default:
            return nullptr;
    }
}

void Value::set_child_value(InterpretScope& scope, const chem::string_view& name, Value* value, Operation op) {
    switch(val_kind()) {
        case ValueKind::StructValue:
            as_struct_value_unsafe()->set_child_value(scope, name, value, op);
            return;
        default:
            scope.error("can't set child value with name " + name.str(), this);
            return;
    }
}

void Value::set_value(InterpretScope& scope, Value* value, Operation op, SourceLocation location) {
    switch(val_kind()) {
        case ValueKind::AccessChain: {
            const auto chain = as_access_chain_unsafe();
            chain->set_value(scope, value, op, location);
            return;
        }
        case ValueKind::Identifier:
            as_identifier_unsafe()->set_value(scope, value, op, location);
            return;
            // TODO
//        case ValueKind::FunctionCall:
            // TODO
//        case ValueKind::DereferenceValue:
        default:
            scope.error("couldn't set the value of", this);
    }
}

void Value::set_value_in(InterpretScope& scope, Value* parent, Value* value, Operation op, SourceLocation location) {
    switch(val_kind()) {
        case ValueKind::AccessChain: {
            const auto chain = as_access_chain_unsafe();
            if(chain->values.size() == 1) {
                chain->values.back()->set_value_in(scope, parent, value, op, location);
            } else {
                scope.error("couldn't set the value in parent of", this);
            }
            return;
        }
        case ValueKind::Identifier:
            as_identifier_unsafe()->set_value_in(scope, parent, value, op, location);
            // TODO
//        case ValueKind::FunctionCall:
            // TODO
//        case ValueKind::DereferenceValue:
        default:
            scope.error("couldn't set the value in parent of", this);
    }
}

//BaseType* implicit_constructor_type(ASTAllocator& allocator, BaseType* return_type, Value* value) {
//    auto k = return_type->kind();
//    if(k == BaseTypeKind::Linked || k == BaseTypeKind::Generic) {
//        const auto linked = return_type->linked_node();
//        const auto struc = linked->as_struct_def();
//        if(struc) {
//            const auto constr = struc->implicit_constructor_for(allocator, value);
//            if(constr) {
//                return constr->func_param_for_arg_at(0)->type;
//            }
//        }
//    }
//    return return_type;
//}

Value::~Value() = default;