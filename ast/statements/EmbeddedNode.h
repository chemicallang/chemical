// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class EmbeddedNode;

class ASTBuilder;

typedef BaseType*(EmbeddedNodeKnownTypeFunc)(EmbeddedNode* value);

typedef ASTNode*(EmbeddedNodeChildResolutionFunc)(EmbeddedNode* value, chem::string_view* name);


class EmbeddedNode : public ASTNode {
public:

    /**
     * the name corresponds to the hook, for example #html has 'html' as name
     */
    chem::string_view name;

    /**
     * user can store his ast in this pointer
     */
    void* data_ptr;

    /**
     * type creation function is used to create a type for this value
     */
    EmbeddedNodeKnownTypeFunc* known_type_fn;

    /**
     * child resolution function is used to resolve children of this embedded value
     */
    EmbeddedNodeChildResolutionFunc* child_res_fn;

    /**
     * constructor
     */
    EmbeddedNode(
            chem::string_view name,
            void* data_ptr,
            EmbeddedNodeKnownTypeFunc* known_type_fn,
            EmbeddedNodeChildResolutionFunc* child_res_fn,
            ASTNode* parent_node,
            SourceLocation loc
    ) : ASTNode(ASTNodeKind::EmbeddedNode, parent_node, loc), name(name), data_ptr(data_ptr),
        known_type_fn(known_type_fn), child_res_fn(child_res_fn)
    {

    }

    /**
     * shallow copy is performed
     */
    EmbeddedNode* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<EmbeddedNode>()) EmbeddedNode(
                name,
                data_ptr,
                known_type_fn,
                child_res_fn,
                parent(),
                encoded_location()
        );
    }

    BaseType* known_type() override {
        return known_type_fn(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_pointer(Codegen &gen) override;

#endif



};