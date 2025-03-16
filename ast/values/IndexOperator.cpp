// Copyright (c) Chemical Language Foundation 2025.

#include "IndexOperator.h"
#include "ast/values/AccessChain.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/StructDefinition.h"
#include "ast/types/ArrayType.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
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
    for (auto &value: values) {
        idxList.emplace_back(value->llvm_value(gen));
    }
    idxList.shrink_to_fit();
    return gen.builder->CreateGEP(type, ptr, idxList, "", gen.inbounds);
}

llvm::Value *IndexOperator::llvm_pointer(Codegen &gen) {
    auto pure_type = parent_val->get_pure_type(gen.allocator);
    if(pure_type->is_pointer()) {
        auto parent_value = parent_val->llvm_value(gen, nullptr);
        auto child_type = pure_type->create_child_type(gen.allocator);
        return elem_pointer(gen, child_type->llvm_type(gen), parent_value);
    } else {
        return elem_pointer(gen, parent_val->llvm_type(gen), parent_val->llvm_pointer(gen));
    }
}

llvm::Value *IndexOperator::llvm_value(Codegen &gen, BaseType* expected_type) {
    return Value::load_value(gen, create_type(gen.allocator), llvm_type(gen), llvm_pointer(gen), encoded_location());
}

bool IndexOperator::add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) {
    const auto parent_type = parent->create_type(gen.allocator)->pure_type(gen.allocator);
    if (parent_type->kind() == BaseTypeKind::Array && indexes.empty() && (parent->linked_node() == nullptr || parent->linked_node()->as_func_param() == nullptr )) {
        indexes.push_back(gen.builder->getInt32(0));
    }
    for (auto &value: values) {
        indexes.emplace_back(value->llvm_value(gen));
    }
    return true;
}

bool IndexOperator::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    return create_type(gen.allocator)->linked_node()->add_child_index(gen, indexes, name);
}

llvm::Type *IndexOperator::llvm_type(Codegen &gen) {
    return create_type(gen.allocator)->llvm_type(gen);
}

llvm::Type *IndexOperator::llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &chain, unsigned int index) {
    return create_type(gen.allocator)->llvm_chain_type(gen, chain, index);
}

#endif

BaseType* IndexOperator::create_type(ASTAllocator& allocator) {
//    std::vector<int16_t> active;
//    set_generic_iteration(active, allocator);
    int i = (int) values.size();
    auto current_type = parent_val->create_type(allocator);
    while(i > 0) {
        const auto childType = current_type->create_child_type(allocator);
#ifdef DEBUG
        if(!childType) {
            throw std::runtime_error("couldn't create the child type in index operator");
        }
#endif
        current_type = childType;
        i--;
    }
//    restore_generic_iteration(active, allocator);
    return current_type;
}

bool IndexOperator::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    parent_val->link(linker, (Value*&) parent_val, nullptr);
    for(auto& value : values) {
        value->link(linker, value);
    }
    return true;
}

ASTNode *IndexOperator::linked_node() {
    auto value_type = parent_val->known_type();
    const auto child_type = value_type->known_child_type();
    return child_type->linked_node();
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
            return new (scope.allocate<CharValue>()) CharValue(str->value[index.value()], location);
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
    const auto total = values.size();
    Value* eval = parent_val->evaluated_value(scope);
    if(!eval) return nullptr;
    while(i < total) {
        eval = index_inside(scope, eval, values[i], values[i]->encoded_location());
        i++;
    }
    return eval;
}

Value *IndexOperator::find_in(InterpretScope &scope, Value *parent) {
    auto value = values[0];
    if (values.size() > 1) {
        scope.error("Index operator only supports a single integer index at the moment", this);
    }
#ifdef DEBUG
    try {
        return parent->index(scope, value->evaluated_value(scope)->get_the_int());
    } catch (...) {
        std::cerr << "[InterpretError] index operator only support's integer indexes at the moment";
    }
#endif
    parent->index(scope, value->evaluated_value(scope)->get_the_int());
    return nullptr;
}

IndexOperator* IndexOperator::copy(ASTAllocator& allocator) {
    auto op = new (allocator.allocate<IndexOperator>()) IndexOperator((ChainValue*) parent_val->copy(allocator), encoded_location());
    for(const auto value : values) {
        op->values.emplace_back(value->copy(allocator));
    }
    return op;
}

BaseType* IndexOperator::known_type() {
    return parent_val->known_type()->known_child_type();
}