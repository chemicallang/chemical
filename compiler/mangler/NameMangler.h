// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"

class ASTNode;

class FunctionDeclaration;

class InterfaceDefinition;

class StructDefinition;

/**
 * the name mangler is simply responsible for creating runtime names
 * for a function, struct or anything that has a runtime footprint
 */
class NameMangler {
private:

    /**
     * mangle non_func is a private function that doesn't handle functions
     */
    bool mangle_non_func(std::ostream& stream, ASTNode* node);

public:

    /**
     * same as the mangle function below, only no parent runtime name is written
     * for example when mangling a function, we're not concerned with it's container parent
     * we're concerned with the function only, since parent is handled by the container appropriately
     */
    void mangle_no_parent(std::ostream& stream, ASTNode* node, const chem::string_view& node_id);

    /**
     * this node would be mangled, if this node doesn't have a runtime footprint, we return false
     * if we succeed with producing a name, we return true
     * everything would be written to the given ostream
     */
    bool mangle(std::ostream& stream, ASTNode* node);

    /**
     * functions are handled specially, this is because they can be in interfaces, where we must
     * not use interface names and instead use implementation names
     */
    void mangle(std::ostream& stream, FunctionDeclaration* decl);

    /**
     * mangle the function name to a string, without module or scope naming
     */
    std::string mangle(FunctionDeclaration* decl);

    /**
     * mangle the node's name to a string
     */
    std::string mangle(ASTNode* node);

    /**
     * mangle the vtable name for an interface and struct that implements the interface
     */
    void mangle_vtable_name(std::ostream& stream, InterfaceDefinition* interface, StructDefinition* def);

    /**
     * get the vtable name as a string
     */
    std::string mangle_vtable_name(InterfaceDefinition* interface, StructDefinition* def);

};