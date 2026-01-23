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

void IndexOperator::determine_type(TypeBuilder& typeBuilder, ASTDiagnoser& diagnoser) {
    auto current_type = parent_val->getType();
    const auto can_node = current_type->get_linked_canonical_node(true, false);
    if(can_node) {
        const auto container = can_node->get_members_container();
        if(container) {
            const auto child = container->child("index");
            if(!child) {
                diagnoser.error(this) << "expected a function 'index' to be present for overloading";
                setType(typeBuilder.getVoidType());
                return;
            }
            if(child->kind() == ASTNodeKind::FunctionDecl) {
                // a single function with name 'index' is present
                const auto func = child->as_function_unsafe();
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
            } else if(child->kind() == ASTNodeKind::MultiFunctionNode) {
                const auto node = child->as_multi_func_node_unsafe();
                std::vector<Value*> args { parent_val, idx };
                const auto func = node->func_for_call(args);
                if(func) {
                    setType(func->returnType);
                } else {
                    diagnoser.error(this) << "no function with name 'index' exists for given arguments";
                    setType(typeBuilder.getVoidType());
                }
            } else {
                diagnoser.error(this) << "expected a function 'index' to be present for overloading";
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