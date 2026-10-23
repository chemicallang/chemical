// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include "ast/utils/Operation.h"

class ASTNode;
class InterfaceDefinition;
class ASTAny;
class ImplDefinition;
class GenericImplDecl;
class CoreNodes;
class FunctionDeclaration;
class MembersContainer;

struct ImplementationIndexKey {
    ASTNode* interface; // interface can be GenericInterfaceDecl or InterfaceDefinition
    ASTAny* for_; // for_ can be a type (BaseType*) or (StructDefinition, VariantDefinition or UnionDefinition...)
    bool operator==(const ImplementationIndexKey& other) const {
        return interface == other.interface && for_ == other.for_;
    }
};

struct ImplementationIndexKeyHash {
    std::size_t operator()(const ImplementationIndexKey& k) const {
        const auto h1 = std::hash<const void*>{}((void*) k.interface);
        const auto h2 = std::hash<const void*>{}((void*) k.for_);
        // Combine hashes
        return h1 ^ (h2 << 1);
    }
};

/**
 * basically all implementations are indexed in a map (interface + for struct/type)
 * why ? you can get any impl block by interface and struct/type
 * then you can use impl block to get method definitions
 * this is required for operator overloading, because a container (struct / type) contains indexed children
 * we can get child by name 'add' for example, but that maybe a direct child hiding the actual implementation of
 * 'core::ops::Add' interface, we need exact implementation of 'add', we already index 'core::ops::Add' interface
 * we can get that interface from CoreNodes (in the same directory), then get the struct from where operator is being invoked
 * and use that to get impl and from impl the actual 'add' implementation, this avoids shadowing, chooses the exact implementation
 * user intended
 * why use one map for all instantiations, because we can store a type pointer (BaseType*) too, and a Struct pointer (ASTNode*) too
 */
class ImplementationsIndex {
public:

    std::unordered_map<ImplementationIndexKey, ImplDefinition*, ImplementationIndexKeyHash> map_;

    /**
     * Negative auto-trait impls (`impl !Send for T`). Kept separate from map_
     * so a negative impl never resolves as a normal implementation.
     */
    std::unordered_set<ImplementationIndexKey, ImplementationIndexKeyHash> negative_impls_;

    // reader-writer mutex for thread-safe concurrent reads and exclusive writes
    mutable std::shared_mutex index_mutex;

    /**
     * Dormant generic implementations, keyed by their generic target declaration
     * (GenericStructDecl / GenericUnionDecl / GenericVariantDecl). Nothing is
     * instantiated from this map until a resolution path demands it, so programs
     * that declare no generic impls never allocate here and pay only an atomic
     * load on the fast path.
     */
    std::unordered_map<ASTNode*, std::vector<GenericImplDecl*>> generic_impls_;

    // relaxed fast-path flag; no lock is taken when it is zero
    std::atomic<size_t> generic_impl_count_{0};

    // serialises adoption (mutating a concrete target's member indexes and an
    // interface's users/vtable bookkeeping). Recursive: nested instantiation can
    // re-enter it.
    std::recursive_mutex generic_impl_adopt_mutex;

    // protects reads of a concrete generic target's member indexes against
    // adoption. Only lookups on concrete generic containers pay for it.
    mutable std::shared_mutex generic_impl_lookup_mutex;

    void add(ASTNode* interface, ASTAny* for_, ImplDefinition* impl) {
        std::unique_lock lock(index_mutex);
        map_.emplace(ImplementationIndexKey{ interface, for_ }, impl);
    }

    /**
     * Register a negative auto-trait implementation. Returns false when the
     * same negative impl is already present.
     */
    bool add_negative_impl(ASTNode* interface, ASTAny* for_) {
        std::unique_lock lock(index_mutex);
        return negative_impls_.insert(ImplementationIndexKey{ interface, for_ }).second;
    }

    bool has_negative_impl(ASTNode* interface, ASTAny* for_) const {
        std::shared_lock lock(index_mutex);
        return negative_impls_.find(ImplementationIndexKey{ interface, for_ }) != negative_impls_.end();
    }

    inline bool has_any_generic_impls() const noexcept {
        return generic_impl_count_.load(std::memory_order_relaxed) != 0;
    }

    /**
     * Register a dormant generic implementation for `target`. The caller is
     * responsible for validating the declaration and for rejecting an identical
     * pattern; returns false when this exact node is already registered.
     */
    bool add_generic_impl(ASTNode* target, GenericImplDecl* impl);

    /**
     * Snapshot the candidates for `target`. The returned vector is owned by the
     * caller; instantiation must happen after this returns (it re-enters the
     * index).
     */
    std::vector<GenericImplDecl*> generic_impls_for(ASTNode* target) const;

    inline std::recursive_mutex& getGenericImplAdoptMutex() {
        return generic_impl_adopt_mutex;
    }

    inline std::shared_mutex& getGenericImplLookupMutex() const {
        return generic_impl_lookup_mutex;
    }

    /**
     * this method automatically gets the generic parent and adds that if the interface is generic
     * otherwise uses the interface as the key node
     */
    bool add_interface(ASTNode* interface, ASTAny* for_, ImplDefinition* impl);

    void add_interface(InterfaceDefinition* interface, ASTAny* for_, ImplDefinition* impl);

    ImplDefinition* get(ASTNode* interface, ASTAny* for_) const {
        std::shared_lock lock(index_mutex);
        const auto it = map_.find(ImplementationIndexKey{ interface, for_ });
        if(it == map_.end()) return nullptr;
        return it->second;
    }

    ImplDefinition* get_impl(ASTNode* interface, ASTAny* for_);

    ImplDefinition* get_impl(InterfaceDefinition* interface, ASTAny* for_);

    void clear() {
        {
            std::unique_lock lock(index_mutex);
            map_.clear();
            negative_impls_.clear();
            generic_impls_.clear();
        }
        generic_impl_count_.store(0, std::memory_order_relaxed);
    }

    FunctionDeclaration* get_expr_op_impl(const CoreNodes& coreNodes, MembersContainer* container, Operation op) const;

    FunctionDeclaration* get_ass_op_impl(const CoreNodes& coreNodes, MembersContainer* container, Operation op) const;

    FunctionDeclaration* get_inc_dec_op_impl(const CoreNodes& coreNodes, MembersContainer* container, bool increment, bool post) const;

    FunctionDeclaration* get_index_op_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_neg_op_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_not_op_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_bitnot_op_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_linear_data_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_linear_size_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_chunked_begin_chunks_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_chunked_valid_chunk_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_chunked_current_chunk_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_chunked_next_chunk_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_chunked_rbegin_chunks_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_chunked_previous_chunk_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_chunked_total_size_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_iterable_begin_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_iterable_valid_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_iterable_current_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_iterable_next_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_reversible_iterable_rbegin_impl(const CoreNodes& coreNodes, MembersContainer* container) const;
    FunctionDeclaration* get_reversible_iterable_previous_impl(const CoreNodes& coreNodes, MembersContainer* container) const;
    FunctionDeclaration* get_reversible_iterable_count_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

    FunctionDeclaration* get_stream_write_signed_impl(const CoreNodes& coreNodes, MembersContainer* container) const;
    FunctionDeclaration* get_stream_write_unsigned_impl(const CoreNodes& coreNodes, MembersContainer* container) const;
    FunctionDeclaration* get_stream_write_str_impl(const CoreNodes& coreNodes, MembersContainer* container) const;
    FunctionDeclaration* get_stream_write_str_no_len_impl(const CoreNodes& coreNodes, MembersContainer* container) const;
    FunctionDeclaration* get_stream_write_float_impl(const CoreNodes& coreNodes, MembersContainer* container) const;
    FunctionDeclaration* get_stream_write_double_impl(const CoreNodes& coreNodes, MembersContainer* container) const;
    FunctionDeclaration* get_stream_write_char_impl(const CoreNodes& coreNodes, MembersContainer* container) const;
    FunctionDeclaration* get_stream_write_uchar_impl(const CoreNodes& coreNodes, MembersContainer* container) const;

};
