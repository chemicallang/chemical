// Copyright (c) Qinetik 2024.

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include "ast/base/ASTNode.h"
#include "ast/types/AnyType.h"
#include "ast/types/ArrayType.h"
#include "ast/types/BoolType.h"
#include "ast/types/CharType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/IntNType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferencedType.h"
#include "ast/types/StringType.h"
#include "ast/types/StructType.h"
#include "ast/types/VoidType.h"
#include "ast/values/BoolValue.h"
#include "ast/values/CharValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/Expression.h"
#include "ast/values/CastedValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Return.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Break.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Import.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/statements/ThrowStatement.h"

// -------------------- Types

llvm::Type *AnyType::llvm_type(Codegen &gen) const {
    throw std::runtime_error("llvm_type called on any type");
}

llvm::Type *ArrayType::llvm_type(Codegen &gen) const {
    return llvm::ArrayType::get(elem_type->llvm_type(gen), array_size);
}

llvm::Type *ArrayType::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *BoolType::llvm_type(Codegen &gen) const {
    return gen.builder->getInt1Ty();
}

llvm::Type *CharType::llvm_type(Codegen &gen) const {
    return gen.builder->getInt8Ty();
}

llvm::Type *DoubleType::llvm_type(Codegen &gen) const {
    return gen.builder->getDoubleTy();
}

llvm::Type *FloatType::llvm_type(Codegen &gen) const {
    return gen.builder->getFloatTy();
}

llvm::Type *IntNType::llvm_type(Codegen &gen) const {
    auto ty = gen.builder->getIntNTy(number);
    if(!ty) {
        gen.error("Couldn't get intN type for int:" + std::to_string(number));
    }
    return ty;
}

llvm::Type *PointerType::llvm_type(Codegen &gen) const {
    return gen.builder->getPtrTy();
}

llvm::Type *ReferencedType::llvm_type(Codegen &gen) const {
    return linked->llvm_type(gen);
}

llvm::Type *StringType::llvm_type(Codegen &gen) const {
    return gen.builder->getInt8PtrTy();
}

llvm::Type *StructType::llvm_type(Codegen &gen) const {
    std::vector<llvm::Type *> types;
    for (auto &elem: elem_types) {
        types.emplace_back(elem->llvm_type(gen));
    }
    return llvm::StructType::get(*gen.ctx, types);
}

llvm::Type *VoidType::llvm_type(Codegen &gen) const {
    return gen.builder->getVoidTy();
}

// ------------------------------ Values

llvm::Type *BoolValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt1Ty();
}

llvm::Value *BoolValue::llvm_value(Codegen &gen) {
    return gen.builder->getInt1(value);
}

llvm::Type *CharValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt8Ty();
}

llvm::Value *CharValue::llvm_value(Codegen &gen) {
    return gen.builder->getInt8((int) value);
}

llvm::Type *DoubleValue::llvm_type(Codegen &gen) {
    return gen.builder->getDoubleTy();
}

llvm::Value *DoubleValue::llvm_value(Codegen &gen) {
    return llvm::ConstantFP::get(llvm_type(gen), value);
}

llvm::Type * FloatValue::llvm_type(Codegen &gen) {
    return gen.builder->getFloatTy();
}

llvm::Value * FloatValue::llvm_value(Codegen &gen) {
    return llvm::ConstantFP::get(llvm_type(gen), llvm::APFloat(value));
}

llvm::Type *IntNumValue::llvm_type(Codegen &gen) {
    return gen.builder->getIntNTy(get_num_bits());
}

llvm::Value *IntNumValue::llvm_value(Codegen &gen) {
    return gen.builder->getIntN(get_num_bits(), get_num_value());
}

llvm::Value *NegativeValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateNeg(value->llvm_value(gen));
}

llvm::Value *NotValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateNot(value->llvm_value(gen));
}

llvm::Value *NullValue::llvm_value(Codegen &gen) {
    auto ptrType = llvm::PointerType::get(llvm::IntegerType::get(*gen.ctx, 32), 0);
    return llvm::ConstantPointerNull::get(ptrType);
}

llvm::Type *StringValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt8PtrTy();
}

llvm::Value * StringValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateGlobalStringPtr(value);
}

llvm::GlobalVariable * StringValue::llvm_global_variable(Codegen &gen, bool is_const, const std::string &name) {
    if(!is_const) {
        gen.error("Global string variables aren't supported at the moment");
    }
    // TODO global constant string must have type pointer to array
    // because it returns an array pointer, and we must take [0] from it to reach first pointer
    return gen.builder->CreateGlobalString(value, name, 0, gen.module.get());
}

llvm::Type *VariableIdentifier::llvm_type(Codegen &gen) {
    return linked->llvm_type(gen);
}

llvm::FunctionType *VariableIdentifier::llvm_func_type(Codegen &gen) {
    return linked->llvm_func_type(gen);
}

llvm::Value *VariableIdentifier::llvm_pointer(Codegen &gen) {
    return linked->llvm_pointer(gen);
}

llvm::Value *VariableIdentifier::llvm_value(Codegen &gen) {
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

llvm::Value *VariableIdentifier::llvm_ret_value(Codegen &gen, ReturnStatement *returnStmt) {
    return linked->llvm_ret_load(gen, returnStmt);
}

llvm::Value *VariableIdentifier::access_chain_value(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, unsigned until) {
    if(linked->as_enum_member() != nullptr) {
        return llvm_value(gen);
    } else {
        return Value::access_chain_value(gen, values, until);
    }
}

llvm::Type *DereferenceValue::llvm_type(Codegen &gen) {
    auto addr = value->create_type();
    if(addr->kind() == BaseTypeKind::Pointer) {
        return ((PointerType*) (addr.get()))->type->llvm_type(gen);
    } else {
        gen.error("Derefencing a value that is not a pointer " + value->representation());
        return nullptr;
    }
}

llvm::Value *DereferenceValue::llvm_pointer(Codegen& gen) {
    return value->llvm_value(gen);
}

llvm::Value *DereferenceValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateLoad(llvm_type(gen), value->llvm_value(gen), "deref");
}

llvm::Value *Expression::llvm_value(Codegen &gen) {
    auto firstType = firstValue->create_type();
    auto secondType = secondValue->create_type();
    replace_number_values(firstType.get(), secondType.get());
    shrink_literal_values(firstType.get(), secondType.get());
    promote_literal_values(firstType.get(), secondType.get());
    firstType = firstValue->create_type();
    secondType = secondValue->create_type();
    return gen.operate(operation, firstValue.get(), secondValue.get(), firstType.get(), secondType.get());
}

llvm::Type *Expression::llvm_type(Codegen &gen) {
    return create_type()->llvm_type(gen);
}

llvm::Type *CastedValue::llvm_type(Codegen &gen) {
    return type->llvm_type(gen);
}

llvm::Value *CastedValue::llvm_value(Codegen &gen) {
    auto llvm_val = value->llvm_value(gen);
    auto value_type = value->create_type();
    if(value_type->kind() == BaseTypeKind::IntN && type->kind() == BaseTypeKind::IntN) {
        auto from_num_type = (IntNType*) value_type.get();
        auto to_num_type = (IntNType*) type.get();
        if(from_num_type->number < to_num_type->number) {
            if (from_num_type->is_unsigned) {
                return gen.builder->CreateZExt(llvm_val, to_num_type->llvm_type(gen));
            } else {
                return gen.builder->CreateSExt(llvm_val, to_num_type->llvm_type(gen));
            }
        } else if(from_num_type->number > to_num_type->number) {
            return gen.builder->CreateTrunc(llvm_val, to_num_type->llvm_type(gen));
        }
    } else if((value_type->kind() == BaseTypeKind::Float || value_type->kind() == BaseTypeKind::Double) && type->kind() == BaseTypeKind::IntN) {
        if(((IntNType*) type.get())->is_unsigned) {
            return gen.builder->CreateFPToUI(llvm_val, type->llvm_type(gen));
        } else {
            return gen.builder->CreateFPToSI(llvm_val, type->llvm_type(gen));
        }
    } else if((value_type->kind() == BaseTypeKind::IntN && (type->kind() == BaseTypeKind::Float || type->kind() == BaseTypeKind::Double))) {
        if(((IntNType*) type.get())->is_unsigned) {
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

llvm::Type *AddrOfValue::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *AddrOfValue::llvm_value(Codegen &gen) {
    return value->llvm_pointer(gen);
}

void AccessChain::code_gen(Codegen &gen) {
    llvm_value(gen);
}

llvm::Type *AccessChain::llvm_type(Codegen &gen) {
    return values[values.size() - 1]->llvm_type(gen);
}

llvm::Value *AccessChain::llvm_value(Codegen &gen) {
    return values[values.size() - 1]->access_chain_value(gen, values, values.size());
}

llvm::Value *AccessChain::llvm_pointer(Codegen &gen) {
    return values[values.size() - 1]->access_chain_pointer(gen, values, values.size());
}

llvm::AllocaInst *AccessChain::llvm_allocate(Codegen &gen, const std::string &identifier) {
    return values[values.size() - 1]->access_chain_allocate(gen, values, values.size());
}

void AccessChain::llvm_destruct(Codegen &gen, llvm::Value *allocaInst) {
    values[values.size() - 1]->llvm_destruct(gen, allocaInst);
}

llvm::FunctionType *AccessChain::llvm_func_type(Codegen &gen) {
    return values[values.size() - 1]->llvm_func_type(gen);
}

bool AccessChain::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
    return values[values.size() - 1]->add_child_index(gen, indexes, name);
}

// --------------------------------------- Statements

void ContinueStatement::code_gen(Codegen &gen) {
    gen.CreateBr(gen.current_loop_continue);
}

void ReturnStatement::code_gen(Codegen &gen, Scope *scope, unsigned int index) {
    if(!gen.has_current_block_ended) {
        int i = gen.destruct_nodes.size() - 1;
        while(i >= 0) {
            gen.destruct_nodes[i]->code_gen_destruct(gen, value.has_value() ? value->get() : nullptr);
            i--;
        }
        gen.destroy_current_scope = false;
    }
    if (value.has_value()) {
        if(value.value()->reference() && value.value()->value_type() == ValueType::Struct) {
            llvm::MaybeAlign noAlign;
            gen.builder->CreateMemCpy(gen.current_function->getArg(0), noAlign, value.value()->llvm_pointer(gen), noAlign, value.value()->byte_size(gen.is64Bit));
        } else {
            gen.CreateRet(value.value()->llvm_ret_value(gen, this));
        }
    } else {
        gen.CreateRet(nullptr);
    }
}

void TypealiasStatement::code_gen(Codegen &gen) {

}

llvm::Type *TypealiasStatement::llvm_type(Codegen &gen) {
    return to->llvm_type(gen);
}

void BreakStatement::code_gen(Codegen &gen) {
    gen.CreateBr(gen.current_loop_exit);
}

void Scope::code_gen(Codegen &gen) {
    unsigned begin = gen.destruct_nodes.size();
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
        i = nodes.size() - 1;
        while (i >= 0) {
            nodes[i]->code_gen_destruct(gen, nullptr);
            i--;
        }
    } else {
        gen.destroy_current_scope = true;
    }
    auto itr = gen.destruct_nodes.begin() + begin;
    gen.destruct_nodes.erase(itr, gen.destruct_nodes.end());
}

void ThrowStatement::code_gen(Codegen &gen) {
    throw std::runtime_error("[UNIMPLEMENTED]");
}

void AssignStatement::code_gen(Codegen &gen) {
    if (assOp == Operation::Assignment) {
        gen.builder->CreateStore(value->llvm_value(gen), lhs->llvm_pointer(gen));
    } else {
        auto operated = gen.operate(assOp, lhs.get(), value.get());
        gen.builder->CreateStore(operated, lhs->llvm_pointer(gen));
    }
}

void ImportStatement::code_gen(Codegen &gen) {

}

// ----------- Members

llvm::Value *EnumMember::llvm_load(Codegen &gen) {
    return gen.builder->getInt32(index);
}

llvm::Type *EnumMember::llvm_type(Codegen &gen) {
    return gen.builder->getInt32Ty();
}
