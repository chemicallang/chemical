// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ordered_map.h"
#include "BaseDefMember.h"
#include "InheritedType.h"
#include "ast/base/BaseType.h"
#include <string>
#include <memory>

class ASTDiagnoser;

class VariablesContainer {
protected:

    /**
     * the nodes can be any type of node, when we parse, we put all nodes into this vector
     * then specialized functions are called during symbol resolution, to take variables
     * and functions into their specific maps
     * this allows putting nodes like if statement, for conditional compilation or a node
     * that may not be a member variable or function
     */
    std::vector<ASTNode*> nodes;

public:

    /**
     * indexes are maintained to nodes by their names, this allows us to get the function or variable in a single check
     * the pair contains the pointer to the node, which could be a variable, function (maybe generic), the second
     * the second in the pair is an int index, which if -1 means that we don't know what is it's index
     * the index for a variable means index of the variable -> inside the variables vector, this allows us to quickly
     * get the index to the variable, with the name, since llvm works with indexes and not names
     */
    std::unordered_map<chem::string_view, ASTNode*> indexes;

protected:

    /**
     * the variables, this vector is just calculated so we can traverse over it fast
     */
    std::vector<BaseDefMember*> variables_container;

#ifdef COMPILER_BUILD

    /**
     * need to keep track of extension functions, because llvm backend
     * requires to declare the functions, we must know which extension functions are being declared
     */
    std::vector<ASTNode*> extension_functions;

#endif

public:

    std::vector<InheritedType> inherited;

    /**
     * get the variables container
     */
    const std::vector<BaseDefMember*>& variables() {
        return variables_container;
    }

    /**
     * get the variable type along with index
     */
    std::pair<long, BaseType*> variable_type_index(const chem::string_view &name, bool consider_inherited_structs = true);

    long variable_index(const chem::string_view& name, bool consider_inherited_structs = true) {
        return variable_type_index(name, consider_inherited_structs).first;
    }

    long direct_mem_index(BaseDefMember* member);

    long direct_child_index(const chem::string_view &varName);

    uint64_t total_byte_size(bool is64Bit);

    uint64_t largest_member_byte_size(bool is64Bit);

    ASTNode* get_child(const chem::string_view& name) {
        auto found = indexes.find(name);
        return found != indexes.end() ? found->second : nullptr;
    }

    // no longer gives direct child, can also find inherited
    [[deprecated]]
    ASTNode* direct_child(const chem::string_view& name) {
        auto found = indexes.find(name);
        return found != indexes.end() ? found->second : nullptr;
    }

    BaseDefMember *child_def_member(const chem::string_view& name) {
        auto found = indexes.find(name);
        if(found == indexes.end()) return nullptr;
        const auto node = found->second;
        switch(node->kind()) {
            case ASTNodeKind::UnnamedUnion:
            case ASTNodeKind::UnnamedStruct:
            case ASTNodeKind::StructMember:
                return node->as_base_def_member_unsafe();
            default:
                return nullptr;
        }
    }

    void copy_variables_in_place(ASTAllocator& allocator, ASTNode* new_parent) {
        int i = 0;
        for(auto& var : variables_container) {
            const auto copied = var->copy_member(allocator);
            copied->set_parent(new_parent);
            var = copied;
            indexes[copied->name] = copied;
            i++;
        }
    }

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

    /**
     * ensure the inherited variables refer to structs / interfaces / unions that
     * are at least this spec
     */
    void ensure_inherited_visibility(ASTDiagnoser& diagnoser, AccessSpecifier at_least_spec);

    /**
     * this will remove all the variables and their indexes
     */
    void clear_variables_and_indexes() {
        variables_container.clear();
        indexes.clear();
    }

    /**
     * a variable is inserted into the container without check
     */
    void insert_variable_no_check(BaseDefMember* member) {
        const auto index = static_cast<int>(variables_container.size());
        variables_container.emplace_back(member);
        indexes[member->name] = member;
    }

    /**
     * insert a variable into this container
     */
    bool insert_variable(BaseDefMember* member) {
        auto found = indexes.find(member->name);
        if(found == indexes.end()) {
            insert_variable_no_check(member);
            return true;
        } else {
            return false;
        }
    }

    /**
     * this gives the reference to parsed nodes, which can be used to put
     * parsed nodes inside this container
     */
    std::vector<ASTNode*>& get_parsed_nodes_container() {
        return nodes;
    }

    bool is_one_of_inherited_type(BaseType* type) {
        for(auto& inh : inherited) {
            if(inh.type == type) {
                return true;
            }
        }
        return false;
    }

    /**
     * this method will automatically take variables from parsed nodes
     */
    void take_variables_from_parsed_nodes(SymbolResolver& linker, std::vector<ASTNode*>& nodes);

    inline void take_variables_from_parsed_nodes(SymbolResolver& linker) {
        if(!nodes.empty()) {
            take_variables_from_parsed_nodes(linker, nodes);
        }
    }

    /**
     * this method will automatically declare nodes from parsed nodes
     * THIS WILL NOT DECLARE variables, only aliases are declared
     */
    void declare_parsed_nodes(SymbolResolver& linker, std::vector<ASTNode*>& nodes);

    inline void declare_parsed_nodes(SymbolResolver& linker) {
        if(!nodes.empty()) {
            declare_parsed_nodes(linker, nodes);
        }
    }

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
            other.inherited.emplace_back(inh.type.copy(allocator), inh.specifier);
        }
        other.variables_container.reserve(variables_container.size());
        for(auto& var : variables_container) {
            const auto var_copy = var->copy_member(allocator);
            var_copy->set_parent(new_parent);
            other.insert_variable_no_check(var_copy);
        }
    }

    /**
     * shallow copies this container into the given container
     */
    void shallow_copy_into(VariablesContainer& other, ASTAllocator& allocator) {
        other.nodes = nodes;
        other.inherited = inherited;
        other.variables_container = variables_container;
        other.indexes = indexes;
#ifdef COMPILER_BUILD
        other.extension_functions = extension_functions;
#endif
    }

    /**
     * add an extension function
     */
    inline void add_extension_func(const chem::string_view& name, FunctionDeclaration* decl) {
        indexes[name] = (ASTNode*) decl;
#ifdef COMPILER_BUILD
        extension_functions.emplace_back((ASTNode*) decl);
#endif
    }

    /**
     * add extension function
     */
    inline void add_extension_func(const chem::string_view& name, GenericFuncDecl* decl) {
        indexes[name] = (ASTNode*) decl;
#ifdef COMPILER_BUILD
        extension_functions.emplace_back((ASTNode*) decl);
#endif
    }

    /**
     * all the methods of this interface will be extended to this
     * struct / member, so user can call it using dot notation obj.method
     */
    void adopt(MembersContainer* definition);

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