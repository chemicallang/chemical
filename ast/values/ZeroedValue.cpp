// Copyright (c) Chemical Language Foundation 2025.

#include "ZeroedValue.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/values/StructValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/base/InterpretScope.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/NullValue.h"
#include "ast/types/IntNType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"

Value* ZeroedValue::evaluated_value(InterpretScope& scope) {
    // For struct types, create a properly zero-initialized StructValue so
    // that member access like z.a works via child() on the resulting StructValue.
    auto type = getType();
    if(type) {
        auto linkedNode = type->get_direct_linked_canonical_node();
        if(linkedNode && linkedNode->kind() == ASTNodeKind::StructDecl) {
            auto structDef = (StructDefinition*) linkedNode;
            auto container = (VariablesContainerBase*) structDef;
            auto structVal = new (scope.allocate<StructValue>()) StructValue(
                TypeLoc(getType()->copy(scope.allocator), encoded_location()),
                structDef,
                container,
                encoded_location()
            );
            for(const auto field : structDef->variables()) {
                auto defValue = field->default_value();
                if(defValue) {
                    structVal->values.emplace(
                        field->name,
                        StructMemberInitializer{field->name, defValue->scope_value(scope)}
                    );
                } else {
                    // Create proper zero value based on field type
                    auto fieldType = field->known_type();
                    if(!fieldType) {
                        structVal->values.emplace(
                            field->name,
                            StructMemberInitializer{field->name, scope.getNullValue()}
                        );
                    } else {
                        auto pureType = fieldType->canonical();
                        auto kind = pureType->kind();
                        Value* zeroVal = nullptr;
                        if(kind == BaseTypeKind::IntN) {
                            zeroVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                                0, (IntNType*)pureType, encoded_location()
                            );
                        } else if(kind == BaseTypeKind::Bool) {
                            zeroVal = new (scope.allocate<BoolValue>()) BoolValue(
                                false, scope.global->typeBuilder.getBoolType(), encoded_location()
                            );
                        } else if(kind == BaseTypeKind::Float) {
                            zeroVal = new (scope.allocate<FloatValue>()) FloatValue(
                                0.0f, (FloatType*)pureType, encoded_location()
                            );
                        } else if(kind == BaseTypeKind::Double) {
                            zeroVal = new (scope.allocate<DoubleValue>()) DoubleValue(
                                0.0, (DoubleType*)pureType, encoded_location()
                            );
                        } else if(kind == BaseTypeKind::Pointer) {
                            zeroVal = new (scope.allocate<NullValue>()) NullValue(
                                scope.global->typeBuilder.getNullPtrType(), encoded_location()
                            );
                        } else {
                            zeroVal = scope.getNullValue();
                        }
                        if(zeroVal) {
                            structVal->values.emplace(
                                field->name,
                                StructMemberInitializer{field->name, zeroVal}
                            );
                        }
                    }
                }
            }
            return structVal;
        }
    }
    // For primitive types, return the ZeroedValue itself
    return this;
}

Value* ZeroedValue::child(InterpretScope& scope, const chem::string_view& name) {
    // First evaluate to get the proper struct value, then access child
    auto eval = evaluated_value(scope);
    if(eval && eval != this) {
        return eval->child(scope, name);
    }
    return nullptr;
}

#ifdef COMPILER_BUILD
#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Value* ZeroedValue::llvm_pointer(Codegen &gen) {
    const auto type = llvm_type(gen);
    const auto alloca = gen.builder->CreateAlloca(type);
    gen.builder->CreateStore(llvm::Constant::getNullValue(type), alloca);
    return alloca;
}

llvm::Value* ZeroedValue::llvm_value(Codegen &gen, BaseType *type) {
    return llvm::Constant::getNullValue(llvm_type(gen));
}

llvm::Value* ZeroedValue::llvm_arg_value(Codegen &gen, BaseType *expected_type) {
    const auto type = llvm_type(gen);
    const auto alloca = gen.builder->CreateAlloca(type, nullptr);
    gen.builder->CreateStore(llvm::Constant::getNullValue(type), alloca);
    return (llvm::AllocaInst*) alloca;
}

llvm::Value* ZeroedValue::llvm_ret_value(Codegen &gen, Value *returnValue) {
    const auto type = llvm_type(gen);
    if (type->isAggregateType()) {
        auto structPassed = gen.current_function->getArg(gen.current_func_type->getStructReturnArgIndex());
        gen.builder->CreateStore(llvm::Constant::getNullValue(type), structPassed);
        return nullptr;
    }
    return llvm::Constant::getNullValue(type);
}

llvm::AllocaInst* ZeroedValue::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    const auto type = llvm_type(gen);
    const auto alloca = gen.builder->CreateAlloca(type, nullptr, identifier);
    gen.builder->CreateStore(llvm::Constant::getNullValue(type), alloca);
    return (llvm::AllocaInst*) alloca;
}

void ZeroedValue::llvm_assign_value(Codegen &gen, llvm::Value *storagePtr, Value *lhs, llvm::Value *lhsPtr) {
    const auto type = llvm_type(gen);
    gen.builder->CreateStore(llvm::Constant::getNullValue(type), storagePtr);
}

unsigned int ZeroedValue::store_in_struct(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) {
    auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    gen.builder->CreateStore(llvm::Constant::getNullValue(llvm_type(gen)), elementPtr);
    return index + 1;
}

unsigned int ZeroedValue::store_in_array(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) {
    auto elementPtr = Value::get_element_pointer(gen, allocated_type, allocated, idxList, index);
    gen.builder->CreateStore(llvm::Constant::getNullValue(llvm_type(gen)), elementPtr);
    return index + 1;
}

llvm::Type* ZeroedValue::llvm_type(Codegen& gen) {
    return getType()->llvm_type(gen);
}

#endif
