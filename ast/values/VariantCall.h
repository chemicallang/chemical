// Copyright (c) Qinetik 2024.

#include "ast/base/Value.h"
#include "TypeLinkedValue.h"

class VariantCall : public Value, public TypeLinkedValue {
public:

    std::unique_ptr<AccessChain> chain;
    std::vector<std::unique_ptr<Value>> values;
    std::vector<std::unique_ptr<BaseType>> generic_list;

    /**
     * the generic iteration is determined and stored at resolution phase
     * it represents the usage of generic parameters in variant definition for
     * the given generic args
     */
    int16_t generic_iteration = 0;

    /**
     * this will take the access chain, if has function call at least, own it's values
     */
    explicit VariantCall(std::unique_ptr<AccessChain> chain);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void relink_values(SymbolResolver &linker);

    void infer_generic_args(ASTDiagnoser& diagnoser, std::vector<BaseType*>& inferred);

    void link_args_implicit_constructor(SymbolResolver &linker);

    void link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr, BaseType *type) override;

    void link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) override;

    void link(SymbolResolver &linker, ReturnStatement *returnStmt) override {
        TypeLinkedValue::link(linker, returnStmt);
    }

    void link(SymbolResolver &linker, StructValue *value, const std::string &name) override {
        TypeLinkedValue::link(linker, value, name);
    }

    void link(SymbolResolver &linker, FunctionCall *call, unsigned int index) override {
        TypeLinkedValue::link(linker, call, index);
    }

    void link(SymbolResolver &linker, VarInitStatement *stmnt) override {
        TypeLinkedValue::link(linker, stmnt);
    }

    void link(SymbolResolver &linker, AssignStatement *stmnt, bool lhs) override {
        TypeLinkedValue::link(linker, stmnt, lhs);
    }

    void link(SymbolResolver &linker, ArrayValue *value, unsigned int index) override {
        TypeLinkedValue::link(linker, value, index);
    }

    std::unique_ptr<BaseType> create_type() override;

    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

    VariantCall* as_variant_call() override {
        return this;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Struct;
    }

    VariantMember* get_member();

    VariantDefinition* get_definition();

#ifdef COMPILER_BUILD

    bool initialize_allocated(Codegen &gen, llvm::Value* allocated, llvm::Type* def_type, VariantMember* member);

    llvm::Value* initialize_allocated(Codegen &gen, llvm::Value* allocated);

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

    llvm::AllocaInst* llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) override;

    llvm::Type* llvm_type(Codegen &gen) override;

    unsigned int store_in_struct(Codegen &gen, Value *parent, llvm::Value *allocated, llvm::Type *allocated_type, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) override;

    unsigned int store_in_array(Codegen &gen, ArrayValue *parent, llvm::AllocaInst *ptr, std::vector<llvm::Value *> idxList, unsigned int index, BaseType *expected_type) override;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

#endif

};