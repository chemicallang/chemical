// Copyright (c) Chemical Language Foundation 2025.

#include "IndexOperator.h"
#include "ast/values/AccessChain.h"
#include "ast/base/ASTNode.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/VoidType.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/base/InterpretScope.h"
#include "PointerValue.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Value *IndexOperator::elem_pointer(Codegen &gen, llvm::Type *type, llvm::Value *ptr) {
    std::vector<llvm::Value *> idxList;
    if (type->isArrayTy()) {
        idxList.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    idxList.emplace_back(idx->llvm_value(gen));
    idxList.shrink_to_fit();
    return gen.builder->CreateGEP(type, ptr, idxList, "", gen.inbounds);
}

llvm::Value *IndexOperator::llvm_pointer(Codegen &gen) {
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

bool IndexOperator::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    const auto parent_type = parent->getType()->canonical();
    if (parent_type->kind() == BaseTypeKind::Array && indexes.empty() && (parent->linked_node() == nullptr || parent->linked_node()->as_func_param() == nullptr )) {
        indexes.push_back(gen.builder->getInt32(0));
    }
    indexes.emplace_back(idx->llvm_value(gen));
    return true;
}

bool IndexOperator::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return getType()->linked_node()->add_child_index(gen, indexes, name);
}

llvm::Type *IndexOperator::llvm_type(Codegen &gen) {
    return getType()->llvm_type(gen);
}

llvm::Type *IndexOperator::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &chain, unsigned int index) {
    return getType()->llvm_chain_type(gen, chain, index);
}

#endif

// TODO: make this function on BaseType
BaseType* get_child_type(TypeBuilder& typeBuilder, BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Array:
            return type->as_array_type_unsafe()->elem_type;
        case BaseTypeKind::Pointer:
            return type->as_pointer_type_unsafe()->type;
        case BaseTypeKind::Reference:
            return type->as_reference_type_unsafe()->type;
        case BaseTypeKind::String:
            return typeBuilder.getCharType();
        default:
            return nullptr;
    }
}

void IndexOperator::determine_type(TypeBuilder& typeBuilder) {
    auto current_type = parent_val->getType();
    const auto childType = get_child_type(typeBuilder, current_type);
    if(childType) {
        setType(childType);
    } else {
        setType(typeBuilder.getVoidType());
    }
}

ASTNode *IndexOperator::linked_node() {
    const auto value_type = getType();
    return value_type ? value_type->linked_node() : nullptr;
}

Value* index_inside(InterpretScope& scope, Value* value, Value* indexVal, SourceLocation location) {
    const auto evalIndex = indexVal->evaluated_value(scope);
    const auto index = evalIndex->get_number();
    if(!index.has_value()) {
        scope.error("index value doesn't evaluate to a number", indexVal);
        return nullptr;
    }
    switch(value->val_kind()) {
        case ValueKind::String: {
            const auto str = value->as_string_unsafe();
            return new (scope.allocate<IntNumValue>()) IntNumValue(str->value[index.value()], scope.global->typeBuilder.getCharType(), location);
        }
        case ValueKind::ArrayValue: {
            const auto arr = value->as_array_value_unsafe();
            return arr->values[index.value()]->copy(scope.allocator);
        }
        case ValueKind::PointerValue: {
            const auto ptrVal = (PointerValue*) value;
            const auto incremented = ptrVal->increment(scope, index.value(), location, indexVal);
            return incremented->deref(scope, location, indexVal);
        }
        default:
            scope.error("indexing on unknown value", value);
            return nullptr;
    }
}

Value* IndexOperator::evaluated_value(InterpretScope &scope) {
    unsigned i = 0;
    Value* eval = parent_val->evaluated_value(scope);
    if(!eval) return nullptr;
    return index_inside(scope, eval, idx, idx->encoded_location());
}

Value *IndexOperator::find_in(InterpretScope &scope, Value *parent) {
    const auto eval = idx->evaluated_value(scope);
    const auto num_value = eval->get_number();
    if(num_value.has_value()) {
        return parent->index(scope, num_value.value());
    }
    return nullptr;
}