// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class EmbeddedNode;

typedef void(EmbeddedNodeSymbolResolveFunc)(SymbolResolver* resolver, EmbeddedNode* value);

typedef ASTNode*(EmbeddedNodeReplacementFunc)(ASTAllocator* allocator, EmbeddedNode* value);

typedef BaseType*(EmbeddedNodeKnownTypeFunc)(EmbeddedNode* value);

typedef ASTNode*(EmbeddedNodeChildResolutionFunc)(EmbeddedNode* value, chem::string_view* name);

typedef void(EmbeddedNodeTraversalFunc)(EmbeddedNode* node, void* data, bool(*traverse)(void* data, ASTAny* item));

class EmbeddedNode : public ASTNode {
public:

    /**
     * user can store his ast in this pointer
     */
    void* data_ptr;

    /**
     * the symbol resolve function is called by the symbol resolver
     */
    EmbeddedNodeSymbolResolveFunc* sym_res_fn;

    /**
     * the replacement function is called to replace it as a value by the backend
     */
    EmbeddedNodeReplacementFunc* replacement_fn;

    /**
     * type creation function is used to create a type for this value
     */
    EmbeddedNodeKnownTypeFunc* known_type_fn;

    /**
     * child resolution function is used to resolve children of this embedded value
     */
    EmbeddedNodeChildResolutionFunc* child_res_fn;

    /**
     * the traversal function is used to traverse the embedded node
     */
    EmbeddedNodeTraversalFunc* traversal_fn;

    /**
     * constructor
     */
    EmbeddedNode(
            void* data_ptr,
            EmbeddedNodeSymbolResolveFunc* sym_res_fn,
            EmbeddedNodeReplacementFunc* replacement_fn,
            EmbeddedNodeKnownTypeFunc* known_type_fn,
            EmbeddedNodeChildResolutionFunc* child_res_fn,
            EmbeddedNodeTraversalFunc* traversal_fn,
            ASTNode* parent_node,
            SourceLocation loc
    ) : ASTNode(ASTNodeKind::EmbeddedNode, parent_node, loc), data_ptr(data_ptr), sym_res_fn(sym_res_fn), replacement_fn(replacement_fn),
        known_type_fn(known_type_fn), child_res_fn(child_res_fn), traversal_fn(traversal_fn)
    {

    }

    /**
     * shallow copy is performed
     */
    EmbeddedNode* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<EmbeddedNode>()) EmbeddedNode(
                data_ptr,
                sym_res_fn,
                replacement_fn,
                known_type_fn,
                child_res_fn,
                traversal_fn,
                parent(),
                encoded_location()
        );
    }

    BaseType* known_type() override {
        return known_type_fn(this);
    }

    ASTNode* child(const chem::string_view &name) override {
        return child_res_fn(this, const_cast<chem::string_view*>(&name));
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_pointer(Codegen &gen) override;

#endif



};