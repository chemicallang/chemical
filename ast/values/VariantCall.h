// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"

class VariantCall : public Value {
public:

    AccessChain* chain;
    std::vector<Value*> values;
    std::vector<BaseType*> generic_list;
    BaseType* cached_type = nullptr;
    SourceLocation location;

    /**
     * the generic iteration is determined and stored at resolution phase
     * it represents the usage of generic parameters in variant definition for
     * the given generic args
     */
    int16_t generic_iteration = 0;

    /**
     * this will take the access chain, if has function call at least, own it's values
     */
    explicit VariantCall(AccessChain* chain, SourceLocation location);

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::VariantCall;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void relink_values(SymbolResolver &linker);

    void infer_generic_args(ASTDiagnoser& diagnoser, std::vector<BaseType*>& inferred);

    void link_args_implicit_constructor(SymbolResolver &linker);

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *type) final;

    void set_created_type(ASTAllocator& allocator);

    BaseType* create_type(ASTAllocator &allocator) final;

    BaseType* known_type() final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Struct;
    }

    VariantMember* get_member();

    VariantDefinition* get_definition();

#ifdef COMPILER_BUILD

    bool initialize_allocated(Codegen &gen, llvm::Value* allocated, llvm::Type* def_type, VariantMember* member);

    llvm::Value* initialize_allocated(Codegen &gen, llvm::Value* allocated);

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) final;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) final;

    llvm::Type* llvm_type(Codegen &gen) final;

    unsigned int store_in_struct(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type *allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType *expected_type
    ) final;

    unsigned int store_in_array(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type *allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType *expected_type
    ) final;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) final;

#endif

};