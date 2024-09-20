// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"

class FunctionType;

class BaseFunctionParam : public ASTNode {
public:

    std::string name;
    BaseType* type;
    FunctionType* func_type;

    /**
     * constructor
     */
    BaseFunctionParam(
            std::string name,
            BaseType* type,
            FunctionType* func_type = nullptr
    );

    ASTNodeKind kind() override {
        return ASTNodeKind::FunctionParam;
    }

    virtual unsigned calculate_c_or_llvm_index() = 0;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    BaseType *known_type() override {
        return type.get();
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, unsigned int index) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Value *llvm_load(Codegen &gen) override;

    void code_gen_destruct(Codegen &gen, Value *returnValue) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    FunctionParam *copy() const;

    ASTNode *child(const std::string &name) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void redeclare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode> &node_ptr) override;

    ValueType value_type() const override;

    BaseTypeKind type_kind() const override;

};