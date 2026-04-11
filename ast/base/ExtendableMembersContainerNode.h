// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/structures/MembersContainer.h"
#include "ast/structures/GenericFuncDecl.h"

class ExtendableMembersContainerNode : public MembersContainer {
public:

    using MembersContainer::requires_moving;

    /**
     * the identifier of the container
     */
    chem::string_view identifier;

    /**
     * need to keep track of extension functions, because llvm backend
     * requires to declare the functions, we must know which extension functions are being declared
     */
    std::vector<ASTNode*> extension_functions;

    /**
     * constructor
     */
    ExtendableMembersContainerNode(
        chem::string_view identifier,
        ASTNodeKind k,
        ASTNode* parent,
        SourceLocation location
    ) : MembersContainer(k, parent, location), identifier(identifier) {

    }

    /**
     * get the name of the container
     */
    inline const chem::string_view& name_view() const {
        return identifier;
    }

    /**
     * get name as a string
     */
    inline std::string name_str() const {
        return name_view().str();
    }

    inline void shallow_copy_into(ExtendableMembersContainerNode& other, ASTAllocator& allocator) {
        MembersContainer::shallow_copy_into(other, allocator);
        other.extension_functions = extension_functions;
    }

    /**
 * add an extension function
 */
    inline void add_extension_func(const chem::string_view& name, FunctionDeclaration* decl) {
        indexes[name] = (ASTNode*) decl;
        extension_functions.emplace_back((ASTNode*) decl);
    }

    /**
     * add extension function
     */
    inline void add_extension_func(const chem::string_view& name, GenericFuncDecl* decl) {
        indexes[name] = (ASTNode*) decl;
        extension_functions.emplace_back((ASTNode*) decl);
    }

#ifdef COMPILER_BUILD

    /**
     * this method is dedicated for this extendable containers to externally declare themselves
     * this should be called in code_gen_external_declare, which is called upon nodes to declare
     * themselves in other modules when they are imported
     * this declares the functions inside the container (like MembersContainer) but it also
     * externally declares the extension functions inside this container
     */
    void extendable_external_declare(Codegen& gen);

#endif

};
