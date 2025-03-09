// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class FunctionType;

struct FunctionParamAttributes {

    /**
     * has the address been taken of the function param
     * this means now a variable will be generated in which the parameter value
     * will be stored and loaded subsequently
     */
    bool has_address_taken_of = false;

    /**
     * has moved is used to indicate that an object at this location has moved
     * destructor is not called on moved objects, once moved, any attempt to access
     * this variable causes an error
     */
    bool has_moved = false;
    /**
     * has moved is used to track that var init statement has a assignment to it
     * if it has, during symbol resolution the assignment statement notifies this
     * var init statement
     */
    bool has_assignment = false;

    /**
     * is this parameter implicit
     */
    bool is_implicit = false;


};

class FunctionParam : public ASTNode {
public:

    chem::string_view name;
    BaseType* type;
    unsigned int index;
    Value* defValue;
    FunctionParamAttributes attrs;

#ifdef COMPILER_BUILD
    // the pointer function param, loaded once
    llvm::Value* pointer = nullptr;
#endif

    /**
     * constructor
     */
    constexpr FunctionParam(
            chem::string_view name,
            BaseType* type,
            unsigned int index,
            Value* defValue,
            bool is_implicit,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::FunctionParam, parent_node, location), name(name), type(type),
        index(index), defValue(defValue), attrs(false, false, false, is_implicit)
    {

    }

    inline const chem::string_view& name_view() const noexcept {
        return name;
    }

    inline bool has_address_taken() const noexcept {
        return attrs.has_address_taken_of;
    }

    inline void set_has_address_taken(bool value) {
        attrs.has_address_taken_of = value;
    }

    inline bool is_implicit() const {
        return attrs.is_implicit;
    }

    inline void set_is_implicit(bool value) {
        attrs.is_implicit = value;
    }

    inline bool get_has_moved() const noexcept {
        return attrs.has_moved;
    }

    inline void set_has_moved(bool value) {
        attrs.has_moved = value;
    }

    inline void moved() {
        attrs.has_moved = true;
    }

    inline void unmove() {
        attrs.has_moved = false;
    }

    inline bool get_has_assignment() const noexcept {
        return attrs.has_assignment;
    }

    inline void set_has_assignment() {
        attrs.has_assignment = true;
    }

    Value *holding_value() final {
        return defValue;
    }

    unsigned calculate_c_or_llvm_index(FunctionType* func_type);

    BaseType* create_value_type(ASTAllocator &allocator) final;

    BaseType *known_type() final {
        return type;
    }

    [[nodiscard]]
    FunctionParam *copy(ASTAllocator& allocator) const;

    bool link_param_type(SymbolResolver &linker);

#ifdef COMPILER_BUILD

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    llvm::Value *llvm_load(Codegen& gen, SourceLocation location) final;

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