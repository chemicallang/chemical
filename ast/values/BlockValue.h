// Copyright (c) Qinetik 2025.

#include "ast/base/Value.h"
#include "ast/structures/Scope.h"

class BlockValue : public Value {
public:

    Scope scope;

    /**
     * this is calculated during symbol resolution
     */
    Value* calculated_value = nullptr;

    /**
     * constructor
     */
    BlockValue(Scope scope) : scope(std::move(scope)) {

    }

    SourceLocation encoded_location() override {
        return scope.encoded_location();
    }

    ValueKind val_kind() override {
        return ValueKind::BlockValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override;

    BaseType* create_type(ASTAllocator &allocator);

    Value* copy(ASTAllocator &allocator) override {
#ifdef DEBUG
       throw std::runtime_error("block value cannot be copied");
#endif
        const auto blockVal = new (allocator.allocate<BlockValue>()) BlockValue(scope.shallow_copy());
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