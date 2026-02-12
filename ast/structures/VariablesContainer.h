// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ordered_map.h"
#include "BaseDefMember.h"
#include "InheritedType.h"
#include "ast/base/BaseType.h"
#include <string>
#include <memory>

class ASTDiagnoser;

/**
 * this class exists solely for containing variables in a container
 * it can contain other nodes like if statement which then contains variables inside (for comptime conditional)
 * but the sole purpose is to contain variables, be able to traverse variables chronologically, be able to
 * find a variable using a given name
 */
class VariablesContainerBase {
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
     * the variables, this vector is just calculated so we can traverse over it fast
     */
    std::vector<BaseDefMember*> variables_container;

    /**
     * indexes are maintained to nodes by their names, this allows us to get the function or variable in a single check
     * contains the pointer to the node, which could be a variable, function (maybe generic)
     * indexes are passed to containers that inherit this container
     */
    std::unordered_map<chem::string_view, ASTNode*> indexes;

    /**
     * get the variables container
     */
    const std::vector<BaseDefMember*>& variables() {
        return variables_container;
    }

    /**
     * gets any child (inherited or direct)
     */
    ASTNode* any_child(const chem::string_view& name) {
        auto found = indexes.find(name);
        return found != indexes.end() ? found->second : nullptr;
    }

    /**
     * any child variable (member) inherited / direct
     */
    BaseDefMember* any_child_def_member(const chem::string_view& name) {
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

    /**
     * any child variable (member) inherited or direct
     */
    inline BaseDefMember* child_member(const chem::string_view& name) {
        return any_child_def_member(name);
    }

    /**
     * any child variable (member) inherited or direct
     */
    inline BaseDefMember* any_child_variable(const chem::string_view& name) {
        return any_child_def_member(name);
    }

    /**
     * get the index of given variable (should be direct child)
     */
    long direct_mem_index(BaseDefMember* member);

    /**
     * get direct variable with its index
     */
    std::pair<BaseType*, long> variable_type_w_index_no_inherited(const chem::string_view &name) {
        auto found = indexes.find(name);
        if(found == indexes.end()) return { nullptr, -1 };
        const auto mem = found->second->as_base_def_member();
        if(!mem) return { nullptr, -1 };
        return { mem->known_type(), direct_mem_index(found->second->as_base_def_member_unsafe()) };
    }

    /**
     * get index of a direct variable
     */
    long direct_variable_index_no_inherited(const chem::string_view& varName) {
        auto found = indexes.find(varName);
        return found == indexes.end() ? -1 : direct_mem_index(found->second->as_base_def_member_unsafe());
    }

    /**
     * get the largest member (calculated by comparing byte_size)
     */
    BaseDefMember* largest_member();

    /**
     * calculate the total byte size of the variables
     */
    uint64_t total_byte_size(TargetData& target);

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
     * deep copies this container into the given container
     */
    void copy_direct_variables_into(VariablesContainerBase& other, ASTAllocator& allocator, ASTNode* new_parent) {
        other.variables_container.reserve(variables_container.size());
        for(auto& var : variables_container) {
            const auto var_copy = var->copy_member(allocator);
            var_copy->set_parent(new_parent);
            other.insert_variable_no_check(var_copy);
        }
    }


#ifdef COMPILER_BUILD

    /**
     * get types of all direct variables into a vector
     */
    std::vector<llvm::Type*> direct_variables_type(Codegen &gen);

    /**
     * get types of all direct variables into a vector considering that member with given index is being
     * accessed, this propagates the index forward to the member so it can give accurate type based on union
     * member access
     */
    std::vector<llvm::Type*> direct_variables_type(Codegen &gen, std::vector<Value*>& chain, unsigned index);

    /**
     * add the child index for the given struct variable
     */
    bool llvm_variables_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const chem::string_view &name
    );

    /**
     * add the child index for member of given name if considering the current container
     * is a union
     */
    bool llvm_union_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const chem::string_view &name
    );

#endif

};

class VariablesContainer : public ASTNode, public VariablesContainerBase {
protected:

#ifdef COMPILER_BUILD

    /**
     * need to keep track of extension functions, because llvm backend
     * requires to declare the functions, we must know which extension functions are being declared
     */
    std::vector<ASTNode*> extension_functions;

#endif

public:

    /**
     * the inherited types
     */
    std::vector<InheritedType> inherited;

    /**
     * constructor
     */
    VariablesContainer(ASTNodeKind k, ASTNode* parent, SourceLocation loc) : ASTNode(k, parent, loc) {

    }

    /**
     * get the variable type along with index
     */
    std::pair<BaseType*, long> variable_type_w_index(const chem::string_view &name, bool check_inherited_names = true);

    /**
     * get index of the given variable
     */
    inline long variable_index(const chem::string_view& name, bool check_inherited_names = true) {
        return variable_type_w_index(name, check_inherited_names).second;
    }

    long direct_child_index(const chem::string_view &varName);

    uint64_t largest_member_byte_size(TargetData& target);

    /**
     * checks the given node is a direct chil of this container
     */
    bool isDirectChild(ASTNode* node);

    /**
     * child member (variable / function) direct only (NOT inherited)
     */
    ASTNode* direct_child(const chem::string_view& name) {
        const auto c = any_child(name);
        return c ? isDirectChild(c) ? c : nullptr : nullptr;
    }

    /**
     * child variable (member) direct only (NOT inherited)
     */
    BaseDefMember *direct_variable(const chem::string_view& name) {
        const auto c = any_child_def_member(name);
        return c ? isDirectChild(c) ? c : nullptr : nullptr;
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

    // searches for member or a inherited struct by the given name
    ASTNode *child_member_or_inherited_struct(const chem::string_view& name);

    /**
     * ensure the inherited variables refer to structs / interfaces / unions that
     * are at least this spec
     */
    void ensure_inherited_visibility(ASTDiagnoser& diagnoser, AccessSpecifier at_least_spec);

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

    std::vector<llvm::Type *> elements_type(Codegen &gen, std::vector<Value*>& chain, unsigned index);

    bool llvm_struct_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const chem::string_view &name
    );

#endif

};