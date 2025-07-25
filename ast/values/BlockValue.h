// Copyright (c) Qinetik 2025.

#include "ast/base/Value.h"
#include "ast/structures/Scope.h"

class BlockValue : public Value {

    /**
     * this is calculated during symbol resolution
     */
    Value* calculated_value = nullptr;

public:

    Scope scope;

#ifdef COMPILER_BUILD
    /**
     * tracks to only output code for scope once
     */
    bool has_code_gen_scope = false;
#endif

    /**
     * constructor
     */
    constexpr BlockValue(
            ASTNode* parent_node,
            SourceLocation location
    ) : Value(ValueKind::BlockValue, location), scope(parent_node, location) {

    }

    /**
     * constructor
     */
    BlockValue(Scope scope, BaseType* type) : Value(ValueKind::BlockValue, type, scope.encoded_location()), scope(std::move(scope)) {

    }

    BaseType* create_type(ASTAllocator &allocator);

    inline Value* getCalculatedValue() {
        return calculated_value;
    }

    void setCalculatedValue(Value* value) {
        calculated_value = value;
        if(value) {
            setType(value->getType());
        }
    }

    Value* copy(ASTAllocator &allocator) override {
#ifdef DEBUG
       throw std::runtime_error("block value cannot be copied");
#endif
        const auto blockVal = new (allocator.allocate<BlockValue>()) BlockValue(scope.shallow_copy(), getType());
        blockVal->calculated_value = calculated_value;
        return blockVal;
    }

#ifdef COMPILER_BUILD

    llvm::AllocaInst* llvm_allocate(
            Codegen& gen,
            const std::string& identifier,
            BaseType* expected_type
    ) final;

    unsigned int store_in_struct(
            Codegen& gen,
            Value* parent,
            llvm::Value* allocated,
            llvm::Type* allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType* expected_type
    ) final;

    unsigned int store_in_array(
            Codegen& gen,
            Value* parent,
            llvm::Value* allocated,
            llvm::Type* allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType* expected_type
    ) final;

    llvm::Value* llvm_pointer(Codegen& gen) final;

    llvm::Value* llvm_value(Codegen& gen, BaseType* type = nullptr) final;

    void llvm_conditional_branch(Codegen& gen, llvm::BasicBlock* then_block, llvm::BasicBlock* otherwise_block) final;

#endif


};