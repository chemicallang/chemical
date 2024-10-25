// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "MembersContainer.h"
#include <optional>
#include <map>
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/types/StructType.h"
#include "ast/types/LinkedType.h"

class StructDefinition : public ExtendableMembersContainerNode, public StructType {
public:

    AccessSpecifier specifier;
    bool is_direct_init = false;
    ASTNode* parent_node;
    SourceLocation location;
    LinkedType linked_type;

#ifdef COMPILER_BUILD
    /**
     * the key here is the generic iteration, where as the value corresponds to
     * a llvm struct type, we create a struct type once, and then cache it
     */
    std::unordered_map<int16_t, llvm::StructType*> llvm_struct_types;
    llvm::GlobalVariable* vtable_pointer = nullptr;
#endif

    /**
     * @brief Construct a new StructDeclaration object.
     *
     * @param name The name of the struct.
     * @param fields The members of the struct.
     */
    StructDefinition(
            std::string name,
            ASTNode* parent_node,
            SourceLocation location,
            AccessSpecifier specifier = AccessSpecifier::Internal
    );

    ASTNodeKind kind() final {
        return ASTNodeKind::StructDecl;
    }

    std::string get_runtime_name() final {
        if(has_annotation(AnnotationKind::Anonymous)) {
            return "";
        }
        return runtime_name_str();
    }

    int16_t get_generic_iteration() final {
        return active_iteration;
    }

    VariablesContainer *copy_container(ASTAllocator& allocator) final;

    ASTNode *linked_node() final {
        return this;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    SourceLocation encoded_location() override {
        return location;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    VariablesContainer *as_variables_container() final {
        return this;
    }

    const std::string& ns_node_identifier() final {
        return name;
    }

    VariablesContainer *variables_container() final {
        return this;
    }

    bool is_exported_fast() {
        return specifier == AccessSpecifier::Public;
    }

    bool is_generic() {
        return !generic_params.empty();
    }

    void accept(Visitor *visitor) final;

    void declare_top_level(SymbolResolver &linker) final;

    void redeclare_top_level(SymbolResolver &linker) final;

    void declare_and_link(SymbolResolver &linker) final;

    ASTNode *child(const std::string &name) final;

    BaseType* create_value_type(ASTAllocator& allocator) final;

    BaseType* known_type() final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Struct;
    }

    uint64_t byte_size(bool is64Bit) final {
        return total_byte_size(is64Bit);
    }

    [[nodiscard]]
    BaseType* copy(ASTAllocator &allocator) const final;

#ifdef COMPILER_BUILD

    llvm::StructType* llvm_stored_type() final;

    void llvm_store_type(llvm::StructType* type) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    /**
     * will try to override the given function if there's an interface and it exists
     * in the inherited struct / interface, otherwise returns false
     */
    bool llvm_override(Codegen& gen, FunctionDeclaration* declaration);

    /**
     * generate code for all functions in this struct
     */
    void struct_func_gen(
        Codegen& gen,
        const std::vector<FunctionDeclaration*>& funcs,
        bool declare
    );

    /**
     * this function is responsible for declaring this single function
     * that is present inside this struct, also read the docs of body
     */
    void code_gen_function_declare(Codegen& gen, FunctionDeclaration* decl);

    /**
     * this function is responsible for generating code for a single function
     * this function is not supposed to be called, because struct decl tends to
     * generate declarations for all it's functions and then bodies, so that
     * functions above can call functions declared below
     * However this is required because generic functions inside structs can have
     * uses outside the current file, the function is queued for generation for that type
     * and then function declaration calls this function (if this struct is parent)
     */
    void code_gen_function_body(Codegen& gen, FunctionDeclaration* decl);

    void code_gen(Codegen &gen, bool declare);

    void code_gen_declare(Codegen &gen) final {
        code_gen(gen, true);
    }

    void code_gen(Codegen &gen) final {
        code_gen(gen, false);
    }

    void code_gen_generic(Codegen &gen) final;

    void code_gen_external_declare(Codegen &gen) final;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) final;

#endif

};