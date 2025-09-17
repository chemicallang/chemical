// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
#include "preprocess/2c/BufferedWriter.h"

class ASTNode;

class StructValue;

class FunctionDeclaration;

class InterfaceDefinition;

class StructDefinition;

/**
 * the name mangler is simply responsible for creating runtime names
 * for a function, struct or anything that has a runtime footprint
 */
class NameMangler {
public:

    /**
     * mangle non_func is a function that doesn't handle functions
     */
    bool mangle_non_func(BufferedWriter& stream, ASTNode* node);

    /**
     * mangle the linked container of given struct value
     */
    void mangle_linked(BufferedWriter& stream, StructValue* value);

    /**
     * same as the mangle function below, only no parent runtime name is written
     * for example when mangling a function, we're not concerned with it's container parent
     * we're concerned with the function only, since parent is handled by the container appropriately
     */
    void mangle_no_parent(BufferedWriter& stream, ASTNode* node);

    /**
     * this node would be mangled, if this node doesn't have a runtime footprint, we return false
     * if we succeed with producing a name, we return true
     * everything would be written to the given ostream
     */
    bool mangle(BufferedWriter& stream, ASTNode* node);

    /**
     * this will use the specified parent and mangle it as the parent of the given function
     */
    void mangle_func_parent(BufferedWriter& stream, FunctionDeclaration* decl, ASTNode* parent);

    /**
     * this will get the parent automatically
     */
    void mangle_func_parent(BufferedWriter& stream, FunctionDeclaration* decl);

    /**
     * functions are handled specially, this is because they can be in interfaces, where we must
     * not use interface names and instead use implementation names
     */
    void mangle(BufferedWriter& stream, FunctionDeclaration* decl);

    /**
     * mangle the vtable name for an interface and struct that implements the interface
     */
    void mangle_vtable_name(BufferedWriter& stream, InterfaceDefinition* interface, StructDefinition* def);

};