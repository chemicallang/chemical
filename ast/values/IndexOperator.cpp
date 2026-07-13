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
#include "ast/values/FunctionCall.h"
#include "ast/values/StructValue.h"
#include "compiler/symres/ImplementationsIndex.h"
#include "compiler/lab/LabBuildCompiler.h"
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
BaseType* get_child_type(const TypeBuilder& typeBuilder, BaseType* type, bool unwrap_ref = false) {
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

void IndexOperator::determine_type(const TypeBuilder& typeBuilder, const CoreNodes& coreNodes, const ImplementationsIndex& implsIndex, ASTDiagnoser& diagnoser) {
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
    switch(value->val_kind()) {
        case ValueKind::String: {
            if(!index.has_value()) {
                scope.error("index value doesn't evaluate to a number for string", indexVal);
                return nullptr;
            }
            const auto str = value->as_string_unsafe();
            return new (scope.allocate<IntNumValue>()) IntNumValue(str->value[index.value()], scope.global->typeBuilder.getCharType(), location);
        }
        case ValueKind::ArrayValue: {
            if(!index.has_value()) {
                scope.error("index value doesn't evaluate to a number for array", indexVal);
                return nullptr;
            }
            const auto arr = value->as_array_value_unsafe();
            // Use contiguousData for numeric primitive types (IntN, Bool, Float, Double)
            // where writes through pointers (from &mut arr[i]) are stored. This keeps
            // reads in sync with pointer writes that bypass the values vector.
            // For reference/pointer/struct element types, use the values vector since
            // their data cannot be serialized into contiguousData via get_number().
            if (arr->contiguousData) {
                // Check element type to confirm it's a numeric type
                auto elemType = arr->getType() ? arr->getType()->elem_type->canonical() : nullptr;
                if(elemType) {
                    auto elemKind = elemType->kind();
                    if(elemKind == BaseTypeKind::IntN || elemKind == BaseTypeKind::Bool ||
                       elemKind == BaseTypeKind::Float || elemKind == BaseTypeKind::Double) {
                        const auto elemSize = elemType->byte_size(scope.global->target_data);
                        if (elemSize > 0 && elemSize <= 8) {
                            const auto maxIdx = arr->contiguousSize / elemSize;
                            if (index.value() < maxIdx) {
                                uint64_t val = 0;
                                std::memcpy(&val, (char*)arr->contiguousData + index.value() * elemSize, elemSize);
                                return new (scope.allocate<IntNumValue>()) IntNumValue(val, scope.global->typeBuilder.getIntNType((unsigned int)(elemSize * 8), true), location);
                            }
                        }
                    }
                }
            }
            // Fallback: use the stored values vector for struct elements or when
            // contiguousData is not available (uninitialized arrays).
            if (index.value() < arr->values.size()) {
                auto rawElem = arr->values[index.value()];
                // For StructValue and ArrayValue elements, return the raw element directly
                // (no copy) so that modifications through the array index
                // (e.g. arr[0].data = 5, arr[0][0] = 2) are preserved.
                // The evaluated_value+copy path would create a fresh copy losing modifications.
                if(rawElem && (rawElem->val_kind() == ValueKind::StructValue ||
                               rawElem->val_kind() == ValueKind::ArrayValue)) {
                    return rawElem;
                }
                auto evalVal = rawElem->evaluated_value(scope);
                if(evalVal) {
                    return evalVal->copy(scope.allocator);
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
        case ValueKind::StructValue: {
            // Index operator overloading: find the 'index' method via impl blocks
            auto structVal = value->as_struct_value_unsafe();
            auto ext = structVal->linked_extendable();
            FunctionDeclaration* indexFn = nullptr;
            if(ext) {
                // Try direct child lookup
                auto memberFn = ext->child("index");
                if(memberFn && memberFn->kind() == ASTNodeKind::FunctionDecl) {
                    indexFn = memberFn->as_function_unsafe();
                }
            }
            // Fallback: use implsIndex to find the index implementation
            if(!indexFn && scope.global && scope.global->build_compiler) {
                auto& implsIndex = scope.global->build_compiler->implsIndex;
                auto& coreNodes = scope.global->build_compiler->coreNodes;
                if(ext) {
                    auto container = ext->get_members_container();
                    if(container) {
                        auto foundFn = implsIndex.get_index_op_impl(coreNodes, container);
                        if(foundFn) indexFn = foundFn;
                    }
                }
            }
            if(indexFn) {
                auto call = new (scope.allocate<FunctionCall>()) FunctionCall(
                    nullptr, indexFn->returnType, location
                );
                call->values = { indexVal };
                return indexFn->call(&scope, scope.allocator, call, value);
            }
            // Fall through to error
            scope.error("indexing on unknown value", value);
            return nullptr;
        }
        default:
            scope.error("indexing on unknown value", value);
            return nullptr;
    }
}

Value* IndexOperator::evaluated_value(InterpretScope &scope) {
    Value* eval = parent_val->evaluated_value(scope);
    if(!eval) return nullptr;
    auto result = index_inside(scope, eval, idx, idx->encoded_location());
    // If the parent was a FunctionCall producing a temp struct with a destructor,
    // destruct it after the index operation completes. The temp is NOT tracked
    // by any scope, and the self-ref fix prevents method scopes from destructing
    // it via the &self parameter. Without this, temps from expressions like
    // create_indexable(...)[0] would be leaked.
    if(parent_val && parent_val->val_kind() == ValueKind::FunctionCall && eval->val_kind() == ValueKind::StructValue) {
        auto structVal = eval->as_struct_value_unsafe();
        auto ext = structVal->linked_extendable();
        if(ext && ext->has_destructor()) {
            auto destructor_fn = ext->destructor_func();
            if(destructor_fn && destructor_fn->body.has_value()) {
                InterpretScope child_scope(scope.global, scope.allocator, scope.global);
                child_scope.declare("self", eval);
                child_scope.interpret(&destructor_fn->body.value());
                auto self_it = child_scope.values.find("self");
                if(self_it != child_scope.values.end()) {
                    child_scope.values.erase(self_it);
                }
            }
        }
    }
    return result;
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
        case ValueKind::StructValue: {
            // Index operator overloading: look up the 'index' method on the struct
            auto structVal = parentEval->as_struct_value_unsafe();
            auto ext = structVal->linked_extendable();
            if(ext) {
                // Try index_mut (mutable index) first, then index
                auto memberFn = ext->child("index");
                if(memberFn && memberFn->kind() == ASTNodeKind::FunctionDecl) {
                    auto indexFn = memberFn->as_function_unsafe();
                    // Create a function call to set via index: indexFn(structVal, idx) = newVal
                    // Call the index function to get a reference, then assign through it
                    InterpretScope child_scope(scope.global, scope.allocator, scope.global);
                    std::vector<Value*> idxArgs = { idx, rawValue };
                    // For index + assignment (like s[0] = 76), we call index(&self, idx) which
                    // returns a reference, then assign the value through that reference
                    auto call = new (scope.allocate<FunctionCall>()) FunctionCall(
                        nullptr, indexFn->returnType, location
                    );
                    call->values = { idx };
                    auto ref = indexFn->call(&scope, scope.allocator, call, structVal);
                    if(ref && ref->val_kind() == ValueKind::PointerValue) {
                        auto ptrRef = static_cast<PointerValue*>(ref);
                        // Assign the new value through the pointer
                        auto num = newVal->get_number();
                        if(num.has_value()) {
                            const auto byteSize = getType()->byte_size(scope.global->target_data);
                            if(byteSize <= ptrRef->ahead) {
                                switch(byteSize) {
                                    case 1: *((char*)ptrRef->data) = (char)num.value(); break;
                                    case 2: *((short*)ptrRef->data) = (short)num.value(); break;
                                    case 4: *((int*)ptrRef->data) = (int)num.value(); break;
                                    case 8: *((uint64_t*)ptrRef->data) = num.value(); break;
                                }
                            }
                        }
                    }
                    break;
                }
            }
            scope.error("cannot set value through indexing on this type", this);
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
        // First, evaluate the parent to get the actual array/pointer value
        auto parentEval = parent->evaluated_value(scope);
        if(!parentEval) return nullptr;
        // For ArrayValue, return the raw element directly (without copy)
        // so that assignments through the chain (e.g., arr[i].field = val)
        // modify the original element.  Evaluate non-StructValue elements
        // on access (DereferenceValue, FunctionCall) so they produce their
        // computed values and subsequent field access works on the real element.
        if(parentEval->val_kind() == ValueKind::ArrayValue) {
            auto arr = parentEval->as_array_value_unsafe();
            if(num_value.value() < arr->values.size()) {
                auto rawElem = arr->values[num_value.value()];
                // Only evaluate non-StructValue && non-Identifier elements
                // (skip VariableIdentifiers to avoid sharing struct pointers)
                if(rawElem && rawElem->val_kind() != ValueKind::StructValue &&
                   rawElem->val_kind() != ValueKind::Identifier) {
                    auto evalElem = rawElem->evaluated_value(scope);
                    if(evalElem && evalElem != rawElem) {
                        arr->values[num_value.value()] = evalElem;
                        return evalElem;
                    }
                }
                return rawElem;
            }
        }
        return parentEval->index(scope, num_value.value());
    }
    return nullptr;
}