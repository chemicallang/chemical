// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"

class VariantCall : public Value {
public:

    std::unique_ptr<AccessChain> chain;
    std::vector<std::unique_ptr<Value>> values;
    std::vector<std::unique_ptr<BaseType>> generic_list;

    /**
     * this will take the access chain, if has function call at least, own it's values
     */
    explicit VariantCall(std::unique_ptr<AccessChain> chain);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) override;

    VariantCall* as_variant_call() override {
        return this;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Struct;
    }

#ifdef COMPILER_BUILD

    bool initialize_allocated(Codegen &gen, llvm::Value* allocated, llvm::Type* def_type, VariantMember* member);

    llvm::Value* initialize_allocated(Codegen &gen, llvm::Value* allocated);

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

    unsigned int store_in_struct(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) override;

    unsigned int store_in_array(Codegen &gen, ArrayValue *parent, llvm::AllocaInst *ptr, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) override;

#endif

};