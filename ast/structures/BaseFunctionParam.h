// Copyright (c) Qinetik 2024.

#include "ast/base/ASTNode.h"

class FunctionType;

class BaseFunctionParam : public ASTNode {
public:

    chem::string_view name;
    BaseType* type;
    FunctionType* func_type;

    /**
     * constructor
     */
    BaseFunctionParam(
            chem::string_view name,
            BaseType* type,
            FunctionType* func_type = nullptr
    ) : name(name), type(type), func_type(func_type) {

    };

    ASTNodeKind kind() {
        return ASTNodeKind::FunctionParam;
    }

    virtual unsigned calculate_c_or_llvm_index() = 0;

    BaseType* create_value_type(ASTAllocator &allocator) final;

    BaseType *known_type() {
        return type;
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    llvm::Value *llvm_load(Codegen &gen) final;

    void code_gen_destruct(Codegen &gen, Value *returnValue) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

    FunctionParam *copy() const;

    ASTNode *child(const chem::string_view &name);

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr);

    void redeclare_top_level(SymbolResolver &linker) final;

    [[nodiscard]]
    ValueType value_type() const final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};