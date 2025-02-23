// Copyright (c) Qinetik 2025.

// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/ASTNode.h"

/**
 * the function is called on sym res node, to allocate the top level node and declare it
 * in this function you can do the following
 * 1 - the data pointer can be de-referenced to get any data stored during parsing
 * 2 - the data pointer can be used to store any data required for actual replacement
 * 3 - the allocator and resolver pointer can be used to allocate and then declare and top level nodes
 *   - any nodes declared can actually be referenced by nodes above the current sym res node
 * 4 - the data pointer can be used to again store any data (pointer to root node) that is allocated
 *   - that we declared symbols for, which then will be available during replacement
 */
typedef void(*SymResNodeDeclarationFn)(ASTAllocator* allocator, SymbolResolver* resolver, void** data);

/**
 * the function is called on sym res node, to replace the sym res node
 * the data pointer contains any information that declaration function stored, we can dereference it
 * it mostly will be used to root node allocated during declaration function that is declared in symbol
 * resolver, in this replacement function you cannot declare top level nodes, because top level
 * nodes are first declared so nodes above them can resolve them
 * you can declare nested level nodes that can be resolved using the symbol resolver in a sequential manner
 * you must return the pointer to root node here so we can replace it, this may just be the data pointer
 * that you allocated during declaration function
 */
typedef ASTNode*(*SymResNodeReplacementFn)(ASTAllocator* allocator, SymbolResolver* resolver, void* data);

/**
 * sym res node replaces itself during symbol resolution
 */
class SymResNode : public ASTNode {
public:

    /**
     * The allocator is stored during parsing to allocate in symbol resolution
     * This is because during parsing allocator is used based on passed parameter
     * Because public nodes should be allocated on job allocator and non public on module allocator
     */
    ASTAllocator* allocator;

    /**
     * the declaration function is called to just declare the node
     */
    SymResNodeDeclarationFn  decl_fn;

    /**
     * this method is called during symbol linkage, so we can get real node, which
     * will replace the current sym res node
     */
    SymResNodeReplacementFn repl_fn;

    /**
     * the data pointer is any pointer that is stored for holding information
     * that is needed during symbol resolution, just for information passing between
     * parsing phase and symbol resolution
     */
    void* data_ptr;

    /**
     * parent node
     */
    ASTNode* parent_node;

    /**
     * constructor
     */
    inline SymResNode(
            ASTAllocator* allocator,
            SymResNodeDeclarationFn decl_fn,
            SymResNodeReplacementFn repl_fn,
            void* data_ptr,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::SymResNode, location), allocator(allocator), decl_fn(decl_fn), repl_fn(repl_fn), data_ptr(data_ptr), parent_node(parent_node) {

    }

    ASTNode* parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        throw std::runtime_error("SymResNode cannot be visited, as it should be replaced during symbol resolution");
    }

//    Value* copy(ASTAllocator &alloc) override {
//        return new (alloc.allocate<SymResNode>()) SymResNode(
//            this->allocator, fn, data_ptr, location
//        );
//    }

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) override {
        decl_fn(allocator, &linker, &data_ptr);
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) override {
        node_ptr = repl_fn(allocator, &linker, data_ptr);
    }

};