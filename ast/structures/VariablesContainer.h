// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ordered_map.h"
#include "BaseDefMember.h"
#include "InheritedType.h"
#include "ast/base/BaseType.h"
#include <string>
#include <memory>

class VariablesContainer {
public:

    std::vector<InheritedType> inherited;
    tsl::ordered_map<chem::string_view, BaseDefMember*> variables;

    /**
     * get the variable type along with index
     */
    std::pair<long, BaseType*> variable_type_index(const chem::string_view &name, bool consider_inherited_structs = true);

    long variable_index(const chem::string_view& name, bool consider_inherited_structs = true) {
        return variable_type_index(name, consider_inherited_structs).first;
    }

    long direct_child_index(const chem::string_view &varName);

    uint64_t total_byte_size(bool is64Bit);

    uint64_t largest_member_byte_size(bool is64Bit);

    BaseDefMember *child_def_member(const chem::string_view& name);

    inline BaseDefMember *direct_variable(const chem::string_view& name) {
        return child_def_member(name);
    }

    // searches for member or a inherited struct by the given name
    ASTNode *child_member_or_inherited_struct(const chem::string_view& name);

    // member of this name that exists in inherited structs
    BaseDefMember *inherited_member(const chem::string_view& name);

    // direct member or inherited member
    BaseDefMember *child_member(const chem::string_view& name);

    BaseDefMember* largest_member();

    virtual void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr);

    /**
     * when child is located up somewhere in inheritance tree, we build the path to it
     * the last index in the path is of the child, all the indexes before are indexes to
     * the inherited structs
     * The path starts from this variables container, so only this container can resolve it
     * @return true if child is found, false if not
     */
    bool build_path_to_child(std::vector<int>& path, const chem::string_view& child_name);

    /**
     * check if given interface is overridden by this
     */
    bool does_override(InterfaceDefinition* interface);

    /**
     * deep copies this container into the given container
     */
    void copy_into(VariablesContainer& other, ASTAllocator& allocator, ASTNode* new_parent) {
        other.inherited.reserve(inherited.size());
        for(auto& inh : inherited) {
            other.inherited.emplace_back(inh.type->copy(allocator), inh.specifier);
        }
        other.variables.reserve(variables.size());
        for(auto& var : variables) {
            const auto var_copy = var.second->copy_member(allocator);
            var_copy->set_parent(new_parent);
            other.variables[var.first] = var_copy;
        }
    }

    virtual VariablesContainer* copy_container(ASTAllocator& allocator) {
        throw std::runtime_error("copy container called on variables container, should be overridden");
    }

#ifdef COMPILER_BUILD

    std::vector<llvm::Type *> elements_type(Codegen &gen);

    std::vector<llvm::Type *> elements_type(Codegen &gen, std::vector<ChainValue*>& chain, unsigned index);

    bool llvm_struct_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const chem::string_view &name
    );

    bool llvm_union_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const chem::string_view &name
    );

#endif

};