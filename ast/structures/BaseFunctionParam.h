// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/ASTNode.h"

class FunctionType;

struct FunctionParamAttributes {

    /**
     * has the address been taken of the function param
     * this means now a variable will be generated in which the parameter value
     * will be stored and loaded subsequently
     */
    bool has_address_taken_of;

};

class BaseFunctionParam : public ASTNode {
public:

    chem::string_view name;
    BaseType* type;
    FunctionType* func_type;
    FunctionParamAttributes attrs;

#ifdef COMPILER_BUILD
    // the pointer function param, loaded once
    llvm::Value* pointer = nullptr;
#endif

    /**
     * constructor
     */
    constexpr BaseFunctionParam(
            chem::string_view name,
            BaseType* type,
            FunctionType* func_type,
            ASTNodeKind k,
            SourceLocation loc
    ) : ASTNode(k, loc), name(name), type(type), func_type(func_type), attrs(false) {

    };

    inline bool has_address_taken() {
        return attrs.has_address_taken_of;
    }

    inline void set_has_address_taken(bool value) {
        attrs.has_address_taken_of = value;
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

    void code_gen(Codegen &gen) override;

    void code_gen_destruct(Codegen &gen, Value *returnValue) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

    FunctionParam *copy() const;

    ASTNode *child(const chem::string_view &name);

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr);

    void redeclare_top_level(SymbolResolver &linker) final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

};