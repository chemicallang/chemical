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
#include "ast/statements/Typealias.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/IntNumValue.h"
#include "ast/base/InterpretScope.h"
#include "PointerValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "compiler/symres/ImplementationsIndex.h"
#include <cstring>

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Value* auto_deref_index(Codegen& gen, Value* value) {
    const auto ref_type = value->getType()->canonical();
    if(ref_type->kind() == BaseTypeKind::Reference) {
        const auto underlying_type = ref_type->as_reference_type_unsafe()->type;
        const auto loadInst = gen.builder->CreateLoad(underlying_type->llvm_type(gen), value->llvm_value(gen));
        gen.di.instr(loadInst, value);
        return loadInst;
    }
    return value->llvm_value(gen);
}

llvm::Value *IndexOperator::elem_pointer(Codegen &gen, llvm::Type *type, llvm::Value *ptr) {
    std::vector<llvm::Value *> idxList;
    if (type->isArrayTy()) {
        idxList.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), 0));
    }
    idxList.emplace_back(auto_deref_index(gen, idx));
    idxList.shrink_to_fit();
    return gen.builder->CreateGEP(type, ptr, idxList, "", gen.inbounds);
}

bool IndexOperator::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    const auto parent_type = parent->getType()->canonical();
    if (parent_type->kind() == BaseTypeKind::Array && indexes.empty() && (parent->linked_node() == nullptr || parent->linked_node()->as_func_param() == nullptr )) {
        indexes.push_back(gen.builder->getInt32(0));
    }
    indexes.emplace_back(auto_deref_index(gen, idx));
    return true;
}

llvm::Type *IndexOperator::llvm_type(Codegen &gen) {
    return getType()->llvm_type(gen);
}

llvm::Type *IndexOperator::llvm_chain_type(Codegen &gen, std::vector<Value*> &chain, unsigned int index) {
    return getType()->llvm_chain_type(gen, chain, index);
}

#endif

// TODO: make this function on BaseType
BaseType* get_child_type(TypeBuilder& typeBuilder, BaseType* type, bool unwrap_ref = false) {
    switch(type->kind()) {
        case BaseTypeKind::Array:
            return type->as_array_type_unsafe()->elem_type;
        case BaseTypeKind::Pointer:
            return type->as_pointer_type_unsafe()->type;
        case BaseTypeKind::Reference:
            // since reference is automatically dereferenced
            if(unwrap_ref) {
                return get_child_type(typeBuilder, type->as_reference_type_unsafe()->type, false);
            } else {
                return type->as_reference_type_unsafe()->type;
            }
        case BaseTypeKind::String:
            return typeBuilder.getCharType();
        case BaseTypeKind::Linked:{
            const auto linked = type->as_linked_type_unsafe()->linked;
            if(linked->kind() == ASTNodeKind::TypealiasStmt) {
                return get_child_type(typeBuilder, linked->as_typealias_unsafe()->actual_type, unwrap_ref);
            } else {
                return nullptr;
            }
        }
        default:
            return nullptr;
    }
}

void IndexOperator::determine_type(TypeBuilder& typeBuilder, CoreNodes& coreNodes, ImplementationsIndex& implsIndex, ASTDiagnoser& diagnoser) {
    auto current_type = parent_val->getType();
    const auto can_node = current_type->get_linked_canonical_node(true, false);
    if(can_node) {
        const auto container = can_node->get_members_container();
        if(container) {
            const auto func = implsIndex.get_index_op_impl(coreNodes, container);
            if (func == nullptr) {
                diagnoser.error(this) << "couldn't find 'index' or 'index_mut' function implementation for operator overloading";
                setType(typeBuilder.getVoidType());
                return;
            }
            // a single function with name 'index' is present
            if (func->params.size() != 2) {
                diagnoser.error(this) << "expected 'index' operator function to have exactly two parameters";
                setType(typeBuilder.getVoidType());
                return;
            }
            const auto childType = get_child_type(typeBuilder, func->returnType);
            if(childType) {
                setType(childType);
            } else {
                setType(typeBuilder.getVoidType());
            }
            return;
        }
    }
    const auto childType = get_child_type(typeBuilder, current_type, true);
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
            // Prefer the stored values vector — it contains evaluated AST results
            // (e.g. FunctionCall that returns a struct). Only fall back to contiguousData
            // for uninitialized arrays with primitive element types.
            if (index.value() < arr->values.size()) {
                auto evalVal = arr->values[index.value()]->evaluated_value(scope);
                if(evalVal) {
                    return evalVal->copy(scope.allocator);
                }
            }
            // Fallback: read from contiguousData (handles uninitialized primitive arrays)
            if (arr->contiguousData) {
                const auto elemSize = arr->getType()->elem_type->byte_size(scope.global->target_data);
                if (elemSize > 0 && elemSize <= 8) {
                    const auto maxIdx = arr->contiguousSize / elemSize;
                    if (index.value() < maxIdx) {
                        uint64_t val = 0;
                        std::memcpy(&val, (char*)arr->contiguousData + index.value() * elemSize, elemSize);
                        return new (scope.allocate<IntNumValue>()) IntNumValue(val, scope.global->typeBuilder.getIntNType((unsigned int)(elemSize * 8), true), location);
                    }
                }
            }
            // Out of bounds or uninitialized: return zero
            return new (scope.allocate<IntNumValue>()) IntNumValue(0, scope.global->typeBuilder.getIntNType(32, true), location);
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
    Value* eval = parent_val->evaluated_value(scope);
    if(!eval) return nullptr;
    return index_inside(scope, eval, idx, idx->encoded_location());
}

void IndexOperator::set_value(InterpretScope& scope, Value* rawValue, Operation op, SourceLocation location) {
    // Evaluate the parent to get the array or pointer
    auto parentEval = parent_val->evaluated_value(scope);
    if (!parentEval) {
        scope.error("cannot set value: parent could not be evaluated", this);
        return;
    }
    
    // Evaluate the index
    auto idxEval = idx->evaluated_value(scope);
    auto idxOpt = idxEval->get_number();
    if (!idxOpt.has_value()) {
        scope.error("cannot set value: index is not a number", idx);
        return;
    }
    auto i = idxOpt.value();
    
    // Evaluate the new value
    auto newVal = rawValue->evaluated_value(scope);
    if (!newVal) return;
    
    switch (parentEval->val_kind()) {
        case ValueKind::ArrayValue: {
            auto arrVal = (ArrayValue*)parentEval;
            const auto elemSize = getType()->byte_size(scope.global->target_data);
            // Write into contiguous data if available
            if (arrVal->contiguousData && elemSize > 0 && elemSize <= 8) {
                const auto maxIdx = arrVal->contiguousSize / elemSize;
                if (i < maxIdx) {
                    auto num = newVal->get_number();
                    if (num.has_value()) {
                        std::memcpy((char*)arrVal->contiguousData + i * elemSize, &num.value(), elemSize);
                    }
                    break;
                }
            }
            // Fallback: write into values vector
            if (i < arrVal->values.size()) {
                arrVal->values[i] = newVal->copy(scope.allocator);
            }
            break;
        }
        case ValueKind::PointerValue: {
            // ptr[i] = value — increment and write through pointer
            auto ptrVal = (PointerValue*)parentEval;
            auto incremented = ptrVal->increment(scope, i, location, this);
            if (incremented) {
                const auto byteSize = getType()->byte_size(scope.global->target_data);
                if (byteSize <= incremented->ahead) {
                    auto num = newVal->get_number();
                    if (num.has_value()) {
                        switch (byteSize) {
                            case 1: *((char*)incremented->data) = (char)num.value(); break;
                            case 2: *((short*)incremented->data) = (short)num.value(); break;
                            case 4: *((int*)incremented->data) = (int)num.value(); break;
                            case 8: default: *((uint64_t*)incremented->data) = num.value(); break;
                        }
                    }
                }
            }
            break;
        }
        default:
            scope.error("cannot set value through indexing on this type", this);
            break;
    }
}

Value *IndexOperator::find_in(InterpretScope &scope, Value *parent) {
    const auto eval = idx->evaluated_value(scope);
    const auto num_value = eval->get_number();
    if(num_value.has_value()) {
        return parent->index(scope, num_value.value());
    }
    return nullptr;
}