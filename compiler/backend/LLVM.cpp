// Copyright (c) Qinetik 2024.

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include "ast/base/ASTNode.h"
#include "ast/types/AnyType.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/types/ArrayType.h"
#include "ast/types/GenericType.h"
#include "ast/types/BoolType.h"
#include "LLVMBackendContext.h"
#include "ast/types/CharType.h"
#include "ast/types/UCharType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DynamicType.h"
#include "ast/types/IntNType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/LinkedType.h"
#include "ast/types/UnionType.h"
#include "ast/types/StringType.h"
#include "ast/types/StructType.h"
#include "ast/types/VoidType.h"
#include "ast/values/BoolValue.h"
#include "ast/values/CharValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/SizeOfValue.h"
#include "ast/structures/Namespace.h"
#include "ast/values/StringValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/VariantCall.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/Expression.h"
#include "ast/values/CastedValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/ValueNode.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Return.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Break.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Import.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/InitBlock.h"
#include "ast/values/StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariablesContainer.h"
#include "ast/structures/MembersContainer.h"
#include "ast/statements/ThrowStatement.h"
#include "ast/structures/LoopBlock.h"
#include "ast/statements/DestructStmt.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/ArrayValue.h"
#include "ast/types/FunctionType.h"

// -------------------- Types

llvm::Type *AnyType::llvm_type(Codegen &gen) {
    throw std::runtime_error("llvm_type called on any type");
}

llvm::Type *ArrayType::llvm_type(Codegen &gen) {
    return llvm::ArrayType::get(elem_type->llvm_type(gen), array_size);
}

llvm::Type *ArrayType::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *BoolType::llvm_type(Codegen &gen) {
    return gen.builder->getInt1Ty();
}

llvm::Type *CharType::llvm_type(Codegen &gen) {
    return gen.builder->getInt8Ty();
}

llvm::Type *UCharType::llvm_type(Codegen &gen) {
    return gen.builder->getInt8Ty();
}

llvm::Type *DoubleType::llvm_type(Codegen &gen) {
    return gen.builder->getDoubleTy();
}

llvm::Type *FloatType::llvm_type(Codegen &gen) {
    return gen.builder->getFloatTy();
}

llvm::Type *IntNType::llvm_type(Codegen &gen) {
    auto ty = gen.builder->getIntNTy(num_bits());
    if(!ty) {
        gen.error("Couldn't get intN type for int:" + std::to_string(num_bits()), this);
    }
    return ty;
}

llvm::Type *PointerType::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *PointerType::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    if(index == values.size() - 1) {
        return gen.builder->getPtrTy();
    } else {
        return type->llvm_chain_type(gen, values, index);
    }
}

llvm::Type *ReferenceType::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *ReferenceType::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    if(index == values.size() - 1) {
        return gen.builder->getPtrTy();
    } else {
        return type->llvm_chain_type(gen, values, index);
    }
}

llvm::Type *LinkedType::llvm_type(Codegen &gen) {
    return linked->llvm_type(gen);
}

llvm::Type *LinkedType::llvm_param_type(Codegen &gen) {
    return linked->llvm_param_type(gen);
}

llvm::Type *LinkedType::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return linked->llvm_chain_type(gen, values, index);
}

llvm::Type *GenericType::llvm_type(Codegen &gen) {
    const auto gen_struct = referenced->linked_node()->as_members_container();
    const auto prev_itr = gen_struct->get_active_iteration();
    gen_struct->set_active_iteration(generic_iteration);
    auto type = referenced->llvm_type(gen);
    gen_struct->set_active_iteration(prev_itr);
    return type;
}

llvm::Type *GenericType::llvm_param_type(Codegen &gen) {
    const auto gen_struct = referenced->linked;
    const auto prev_itr = gen_struct->get_active_iteration();
    gen_struct->set_active_iteration(generic_iteration);
    auto type = referenced->llvm_param_type(gen);
    gen_struct->set_active_iteration(prev_itr);
    return type;
}

llvm::Type *StringType::llvm_type(Codegen &gen) {
    return gen.builder->getInt8PtrTy();
}

llvm::Type *StructType::with_elements_type(Codegen &gen, const std::vector<llvm::Type *>& elements, const std::string& runtime_name) {
    if(runtime_name.empty()) {
        return llvm::StructType::get(*gen.ctx, elements);
    }
    auto stored = llvm_stored_type();
    if(!stored) {
        auto new_stored = llvm::StructType::create(*gen.ctx, elements, runtime_name);
        llvm_store_type(new_stored);
        return new_stored;
    }
    return stored;
}

llvm::Type *StructType::llvm_type(Codegen &gen) {
    auto container = variables_container();
    return with_elements_type(gen, container->elements_type(gen), get_runtime_name());
}

llvm::Type *StructType::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *StructType::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    auto container = variables_container();
    return with_elements_type(gen, container->elements_type(gen, values, index), "");
}

llvm::Type *UnionType::llvm_type(Codegen &gen) {
    auto container = variables_container();
    auto largest = container->largest_member();
    if(!largest) {
        gen.error("Couldn't determine the largest member of the union with name " + union_name(), this);
        return nullptr;
    }
    auto stored = llvm_union_get_stored_type();
    if(!stored) {
        std::vector<llvm::Type*> members {largest->llvm_type(gen)};
        if(is_anonymous()) {
            return llvm::StructType::get(*gen.ctx, members);
        }
        stored = llvm::StructType::create(*gen.ctx, members, "union." + union_name());
        llvm_union_type_store(stored);
        return stored;
    }
    return stored;
}

llvm::Type *UnionType::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    auto container = variables_container();
    if(index + 1 < values.size()) {
        auto linked = values[index + 1]->linked_node();
        if(linked) {
            for (auto &member: container->variables) {
                if (member.second == linked) {
                    std::vector<llvm::Type *> struct_type{member.second->llvm_chain_type(gen, values, index + 1)};
                    return llvm::StructType::get(*gen.ctx, struct_type);
                }
            }
        }
    }
    return llvm_type(gen);
}

llvm::Type *VoidType::llvm_type(Codegen &gen) {
    return gen.builder->getVoidTy();
}

llvm::Type* DynamicType::llvm_type(Codegen& gen) {
    return llvm::StructType::get(*gen.ctx, { gen.builder->getPtrTy(), gen.builder->getPtrTy() });
}

llvm::Type* DynamicType::llvm_param_type(Codegen& gen) {
    return gen.builder->getPtrTy();
}

// ------------------------------ Values

llvm::Type *BoolValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt1Ty();
}

llvm::Value *BoolValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return gen.builder->getInt1(value);
}

llvm::Type *CharValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt8Ty();
}

llvm::Value *CharValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return gen.builder->getInt8((int) value);
}

llvm::Type *DoubleValue::llvm_type(Codegen &gen) {
    return gen.builder->getDoubleTy();
}

llvm::Value *DoubleValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return llvm::ConstantFP::get(llvm_type(gen), value);
}

llvm::Type * FloatValue::llvm_type(Codegen &gen) {
    return gen.builder->getFloatTy();
}

llvm::Value * FloatValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return llvm::ConstantFP::get(llvm_type(gen), llvm::APFloat(value));
}

llvm::Type *IntNumValue::llvm_type(Codegen &gen) {
    return gen.builder->getIntNTy(get_num_bits());
}

llvm::Value *IntNumValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return gen.builder->getIntN(get_num_bits(), get_num_value());
}

llvm::Value *NegativeValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return gen.builder->CreateNeg(value->llvm_value(gen));
}

llvm::Value *NotValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return gen.builder->CreateNot(value->llvm_value(gen));
}

llvm::Value* NullValue::null_llvm_value(Codegen &gen) {
    auto ptrType = llvm::PointerType::get(llvm::IntegerType::get(*gen.ctx, 32), 0);
    return llvm::ConstantPointerNull::get(ptrType);
}

llvm::Value *NullValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return null_llvm_value(gen);
}

llvm::Type *StringValue::llvm_type(Codegen &gen) {
    if(is_array) {
        return llvm::ArrayType::get(gen.builder->getInt8Ty(), length);
    } else {
        return gen.builder->getInt8PtrTy();
    }
}

llvm::Value *StringValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    if(is_array) {
        std::vector<llvm::Constant*> arr;
        for(auto c : value) {
            arr.emplace_back(gen.builder->getInt8(c));
        }
        int remaining = length - value.size();
        while(remaining > 0) {
            arr.emplace_back(gen.builder->getInt8('\0'));
            remaining--;
        }
        auto array_type = (llvm::ArrayType*) llvm_type(gen);
        auto initializer = llvm::ConstantArray::get(array_type, arr);
        return new llvm::GlobalVariable(
            *gen.module,
            array_type,
            true,
            llvm::GlobalValue::LinkageTypes::PrivateLinkage,
            initializer
        );
    } else {
        return gen.builder->CreateGlobalStringPtr(value);
    }
}

llvm::AllocaInst *StringValue::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType* expected_type) {
    if(is_array) {
        // when user creates a array of characters, we memcopy the string value to the allocated array
        auto alloc = gen.builder->CreateAlloca(llvm_type(gen), nullptr);
        auto arr = llvm_value(gen, nullptr);
        gen.builder->CreateMemCpy(alloc, llvm::MaybeAlign(), arr, llvm::MaybeAlign(), length);
        return alloc;
    } else {
        return Value::llvm_allocate(gen, identifier, expected_type);
    }
}

llvm::Type *VariableIdentifier::llvm_type(Codegen &gen) {
    return linked->llvm_type(gen);
}

llvm::Type *VariableIdentifier::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &chain, unsigned int index) {
    return linked->llvm_chain_type(gen, chain, index);
}

llvm::FunctionType *VariableIdentifier::llvm_func_type(Codegen &gen) {
    return linked->llvm_func_type(gen);
}

llvm::Value *VariableIdentifier::llvm_pointer(Codegen &gen) {
    return linked->llvm_pointer(gen);
}

llvm::Value *VariableIdentifier::llvm_value(Codegen &gen, BaseType* expected_type) {
    if(linked->value_type() == ValueType::Array) {
        return gen.builder->CreateGEP(llvm_type(gen), llvm_pointer(gen), {gen.builder->getInt32(0), gen.builder->getInt32(0)}, "", gen.inbounds);;
    }
    return linked->llvm_load(gen);
}

bool VariableIdentifier::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    if(parent) {
        return parent->linked_node()->add_child_index(gen, indexes, value);
    }
    return true;
}

bool VariableIdentifier::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return linked->add_child_index(gen, indexes, name);
}

llvm::Value *VariableIdentifier::llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) {
    return linked->llvm_ret_load(gen, returnStmt);
}

llvm::Value *VariableIdentifier::access_chain_value(Codegen &gen, std::vector<ChainValue*> &values, unsigned until, std::vector<std::pair<Value*, llvm::Value*>>& destructibles, BaseType* expected_type) {
    if(linked->as_enum_member() != nullptr) {
        return llvm_value(gen, nullptr);
    } else {
        return ChainValue::access_chain_value(gen, values, until, destructibles, expected_type);
    }
}

llvm::Type *DereferenceValue::llvm_type(Codegen &gen) {
    auto addr = value->create_type(gen.allocator);
    if(addr->kind() == BaseTypeKind::Pointer) {
        return ((PointerType*) (addr))->type->llvm_type(gen);
    } else {
        gen.error("De-referencing a value that is not a pointer " + value->representation(), this);
        return nullptr;
    }
}

llvm::Value *DereferenceValue::llvm_pointer(Codegen& gen) {
    return value->llvm_value(gen);
}

llvm::Value *DereferenceValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return gen.builder->CreateLoad(llvm_type(gen), value->llvm_value(gen), "deref");
}

llvm::Value *Expression::llvm_logical_expr(Codegen &gen, BaseType* firstType, BaseType* secondType) {
    if((operation == Operation::LogicalAND || operation == Operation::LogicalOR)) {
        auto first = firstValue->llvm_value(gen);
        auto current_block = gen.builder->GetInsertBlock();
        llvm::BasicBlock* second_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        if(operation == Operation::LogicalAND) {
            gen.CreateCondBr(first, second_block, end_block);
        } else {
            gen.CreateCondBr(first, end_block, second_block);
        }
        gen.SetInsertPoint(second_block);
        auto second = secondValue->llvm_value(gen);
        gen.CreateBr(end_block);
        gen.SetInsertPoint(end_block);
        auto phi = gen.builder->CreatePHI(gen.builder->getInt1Ty(), 2);
        phi->addIncoming(gen.builder->getInt1(operation == Operation::LogicalOR), current_block);
        phi->addIncoming(second, second_block);
        return phi;
    }
    return nullptr;
}

llvm::Value *Expression::llvm_value(Codegen &gen, BaseType* expected_type) {
    created_type = create_type(gen.allocator);
    auto firstType = firstValue->create_type(gen.allocator);
    auto secondType = secondValue->create_type(gen.allocator);
    auto first_pure = firstType->pure_type();
    auto second_pure = secondType->pure_type();
    replace_number_values(first_pure, second_pure);
    shrink_literal_values(first_pure, second_pure);
    promote_literal_values(first_pure, second_pure);
    firstType = firstValue->create_type(gen.allocator);
    first_pure = firstType->pure_type();
    secondType = secondValue->create_type(gen.allocator);
    second_pure = secondType->pure_type();
    auto logical = llvm_logical_expr(gen, first_pure, second_pure);
    if(logical) return logical;
    return gen.operate(operation, firstValue, secondValue, first_pure, second_pure);
}

// a || b
// if a is true, goto then block
// if a is false, goto second block, if b is true, goto then block otherwise goto end/else block
// a && b
// ((a && b) && b)
// ((a || b) && b)
// ((a && b) || b)
// ((a || b) || b)
// if a is true, goto second block, if b is true, goto then block otherwise goto end/else block
// if a is false, goto end/else block
void Expression::llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block) {
    if((operation == Operation::LogicalAND || operation == Operation::LogicalOR)) {
        llvm::BasicBlock* second_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        if(operation == Operation::LogicalAND) {
            firstValue->llvm_conditional_branch(gen, second_block, otherwise_block);
        } else {
            firstValue->llvm_conditional_branch(gen, then_block, second_block);
        }
        gen.SetInsertPoint(second_block);
        secondValue->llvm_conditional_branch(gen, then_block, otherwise_block);
    } else {
        return Value::llvm_conditional_branch(gen, then_block, otherwise_block);
    }
}

llvm::Type *Expression::llvm_type(Codegen &gen) {
    return known_type()->llvm_type(gen);
}

bool Expression::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return linked_node()->add_child_index(gen, indexes, name);
}

llvm::Type *CastedValue::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Value *CastedValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    auto llvm_val = value->llvm_value(gen);
    auto value_type = value->create_type(gen.allocator);
    if(value_type->kind() == BaseTypeKind::IntN && type->kind() == BaseTypeKind::IntN) {
        auto from_num_type = (IntNType*) value_type;
        auto to_num_type = (IntNType*) type;
        if(from_num_type->num_bits() < to_num_type->num_bits()) {
            if (from_num_type->is_unsigned()) {
                return gen.builder->CreateZExt(llvm_val, to_num_type->llvm_type(gen));
            } else {
                return gen.builder->CreateSExt(llvm_val, to_num_type->llvm_type(gen));
            }
        } else if(from_num_type->num_bits() > to_num_type->num_bits()) {
            return gen.builder->CreateTrunc(llvm_val, to_num_type->llvm_type(gen));
        }
    } else if((value_type->kind() == BaseTypeKind::Float || value_type->kind() == BaseTypeKind::Double) && type->kind() == BaseTypeKind::IntN) {
        if(((IntNType*) type)->is_unsigned()) {
            return gen.builder->CreateFPToUI(llvm_val, type->llvm_type(gen));
        } else {
            return gen.builder->CreateFPToSI(llvm_val, type->llvm_type(gen));
        }
    } else if((value_type->kind() == BaseTypeKind::IntN && (type->kind() == BaseTypeKind::Float || type->kind() == BaseTypeKind::Double))) {
        if(((IntNType*) value_type)->is_unsigned()) {
            return gen.builder->CreateUIToFP(llvm_val, type->llvm_type(gen));
        } else {
            return gen.builder->CreateSIToFP(llvm_val, type->llvm_type(gen));
        }
    } else if(value_type->kind() == BaseTypeKind::Double && type->kind() == BaseTypeKind::Float) {
        return gen.builder->CreateFPTrunc(llvm_val, type->llvm_type(gen));
    } else if(value_type->kind() == BaseTypeKind::Float && type->kind() == BaseTypeKind::Double) {
        return gen.builder->CreateFPExt(llvm_val, type->llvm_type(gen));
    }
//    auto found= gen.casters.find(Codegen::caster_index(value->value_type(), type->kind()));
//    if(found != gen.casters.end()) {
//        return std::unique_ptr<Value>(found->second(&gen, value.get(), type.get()))->llvm_value(gen);
//    }
    return llvm_val;
}

llvm::Type* IsValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt1Ty();
}

llvm::Value* IsValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    bool result = false;
    auto comp_time = get_comp_time_result();
    if(comp_time.has_value()) {
        result = comp_time.value();
    }
    return gen.builder->getInt1(result);
}

bool CastedValue::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return type->linked_node()->add_child_index(gen, indexes, name);
}

llvm::Type *AddrOfValue::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *AddrOfValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return value->llvm_pointer(gen);
}

bool AddrOfValue::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    return value->add_member_index(gen, parent, indexes);
}

bool AddrOfValue::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return value->add_child_index(gen, indexes, name);
}

llvm::Value* RetStructParamValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    if(gen.current_func_type->returnType->value_type() != ValueType::Struct) {
        gen.error("expected current function to have a struct return type for compiler::return_struct", this);
        return nullptr;
    }
    // TODO implicitly returning struct parameter index is hardcoded
    return gen.current_function->getArg(0);
}

llvm::Value* SizeOfValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    auto type = for_type->llvm_type(gen);
    return gen.builder->getInt64(gen.module->getDataLayout().getTypeAllocSize(type));
}

llvm::Value* llvm_load_chain_until(
        Codegen& gen,
        std::vector<ChainValue*>& chain,
        int until,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    if(until >= 0 && until < chain.size()) {
        auto& value = *chain[until];
        if (value.value_type() == ValueType::Pointer) {
            return value.access_chain_value(gen, chain, until, destructibles, nullptr);
        } else {
            return value.access_chain_pointer(gen, chain, destructibles, until);
        }
    } else {
        return nullptr;
    }
}

llvm::Value* llvm_next_value(
        Codegen& gen,
        std::vector<ChainValue*> chain,
        unsigned int index,
        llvm::Value* grandpa_value,
        llvm::Value* parent_value,
        std::vector<std::pair<Value*, llvm::Value*>>& destructibles
) {
    // queue parent for destruction (sounds dark, villainous and evil)
    const auto parent_index = index - 1;
    const auto parent = chain[parent_index];
    if(parent->as_func_call()) {
        destructibles.emplace_back(parent, parent_value);
    }
    const auto current = chain[index];
    const auto curr_func_call = current->as_func_call();
    if(curr_func_call) {
        return curr_func_call->chain_value_with_callee(gen, chain, index, grandpa_value, parent_value, destructibles);
    }
    // taking an index into the parent value like usual
    llvm::Value* value_ref = parent_value;
    std::vector<llvm::Value*> idxList;
    if(!current->add_member_index(gen, parent, idxList)) {
        gen.error("couldn't add member index for next value in llvm_next_value for " + current->representation(), current);
    }
    value_ref = create_gep(gen, chain, parent_index, parent_value, idxList);
    if(current->is_pointer()) {
        // load the pointer and return
        value_ref = gen.builder->CreateLoad(current->llvm_type(gen), value_ref);
    }
    return value_ref;
}

void AccessChain::code_gen(Codegen &gen) {
    auto value = llvm_value(gen, nullptr);
    const auto call = values.back()->as_func_call();
    if(call) {
        auto ret_type = call->create_type(gen.allocator);
        auto linked_struct = ret_type->linked_struct_def();
        if(linked_struct) {
            linked_struct->llvm_destruct(gen, value);
        }
    }
}

llvm::Type *AccessChain::llvm_type(Codegen &gen) {
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(gen.allocator, active);
    auto type = values[values.size() - 1]->llvm_type(gen);
    restore_active_iterations(active);
    return type;
}

llvm::Value *AccessChain::llvm_value(Codegen &gen, BaseType* expected_type, llvm::Value** parent_pointer) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(gen.allocator,active);
    const auto last_ind = values.size() - 1;
    auto& last = values[last_ind];
    llvm::Value* value;
    if(parent_pointer) {
        value = last->access_chain_value(gen, values, last_ind, destructibles, expected_type, *parent_pointer);
    } else {
        value = last->access_chain_value(gen, values, last_ind, destructibles, expected_type);
    }
    restore_active_iterations(active);
    Value::destruct(gen, destructibles);
    return value;
}

llvm::Value *AccessChain::llvm_value(Codegen &gen, BaseType* expected_type) {
    return llvm_value(gen, expected_type, nullptr);
}

llvm::Value *AccessChain::llvm_assign_value(Codegen &gen, Value *lhs) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(gen.allocator, active);
    const auto last_ind = values.size() - 1;
    auto& last = values[last_ind];
    llvm::Value* value;
    value = last->access_chain_assign_value(gen, values, last_ind, destructibles, lhs, nullptr);
    restore_active_iterations(active);
    Value::destruct(gen, destructibles);
    return value;
}

llvm::Value *AccessChain::llvm_pointer(Codegen &gen) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(gen.allocator, active);
    auto value = values[values.size() - 1]->access_chain_pointer(gen, values, destructibles, values.size() - 1);
    restore_active_iterations(active);
    Value::destruct(gen, destructibles);
    return value;
}

llvm::AllocaInst *AccessChain::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType* expected_type) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(gen.allocator, active);
    auto value = values[values.size() - 1]->access_chain_allocate(gen, values, values.size() - 1, expected_type);
    restore_active_iterations(active);
    Value::destruct(gen, destructibles);
    return value;
}

void AccessChain::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(gen.allocator, active);
    values[values.size() - 1]->llvm_destruct(gen, allocaInst);
    restore_active_iterations(active);
}

llvm::FunctionType *AccessChain::llvm_func_type(Codegen &gen) {
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(gen.allocator, active);
    auto result =  values[values.size() - 1]->llvm_func_type(gen);
    restore_active_iterations(active);
    return result;
}

bool AccessChain::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    std::unordered_map<uint16_t, int16_t> active;
    set_generic_iterations(gen.allocator, active);
    auto result = values[values.size() - 1]->add_child_index(gen, indexes, name);
    restore_active_iterations(active);
    return result;
}

bool access_chain_store_in_parent(
    Codegen &gen,
    AccessChain* chain,
    Value *parent,
    llvm::Value *allocated,
    llvm::Type* allocated_type,
    std::vector<llvm::Value *>& idxList,
    unsigned int index,
    BaseType* expected_type
) {
    auto func_call = chain->values[chain->values.size() - 1]->as_func_call();
    if(func_call) {
        auto func_type = func_call->known_function_type();
        if(func_type->returnType->value_type() == ValueType::Struct) {
            auto elem_pointer = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
            std::vector<llvm::Value *> args;
            std::vector<std::pair<Value*, llvm::Value*>> destructibles;
            func_call->llvm_chain_value(gen, args, chain->values, chain->values.size() - 1, destructibles,elem_pointer);
            Value::destruct(gen, destructibles);
            return true;
        }
    }
    return false;
}

unsigned int AccessChain::store_in_struct(
        Codegen &gen,
        Value *parent,
        llvm::Value *allocated,
        llvm::Type *allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType* expected_type
) {
    if(access_chain_store_in_parent(gen, this, (Value*) parent, allocated, allocated_type, idxList, index, expected_type)) {
        return index + 1;
    }
    return Value::store_in_struct(gen, parent, allocated, allocated_type, idxList, index, expected_type);
}

unsigned int AccessChain::store_in_array(
        Codegen &gen,
        Value *parent,
        llvm::Value *allocated,
        llvm::Type *allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType *expected_type
) {
    if(access_chain_store_in_parent(gen, this, parent, allocated, allocated_type, idxList, index, expected_type)) {
        return index + 1;
    }
    return Value::store_in_array(gen, parent, allocated, allocated_type, idxList, index, expected_type);
};

// --------------------------------------- Statements

void ContinueStatement::code_gen(Codegen &gen) {
    gen.CreateBr(gen.current_loop_continue);
}

void ReturnStatement::code_gen(Codegen &gen, Scope *scope, unsigned int index) {
    // before destruction, get the return value
    llvm::Value* return_value = nullptr;
    if(value) {
        if(value->reference() && value->value_type() == ValueType::Struct) {
            // TODO hardcoded the function implicit struct return argument at index 0
            auto dest = gen.current_function->getArg(0);
            auto value_ptr = value->llvm_pointer(gen);
            if(!gen.assign_dyn_obj(value, func_type->returnType, dest, value_ptr)) {
                llvm::MaybeAlign noAlign;
                auto alloc_size = gen.module->getDataLayout().getTypeAllocSize(value->llvm_type(gen));
                gen.builder->CreateMemCpy(dest, noAlign, value_ptr, noAlign, alloc_size);
            }
        } else if(value->as_variant_call()) {
            auto dest = gen.current_function->getArg(0);
            value->as_variant_call()->initialize_allocated(gen, dest);
        } else {
            return_value = value->llvm_ret_value(gen, this);
            if(func_type) {
                auto value_type = value->get_pure_type();
                auto to_type = func_type->returnType->pure_type();
                return_value = gen.implicit_cast(return_value, value_type.get(), to_type);
            }
        }
    }
    // destruction
    if(!gen.has_current_block_ended) {
        int i = gen.destruct_nodes.size() - 1;
        while(i >= 0) {
            gen.destruct_nodes[i]->code_gen_destruct(gen, value);
            i--;
        }
        gen.destroy_current_scope = false;
    }
    // return the return value calculated above
    if (value) {
        gen.CreateRet(return_value);
    } else {
        gen.DefaultRet();
    }
}

void TypealiasStatement::code_gen(Codegen &gen) {

}

void ValueNode::code_gen(Codegen& gen) {
    if(gen.current_assignable) {
        auto llvm_val = value->llvm_value(gen, nullptr);
        if(llvm_val) {
            gen.builder->CreateStore(llvm_val, gen.current_assignable);
        }
    } else {
        gen.error("couldn't assign value node to current assignable", this);
    }
}

llvm::Type *TypealiasStatement::llvm_type(Codegen &gen) {
    return actual_type->llvm_type(gen);
}

void BreakStatement::code_gen(Codegen &gen) {
    if(value) {
        if(gen.current_assignable) {
            gen.builder->CreateStore(value->llvm_value(gen, nullptr), gen.current_assignable);
        } else {
            gen.error("couldn't assign value in break statement", this);
        }
    }
    gen.CreateBr(gen.current_loop_exit);
}

void Scope::code_gen(Codegen &gen, unsigned destruct_begin) {
    for(auto& node : nodes) {
        node->code_gen_declare(gen);
    }
    int i = 0;
    while(i < nodes.size()) {
//        std::cout << "Generating " + std::to_string(i) << std::endl;
        nodes[i]->code_gen(gen, this, i);
//        std::cout << "Success " + std::to_string(i) << " : " << nodes[i]->representation() << std::endl;
        i++;
    }
    if(gen.destroy_current_scope) {
        i = ((int) gen.destruct_nodes.size()) - 1;
        while (i >= (int) destruct_begin) {
            gen.destruct_nodes[i]->code_gen_destruct(gen, nullptr);
            i--;
        }
    } else {
        gen.destroy_current_scope = true;
    }
    auto itr = gen.destruct_nodes.begin() + destruct_begin;
    gen.destruct_nodes.erase(itr, gen.destruct_nodes.end());
}

void Scope::code_gen(Codegen &gen) {
    code_gen(gen, gen.destruct_nodes.size());
}


void InitBlock::code_gen(Codegen &gen) {
    auto self_arg = gen.current_function->getArg(0);
    auto parent_type = container->llvm_type(gen);
    auto is_union = container->kind() == ASTNodeKind::UnionDecl;
    for(auto& init : initializers) {
        auto value = init.second.value;
        auto variable = container->variable_type_index(init.first, init.second.is_inherited_type);
        std::vector<llvm::Value*> idx { gen.builder->getInt32(0) };
        if(init.second.is_inherited_type) {
            auto chain = value->as_access_chain();
            auto val = chain->values.back();
            auto call = val->as_func_call();
            auto called_struct = call->parent_val->linked_node();
            if(call->values.size() == 1) {
                auto struc_val = call->values[0]->as_struct();
                if(struc_val && struc_val->linked_node() == called_struct) {
                    // initializing directly using a struct
                    auto elementPtr = Value::get_element_pointer(gen, parent_type, self_arg, idx, is_union ? 0 : variable.first);
                    struc_val->initialize_alloca(elementPtr, gen, nullptr);
                    continue;
                }
            }
            std::vector<llvm::Value*> args;
            std::vector<std::pair<Value*, llvm::Value*>> destructibles;
            auto elementPtr = Value::get_element_pointer(gen, parent_type, self_arg, idx, is_union ? 0 : variable.first);
            call->llvm_chain_value(
                gen, args, chain->values, chain->values.size(), destructibles, elementPtr, nullptr, nullptr
            );
            Value::destruct(gen, destructibles);
        } else {
            if(gen.requires_memcpy_ref_struct(variable.second, value)) {
                auto elementPtr = Value::get_element_pointer(gen, parent_type, self_arg, idx, is_union ? 0 : variable.first);
                gen.memcpy_struct(value->llvm_type(gen), elementPtr, value->llvm_value(gen, nullptr));
            } else {
                // couldn't move struct
                value->store_in_struct(gen, nullptr, self_arg, parent_type, idx, is_union ? 0 : variable.first, variable.second);
            }
        }
    }
}

void ThrowStatement::code_gen(Codegen &gen) {
    throw std::runtime_error("[UNIMPLEMENTED]");
}

// TODO inline this
bool Codegen::requires_memcpy_ref_struct(BaseType* known_type, Value* value) {
    return value->requires_memcpy_ref_struct(known_type);
}

llvm::Value* Codegen::memcpy_ref_struct(BaseType* known_type, Value* value, llvm::Value* llvm_ptr, llvm::Type* type) {
//    const auto pure = known_type->pure_type();
    if(requires_memcpy_ref_struct(known_type, value)) {
        if(!llvm_ptr) {
            llvm_ptr = builder->CreateAlloca(type, nullptr);
        }
        memcpy_struct(type, llvm_ptr, value->llvm_value(*this, nullptr));
        return llvm_ptr;
    }
    return nullptr;
}

void AssignStatement::code_gen(Codegen &gen) {
    const auto pointer = lhs->llvm_pointer(gen);
    llvm::Value* llvm_value;
    if(assOp == Operation::Assignment) {
        auto& func_type = *gen.current_func_type;
        if(value->is_ref_moved()) {
            const auto is_ref_moved = lhs->is_ref_moved();
            const auto lhs_type = lhs->known_type();
            if(!is_ref_moved) {
                // we must destruct the previous value before we memcpy this value into the pointer, because lhs ref is moved
                // this is set by symbol resolver, to indicate that this value should be destructed before assigning new moved value
                llvm::FunctionType* llvm_func_type;
                llvm::Value* llvm_func_callee;
                auto destr_fn = gen.determine_destructor_for(lhs_type, llvm_func_type, llvm_func_callee);
                if(destr_fn) {
                    gen.builder->CreateCall(llvm_func_type, llvm_func_callee, { pointer });
                }
            }
            if(gen.move_by_memcpy(lhs_type, value, pointer, value->llvm_value(gen))) {
                return;
            }
        }
        if(gen.memcpy_ref_struct(lhs->known_type(), value, pointer, lhs->llvm_type(gen)) != nullptr) {
            return;
        }
    }
    if (assOp == Operation::Assignment) {
        llvm_value = value->llvm_assign_value(gen, lhs);
    } else {
        llvm_value = gen.operate(assOp, lhs, value);
    }
    if(llvm_value) {
        if (!gen.assign_dyn_obj(value, lhs->known_type(), pointer, llvm_value)) {
            gen.builder->CreateStore(llvm_value, pointer);
        }
    }
}

void DestructStmt::code_gen(Codegen &gen) {

    auto created_type = identifier->create_type(gen.allocator);
    auto pure_type = created_type->pure_type();
//    auto pure_type = identifier->get_pure_type();
    bool determined_array = false;
    if(pure_type->kind() == BaseTypeKind::Array) {
        determined_array = true;
    }
    // pointer to array can't be typed directly like []* <--- doesn't work
    // unknown if this code should exist
//    else if(pure_type->kind() == BaseTypeKind::Pointer) {
//        auto child_t = pure_type->known_child_type();
//        if(child_t->kind() == BaseTypeKind::Array) {
//            determined_array = true;
//        }
//    }
    if(!is_array && !determined_array) {
        if(pure_type->kind() != BaseTypeKind::Pointer) {
            gen.error("value given to destruct statement must be of pointer type, value '" + identifier->representation() + "'", this);
            return;
        }
        const auto struct_type = ((PointerType*) pure_type)->type->pure_type();
        auto def = struct_type->get_direct_linked_struct();
        if(!def) {
            return;
        }
        auto destructor = def->destructor_func();
        if(!destructor) {
            return;
        }

        llvm::BasicBlock* destruct_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);

        auto identifier_value = identifier->llvm_value(gen);

        // checking null value
        gen.CheckNullCondBr(identifier_value, end_block, destruct_block);

        // generating code for destructor
        gen.SetInsertPoint(destruct_block);
        std::vector<llvm::Value *> destr_args;
        if (destructor->has_self_param()) {
            destr_args.emplace_back(identifier_value);
        }
        gen.builder->CreateCall(destructor->llvm_func_type(gen), destructor->llvm_pointer(gen), destr_args);
        gen.CreateBr(end_block);

        // end block
        gen.SetInsertPoint(end_block);
        return;
    }

    llvm::Value* arr_size_llvm;
    BaseType* elem_type;
    if(pure_type->kind() == BaseTypeKind::Array) {
        auto arr_type = (ArrayType*) pure_type;
        int array_size = arr_type->array_size;
        elem_type = arr_type->elem_type->pure_type();
        if (!is_array) {
            gen.error("expected brackets '[]' after 'destruct' for destructing an array, with value " + identifier->representation(), this);
            return;
        } else if (array_size != -1 && array_value) {
            gen.error("array size given in brackets '[" + array_value->representation() + "]' is redundant as array size is known to be " + std::to_string(array_size) + " with value " + identifier->representation(), this);
            return;
        } else if (array_size == -1 && !array_value) {
            gen.error("array is size is not known, so it must be provided in brackets for destructing value " + identifier->representation(), this);
            return;
        }
        auto def = elem_type->get_direct_linked_struct();
        if(!def) {
            gen.error("value given to destruct statement, doesn't reference a struct directly, value '" + identifier->representation() + "'", this);
            return;
        }
        arr_size_llvm = array_size != -1 ? gen.builder->getInt32(array_size) : array_value->llvm_value(gen);
    } else if(pure_type->kind() == BaseTypeKind::Pointer) {
        if(!array_value) {
            gen.error("array size is required when destructing a pointer, for destructing array pointer value" + identifier->representation(), this);
            return;
        }
        auto ptr_type = (PointerType*) pure_type;
        elem_type = ptr_type->type->pure_type();
        auto def = ptr_type->type->pure_type()->get_direct_linked_struct();
        if(!def) {
            return;
        }

        arr_size_llvm = array_value->llvm_value(gen, nullptr);
    }

    auto id_value = identifier->llvm_value(gen);

    // we could further free the pointer, using
//        std::vector<llvm::Value*> args;
//        args.emplace_back(structPtr);
//        gen.builder->CreateCall(free_func_linked->llvm_func_type(gen), free_func_linked->llvm_pointer(gen), args);

    gen.destruct(id_value, arr_size_llvm, elem_type, true, [&](llvm::Value*){});

}

llvm::Type* LoopBlock::llvm_type(Codegen &gen) {
    return get_first_broken()->llvm_type(gen);
}

llvm::Value* LoopBlock::llvm_value(Codegen &gen, BaseType *type) {
    code_gen(gen);
    return nullptr;
}

llvm::Value* LoopBlock::llvm_assign_value(Codegen &gen, Value *lhs) {
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = lhs->llvm_pointer(gen);
    code_gen(gen);
    gen.current_assignable = prev_assignable;
    return nullptr;
}

llvm::AllocaInst* LoopBlock::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    auto allocated = gen.builder->CreateAlloca(expected_type ? expected_type->llvm_type(gen) : llvm_type(gen));
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = allocated;
    code_gen(gen);
    gen.current_assignable = prev_assignable;
    return allocated;
}

void LoopBlock::code_gen(Codegen &gen) {
    llvm::BasicBlock* current = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
    llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
    gen.CreateBr(current);
    gen.SetInsertPoint(current);
    gen.loop_body_gen(body, current, end_block);
    gen.CreateBr(current);
    gen.SetInsertPoint(end_block);
}

void ImportStatement::code_gen(Codegen &gen) {

}

llvm::Type *EnumDeclaration::llvm_type(Codegen &gen) {
    return gen.builder->getInt32Ty();
}

// ----------- Members

llvm::Value *EnumMember::llvm_load(Codegen &gen) {
    return gen.builder->getInt32(index);
}

llvm::Type *EnumMember::llvm_type(Codegen &gen) {
    return gen.builder->getInt32Ty();
}

// ------------ Structures

void Namespace::code_gen_declare(Codegen &gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    for(auto& node : nodes) {
        node->code_gen_declare(gen);
    }
}

void Namespace::code_gen(Codegen &gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    for(auto& node : nodes) {
        node->code_gen(gen);
    }
}

void Namespace::code_gen(Codegen &gen, Scope *scope, unsigned int index) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    for(auto& node : nodes) {
        node->code_gen(gen, scope, index);
    }
}

void Namespace::code_gen_external_declare(Codegen &gen) {
    if(has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    for(auto& node : nodes) {
        node->code_gen_external_declare(gen);
    }
}

void Namespace::code_gen_destruct(Codegen &gen, Value *returnValue) {
    throw std::runtime_error("code_gen_destruct on namespace called");
}

void LLVMBackendContext::mem_copy(Value* lhs, Value* rhs) {
    auto& gen = *gen_ptr;
    auto pointer = lhs->llvm_pointer(gen);
    auto val = rhs->llvm_value(gen, nullptr);
    gen.memcpy_struct(rhs->llvm_type(gen), pointer, val);
}