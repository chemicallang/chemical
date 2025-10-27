// Copyright (c) Chemical Language Foundation 2025.

#include "compiler/Codegen.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "compiler/llvmimpl.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/TargetParser/Triple.h>
#include "ast/base/ASTNode.h"
#include "ast/types/AnyType.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/RetStructParamValue.h"
#include "ast/values/DynamicValue.h"
#include "ast/types/ArrayType.h"
#include "ast/types/GenericType.h"
#include "ast/types/BoolType.h"
#include "LLVMBackendContext.h"
#include "ast/types/DoubleType.h"
#include "ast/types/FloatType.h"
#include "ast/types/Float128Type.h"
#include "ast/types/DynamicType.h"
#include "ast/types/IntNType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/LinkedType.h"
#include "ast/types/LongDoubleType.h"
#include "ast/types/UnionType.h"
#include "ast/types/StringType.h"
#include "ast/types/StructType.h"
#include "ast/types/VoidType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/types/ComplexType.h"
#include "ast/values/BoolValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/IsValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/NewTypedValue.h"
#include "ast/values/NewValue.h"
#include "ast/values/PlacementNewValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NotValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/structures/Namespace.h"
#include "ast/values/StringValue.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/Expression.h"
#include "ast/values/CastedValue.h"
#include "ast/values/AccessChain.h"
#include "ast/values/IncDecValue.h"
#include "ast/values/ValueNode.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/InValue.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/ExtractionValue.h"
#include "ast/values/EmbeddedValue.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Unreachable.h"
#include "ast/statements/Return.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Break.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Import.h"
#include "ast/values/IfValue.h"
#include "ast/values/SwitchValue.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/InitBlock.h"
#include "ast/statements/ProvideStmt.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ComptimeBlock.h"
#include "ast/values/StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariablesContainer.h"
#include "ast/structures/MembersContainer.h"
#include "ast/statements/ThrowStatement.h"
#include "ast/statements/DeallocStmt.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/LoopBlock.h"
#include "ast/statements/DestructStmt.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/LambdaFunction.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/BlockValue.h"
#include "ast/types/FunctionType.h"
#include "ast/utils/ASTUtils.h"
#include "compiler/lab/TargetData.h"
#include "ast/values/NewValue.h"
#include "ast/values/LoopValue.h"
#include "ast/types/NullPtrType.h"
#include "compiler/cbi/model/ASTBuilder.h"

// -------------------- Types

llvm::Type *AnyType::llvm_type(Codegen &gen) {
    CHEM_THROW_RUNTIME("llvm_type called on any type");
    return nullptr;
}

llvm::Type* NullPtrType::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
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

llvm::Type *DoubleType::llvm_type(Codegen &gen) {
    return gen.builder->getDoubleTy();
}

llvm::Type *FloatType::llvm_type(Codegen &gen) {
    return gen.builder->getFloatTy();
}

llvm::Type *IntNType::llvm_type(Codegen &gen) {
    const auto ty = gen.builder->getIntNTy(num_bits(gen.is64Bit));
#ifdef DEBUG
    if(!ty) {
        CHEM_THROW_RUNTIME("couldn't get int n type for bits");
    }
#endif
    return ty;
}

llvm::Type *Float128Type::llvm_type(Codegen &gen) {
    // TODO store target triple better
    auto targetTriple = llvm::Triple(gen.module->getTargetTriple());
    const auto archType = targetTriple.getArch();
    if (archType == llvm::Triple::x86 || archType == llvm::Triple::x86_64) {
        // On x86 and x86-64, use 80-bit extended precision
        return llvm::Type::getX86_FP80Ty(*gen.ctx);
    } else if (targetTriple.isPPC64()) {
        // On PowerPC 64, use 128-bit floating point
        return llvm::Type::getPPC_FP128Ty(*gen.ctx);
    } else {
        // Default to double if long double is not distinctly supported
        return llvm::Type::getFP128Ty(*gen.ctx);
    }
}

llvm::Type *LongDoubleType::llvm_type(Codegen &gen) {
    auto targetTriple = llvm::Triple(gen.module->getTargetTriple());
    const auto archType = targetTriple.getArch();
    if (archType == llvm::Triple::x86 || archType == llvm::Triple::x86_64) {
        // On x86 and x86-64, use 80-bit extended precision
        return llvm::Type::getX86_FP80Ty(*gen.ctx);
    } else if (targetTriple.isPPC64()) {
        // On PowerPC 64, use 128-bit floating point
        return llvm::Type::getPPC_FP128Ty(*gen.ctx);
    } else {
        return gen.builder->getDoubleTy();
    }
}

llvm::Type *ComplexType::llvm_type(Codegen &gen) {
    // TODO dummy type, we should be compatible with C
    return gen.builder->getFloatTy();
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
    auto type = referenced->llvm_type(gen);
    return type;
}

llvm::Type *GenericType::llvm_param_type(Codegen &gen) {
    const auto gen_struct = referenced->linked;
    auto type = referenced->llvm_param_type(gen);
    return type;
}

llvm::Type *StringType::llvm_type(Codegen &gen) {
    return gen.builder->getInt8PtrTy();
}

llvm::Type *StructType::llvm_type(Codegen &gen) {
    return llvm::StructType::get(*gen.ctx, elements_type(gen));
}

llvm::Type *StructType::llvm_param_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Type *StructType::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    return llvm::StructType::get(*gen.ctx, elements_type(gen, values, index));
}

llvm::Type *UnionType::llvm_type(Codegen &gen) {
    auto largest = largest_member();
    if(!largest) {
        gen.error("Couldn't determine the largest member of the unnamed union", ASTNode::encoded_location());
        return nullptr;
    }
    std::vector<llvm::Type*> members {largest->llvm_type(gen)};
    return llvm::StructType::get(*gen.ctx, members);
}

llvm::Type *UnionType::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) {
    if(index + 1 < values.size()) {
        auto linked = values[index + 1]->linked_node();
        if(linked) {
            for (const auto member: variables()) {
                if (member == linked) {
                    std::vector<llvm::Type *> struct_type{member->llvm_chain_type(gen, values, index + 1)};
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

llvm::Type* CapturingFunctionType::llvm_type(Codegen &gen) {
    return instance_type->llvm_type(gen);
}

llvm::Type* CapturingFunctionType::llvm_param_type(Codegen &gen) {
    return instance_type->llvm_param_type(gen);
}

// ------------------------------ Values

llvm::Type *BoolValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt1Ty();
}

llvm::Value *BoolValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return gen.builder->getInt1(value);
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
    return gen.builder->getIntNTy(get_num_bits(gen.is64Bit));
}

llvm::Value *IntNumValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    return gen.builder->getIntN(get_num_bits(gen.is64Bit), get_num_value());
}

llvm::Value* call_single_param_op_impl(Codegen& gen, FunctionDeclaration* decl, Value* arg) {
    if(decl->params.size() != 1) {
        gen.error(arg) << "expected overloaded operator to have exactly single parameter";
        return nullptr;
    }
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    std::vector<llvm::Value*> args;
    llvm::Instruction* returnedStruct = nullptr;
    // handle secret struct param sent to call inst when call returns a struct
    if(decl->returnType->isStructLikeType()) {
        returnedStruct = gen.builder->CreateAlloca(decl->returnType->llvm_type(gen));
        gen.di.instr(returnedStruct, arg->encoded_location());
        args.emplace_back(returnedStruct);
    }
    args.emplace_back(FunctionCall::arg_value(gen, decl, decl->params[0], arg, 0, destructibles));
    // create call
    auto callInst = gen.builder->CreateCall(decl->llvm_func(gen), args);
    gen.di.instr(callInst, arg->encoded_location());
    Value::destruct(gen, destructibles);
    return returnedStruct ? returnedStruct : callInst;
}

llvm::Value* call_single_param_op_impl(Codegen& gen, MembersContainer* container, Value* first, const chem::string_view& func_name) {
    // find the function
    auto found_child = container->child(func_name);
    if(!found_child) {
        return nullptr;
    }
    // call the function
    if(found_child->kind() == ASTNodeKind::FunctionDecl) {
        const auto func = found_child->as_function_unsafe();
        return call_single_param_op_impl(gen, func, first);
    } else if(found_child->kind() == ASTNodeKind::MultiFunctionNode) {
        const auto multi_node = found_child->as_multi_func_node_unsafe();
        std::vector<Value*> args;
        args.emplace_back(first);
        const auto func = multi_node->func_for_call(args);
        if(!func) return nullptr;
        return call_single_param_op_impl(gen, func, first);
    } else {
        return nullptr;
    }
}

llvm::Value* call_two_param_op_impl(Codegen& gen, FunctionDeclaration* decl, Value* first, Value* second) {
    if(decl->params.size() != 2) {
        gen.error(first) << "expected overloaded operator to have exactly two parameters";
        return nullptr;
    }
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    std::vector<llvm::Value*> args;
    llvm::Instruction* returnedStruct = nullptr;
    // handle secret struct param sent to call inst when call returns a struct
    if(decl->returnType->isStructLikeType()) {
        returnedStruct = gen.builder->CreateAlloca(decl->returnType->llvm_type(gen));
        gen.di.instr(returnedStruct, first->encoded_location());
        args.emplace_back(returnedStruct);
    }
    args.emplace_back(FunctionCall::arg_value(gen, decl, decl->params[0], first, 0, destructibles));
    args.emplace_back(FunctionCall::arg_value(gen, decl, decl->params[1], second, 1, destructibles));
    // create call
    auto callInst = gen.builder->CreateCall(decl->llvm_func(gen), args);
    gen.di.instr(callInst, first->encoded_location());
    Value::destruct(gen, destructibles);
    return returnedStruct ? returnedStruct : callInst;
}

llvm::Value* call_two_param_op_impl(Codegen& gen, MembersContainer* container, Value* first, Value* second, const chem::string_view& func_name) {
    // find the function
    auto found_child = container->child(func_name);
    if(!found_child) {
        return nullptr;
    }
    // call the function
    if(found_child->kind() == ASTNodeKind::FunctionDecl) {
        const auto func = found_child->as_function_unsafe();
        return call_two_param_op_impl(gen, func, first, second);
    } else if(found_child->kind() == ASTNodeKind::MultiFunctionNode) {
        const auto multi_node = found_child->as_multi_func_node_unsafe();
        std::vector<Value*> args;
        args.emplace_back(first);
        args.emplace_back(second);
        const auto func = multi_node->func_for_call(args);
        if(!func) return nullptr;
        return call_two_param_op_impl(gen, func, first, second);
    } else {
        return nullptr;
    }
}

llvm::Value *NegativeValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    // check if operator is overloaded
    const auto valType = value->getType();
    const auto can_node = valType->get_linked_canonical_node(true, false);
    if(can_node) {
        const auto container = can_node->get_members_container();
        if(container) {
            return call_single_param_op_impl(gen, container, value, "neg");
        }
    }
    // normal flow
    const auto llvmValue = value->llvm_value(gen);
    const auto ty = llvmValue->getType();
    if(ty->isFloatingPointTy()) {
        return gen.builder->CreateFNeg(llvmValue);
    } else {
        return gen.builder->CreateNeg(llvmValue);
    }
}

llvm::AllocaInst* NegativeValue::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    // check if operator is overloaded
    const auto valType = value->getType();
    const auto can_node = valType->get_linked_canonical_node(true, false);
    if(can_node) {
        const auto container = can_node->get_members_container();
        if(container) {
            const auto called = call_single_param_op_impl(gen, container, value, "neg");
            if(llvm::isa<llvm::AllocaInst>(called)) {
                return (llvm::AllocaInst*) called;
            } else {
                // basically alloca + store (without llvm_value)
                return Value::llvm_alloca_store(gen, expected_type, called);
            }
        }
    }
    // normal flow
    return Value::llvm_allocate(gen, identifier, expected_type);
}

llvm::Value *NotValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    const auto val_type = value->getType();
    // check operator overloading
    const auto can_node = val_type->get_linked_canonical_node(true, false);
    if(can_node) {
        const auto container = can_node->get_members_container();
        if(container) {
            return call_single_param_op_impl(gen, container, value, "not");
        }
    }
    // normal flow
    const auto val = value->llvm_value(gen);
    const auto type = val->getType();
    if(type->isPointerTy()) {
        return gen.builder->CreateICmpEQ(val, NullValue::null_llvm_value(gen));
    }
#ifdef DEBUG
    if(!type->isIntegerTy()) {
        CHEM_THROW_RUNTIME("only integer / boolean values can be used with not");
    }
#endif
    const auto bitWidth = type->getIntegerBitWidth();
    if(bitWidth != 1) {
        // since its a different integer bitwidth, let's compare with zero as condition
        return gen.builder->CreateICmpEQ(val, gen.builder->getIntN(bitWidth, 0));
    }
    return gen.builder->CreateNot(val);
}

llvm::AllocaInst* NotValue::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    const auto val_type = value->getType();
    // check operator overloading
    const auto can_node = val_type->get_linked_canonical_node(true, false);
    if(can_node) {
        const auto container = can_node->get_members_container();
        if(container) {
            const auto called = call_single_param_op_impl(gen, container, value, "not");
            if(llvm::isa<llvm::AllocaInst>(called)) {
                return (llvm::AllocaInst*) called;
            } else {
                // basically alloca + store (without llvm_value)
                return Value::llvm_alloca_store(gen, expected_type, called);
            }
        }
    }
    // normal flow
    return Value::llvm_allocate(gen, identifier, expected_type);
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

llvm::Value* StringValue::llvm_pointer(Codegen &gen) {
    // TODO reuse the strings by declaring them once
    return gen.builder->CreateGlobalStringPtr(llvm::StringRef(value.data(), value.size()));
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
        // TODO reuse the strings by declaring them once
        return gen.builder->CreateGlobalStringPtr(llvm::StringRef(value.data(), value.size()));
    }
}

llvm::AllocaInst *StringValue::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType* expected_type) {
    if(is_array) {
        // when user creates a array of characters, we memcopy the string value to the allocated array
        const auto alloc = gen.builder->CreateAlloca(llvm_type(gen), nullptr);
        gen.di.instr(alloc, this);
        auto arr = llvm_value(gen, nullptr);
        const auto callInst = gen.builder->CreateMemCpy(alloc, llvm::MaybeAlign(), arr, llvm::MaybeAlign(), length);
        gen.di.instr(callInst, this);
        return alloc;
    } else {
        return Value::llvm_allocate(gen, identifier, expected_type);
    }
}

llvm::Value *IndexOperator::llvm_pointer(Codegen &gen)  {
    // check if operator is overloaded
    const auto can_node = parent_val->getType()->get_linked_canonical_node(true, false);
    if(can_node) {
        const auto container = can_node->get_members_container();
        if(container) {
            return call_two_param_op_impl(gen, container, parent_val, idx, "index");
        }
    }
    // normal flow
    auto pure_type = parent_val->getType()->canonical();
    if(pure_type->is_pointer()) {
        auto parent_value = parent_val->llvm_value(gen, nullptr);
        auto child_type = pure_type->create_child_type(gen.allocator);
        return elem_pointer(gen, child_type->llvm_type(gen), parent_value);
    } else {
        return elem_pointer(gen, parent_val->llvm_type(gen), parent_val->llvm_pointer(gen));
    }
}

llvm::Value *IndexOperator::llvm_value(Codegen &gen, BaseType* expected_type) {
    return Value::load_value(gen, getType(), llvm_type(gen), llvm_pointer(gen), encoded_location());
}

llvm::Type *VariableIdentifier::llvm_type(Codegen &gen) {
    return linked->llvm_type(gen);
}

llvm::Type *VariableIdentifier::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &chain, unsigned int index) {
    return linked->llvm_chain_type(gen, chain, index);
}

llvm::Value *VariableIdentifier::llvm_pointer(Codegen &gen) {
    return linked->llvm_pointer(gen);
}

llvm::Value *VariableIdentifier::llvm_value(Codegen &gen, BaseType* expected_type) {
    const auto linked_type = linked->known_type()->pure_type(gen.allocator);
    if(linked_type->kind() == BaseTypeKind::Array) {
        return gen.builder->CreateGEP(llvm_type(gen), llvm_pointer(gen), {gen.builder->getInt32(0), gen.builder->getInt32(0)}, "", gen.inbounds);;
    }
    return linked->llvm_load(gen, encoded_location());
}

bool VariableIdentifier::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    if(parent) {
        return parent->linked_node()->add_child_index(gen, indexes, value);
    }
    return true;
}

bool VariableIdentifier::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return linked->add_child_index(gen, indexes, name);
}

llvm::Type *DereferenceValue::llvm_type(Codegen &gen) {
    auto addr = value->getType();
    const auto addr_kind = addr->kind();
    if(addr_kind == BaseTypeKind::Pointer) {
        return ((PointerType*) (addr))->type->llvm_type(gen);
    } else if(addr_kind == BaseTypeKind::Reference) {
        return ((ReferenceType*) (addr))->type->llvm_type(gen);
    } else {
        gen.error(this) << "De-referencing a value that is not a pointer " << value->representation();
        return nullptr;
    }
}

llvm::Value *DereferenceValue::llvm_pointer(Codegen& gen) {
    return value->llvm_value(gen);
}

llvm::Value *DereferenceValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    const auto llvm_val = value->llvm_value(gen);
    if(expected_type && expected_type->canonical()->kind() == BaseTypeKind::Reference) {
        return llvm_val;
    }
    const auto loadInst = gen.builder->CreateLoad(llvm_type(gen), llvm_val, "deref");
    gen.di.instr(loadInst, this);
    return loadInst;
}

llvm::Type* ComptimeValue::llvm_type(Codegen &gen) {
    return value->llvm_type(gen);
}

llvm::Value* ComptimeValue::llvm_value(Codegen &gen, BaseType *type) {
    const auto evaluated = evaluate(gen.allocator, &gen.comptime_scope);
    if(evaluated) {
        return evaluated->llvm_value(gen, type);
    } else {
        gen.error(this) << "couldn't evaluate comptime value";
    }
}

llvm::Value *Expression::llvm_logical_expr(Codegen &gen, BaseType* firstType, BaseType* secondType) {
    if((operation == Operation::LogicalAND || operation == Operation::LogicalOR)) {
        auto first = firstValue->llvm_value(gen);
        auto current_block = gen.builder->GetInsertBlock();
        llvm::BasicBlock* second_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);

        const auto firstValType = first->getType();
        if(firstValType->isIntegerTy()) {
            const auto bitWidth = firstValType->getIntegerBitWidth();
            if (bitWidth != 1) {
                first = gen.builder->CreateICmpNE(first, gen.builder->getIntN(bitWidth, 0));
            }
        } else {
            gen.error("value doesn't result in an integer", firstValue);
            return first;
        }

        if(operation == Operation::LogicalAND) {
            gen.CreateCondBr(first, second_block, end_block, encoded_location());
        } else {
            gen.CreateCondBr(first, end_block, second_block, encoded_location());
        }

        gen.SetInsertPoint(second_block);
        auto second = secondValue->llvm_value(gen);

        const auto secondValType = second->getType();
        if(secondValType->isIntegerTy()) {
            const auto bitWidth = secondValType->getIntegerBitWidth();
            if (bitWidth != 1) {
                second = gen.builder->CreateICmpNE(second, gen.builder->getIntN(bitWidth, 0));
            }
        } else {
            gen.error("value doesn't result in an integer", secondValue);
            return first;
        }

        // this is here because the second value maybe a logical expressions and it may create
        // call this method and create two more blocks, in that case we should point at the right block
        // the final block which contains the second evaluated value
        auto final_second = gen.builder->GetInsertBlock();

        gen.CreateBr(end_block, encoded_location());
        gen.SetInsertPoint(end_block);
        auto phi = gen.builder->CreatePHI(gen.builder->getInt1Ty(), 2);
        phi->addIncoming(gen.builder->getInt1(operation == Operation::LogicalOR), current_block);
        // the reason we use the second value, if it came from the second block
        // is because if && then both values have to be true for it to go to second block (true in that case)
        // if || then second value is only calculated if first was true and second value would determine the result of expression
        phi->addIncoming(second, final_second);
        return phi;
    }
    return nullptr;
}

llvm::Value* normal_flow_expr(Codegen& gen, Expression* expr, BaseType* first, BaseType* second) {
    return gen.operate(expr->operation, expr->firstValue, expr->secondValue, first->canonical(), second->canonical());
}

llvm::Value *Expression::llvm_value(Codegen &gen, BaseType* expected_type) {

    auto firstType = firstValue->getType();
    auto secondType = secondValue->getType();
    auto first_pure = firstType->canonical();
    auto second_pure = secondType->canonical();

    // promote literal values is using gen allocator
    // TODO there's probably a better way of doing this
    auto prev_firstValue = firstValue;
    auto prev_secondValue = secondValue;

    replace_number_values(gen.allocator, gen.comptime_scope.typeBuilder, first_pure, second_pure);
    // shrink_literal_values(gen.allocator, first_pure, second_pure);
    // promote_literal_values(gen.allocator, first_pure, second_pure);

    firstType = firstValue->getType();
    first_pure = firstType->canonical();

    secondType = secondValue->getType();
    second_pure = secondType->canonical();

    // logical expressions
    auto logical = llvm_logical_expr(gen, first_pure, second_pure);
    if(logical) return logical;

    // check for operator overloading
    const auto can_node = firstType->get_linked_canonical_node(true, false);
    if(can_node) {
        const auto container = can_node->get_members_container();
        if(container) {
            // operator overloading
            // get operator info
            auto impl_info = operator_impl_info(operation);
            if(impl_info.name.empty()) {
                gen.error("operator cannot be overloaded", this);
                return normal_flow_expr(gen, this, firstType, secondType);
            }
            // call the operator implementation
            const auto called = call_two_param_op_impl(gen, container, firstValue, secondValue, impl_info.name);
            if(!called) {
                gen.error(this) << "couldn't find operator implementation with name '" << impl_info.name << "'";
                return normal_flow_expr(gen, this, firstType, secondType);
            }
            return called;
        }
    }

    // normal flow
    auto value = gen.operate(operation, firstValue, secondValue, first_pure, second_pure);
    firstValue = prev_firstValue;
    secondValue = prev_secondValue;
    return value;

}

llvm::AllocaInst* Expression::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {

    auto firstType = firstValue->getType();
    auto secondType = secondValue->getType();

    // check for operator overloading
    const auto can_node = firstType->get_linked_canonical_node(true, false);
    if(can_node) {
        const auto container = can_node->get_members_container();
        if(container) {
            // operator overloading
            // get operator info
            auto impl_info = operator_impl_info(operation);
            if(impl_info.name.empty()) {
                gen.error("operator cannot be overloaded", this);
                return Value::llvm_allocate(gen, identifier, expected_type);
            }
            // call the operator implementation
            const auto called = call_two_param_op_impl(gen, container, firstValue, secondValue, impl_info.name);
            if(!called) {
                gen.error(this) << "couldn't find operator implementation with name '" << impl_info.name << "'";
                return Value::llvm_allocate(gen, identifier, expected_type);
            }
            if(llvm::isa<llvm::AllocaInst>(called)) {
                return (llvm::AllocaInst*) called;
            } else {
                // basically alloca + store (without llvm_value)
                return Value::llvm_alloca_store(gen, expected_type, called);
            }
        }
    }

    // default llvm_allocate
    return Value::llvm_allocate(gen, identifier, expected_type);

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
    return getType()->llvm_type(gen);
}

bool Expression::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return linked_node()->add_child_index(gen, indexes, name);
}

llvm::Value* IncDecValue::llvm_value(Codegen &gen, BaseType* exp_type) {
    const auto val_type = value->getType();
    // check operator overloading
    const auto can_type = val_type->get_linked_canonical_node(true, false);
    if(can_type) {
        const auto container = can_type->get_members_container();
        if(container) {
            return call_single_param_op_impl(gen, container, value, get_overloaded_func_name());
        }
    }
    // normal flow
    auto type = val_type->pure_type(gen.allocator);
    const auto rhs = new (gen.allocator.allocate<IntNumValue>()) IntNumValue(1, gen.comptime_scope.typeBuilder.getShortType(), encoded_location());
    auto value_pointer = value->llvm_pointer(gen);
    // TODO loading the value after pointer, value is not being loaded using the pointer we have
    auto value_loaded = value->llvm_value(gen);
    const auto op = increment ? Operation::Addition : Operation::Subtraction;
    // automatic de-referencing
    const auto referred = type->pure_type(gen.allocator)->getLoadableReferredType();
    if(referred) {
        value_pointer = value_loaded;
        const auto loadInst = gen.builder->CreateLoad(referred->llvm_type(gen), value_loaded);
        gen.di.instr(loadInst, this);
        value_loaded = loadInst;
        type = referred;
    }
    ShortType shortType;
    const auto result = gen.operate(op, value, rhs, type, &shortType, value_loaded, rhs->llvm_value(gen, nullptr));
    const auto storeInst = gen.builder->CreateStore(result, value_pointer);
    gen.di.instr(storeInst, this);
    return post ? value_loaded : result;
}

llvm::AllocaInst* IncDecValue::llvm_allocate(Codegen& gen, const std::string& identifier, BaseType* expected_type) {
    const auto val_type = value->getType();
    // check operator overloading
    const auto can_type = val_type->get_linked_canonical_node(true, false);
    if(can_type) {
        const auto container = can_type->get_members_container();
        if(container) {
            const auto called = call_single_param_op_impl(gen, container, value, get_overloaded_func_name());
            if(llvm::isa<llvm::AllocaInst>(called)) {
                return (llvm::AllocaInst*) called;
            } else {
                // basically alloca + store (without llvm_value)
                return Value::llvm_alloca_store(gen, expected_type, called);
            }
        }
    }
    // normal flow
    return Value::llvm_allocate(gen, identifier, expected_type);
}

llvm::Type *CastedValue::llvm_type(Codegen &gen) {
    return getType()->llvm_type(gen);
}

llvm::Value* CastedValue::llvm_pointer(Codegen &gen) {
    return value->llvm_pointer(gen);
}

llvm::Value *CastedValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    auto llvm_val = value->llvm_value(gen);
    const auto value_type_real = value->getType();
    const auto value_type = value_type_real->canonical();
    const auto type = getType();
    const auto pure_type = getType()->canonical();
    if(value_type->kind() == BaseTypeKind::IntN && pure_type->kind() == BaseTypeKind::Pointer) {
        // casting integer to pointer
        const auto instr = gen.builder->CreateIntToPtr(llvm_val, pure_type->llvm_type(gen));
        return instr;
    }
    if(value_type->kind() == BaseTypeKind::IntN && pure_type->kind() == BaseTypeKind::IntN) {
        // integer to integer cast
        auto from_num_type = (IntNType*) value_type;
        auto to_num_type = (IntNType*) pure_type;
        const auto from_num_bits = from_num_type->num_bits(gen.is64Bit);
        const auto to_num_bits = to_num_type->num_bits(gen.is64Bit);
        if(from_num_bits < to_num_bits) {
            if (from_num_type->is_unsigned()) {
                return gen.builder->CreateZExt(llvm_val, to_num_type->llvm_type(gen));
            } else {
                return gen.builder->CreateSExt(llvm_val, to_num_type->llvm_type(gen));
            }
        } else if(from_num_bits > to_num_bits) {
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
    } else if(value_type->kind() == BaseTypeKind::Pointer && pure_type->kind() == BaseTypeKind::IntN) {
        // pointer to integer conversion
        return gen.builder->CreatePtrToInt(llvm_val, pure_type->llvm_type(gen));
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
    } else {
        const auto linked = type->get_direct_linked_node();
        if(linked->kind() == ASTNodeKind::VariantMember) {
            const auto mem = linked->as_variant_member_unsafe();
            const auto variant_def = mem->parent();

            // loading the type integer from the variant
            const auto expr_value = value->llvm_value(gen);

            const auto int32Ty = gen.builder->getInt32Ty();

            llvm::Type* def_type;
            if(variant_def->generic_parent != nullptr) {
                // when the variant can be generic
                // user writes val is MyVariant.Some (without any generic args)
                // we don't want to call llvm_type on a generic variant (or its master implementation)
                // without knowing which generic instantiation
                // therefore we'll use a small struct type with a single integer
                def_type = llvm::StructType::get(*gen.ctx, std::initializer_list<llvm::Type*> { int32Ty });
            } else {
                def_type = variant_def->llvm_type(gen);
            }

            std::initializer_list<llvm::Value*> idxList { gen.builder->getInt32(0), gen.builder->getInt32(0) };
            const auto gep = gen.builder->CreateGEP(def_type, expr_value, idxList, "",gen.inbounds);
            const auto loadInst = gen.builder->CreateLoad(int32Ty, gep, "");
            gen.di.instr(loadInst, value);

            // getting the index of the variant member type
            const auto indexVal = gen.builder->getInt32(variant_def->variable_index(mem->name, false));

            // comparing the type integer with the index of variant member type
            if(is_negating) {
                return gen.builder->CreateICmpNE(loadInst, indexVal);
            } else {
                return gen.builder->CreateICmpEQ(loadInst, indexVal);
            }

        }
    }
    return gen.builder->getInt1(result);
}


llvm::Type* NewTypedValue::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value* NewTypedValue::llvm_value(Codegen &gen, BaseType *exp_type) {
    auto& mod = *gen.module;
    auto mallocFn = gen.getMallocFn();
    auto size = mod.getDataLayout().getTypeAllocSize(type->llvm_type(gen));
    auto size_val = gen.builder->getIntN(mallocFn->getArg(0)->getType()->getIntegerBitWidth(), size);
    const auto callInst = gen.builder->CreateCall(mallocFn, { size_val });
    gen.di.instr(callInst, this);
    return callInst;
}

llvm::Type* NewValue::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value* new_func_call(Codegen& gen, FunctionCall* call, llvm::Value* pointer_val) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    std::vector<llvm::Value*> args;
    call->llvm_chain_value(gen, args, destructibles, pointer_val, nullptr, nullptr);
    Value::destruct(gen, destructibles);
    return pointer_val;
}

llvm::Value* NewValue::llvm_value(Codegen &gen, BaseType* exp_type) {

    auto& mod = *gen.module;
    auto mallocFn = gen.getMallocFn();
    // chain and struct value both return the exact type (including accounting for generics so we should use that)
    auto size = mod.getDataLayout().getTypeAllocSize(value->llvm_type(gen));
    auto size_val = gen.builder->getIntN(mallocFn->getArg(0)->getType()->getIntegerBitWidth(), size);
    const auto pointer_val = gen.builder->CreateCall(mallocFn, { size_val });
    gen.di.instr(pointer_val, this);

    switch(value->val_kind()) {
        case ValueKind::StructValue: {
            auto struct_val = value->as_struct_value_unsafe();
            struct_val->initialize_alloca(pointer_val, gen, nullptr);
            return pointer_val;
        }
        case ValueKind::AccessChain: {
            auto chain = value->as_access_chain_unsafe();
            auto last_call = chain->values.back()->as_func_call();
            if(last_call) {
                return new_func_call(gen, last_call, pointer_val);
            }
            break;
        }
        case ValueKind::FunctionCall:
            return new_func_call(gen, value->as_func_call_unsafe(), pointer_val);
        default:
            break;
    }

    gen.error("unknown value given to new value", this);

}

llvm::Type* PlacementNewValue::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value* PlacementNewValue::llvm_value(Codegen &gen, BaseType* exp_type) {
    const auto pointer_val = pointer->llvm_value(gen);
    const auto kind = value->val_kind();
    if(kind == ValueKind::StructValue) {
        auto struct_val = value->as_struct_value_unsafe();
        struct_val->initialize_alloca(pointer_val, gen, nullptr);
        return pointer_val;
    } else if(kind == ValueKind::AccessChain) {
        auto chain = value->as_access_chain_unsafe();
        auto last_call = chain->values.back()->as_func_call();
        if(last_call) {
            std::vector<std::pair<Value*, llvm::Value*>> destructibles;
            std::vector<llvm::Value*> args;
            last_call->llvm_chain_value(gen, args, destructibles, pointer_val, nullptr, nullptr);
            Value::destruct(gen, destructibles);
            return pointer_val;
        }
    }
    gen.error("unknown value given to placement new", this);
}

llvm::Type *AddrOfValue::llvm_type(Codegen &gen) {
    return gen.builder->getPtrTy();
}

llvm::Value *AddrOfValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    const auto struct_value = value->as_struct_value();
    if(struct_value) {
        return struct_value->llvm_allocate(gen, "", nullptr);
    }
    const auto ty = value->getType()->canonical();
    if(ty->kind() == BaseTypeKind::Reference) {
        return value->llvm_value(gen);
    }
    return value->llvm_pointer(gen);
}

bool AddrOfValue::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    return value->add_member_index(gen, parent, indexes);
}

bool AddrOfValue::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return value->add_child_index(gen, indexes, name);
}

llvm::Value* RetStructParamValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    if(!gen.current_func_type->returnType->isStructLikeType()) {
        gen.error("expected current function to have a struct return type for intrinsics::return_struct", this);
        return nullptr;
    }
    // TODO implicitly returning struct parameter index is hardcoded
    return gen.current_function->getArg(0);
}

void generate_in_switch(Codegen& gen, InValue* inValue, llvm::BasicBlock *trueBB, llvm::BasicBlock *falseBB) {

    const auto value = inValue->value;
    auto& values = inValue->values;
    const auto is_negating = inValue->is_negating;

    auto& builder = *gen.builder;

    const auto expr_value = value->llvm_value(gen, nullptr);
    const auto expr_type = value->getType();

    // Create the switch (default: falseBB)
    const auto sw = builder.CreateSwitch(expr_value, is_negating ? trueBB : falseBB, values.size());
    // add cases (casting case-consts to lhs width if needed)
    for (const auto val : values) {

        const auto caseValue = val->llvm_value(gen);
        if(llvm::isa<llvm::ConstantInt>(caseValue)) {
            const auto castedCase = Codegen::implicit_cast_constant((llvm::ConstantInt*) caseValue, expr_type, expr_value->getType());
            sw->addCase(castedCase, is_negating ? falseBB : trueBB);
        } else {
            gen.error("in case value is not a constant", val);
        }

    }

}

void InValue::llvm_conditional_branch(Codegen &gen, llvm::BasicBlock *then_block, llvm::BasicBlock *otherwise_block) {

    generate_in_switch(gen, this, then_block, otherwise_block);

}

llvm::Type *InValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt1Ty();
}

llvm::Value *InValue::llvm_value(Codegen &gen, BaseType* expected_type) {

    auto& builder = *gen.builder;
    auto& ctx = *gen.ctx;
    const auto F = gen.current_function;

    // Make blocks
    const auto trueBB  = llvm::BasicBlock::Create(ctx, "in.match", F);
    const auto falseBB = llvm::BasicBlock::Create(ctx, "in.nomatch", F);
    const auto mergeBB  = llvm::BasicBlock::Create(ctx, "in.merge", F);

    generate_in_switch(gen, this, trueBB, falseBB);

    // trueBB: branch to merge
    gen.SetInsertPoint(trueBB);
    gen.CreateBr(mergeBB, encoded_location());

    // falseBB: branch to merge
    gen.SetInsertPoint(falseBB);
    gen.CreateBr(mergeBB, encoded_location());

    // mergeBB: phi
    gen.SetInsertPoint(mergeBB);
    const auto phi = builder.CreatePHI(builder.getInt1Ty(), 2, "in.res");
    phi->addIncoming(llvm::ConstantInt::getTrue(ctx), trueBB);
    phi->addIncoming(llvm::ConstantInt::getFalse(ctx), falseBB);

    // return the phi
    return phi;


}

llvm::Type* ExtractionValue::llvm_type(Codegen &gen) {
    const auto type = getType();
    return type->llvm_type(gen);
}

llvm::Value* ExtractionValue::llvm_value(Codegen &gen, BaseType *type) {
    switch(extractionKind) {
        case ExtractionKind::LambdaFnPtr:
            if(value->kind() != ValueKind::LambdaFunc) {
                gen.error("expected value to be a lambda function", value);
                return nullptr;
            }
            return value->as_lambda_func_unsafe()->llvm_value_unpacked(gen, nullptr);
        case ExtractionKind::LambdaCapturedPtr: {
            if (value->kind() != ValueKind::LambdaFunc) {
                gen.error("expected value to be a lambda function", value);
                return nullptr;
            }
            const auto lambda = value->as_lambda_func_unsafe();
            if(lambda->captured_struct) {
                return lambda->captured_struct;
            } else {
                return NullValue::null_llvm_value(gen);
            }
        }
        case ExtractionKind::LambdaCapturedDestructor:{
            if (value->kind() != ValueKind::LambdaFunc) {
                gen.error("expected value to be a lambda function", value);
                return nullptr;
            }
            const auto lambda = value->as_lambda_func_unsafe();
            if(lambda->capturedDestructor) {
                return lambda->capturedDestructor;
            } else {
                return NullValue::null_llvm_value(gen);
            }
        }
        case ExtractionKind::SizeOfLambdaCaptured:{
            if (value->kind() != ValueKind::LambdaFunc) {
                gen.error("expected value to be a lambda function", value);
                return nullptr;
            }
            const auto lambda = value->as_lambda_func_unsafe();
            if(lambda->captureList.empty()) {
                return gen.builder->getInt64(0);
            } else {
                const auto capType = lambda->capture_struct_type(gen);
                return gen.builder->getInt64(gen.module->getDataLayout().getTypeAllocSize(capType));
            }
        }
        case ExtractionKind::AlignOfLambdaCaptured: {
            if (value->kind() != ValueKind::LambdaFunc) {
                gen.error("expected value to be a lambda function", value);
                return nullptr;
            }
            const auto lambda = value->as_lambda_func_unsafe();
            if(lambda->captureList.empty()) {
                return gen.builder->getInt64(0);
            } else {
                const auto capType = lambda->capture_struct_type(gen);
                auto align = gen.module->getDataLayout().getABITypeAlign(capType);
                return gen.builder->getInt64(align.value());
            }
        }
        case ExtractionKind::ReinterpretLLVMValue:
            return ((llvm::Value*) value);
    }
}

llvm::Type* EmbeddedNode::llvm_type(Codegen &gen) {
    return known_type()->llvm_type(gen);
}

llvm::Value* EmbeddedNode::llvm_pointer(Codegen &gen) {
    ASTBuilder builder(&gen.allocator, gen.comptime_scope.typeBuilder);
    const auto replacement_fn = gen.binder.findHook(name, CBIFunctionType::ReplacementNode);
    if(!replacement_fn) {
        gen.error(this) << "couldn't find replacement function for embedded node with name '" << name << "'";
        return nullptr;
    }
    const auto repl = ((EmbeddedNodeReplacementFunc) replacement_fn)(&builder, this);
    if(repl) {
        return repl->llvm_pointer(gen);
    } else {
        gen.error(this) << "couldn't replace embedded node with name '" << name << "'";
    }
}

llvm::Type* EmbeddedValue::llvm_type(Codegen &gen) {
    const auto type = getType();
    return type->llvm_type(gen);
}

Value* replacement_value(EmbeddedValue* value, Codegen& gen) {
    ASTBuilder builder(&gen.allocator, gen.comptime_scope.typeBuilder);
    const auto replacement_fn = gen.binder.findHook(value->name, CBIFunctionType::ReplacementValue);
    if(!replacement_fn) {
        gen.error(value) << "couldn't find replacement function for embedded value with name '" << value->name << "'";
        return nullptr;
    }
    const auto repl = ((EmbeddedValueReplacementFunc) replacement_fn)(&builder, value);
    if(!repl) {
        gen.error(value) << "couldn't replace embedded value with name '" << value->name << "'";
    }
    return repl;
}

llvm::Value* EmbeddedValue::llvm_pointer(Codegen &gen) {
    const auto repl = replacement_value(this, gen);
    if(!repl) return nullptr;
    repl->llvm_pointer(gen);
}

llvm::Value* EmbeddedValue::llvm_value(Codegen &gen, BaseType *type) {
    const auto repl = replacement_value(this, gen);
    if(!repl) return nullptr;
    return repl->llvm_value(gen);
}

llvm::Type* SizeOfValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt64Ty();
}

llvm::Value* SizeOfValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    auto type = for_type->removeReferenceFromType()->llvm_type(gen);
    return gen.builder->getInt64(gen.module->getDataLayout().getTypeAllocSize(type));
}

llvm::Type* AlignOfValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt64Ty();
}

llvm::Value* AlignOfValue::llvm_value(Codegen &gen, BaseType* expected_type) {
    auto type = for_type->removeReferenceFromType()->llvm_type(gen);
    auto align = gen.module->getDataLayout().getABITypeAlign(type);
    return gen.builder->getInt64(align.value());
}

llvm::Type *AccessChain::llvm_type(Codegen &gen) {
    auto type = values[values.size() - 1]->llvm_type(gen);
    return type;
}

llvm::Value *AccessChain::llvm_value(Codegen &gen, BaseType* expected_type) {
    if(values.front()->kind() == ValueKind::Identifier) {
        const auto id = values.front()->as_identifier_unsafe();
        // when user access a value in a comptime global variable/constant
        // like def.windows (def is comptime, windows is a struct member)
        if(id->linked->kind() == ASTNodeKind::VarInitStmt && id->linked->as_var_init_unsafe()->is_comptime()) {
            const auto evaluated = evaluated_value(gen.comptime_scope);
            if(evaluated) {
                return evaluated->llvm_value(gen);
            } else {
                gen.error("couldn't evaluate the value at compile time", this);
                return gen.builder->getInt32(0);
            }
        }
    }
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    const auto last_ind = values.size() - 1;
    const auto last = values[last_ind];
    const auto value = last->access_chain_value(gen, values, last_ind, destructibles, expected_type);
    Value::destruct(gen, destructibles);
    return value;
}

void AccessChain::llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    const auto last_ind = values.size() - 1;
    auto& last = values[last_ind];
    last->access_chain_assign_value(gen, this, last_ind, destructibles, lhsPtr, lhs, nullptr);
    Value::destruct(gen, destructibles);
}

llvm::Value *AccessChain::llvm_pointer(Codegen &gen) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    auto value = values[values.size() - 1]->access_chain_pointer(gen, values, destructibles, values.size() - 1);
    Value::destruct(gen, destructibles);
    return value;
}

llvm::AllocaInst *AccessChain::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType* expected_type) {
    std::vector<std::pair<Value*, llvm::Value*>> destructibles;
    auto value = values[values.size() - 1]->access_chain_allocate(gen, values, values.size() - 1, expected_type);
    Value::destruct(gen, destructibles);
    return value;
}

bool AccessChain::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    auto result = values[values.size() - 1]->add_child_index(gen, indexes, name);
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
        return func_call->store_in_parent(gen, allocated, allocated_type, idxList, index);
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

inline void gen_BlockValue_scope(Codegen& gen, BlockValue* blockVal) {
    if(blockVal->has_code_gen_scope) {
        return;
    }
    blockVal->scope.code_gen(gen);
}

llvm::AllocaInst* BlockValue::llvm_allocate(
        Codegen& gen,
        const std::string& identifier,
        BaseType* expected_type
) {
    gen_BlockValue_scope(gen, this);
    return calculated_value->llvm_allocate(gen, identifier, expected_type);
}

unsigned int BlockValue::store_in_struct(
        Codegen& gen,
        Value* parent,
        llvm::Value* allocated,
        llvm::Type* allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType* expected_type
) {
    gen_BlockValue_scope(gen, this);
    return calculated_value->store_in_struct(gen, parent, allocated, allocated_type, idxList, index, expected_type);
}

unsigned int BlockValue::store_in_array(
        Codegen& gen,
        Value* parent,
        llvm::Value* allocated,
        llvm::Type* allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType* expected_type
) {
    gen_BlockValue_scope(gen, this);
    return calculated_value->store_in_array(gen, parent, allocated, allocated_type, idxList, index, expected_type);
}

llvm::Value* BlockValue::llvm_pointer(Codegen& gen) {
    gen_BlockValue_scope(gen, this);
    return calculated_value->llvm_pointer(gen);
}

llvm::Value* BlockValue::llvm_value(Codegen& gen, BaseType* type) {
    gen_BlockValue_scope(gen, this);
    return calculated_value->llvm_value(gen);
}

void BlockValue::llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block) {
    gen_BlockValue_scope(gen, this);
    calculated_value->llvm_conditional_branch(gen, then_block, otherwise_block);
}

void dyn_initialize(Codegen& gen, DynamicValue* dyn_value, llvm::Value* fat_ptr) {

    const auto value = dyn_value->value;
    const auto type = dyn_value->getInterfaceType();

    // get the object pointer
    llvm::Value* ptr;
    if(value->kind() == ValueKind::StructValue) {
        ptr = value->llvm_value(gen);
    } else {
        ptr = value->llvm_pointer(gen);
    };

    // get the interface
    const auto inter_node = type->get_direct_linked_canonical_node();
    if(!inter_node || inter_node->kind() != ASTNodeKind::InterfaceDecl) {
        gen.error("couldn't get interface declaration", dyn_value);
        return;
    }
    const auto interface = inter_node->as_interface_def_unsafe();

    // get the impl decl
    const auto impl_node = value->getType()->get_direct_linked_canonical_node();
    if(!impl_node || impl_node->kind() != ASTNodeKind::StructDecl) {
        gen.error("couldn't get implementation declaration", dyn_value);
        return;
    }
    const auto impl_decl = impl_node->as_struct_def_unsafe();

    // get the vtable pointer
    const auto vtable_ptr = interface->llvm_global_vtable(gen, impl_decl);

    // assign the dynamic object
    gen.assign_dyn_obj(fat_ptr, ptr, vtable_ptr, dyn_value->encoded_location());

}

llvm::Value* DynamicValue::llvm_pointer(Codegen &gen) {
    // allocating the fat pointer
    const auto fat_ptr = (llvm::AllocaInst*) gen.llvm.CreateAlloca(gen.fat_pointer_type(), encoded_location());
    dyn_initialize(gen, this, fat_ptr);
    return fat_ptr;
}

llvm::Value* DynamicValue::llvm_ret_value(Codegen &gen, Value *returnValue) {
    // TODO make sure this argument corresponds to the struct
    // TODO hardcoding the struct return index
    auto structPassed = gen.current_function->getArg(0);
    dyn_initialize(gen, this, structPassed);
    return nullptr;
}

void DynamicValue::llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) {
    dyn_initialize(gen, this, lhsPtr);
}

llvm::Value* DynamicValue::llvm_value(Codegen &gen, BaseType *expected_type) {
    return llvm_pointer(gen);
}

llvm::AllocaInst* DynamicValue::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    return (llvm::AllocaInst*) llvm_pointer(gen);
}

unsigned int DynamicValue::store_in_struct(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) {
    dyn_initialize(gen, this, allocated);
    return index + 1;
}

unsigned int DynamicValue::store_in_array(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) {
    dyn_initialize(gen, this, allocated);
    return index + 1;
}

// --------------------------------------- Statements

void ContinueStatement::code_gen(Codegen &gen) {
    gen.CreateBr(gen.current_loop_continue, encoded_location());
}

void UnreachableStmt::code_gen(Codegen &gen) {
    gen.CreateUnreachable(encoded_location());
}

void Codegen::destruct(Destructible& destructible, SourceLocation location) {
    auto& gen = *this;
    const auto node = destructible.getInitializer();
    if(destructible.kind == DestructibleKind::Single) {
        destructible.container->llvm_destruct(gen, destructible.pointer, location);
    } else {
        destruct(destructible.pointer, destructible.array.arrSize, destructible.array.elem_type, location);
    }
}

// this just checks whether the node being destructed is being returned
// in that case we should not destruct the node
bool should_destruct_node(ASTNode* node, Value* returnValue) {
    // TODO: stop using linked node here
    switch(node->kind()) {
        case ASTNodeKind::VarInitStmt:{
            const auto init = node->as_var_init_unsafe();
            if(returnValue && returnValue->linked_node() == node) {
                return false;
            }
            return true;
        }
        case ASTNodeKind::FunctionParam:{
            const auto param = node->as_func_param_unsafe();
            if(returnValue && returnValue->linked_node() == node) {
                return false;
            }
            return true;
        }
        case ASTNodeKind::StructMember: {
            if(returnValue && returnValue->linked_node() == node) {
                return false;
            }
            return true;
        }
        default:
#ifdef DEBUG
            CHEM_THROW_RUNTIME("unknown node, in should_destruct node");
#endif
            return false;
    }
}

void Codegen::conditional_destruct(
        Destructible& pair,
        Value* returnValue,
        SourceLocation location
) {
    const auto initializer = pair.getInitializer();
    if(!should_destruct_node(initializer, returnValue)) {
        return;
    }

    const auto dropFlag = pair.getDropFlag();
    auto& gen = *this;
    if(dropFlag) {

        // we must check the destruct flag before destruction
        const auto loadIns = gen.builder->CreateLoad(gen.builder->getInt1Ty(), dropFlag);
        gen.di.instr(loadIns, location);

        // create a conditional block, for destruction
        const auto destructBlock = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
        const auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);

        // if the condition is true, we destruct
        gen.CreateCondBr(loadIns, destructBlock, endBlock, location);

        // destruction code
        gen.SetInsertPoint(destructBlock);
        destruct(pair, location);
        gen.CreateBr(endBlock, location);

        // generate next code in a other blocks
        gen.SetInsertPoint(endBlock);

    } else {
        // there's no runtime flag, so we just destruct it
        destruct(pair, location);
    }
}

void destruct_current_scope(Codegen& gen, Value* returnValue, SourceLocation location) {
    if(!gen.has_current_block_ended) {
        const auto func_type = gen.current_func_type;
        VariableIdentifier temp_id("", location, false);
        int i = gen.destruct_nodes.size() - 1;
        while(i >= 0) {
            auto& nodePair = gen.destruct_nodes[i];
            // before we send this node for destruction, we must emit an error
            // if it's nested member has been moved somewhere, because we canot
            temp_id.linked = nodePair.getInitializer();
            if(func_type->find_moved_access_chain(&temp_id) == nullptr) {
                gen.conditional_destruct(nodePair, returnValue, location);
            } else {
                gen.error("cannot destruct uninit value at return because it's nested member has been moved, please use std::mem::replace or reinitialize the nested member, or use wrappers like Option", nodePair.getInitializer()->encoded_location(), location);
            }
            i--;
        }
        gen.destroy_current_scope = false;
    }
}

void Codegen::writeReturnStmtFor(Value* value, SourceLocation location) {

    auto& gen = *this;

    const auto func_type = gen.current_func_type;

    if(value) {
        // replace value with call to implicit constructor if there is one
        const auto func = func_type->as_function();
        if (!(func && func->is_constructor_fn())) {
            const auto implicit = func_type->returnType->implicit_constructor_for(value);
            if (implicit && implicit != func_type && (func && func->parent() != implicit->parent())) {
                value = call_with_arg(implicit, value, func_type->returnType, gen.allocator, gen);
            }
        }
    }

    // before destruction, get the return value
    llvm::Value* return_value = nullptr;
    if(value) {
        const auto canonical_return_type = func_type->returnType->canonical();
        const auto value_canonical_type = value->getType()->canonical();
        if(canonical_return_type->kind() == BaseTypeKind::Reference && value_canonical_type->kind() != BaseTypeKind::Reference) {
            return_value = value->llvm_pointer(gen);
        } else if((value->kind() != ValueKind::StructValue && value->getType()->isStructLikeType())) {
            // TODO hardcoded the function implicit struct return argument at index 0
            auto dest = gen.current_function->getArg(0);
            auto value_ptr = value->llvm_pointer(gen);
            llvm::MaybeAlign noAlign;
            auto alloc_size = gen.module->getDataLayout().getTypeAllocSize(func_type->returnType->llvm_type(gen));
            const auto memCpyInst = gen.builder->CreateMemCpy(dest, noAlign, value_ptr, noAlign, alloc_size);
            gen.di.instr(memCpyInst, location);
        } else {
            return_value = value->llvm_ret_value(gen, value);
            if(return_value && func_type) {
                auto value_type = value->getType()->canonical();
                auto to_type = func_type->returnType->pure_type(gen.allocator);

                const auto mutated = gen.mutate_capturing_function(to_type, value);
                if(mutated) {
                    auto dest = gen.current_function->getArg(0);
                    llvm::MaybeAlign noAlign;
                    auto alloc_size = gen.module->getDataLayout().getTypeAllocSize(func_type->returnType->llvm_type(gen));
                    const auto memCpyInst = gen.builder->CreateMemCpy(dest, noAlign, mutated, noAlign, alloc_size);
                    gen.di.instr(memCpyInst, location);
                    return_value = nullptr;
                } else {

                    // automatic dereference if required
                    const auto derefType = value_type->getAutoDerefType(to_type);
                    if (derefType) {
                        const auto loadInst = gen.builder->CreateLoad(derefType->llvm_type(gen), return_value);
                        gen.di.instr(loadInst, value);
                        return_value = loadInst;
                    } else {

                        // implicit cast to value that's required
                        return_value = gen.implicit_cast(return_value, to_type, to_type->llvm_type(gen));

                    }

                }

            }
        }
    }

    // destruction of current scope
    destruct_current_scope(gen, value, location);

    // return the return value calculated above
    if (value) {
        gen.CreateRet(return_value, location);
    } else {
        gen.DefaultRet(location);
    }
}

void ReturnStatement::code_gen(Codegen &gen, Scope *scope, unsigned int index) {
    gen.writeReturnStmtFor(value, encoded_location());
}

void TypealiasStatement::code_gen(Codegen &gen) {

}

void ValueNode::code_gen(Codegen& gen) {
    if(gen.current_assignable.second) {
        value->llvm_assign_value(gen, gen.current_assignable.second, gen.current_assignable.first);
    } else {
        gen.error("couldn't assign value node to current assignable", this);
    }
}

llvm::Type *TypealiasStatement::llvm_type(Codegen &gen) {
    return actual_type->llvm_type(gen);
}

llvm::Type* TypealiasStatement::llvm_param_type(Codegen &gen) {
    return actual_type->llvm_param_type(gen);
}

void BreakStatement::code_gen(Codegen &gen) {
    if(value) {
        auto& assignable = gen.current_assignable;
        if(assignable.second) {
            value->llvm_assign_value(gen, assignable.second, assignable.first);
        } else {
            gen.error("couldn't assign value in break statement", this);
        }
    }
    gen.CreateBr(gen.current_loop_exit, encoded_location());
}

void BaseType::llvm_destruct(Codegen& gen, llvm::Value* pointer, SourceLocation location) {
    switch (kind()) {
        case BaseTypeKind::Linked:
            as_linked_type_unsafe()->linked->llvm_destruct(gen, pointer, location);
            break;
        case BaseTypeKind::Generic:
            as_generic_type_unsafe()->referenced->linked->llvm_destruct(gen, pointer, location);
            break;
        case BaseTypeKind::Array: {
            const auto arr_type = as_array_type_unsafe();
            if (arr_type->elem_type->kind() == BaseTypeKind::Linked || arr_type->elem_type->kind() == BaseTypeKind::Generic) {
                gen.destruct(pointer, arr_type->get_array_size(), arr_type->elem_type, location);
            }
            break;
        }
        default:
            break;
    }
}

llvm::Value* Codegen::find_drop_flag(ASTNode* node) {
    for(auto& pair : destruct_nodes) {
        if(pair.getInitializer() == node) {
            return pair.getDropFlag();
        }
    }
    return nullptr;
}

bool Codegen::set_drop_flag_for_node(ASTNode* node, bool flag, SourceLocation loc) {
    switch (node->kind()) {
        case ASTNodeKind::VarInitStmt:
        case ASTNodeKind::FunctionParam:{
            const auto ref = find_drop_flag(node);
            if(ref) {
                const auto instr = builder->CreateStore(builder->getInt1(flag), ref);
                di.instr(instr, loc);
                return true;
            } else {
                return false;
            }
        }
        default:
            return false;
    }
}

bool Value::set_drop_flag_for_ref(Codegen& gen, bool flag) {
    const auto value = this;
    switch(value->kind()) {
        case ValueKind::AccessChain: {
            if(value->as_access_chain_unsafe()->values.size() == 1) {
                const auto chain = value->as_access_chain_unsafe();
                const auto first = chain->values.front();
                if(first->kind() == ValueKind::Identifier && (chain->is_moved() || first->as_identifier_unsafe()->is_moved)) {
                    return gen.set_drop_flag_for_node(first->as_identifier_unsafe()->linked, flag, first->encoded_location());
                } else {
                    return true;
                }
            } else {
                return !value->as_access_chain_unsafe()->is_moved();
            }
        }
        case ValueKind::Identifier: {
            if(value->as_identifier_unsafe()->is_moved) {
                return gen.set_drop_flag_for_node(value->as_identifier_unsafe()->linked, flag, value->encoded_location());
            } else {
                return true;
            }
        }
        default:
            return true;
    }
}

void Value::llvm_destruct(Codegen& gen, llvm::Value* allocaInst) {
    const auto type = getType();
    type->llvm_destruct(gen, allocaInst, encoded_location());
}

void ASTNode::code_gen_destruct(Codegen &gen, Value* returnValue, SourceLocation location) {
    switch(kind()) {
        case ASTNodeKind::VarInitStmt:{
            const auto init = as_var_init_unsafe();
            if(returnValue && returnValue->linked_node() == this) {
                return;
            }
            const auto type = init->known_type();
            type->llvm_destruct(gen, init->llvm_ptr, location);
            break;
        }
        case ASTNodeKind::FunctionParam:{
            const auto param = as_func_param_unsafe();
            if(!(returnValue && returnValue->linked_node() == this)) {
                param->type->llvm_destruct(gen, gen.current_function->getArg(param->calculate_c_or_llvm_index(gen.current_func_type)), location);
            }
            break;
        }
        default:
            return;
    }
}

void Scope::destruct_current_scope(Codegen& gen, unsigned destruct_begin) {
    const auto func_type = gen.current_func_type;
    // VariableIdentifier temp_id("", encoded_location());
    int i = ((int) gen.destruct_nodes.size()) - 1;
    while (i >= (int) destruct_begin) {
        auto& nodePair = gen.destruct_nodes[i];
        // TODO use the location that represents the scope end
        // we don't need to check if a nested member has been moved
        // because we don't allow moving a nested member
        // temp_id.linked = nodePair.getInitializer();
        // if(func_type->find_moved_access_chain(&temp_id) == nullptr) {
            gen.conditional_destruct(nodePair, nullptr, encoded_location());
        // } else {
        //     gen.error("cannot destruct uninit value at scope end because it's nested member has been moved, please use std::mem::replace or reinitialize the nested member, or use wrappers like Option", nodePair.getInitializer(), this);
        // }
        i--;
    }
}

llvm::Value* Scope::code_gen_value_scope(Codegen& gen, bool allocate, unsigned destruct_begin) {
    if(nodes.size() > 1) {
        int i = 0;
        // do not do the last one
        const auto total = (nodes.size() - 1);
        while (i < total) {
            nodes[i]->code_gen(gen, this, i);
            i++;
        }
    }
    llvm::Value* result;
    // lets do the last one
    const auto last = nodes.back();
    switch(last->kind()) {
        case ASTNodeKind::ValueNode: {
            const auto value = last->as_value_node_unsafe()->value;
            if (allocate) {
                result = value->llvm_allocate(gen, "", nullptr);
            } else {
                result = value->llvm_value(gen);
            }
            break;
        }
        case ASTNodeKind::IfStmt: {
            auto& stmt = *last->as_if_stmt_unsafe();
            result = IfValue::llvm_value(gen, stmt, allocate);
            break;
        }
        case ASTNodeKind::SwitchStmt: {
            auto& stmt = *last->as_switch_stmt_unsafe();
            result = SwitchValue::llvm_value(gen, stmt, allocate);
            break;
        }
        default:
            last->code_gen(gen, this, nodes.size() - 1);
            result = nullptr;
            break;
    }
    if(gen.destroy_current_scope) {
        destruct_current_scope(gen, destruct_begin);
    } else {
        gen.destroy_current_scope = true;
    }
    auto itr = gen.destruct_nodes.begin() + destruct_begin;
    gen.destruct_nodes.erase(itr, gen.destruct_nodes.end());
    return result;
}

void Scope::code_gen_no_scope(Codegen &gen, unsigned destruct_begin) {
    int i = 0;
    while(i < nodes.size()) {
        nodes[i]->code_gen(gen, this, i);
        i++;
    }
    if(gen.destroy_current_scope) {
        if(gen.has_current_block_ended) {
            // why ? so user can write as last statement an if that returns in both branches (body, else)
            // even though both have returned, we start calling destructors here, which causes llvm to crash
            // because we're calling destructors after the return, so we avoid that, since return statement
            // automatically calls destructors
            return;
        }
        destruct_current_scope(gen, destruct_begin);
    } else {
        gen.destroy_current_scope = true;
    }
    auto itr = gen.destruct_nodes.begin() + destruct_begin;
    gen.destruct_nodes.erase(itr, gen.destruct_nodes.end());
}

void Scope::code_gen(Codegen &gen, unsigned destruct_begin) {
    // NOT THE BEST WAY TO CHECK
    // if current function is null, it means the scope is not generating inside a function (contains top level declarations)
    // or the current function hasn't been set prior to generating the body of the function
    if(gen.current_function != nullptr) {
        gen.di.start_scope(encoded_location());
        code_gen_no_scope(gen, destruct_begin);
        gen.di.end_scope();
    } else {
#ifdef DEBUG
        if(destruct_begin != 0) {
            // this happens when the scope contains top level declarations (top level var init)
            // or it can also happen when destruct_nodes contain top level variables (to destruct)
            CHEM_THROW_RUNTIME("cannot destruct nodes while current function does not exist");
        }
#endif
        int i = 0;
        while(i < nodes.size()) {
            nodes[i]->code_gen(gen, this, i);
            i++;
        }
    }
}

void Scope::code_gen(Codegen &gen) {
    code_gen(gen, gen.destruct_nodes.size());
}

void Scope::gen_declare_top_level(Codegen &gen) {
    for(const auto node : nodes) {
        node->code_gen_declare(gen);
    }
}

void Scope::external_declare_top_level(Codegen &gen) {
    for(const auto node : nodes) {
        node->code_gen_external_declare(gen);
    }
}

void ProvideStmt::code_gen(Codegen &gen) {
    const auto val = value->llvm_value(gen, nullptr);
    put_in(gen.implicit_args, val, &gen, [](ProvideStmt* stmt, void* data) {
        stmt->body.code_gen(*((Codegen*) data));
    });
}

void ComptimeBlock::code_gen(Codegen& gen) {
    gen.comptime_scope.interpret(&body);
}

void InitBlock::code_gen(Codegen &gen) {
    const auto container = getContainer();
    auto self_arg = gen.current_function->getArg(0);
    auto parent_type = container->llvm_type(gen);
    auto is_union = container->kind() == ASTNodeKind::UnionDecl;
    for(auto& init : initializers) {
        auto value = init.second.value;
        auto variable = container->variable_type_index(init.first, true);
        std::vector<llvm::Value*> idx { gen.builder->getInt32(0) };
        if(container->is_one_of_inherited_type(variable.second)) {
            auto chain = value->as_access_chain();
            auto val = chain->values.back();
            auto call = val->as_func_call();
            auto called_struct = call->parent_val->linked_node();
            if(call->values.size() == 1) {
                auto struc_val = call->values[0]->as_struct_value();
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
            call->llvm_chain_value(gen, args, destructibles, elementPtr, nullptr, nullptr);
            Value::destruct(gen, destructibles);
        } else {
//            if(gen.requires_memcpy_ref_struct(variable.second, value)) {
//                auto elementPtr = Value::get_element_pointer(gen, parent_type, self_arg, idx, is_union ? 0 : variable.first);
//                gen.memcpy_struct(value->llvm_type(gen), elementPtr, value->llvm_value(gen, nullptr), value->encoded_location());
//            } else {
                // couldn't move struct
                value->store_in_struct(gen, nullptr, self_arg, parent_type, idx, is_union ? 0 : variable.first, variable.second);
//            }
        }
    }
}

void ThrowStatement::code_gen(Codegen &gen) {
    CHEM_THROW_RUNTIME("[UNIMPLEMENTED]");
}

//bool Codegen::requires_memcpy_ref_struct(BaseType* known_type, Value* value) {
//    return value->requires_memcpy_ref_struct(known_type);
//}

//llvm::Value* Codegen::memcpy_ref_struct(BaseType* known_type, Value* value, llvm::Value* llvm_ptr, llvm::Type* type) {
////    const auto pure = known_type->pure_type(allocator);
//    if(requires_memcpy_ref_struct(known_type->pure_type(allocator), value)) {
//        if(!llvm_ptr) {
//            const auto allocaInst = builder->CreateAlloca(type, nullptr);
//            di.instr(allocaInst, value);
//            llvm_ptr = allocaInst;
//        }
//        memcpy_struct(type, llvm_ptr, value->llvm_value(*this, nullptr), value->encoded_location());
//        return llvm_ptr;
//    }
//    return nullptr;
//}

llvm::Value* Codegen::memcpy_shallow_copy(BaseType* known_type, Value* value, llvm::Value* llvm_value) {
    // is referencing another struct, that is non movable and must be mem copied into the pointer
    const auto kind = value->val_kind();
    if(kind == ValueKind::Identifier || (kind == ValueKind::AccessChain && value->as_access_chain_unsafe()->values.back()->as_func_call() == nullptr)) {
        const auto node = known_type->get_direct_linked_node();
        if(node && node->is_shallow_copyable()) {
            const auto type = llvm_value->getType();
            const auto alloc = builder->CreateAlloca(type);
            di.instr(alloc, value->encoded_location());
            memcpy_struct(type, alloc, llvm_value, value->encoded_location());
            return alloc;
        } else {
            return llvm_value;
        }
    }
    return llvm_value;
}

bool Codegen::copy_or_move_struct(BaseType* known_type, Value* value, llvm::Value* memory_pointer) {
    // is referencing another struct, that is non movable and must be mem copied into the pointer
    auto linked = known_type->get_direct_linked_canonical_node();
    if (linked) {
        auto k = linked->kind();
        if(k == ASTNodeKind::UnnamedStruct || k == ASTNodeKind::UnnamedUnion) {
            memcpy_struct(value->llvm_type(*this), memory_pointer, value->llvm_value(*this), value->encoded_location());
            return true;
        } else if(k == ASTNodeKind::StructDecl || k == ASTNodeKind::VariantDecl || k == ASTNodeKind::UnionDecl) {
            const auto container = linked->as_members_container_unsafe();
            // we will always have to memcpy the struct into the location
            memcpy_struct(value->llvm_type(*this), memory_pointer, value->llvm_value(*this), value->encoded_location());
            if(!container->is_shallow_copyable()) {
                // however if struct is not shallow copyable, we must also set the drop flag to false so it won't be dropped
                if(!value->set_drop_flag_for_moved_ref(*this)) {
                    warn("couldn't set the drop flag to false for moved value", value);
                }
            }
            return true;
        }
    }
    return false;
}

void AssignStatement::code_gen(Codegen &gen) {

    const auto pointer = lhs->llvm_pointer(gen);
    const auto lhs_type_non_canon = lhs->getType();
    const auto lhs_type = lhs_type_non_canon->canonical();

    if(assOp != Operation::Assignment) {
        // is operator overloaded
        const auto can_node = lhs_type_non_canon->get_linked_canonical_node(true, false);
        if(can_node) {
            const auto container = can_node->get_members_container();
            if(container) {
                call_two_param_op_impl(gen, container, lhs, value, AssignStatement::overload_op_name(assOp));
                // all assignment operators return void
                return;
            }
        }
    } else {
        // normal flow
        const auto container = lhs_type->kind() == BaseTypeKind::CapturingFunction ? (
                lhs_type->as_capturing_func_type_unsafe()->instance_type->get_members_container()
        ) : lhs_type->get_members_container();
        if(container) {
            const auto id = lhs->get_chain_id();
            if(id != nullptr) {
                // lhs if identifier (can be connected to var init and function params), which must be destructed by checking drop flags
                // we must memcpy the struct into the lhs pointer
                // first if the lhs is not uninit, we must destruct it
                const auto node = id->linked;
                const auto drop_flag = gen.find_drop_flag(node);
                // we must destruct the previous value before we memcpy this value into the pointer
                // we're doing this conditionally, meaning a drop flag is checked to see if value should be dropped (was initialized)
                auto destructible = gen.create_destructible_for(node, drop_flag);
                if(destructible.has_value()) {
                    gen.conditional_destruct(destructible.value(), nullptr, lhs->encoded_location());
                }
                // now we're going to set the drop flag back to true for this
                // because if previously the drop flag was false, since we've reinitialized, we must set it to true,
                // so it can be dropped, when the scope ends
                if (drop_flag) {
                    const auto instr = gen.builder->CreateStore(gen.builder->getInt1(true), drop_flag);
                    gen.di.instr(instr, lhs->encoded_location());
                }
            } else if(!lhs->is_ref_moved()) {
                // previous is not an id, however we must still drop what we are assigning to
                const auto destr = container->destructor_func();
                if(destr) {
                    const auto callInst = gen.builder->CreateCall(destr->llvm_func(gen), { pointer });
                    gen.di.instr(callInst, this);
                }
            }
            if(value->requires_memcpy_ref_struct(lhs_type)) {
                // now we just need to memcpy the rhs by copy or move
                if(!gen.copy_or_move_struct(lhs_type, value, pointer)) {
                    gen.warn("couldn't copy or move the struct to location", encoded_location());
                }
                return;
            }
        }
    }
    // normal flow
    if (assOp == Operation::Assignment) {
        value->llvm_assign_value(gen, pointer, lhs);
    } else {
        llvm::Value* llvm_value = gen.operate(assOp, lhs, value);
        gen.assign_store(lhs, pointer, value, llvm_value, value->encoded_location());
    }
}

void DestructStmt::code_gen(Codegen &gen) {

    auto created_type = identifier->getType();
    auto pure_type = created_type->pure_type(gen.allocator);
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
            gen.error(this) << "value given to destruct statement must be of pointer type, value '" << identifier->representation() << "'";
            return;
        }
        const auto struct_type = ((PointerType*) pure_type)->type->pure_type(gen.allocator);
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
        gen.CheckNullCondBr(identifier_value, end_block, destruct_block, encoded_location());

        // generating code for destructor
        gen.SetInsertPoint(destruct_block);
        std::vector<llvm::Value *> destr_args;
        if (destructor->has_self_param()) {
            destr_args.emplace_back(identifier_value);
        }
        const auto callInst = gen.builder->CreateCall(destructor->llvm_func_type(gen), destructor->llvm_pointer(gen), destr_args);
        gen.di.instr(callInst, this);
        gen.CreateBr(end_block, encoded_location());

        // end block
        gen.SetInsertPoint(end_block);
        return;
    }

    llvm::Value* arr_size_llvm;
    BaseType* elem_type;
    if(pure_type->kind() == BaseTypeKind::Array) {
        auto arr_type = (ArrayType*) pure_type;
        elem_type = arr_type->elem_type->pure_type(gen.allocator);
        if (!is_array) {
            gen.error(this) << "expected brackets '[]' after 'destruct' for destructing an array, with value " << identifier->representation();
            return;
        } else if (arr_type->has_array_size() && array_value) {
            gen.error(this) << "array size given in brackets '[" << array_value->representation() << "]' is redundant as array size is known to be " << std::to_string(arr_type->get_array_size()) << " with value " << identifier->representation();
            return;
        } else if (arr_type->has_no_array_size() && !array_value) {
            gen.error(this) << "array size is not known, so it must be provided in brackets for destructing value " << identifier->representation();
            return;
        }
        auto def = elem_type->get_direct_linked_struct();
        if(!def) {
            gen.error(this) << "value given to destruct statement, doesn't reference a struct directly, value '" << identifier->representation() << "'";
            return;
        }
        arr_size_llvm = arr_type->has_array_size() ? gen.builder->getInt32(arr_type->get_array_size()) : array_value->llvm_value(gen);
    } else if(pure_type->kind() == BaseTypeKind::Pointer) {
        if(!array_value) {
            gen.error(this) << "array size is required when destructing a pointer, for destructing array pointer value" << identifier->representation();
            return;
        }
        auto ptr_type = (PointerType*) pure_type;
        elem_type = ptr_type->type->pure_type(gen.allocator);
        auto def = ptr_type->type->pure_type(gen.allocator)->get_direct_linked_struct();
        if(!def) {
            return;
        }

        arr_size_llvm = array_value->llvm_value(gen, nullptr);
    }

    auto id_value = identifier->llvm_value(gen);

    gen.destruct(id_value, arr_size_llvm, elem_type, encoded_location());

    if(getFreeAfter()) {
        const auto free_func = gen.getFreeFn();
        gen.builder->CreateCall(free_func, { id_value });
    }

}

void DeallocStmt::code_gen(Codegen& gen) {
    const auto free_func = gen.getFreeFn();
    gen.builder->CreateCall(free_func, { ptr->llvm_value(gen) });
}

llvm::Type* LoopValue::llvm_type(Codegen &gen) {
    return stmt.get_first_broken()->llvm_type(gen);
}

llvm::Value* LoopValue::llvm_value(Codegen &gen, BaseType *type) {
    const auto t = llvm_type(gen);
    const auto allocated = gen.builder->CreateAlloca(t);
    gen.di.instr(allocated, encoded_location());
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = { nullptr, allocated };
    stmt.code_gen(gen);
    gen.current_assignable = prev_assignable;
    const auto loadInstr = gen.builder->CreateLoad(t, allocated);
    gen.di.instr(loadInstr, encoded_location());
    return loadInstr;
}

void LoopValue::llvm_assign_value(Codegen &gen, llvm::Value* lhsPtr, Value *lhs) {
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = { lhs, lhsPtr };
    stmt.code_gen(gen);
    gen.current_assignable = prev_assignable;
}

llvm::AllocaInst* LoopValue::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    const auto allocated = gen.builder->CreateAlloca(expected_type ? expected_type->llvm_type(gen) : llvm_type(gen));
    gen.di.instr(allocated, encoded_location());
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = { nullptr, allocated };
    stmt.code_gen(gen);
    gen.current_assignable = prev_assignable;
    return allocated;
}

void LoopBlock::code_gen(Codegen &gen) {
    llvm::BasicBlock* current = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
    llvm::BasicBlock* end_block = llvm::BasicBlock::Create(*gen.ctx, "", gen.current_function);
    gen.CreateBr(current, ASTNode::encoded_location());
    gen.SetInsertPoint(current);
    gen.loop_body_gen(body, current, end_block);
    gen.CreateBr(current, ASTNode::encoded_location());
    gen.SetInsertPoint(end_block);
}

llvm::Type *EnumDeclaration::llvm_type(Codegen &gen) {
    return underlying_type->llvm_type(gen);
}

// ----------- Members

llvm::Value *EnumMember::llvm_load(Codegen& gen, SourceLocation location) {
    // we aren't supplying debug information for the load of this value
    // it's a constant integer, but we need to supply this location information somehow
    if(init_value) {
        return init_value->llvm_value(gen, nullptr);
    } else {
        return parent()->get_underlying_integer_type()->create(gen.allocator, gen.comptime_scope.typeBuilder, get_default_index(), location)->llvm_value(gen);
    }
}

llvm::Type *EnumMember::llvm_type(Codegen &gen) {
    if(init_value) {
        return init_value->llvm_type(gen);
    } else {
        return parent()->get_underlying_integer_type()->llvm_type(gen);
    }
}

// ------------ Structures

void Namespace::code_gen_declare(Codegen &gen) {
    if(is_comptime()) {
        return;
    }
    for(const auto node : nodes) {
        node->code_gen_declare(gen);
    }
}

void Namespace::code_gen(Codegen &gen) {
    if(is_comptime()) {
        return;
    }
    for(const auto node : nodes) {
        node->code_gen(gen);
    }
}

void Namespace::code_gen(Codegen &gen, Scope *scope, unsigned int index) {
    if(is_comptime()) {
        return;
    }
    for(const auto node : nodes) {
        node->code_gen(gen, scope, index);
    }
}

void Namespace::code_gen_external_declare(Codegen &gen) {
    if(is_comptime()) {
        return;
    }
    for(const auto node : nodes) {
        node->code_gen_external_declare(gen);
    }
}

bool LLVMBackendContext::forget(ASTNode* targetNode) {
    auto& gen = *gen_ptr;
    auto it = std::find_if(
            gen.destruct_nodes.begin(),
            gen.destruct_nodes.end(),
            [targetNode](auto& p){
                return p.getInitializer() == targetNode;
            });
    if (it == gen.destruct_nodes.end()) return false;
    gen.destruct_nodes.erase(it);
    return true;
}

void LLVMBackendContext::mem_copy(Value* lhs, Value* rhs) {
    auto& gen = *gen_ptr;
    auto pointer = lhs->llvm_pointer(gen);
    auto val = rhs->llvm_value(gen, nullptr);
    gen.memcpy_struct(rhs->llvm_type(gen), pointer, val, rhs->encoded_location());
}

void LLVMBackendContext::destruct_call_site(SourceLocation location) {
    auto& gen = *gen_ptr;
    destruct_current_scope(gen, nullptr, location);
}

llvm::AtomicOrdering to_llvm_mo(BackendAtomicMemoryOrder order) {
    switch(order) {
        case BackendAtomicMemoryOrder::NotAtomic:
            return llvm::AtomicOrdering::NotAtomic;
        case BackendAtomicMemoryOrder::Unordered:
            return llvm::AtomicOrdering::Unordered;
        case BackendAtomicMemoryOrder::Monotonic:
            return llvm::AtomicOrdering::Monotonic;
        case BackendAtomicMemoryOrder::Acquire:
            return llvm::AtomicOrdering::Acquire;
        case BackendAtomicMemoryOrder::Release:
            return llvm::AtomicOrdering::Release;
        case BackendAtomicMemoryOrder::AcquireRelease:
            return llvm::AtomicOrdering::AcquireRelease;
        case BackendAtomicMemoryOrder::SequentiallyConsistent:
            return llvm::AtomicOrdering::SequentiallyConsistent;
    }
}

uint8_t to_llvm_ss(BackendAtomicSyncScope scope) {
    switch(scope) {
        case BackendAtomicSyncScope::System:
            return llvm::SyncScope::System;
        case BackendAtomicSyncScope::SingleThread:
            return llvm::SyncScope::SingleThread;
    }
}

llvm::AtomicRMWInst::BinOp to_llvm_op(BackendAtomicOp op) {
    switch(op) {
        case BackendAtomicOp::Xchg:
            return llvm::AtomicRMWInst::Xchg;
        case BackendAtomicOp::Add:
            return llvm::AtomicRMWInst::Add;
        case BackendAtomicOp::Sub:
            return llvm::AtomicRMWInst::Sub;
        case BackendAtomicOp::And:
            return llvm::AtomicRMWInst::And;
        case BackendAtomicOp::Nand:
            return llvm::AtomicRMWInst::Nand;
        case BackendAtomicOp::Or:
            return llvm::AtomicRMWInst::Or;
        case BackendAtomicOp::Xor:
            return llvm::AtomicRMWInst::Xor;
        case BackendAtomicOp::Max:
            return llvm::AtomicRMWInst::Max;
        case BackendAtomicOp::Min:
            return llvm::AtomicRMWInst::Min;
        case BackendAtomicOp::UMax:
            return llvm::AtomicRMWInst::UMax;
        case BackendAtomicOp::UMin:
            return llvm::AtomicRMWInst::UMin;
        case BackendAtomicOp::FAdd:
            return llvm::AtomicRMWInst::FAdd;
        case BackendAtomicOp::FSub:
            return llvm::AtomicRMWInst::FSub;
        case BackendAtomicOp::FMax:
            return llvm::AtomicRMWInst::FMax;
        case BackendAtomicOp::FMin:
            return llvm::AtomicRMWInst::FMin;
        case BackendAtomicOp::UIncWrap:
            return llvm::AtomicRMWInst::UIncWrap;
        case BackendAtomicOp::UDecWrap:
            return llvm::AtomicRMWInst::UDecWrap;
    }
}

BaseType* get_atomic_op_type(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Pointer:
            return type->as_pointer_type_unsafe()->type;
        case BaseTypeKind::Linked: {
            const auto linked = type->as_linked_type_unsafe()->linked;
            if(linked->kind() == ASTNodeKind::TypealiasStmt) {
                return get_atomic_op_type(linked->as_typealias_unsafe()->actual_type);
            } else {
                return nullptr;
            }
        }
        default:
            return nullptr;
    }
}

Value* pack_llvm_val(ASTAllocator& allocator, llvm::Value* value, BaseType* type, SourceLocation location) {
    return new (allocator.allocate<ExtractionValue>()) ExtractionValue(
        (Value*) value, type, ExtractionKind::ReinterpretLLVMValue, location
    );
}

void LLVMBackendContext::atomic_fence(BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope, SourceLocation location) {
    auto& gen = *gen_ptr;
    if(order == BackendAtomicMemoryOrder::Monotonic) {
        gen.error("fence cannot use 'monotonic' or 'relaxed' ordering; upgrade to 'acq_rel'", location);
    }
    auto llvm_mo = to_llvm_mo(order);
    const auto instr = gen.builder->CreateFence(
        // fence cannot use 'monotonic' or 'relaxed' ordering
        llvm_mo == llvm::AtomicOrdering::Monotonic ? llvm::AtomicOrdering::AcquireRelease : llvm_mo,
        to_llvm_ss(scope)
    );
    gen.di.instr(instr, location);
}

Value* LLVMBackendContext::atomic_load(Value* ptr, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) {
    auto& gen = *gen_ptr;
    const auto type = ptr->getType();
    const auto atomic_type = get_atomic_op_type(type);
    if(!atomic_type) {
        gen.error("expected a value of pointer to integer type", ptr);
        return ptr;
    }
    const auto loadInst = gen.builder->CreateLoad(atomic_type->llvm_type(gen), ptr->llvm_value(gen));
    gen.di.instr(loadInst, ptr);
    loadInst->setAtomic(to_llvm_mo(order), to_llvm_ss(scope));
    return pack_llvm_val(gen.allocator, loadInst, atomic_type, ptr->encoded_location());
}

void LLVMBackendContext::atomic_store(Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) {
    auto& gen = *gen_ptr;
    const auto type = ptr->getType();
    const auto atomic_type = get_atomic_op_type(type);
    if(!atomic_type) {
        gen.error("expected a value of pointer to integer type", ptr);
        return;
    }
    const auto casted_value = gen.implicit_cast(value->llvm_value(gen), atomic_type, atomic_type->llvm_type(gen));
    const auto storeInst = gen.builder->CreateStore(casted_value, ptr->llvm_value(gen));
    gen.di.instr(storeInst, ptr);
    storeInst->setAtomic(to_llvm_mo(order), to_llvm_ss(scope));
}

Value* LLVMBackendContext::atomic_cmp_exch_weak(Value* ptr, Value* expected, Value* value, BackendAtomicMemoryOrder success_order, BackendAtomicMemoryOrder failure_order, BackendAtomicSyncScope scope) {
    auto& gen = *gen_ptr;
    const auto type = ptr->getType();
    const auto atomic_type = get_atomic_op_type(type);
    if(!atomic_type) {
        gen.error("expected a value of pointer to integer type", ptr);
        return ptr;
    }
    const auto new_value = value->llvm_value(gen);
    const auto new_value_casted = gen.implicit_cast(new_value, atomic_type, atomic_type->llvm_type(gen));
    const auto loc = ptr->encoded_location();
    DereferenceValue deref_expected(expected, atomic_type, loc);
    const auto inst = gen.builder->CreateAtomicCmpXchg(ptr->llvm_value(gen), deref_expected.llvm_value(gen, nullptr), new_value_casted, llvm::MaybeAlign(), to_llvm_mo(success_order), to_llvm_mo(failure_order), to_llvm_ss(scope));
    gen.di.instr(inst, ptr);
    const auto loadedOldValue = gen.builder->CreateExtractValue(inst, 0);
    const auto storedOldValue = gen.builder->CreateStore(loadedOldValue, expected->llvm_value(gen));
    gen.di.instr(storedOldValue, expected);
    const auto successFlagValue = gen.builder->CreateExtractValue(inst, 1);
    return pack_llvm_val(gen.allocator, successFlagValue, gen.comptime_scope.typeBuilder.getBoolType(), ptr->encoded_location());
}

Value* LLVMBackendContext::atomic_cmp_exch_strong(Value* ptr, Value* expected, Value* value, BackendAtomicMemoryOrder success_order, BackendAtomicMemoryOrder failure_order, BackendAtomicSyncScope scope) {
    // maps to the same instruction
    return atomic_cmp_exch_weak(ptr, expected, value, success_order, failure_order, scope);
}

Value* LLVMBackendContext::atomic_op(BackendAtomicOp op, Value* ptr, Value* value, BackendAtomicMemoryOrder order, BackendAtomicSyncScope scope) {
    auto& gen = *gen_ptr;
    const auto type = ptr->getType();
    const auto atomic_type = get_atomic_op_type(type);
    if(!atomic_type) {
        gen.error("expected a value of pointer to integer type", ptr);
        return ptr;
    }
    const auto casted_value = gen.implicit_cast(value->llvm_value(gen), atomic_type, atomic_type->llvm_type(gen));
    const auto atomic_rmw = gen.builder->CreateAtomicRMW(to_llvm_op(op), ptr->llvm_value(gen), casted_value, llvm::MaybeAlign(), to_llvm_mo(order), to_llvm_ss(scope));
    return pack_llvm_val(gen.allocator, atomic_rmw, atomic_type, ptr->encoded_location());
}