// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"

class BaseFunctionType;

class BaseFunctionParam : public ASTNode {
public:

    std::string name;
    std::unique_ptr<BaseType> type;
    BaseFunctionType* func_type;

    /**
     * constructor
     */
    BaseFunctionParam(
        std::string name,
        std::unique_ptr<BaseType> type,
        BaseFunctionType* func_type = nullptr
    );

    virtual unsigned calculate_c_or_llvm_index() = 0;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    BaseFunctionParam *as_base_func_param() override {
        return this;
    }

    BaseType *holding_value_type() override {
        return type.get();
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Value *llvm_load(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    FunctionParam *copy() const;

    ASTNode *child(const std::string &name) override;

    void declare_and_link(SymbolResolver &linker) override;

    ValueType value_type() const override;

    BaseTypeKind type_kind() const override;

};