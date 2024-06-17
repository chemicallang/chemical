// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Visitor.h"
#include <memory>
#include <vector>

/**
 * Shrinking visitor has one purpose, shrink the nodes.
 * Why ? After importing a file, we must not keep it's whole parsed AST in memory
 *
 * We need the symbols inside a file, so files that imported it, can resolve symbols in it
 * But only the top level symbols, we don't need variables inside functions
 * so we can just dispose function bodies
 *
 * This visitor visits functions and delete their bodies, functions inside structs, interfaces, all their bodies are cleared
 * Only things that are required for successful symbol resolution are kept, for example enum members are kept
 *
 */
class ShrinkingVisitor : Visitor {
public:

    void visit(std::vector<std::unique_ptr<ASTNode>>& nodes);

private:

    void visit(FunctionDeclaration *functionDeclaration) override;

    void visit(ExtensionFunction *extensionFunc) override;

    void visit(StructDefinition *structDefinition) override;

    void visit(InterfaceDefinition *interfaceDefinition) override;

    void visit(ImplDefinition *implDefinition) override;

};