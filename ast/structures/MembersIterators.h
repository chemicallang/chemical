// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/structures/GenericFuncDecl.h"

class VariableIterator {
private:

    using UnderlyingIterator = std::vector<ASTNode*>::iterator;

    UnderlyingIterator current_;
    UnderlyingIterator end_;

    void skip_invalid() {
        while (current_ != end_ && !ASTNode::isBaseDefMember((*current_)->kind()))
            ++current_;
    }

public:

    inline VariableIterator(
        UnderlyingIterator current,
        UnderlyingIterator end
    ) : current_(current), end_(end) {
        skip_invalid();
    }

    // Pre-increment
    VariableIterator& operator++() {
        ++current_;
        skip_invalid();
        return *this;
    }

    // Post-increment
    VariableIterator operator++(int) {
        VariableIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    inline bool operator==(const VariableIterator& other) const {
        return current_ == other.current_;
    }

    inline bool operator!=(const VariableIterator& other) const {
        return !(*this == other);
    }

    // Return as BaseDefMember*& by casting the ASTNode* pointer.
    inline BaseDefMember*& operator*() const {
        return reinterpret_cast<BaseDefMember*&>(*current_);
    }

};

class VariablesRange {
private:
    std::vector<ASTNode*>& nodes_;
public:

    using iterator = VariableIterator;

    inline VariablesRange(
        std::vector<ASTNode*>& nodes
    ) : nodes_(nodes) {}

    inline iterator begin() { return {nodes_.begin(), nodes_.end()}; }
    inline iterator end()   { return {nodes_.end(), nodes_.end()}; }

};

class InstFuncIterator {
private:
    using OuterIter = std::vector<ASTNode*>::iterator;

    OuterIter outer_current_;
    OuterIter outer_end_;

    // Inner iterators for GenericFuncDecl instantiations.
    std::vector<FunctionDeclaration*>::iterator inner_current_;
    std::vector<FunctionDeclaration*>::iterator inner_end_;

    // Provide a static empty vector to initialize inner iterators.
    static std::vector<FunctionDeclaration*>& empty_inners() {
        static std::vector<FunctionDeclaration*> empty;
        return empty;
    }

    // Advance the iterator until we find a valid function or instantiation.
    void advanceToNextValid() {
        // If we're in the middle of a generic function's instantiations, try to continue.
        if (inner_current_ != inner_end_) {
            return;
        }
        // Otherwise, move through the outer container.
        while (outer_current_ != outer_end_) {
            ASTNode* node = *outer_current_;
            auto kind = node->kind();
            if (kind == ASTNodeKind::FunctionDecl) {
                // A normal function: set inner to the empty range.
                inner_current_ = empty_inners().begin();
                inner_end_ = empty_inners().end();
                return;
            } else if (kind == ASTNodeKind::GenericFuncDecl) {
                // A generic function: switch to iterating its instantiations.
                GenericFuncDecl* genFunc = reinterpret_cast<GenericFuncDecl*>(node);
                if (!genFunc->instantiations.empty()) {
                    inner_current_ = genFunc->instantiations.begin();
                    inner_end_ = genFunc->instantiations.end();
                    return;
                }
                // If there are no instantiations, fall through and skip this node.
            }
            ++outer_current_;
        }
    }

public:
    // Constructor: initializes outer iterator state.
    InstFuncIterator(OuterIter current, OuterIter outer_end)
            : outer_current_(current), outer_end_(outer_end)
    {
        // Initialize inner iterators to a valid empty range.
        inner_current_ = empty_inners().begin();
        inner_end_ = empty_inners().end();
        advanceToNextValid();
    }

    // Pre-increment: advances inner if active, otherwise moves to the next outer element.
    InstFuncIterator& operator++() {
        if (inner_current_ != inner_end_) {
            ++inner_current_;
            if (inner_current_ != inner_end_) {
                return *this;
            }
            // If inner is exhausted, move to the next outer element.
            ++outer_current_;
        } else {
            // For normal functions, simply advance the outer iterator.
            ++outer_current_;
        }
        // Reset inner iterators to empty and try to find a valid next function.
        inner_current_ = empty_inners().begin();
        inner_end_ = empty_inners().end();
        advanceToNextValid();
        return *this;
    }

    // Post-increment.
    InstFuncIterator operator++(int) {
        InstFuncIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    bool operator==(const InstFuncIterator& other) const {
        return outer_current_ == other.outer_current_ &&
               inner_current_ == other.inner_current_;
    }

    bool operator!=(const InstFuncIterator& other) const {
        return !(*this == other);
    }

    // Dereference: if iterating a generic's instantiations, return that; otherwise, return the outer function.
    FunctionDeclaration*& operator*() const {
        if (inner_current_ != inner_end_) {
            return const_cast<FunctionDeclaration*&>(*inner_current_);
        } else {
            return reinterpret_cast<FunctionDeclaration*&>(*outer_current_);
        }
    }
};

class InstFuncRange {
private:
    std::vector<ASTNode*>& nodes_;
public:
    using iterator = InstFuncIterator;

    InstFuncRange(std::vector<ASTNode*>& nodes) : nodes_(nodes) {}

    iterator begin() { return iterator(nodes_.begin(), nodes_.end()); }
    iterator end()   { return iterator(nodes_.end(), nodes_.end()); }
};

class MasterFuncIterator {
private:

    using UnderlyingIterator = std::vector<ASTNode*>::iterator;

    UnderlyingIterator current_;
    UnderlyingIterator end_;

    static inline constexpr bool isFunc(ASTNodeKind k) {
        return k == ASTNodeKind::FunctionDecl || k == ASTNodeKind::GenericFuncDecl;
    }

    void skip_invalid() {
        while (current_ != end_ && !isFunc((*current_)->kind()))
            ++current_;
    }

public:

    inline MasterFuncIterator(
            UnderlyingIterator current,
            UnderlyingIterator end
    ) : current_(current), end_(end) {
        skip_invalid();
    }

    // Pre-increment
    MasterFuncIterator& operator++() {
        ++current_;
        skip_invalid();
        return *this;
    }

    // Post-increment
    MasterFuncIterator operator++(int) {
        MasterFuncIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    inline bool operator==(const MasterFuncIterator& other) const {
        return current_ == other.current_;
    }

    inline bool operator!=(const MasterFuncIterator& other) const {
        return !(*this == other);
    }

    // Return as BaseDefMember*& by casting the ASTNode* pointer.
    inline FunctionDeclaration*& operator*() const {
        ASTNode*& thing = *current_;
        if(thing->kind() == ASTNodeKind::GenericFuncDecl) {
            return (FunctionDeclaration*&) (thing->as_gen_func_decl_unsafe())->master_impl;
        } else {
            return (FunctionDeclaration*&) thing;
        }
    }

};

class MasterFuncRange {
private:
    std::vector<ASTNode*>& nodes_;
public:
    using iterator = MasterFuncIterator;

    MasterFuncRange(std::vector<ASTNode*>& nodes) : nodes_(nodes) {}

    iterator begin() { return {nodes_.begin(), nodes_.end()}; }
    iterator end()   { return {nodes_.end(), nodes_.end()}; }
};

class FuncNodeIterator {
private:

    using UnderlyingIterator = std::vector<ASTNode*>::iterator;

    UnderlyingIterator current_;
    UnderlyingIterator end_;

    static inline constexpr bool isFunc(ASTNodeKind k) {
        return k == ASTNodeKind::FunctionDecl || k == ASTNodeKind::GenericFuncDecl;
    }

    void skip_invalid() {
        while (current_ != end_ && !isFunc((*current_)->kind()))
            ++current_;
    }

public:

    inline FuncNodeIterator(
            UnderlyingIterator current,
            UnderlyingIterator end
    ) : current_(current), end_(end) {
        skip_invalid();
    }

    // Pre-increment
    FuncNodeIterator& operator++() {
        ++current_;
        skip_invalid();
        return *this;
    }

    // Post-increment
    FuncNodeIterator operator++(int) {
        FuncNodeIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    inline bool operator==(const FuncNodeIterator& other) const {
        return current_ == other.current_;
    }

    inline bool operator!=(const FuncNodeIterator& other) const {
        return !(*this == other);
    }

    // Return as BaseDefMember*& by casting the ASTNode* pointer.
    inline ASTNode*& operator*() const {
        return *current_;
    }

};

class FuncNodeRange {
private:
    std::vector<ASTNode*>& nodes_;
public:
    using iterator = FuncNodeIterator;

    FuncNodeRange(std::vector<ASTNode*>& nodes) : nodes_(nodes) {}

    iterator begin() { return {nodes_.begin(), nodes_.end()}; }
    iterator end()   { return {nodes_.end(), nodes_.end()}; }
};

class NonGenFuncIterator {
private:

    using UnderlyingIterator = std::vector<ASTNode*>::iterator;

    UnderlyingIterator current_;
    UnderlyingIterator end_;

    void skip_invalid() {
        while (current_ != end_ && (*current_)->kind() != ASTNodeKind::FunctionDecl)
            ++current_;
    }

public:

    inline NonGenFuncIterator(
            UnderlyingIterator current,
            UnderlyingIterator end
    ) : current_(current), end_(end) {
        skip_invalid();
    }

    // Pre-increment
    NonGenFuncIterator& operator++() {
        ++current_;
        skip_invalid();
        return *this;
    }

    // Post-increment
    NonGenFuncIterator operator++(int) {
        NonGenFuncIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    inline bool operator==(const NonGenFuncIterator& other) const {
        return current_ == other.current_;
    }

    inline bool operator!=(const NonGenFuncIterator& other) const {
        return !(*this == other);
    }

    // Return as BaseDefMember*& by casting the ASTNode* pointer.
    inline FunctionDeclaration*& operator*() const {
        return (FunctionDeclaration*&) *current_;
    }

};

class NonGenFuncRange {
private:
    std::vector<ASTNode*>& nodes_;
public:
    using iterator = NonGenFuncIterator;

    NonGenFuncRange(std::vector<ASTNode*>& nodes) : nodes_(nodes) {}

    iterator begin() { return {nodes_.begin(), nodes_.end()}; }
    iterator end()   { return {nodes_.end(), nodes_.end()}; }
};