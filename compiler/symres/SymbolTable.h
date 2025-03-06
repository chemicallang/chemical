// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
#include <vector>
#include <functional>
#include <span>
#include <cassert>
#include <cstring>

// Forward declaration.
class ASTNode;

//---------------------------------------------------------------------
// The SymbolEntry remains as metadata storage for a declared symbol.
struct SymbolEntry {
    chem::string_view key;  // AST-owned key; no extra allocation.
    size_t hash;            // Precomputed hash.
    ASTNode* node;          // Pointer to the associated AST node.
};

struct BucketSymbol {

    chem::string_view key = "";    // Key for the active symbol in this bucket.
    size_t hash = 0;              // Precomputed hash for the active symbol.
    ASTNode* activeNode = nullptr;      // Direct pointer to the active AST node.
    int index = -1;                // Index into the symbol metadata array for active symbol (-1 if empty).
    BucketSymbol* next = nullptr;       // Pointer to next symbol in this chain

};

//---------------------------------------------------------------------
// The Bucket now holds the direct (active) symbol plus a pointer to a
// linked list of BucketSymbol nodes that represent shadowed entries (or
// collisions with different keys).
struct Bucket : BucketSymbol {
    BucketSymbol* collision = nullptr; // Symbols that have the same hash, we put these in this linked list chain of symbols
};

//---------------------------------------------------------------------
// Scope information remains the same.
struct SymbolScope {
    int start;  // Index in the symbols vector where this scope started.
    int kind;   // Scope kind; 0 if not provided.
};

//---------------------------------------------------------------------
// The SymbolTable class using chaining (no probing).
class SymbolTable {
public:
    // Compute the hash for a key.
    static inline size_t computeHash(const chem::string_view& key) {
        return std::hash<chem::string_view>{}(key);
    }

private:
    // Contiguous metadata for symbols.
    std::vector<SymbolEntry> symbols;
    // Hash table: an array of buckets.
    std::vector<Bucket> buckets;
    size_t bucketMask;  // bucketMask = buckets.size() - 1 (buckets.size() is a power of two).
    // Scope stack for managing declarations.
    std::vector<SymbolScope> scopeStack;
    // these are symbols allocated for collisions and shadowing
    std::vector<BucketSymbol*> extra_symbols;

    // Dummy batch allocator for BucketSymbol; replace with your actual allocator.
    inline BucketSymbol* allocateBucketSymbol() {
        const auto sym = (BucketSymbol*) ::operator new(sizeof(BucketSymbol), std::align_val_t(alignof(BucketSymbol)));
        extra_symbols.emplace_back(sym);
        return sym;
    }

    inline BucketSymbol* allocateBucketSymbol(const BucketSymbol& bucket) {
        return new (allocateBucketSymbol()) BucketSymbol(bucket);
    }

    inline int put_entry(const chem::string_view& key, const size_t hash, ASTNode* const node) noexcept {
        int newIndex = static_cast<int>(symbols.size());
        symbols.emplace_back(key, hash, node);
        return newIndex;
    }

    inline void set_to_bucket(Bucket& bucket, const chem::string_view& key, const size_t hash, ASTNode* const node, int index, BucketSymbol* const next) noexcept {
        bucket.key = key;
        bucket.hash = hash;
        bucket.index = index;
        bucket.activeNode = node;
        bucket.next = next;
    }

    inline BucketSymbol* allocateBucketSymbol(const chem::string_view& key, const size_t hash, ASTNode* const node, int index, BucketSymbol* const next) noexcept {
        return new (allocateBucketSymbol()) BucketSymbol(key, hash, node, index, next);
    }

    //-----------------------------------------------------------------
    // Helper: Inserts a symbol entry (already in symbols vector) into a given
    // new bucket array (used during rehashing).
    void insert_symbol(int index, const SymbolEntry &entry, std::vector<Bucket>& targetBuckets, size_t mask) {

        const auto bucketIndex = entry.hash & mask;
        Bucket &bucket = targetBuckets[bucketIndex];

        if (bucket.index == -1) {
            // Bucket is empty; insert directly.
            bucket.key = entry.key;
            bucket.hash = entry.hash;
            bucket.index = index;
            bucket.activeNode = entry.node;
            bucket.next = nullptr;
        } else {
            // Bucket already occupied.
            if (bucket.hash == entry.hash && bucket.key == entry.key) {
                // Same key (shadowing). Allocate a new BucketSymbol for the current active entry.
                // This puts this current symbol (stored) inside the new allocated bucket, also taking it's next chain
                const auto extra = allocateBucketSymbol(bucket);
                // Replace bucket's active entry with the new symbol.
                bucket.key = entry.key;
                bucket.hash = entry.hash;
                bucket.index = index;
                bucket.activeNode = entry.node;
                bucket.next = extra;
            } else {
                // Collision occurred since we hashed to same index but key doesn't match
                // we allocate an extra symbol cor current entry and set it to collision pointer
                const auto extra = allocateBucketSymbol(entry.key, entry.hash, entry.node, index, bucket.collision);
                // set the new collision chain to the bucket
                bucket.collision = extra;
            }
        }

    }

    //-----------------------------------------------------------------
    // Rehash: double the bucket array and reinsert all symbols from 'symbols'.
    void rehash() {
        size_t newBucketCount = buckets.size() * 2;
        newBucketCount = next_power_of_two(newBucketCount);
        std::vector<Bucket> newBuckets(newBucketCount, Bucket());
        size_t newMask = newBucketCount - 1;

        // Reinsert every symbol in order.
        for (int i = 0; i < static_cast<int>(symbols.size()); i++) {
            insert_symbol(i, symbols[i], newBuckets, newMask);
        }
        buckets = std::move(newBuckets);
        bucketMask = newMask;
    }

public:
    //-----------------------------------------------------------------
    // Helper: compute the next power of two for a given size.
    static size_t next_power_of_two(size_t x) {
        if (x == 0) return 1;
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        if (sizeof(size_t) > 4) {
            x |= x >> 32;
        }
        return x + 1;
    }

    //-----------------------------------------------------------------
    // Constructors.
    SymbolTable() {
        const auto initialBucketCount = 128; // power of two
        buckets.resize(initialBucketCount, Bucket());
        bucketMask = initialBucketCount - 1;
        symbols.reserve(initialBucketCount);
        scopeStack.reserve(20);
    }

    SymbolTable(size_t initialBucketCount) {
        initialBucketCount = next_power_of_two(initialBucketCount);
        buckets.resize(initialBucketCount, Bucket());
        bucketMask = initialBucketCount - 1;
        symbols.reserve(initialBucketCount);
        scopeStack.reserve(20);
    }

    //-----------------------------------------------------------------
    // declare: add a new symbol to the current scope.
    void declare(const chem::string_view& key, ASTNode* const node) {

        // Check load factor: if symbols exceed 90% of bucket capacity, rehash.
        if (symbols.size() >= static_cast<size_t>(buckets.size() * 0.9)) {
            rehash();
        }

        const auto hash = computeHash(key);
        const auto  bucketIndex = hash & bucketMask;
        Bucket &bucket = buckets[bucketIndex];

        const auto index = put_entry(key, hash, node);

        if (bucket.index == -1) {
            set_to_bucket(bucket, key, hash, node, index, nullptr);
        } else {
            if (bucket.hash == hash && bucket.key == key) {
                // Same key: shadow the current symbol. by allocating previous symbol
                const auto next = allocateBucketSymbol(bucket);
                // Set the new symbol to bucket (the newer symbol is always the first one)
                set_to_bucket(bucket, key, hash, node, index, next);
            } else {
                // Collision since we hashed to same index but key is different
                // we allocate an extra symbol cor current entry and set it to collision pointer
                const auto extra = allocateBucketSymbol(key, hash, node, index, bucket.collision);
                bucket.collision = extra;
            }
        }
    }

    const BucketSymbol* declare_no_shadow_sym(const chem::string_view& key, ASTNode* const node) {
        // Check load factor.
        if (symbols.size() >= static_cast<size_t>(buckets.size() * 0.9)) {
            rehash();
        }

        const auto hash = computeHash(key);
        const auto bucketIndex = hash & bucketMask;
        Bucket &bucket = buckets[bucketIndex];

        if (bucket.index == -1) {
            // Bucket empty; insert directly.
            set_to_bucket(bucket, key, hash, node, put_entry(key, hash, node), nullptr);
            return nullptr;
        } else {
            if (bucket.hash == hash && bucket.key == key) {
                // Already declared in bucket.
                return &bucket;
            } else {
                // Collision since we hashed to same index however key is different
                // check if key exists in collision chain
                BucketSymbol* sym = bucket.collision;
                while(sym) {
                    const auto& ptr = *sym;
                    if(ptr.hash == hash && ptr.key == key) {
                        return &ptr;
                    }
                    sym = ptr.next;
                }
                // Since key doesn't exist in collision chain, we must create a new key in collision chain
                const auto new_coll_chain = allocateBucketSymbol(key, hash, node, put_entry(key, hash, node), bucket.collision);
                // Set the new collision chain to bucket
                bucket.collision = new_coll_chain;
                return nullptr;
            }
        }

    }

    //-----------------------------------------------------------------
    // declare_no_shadow: add a new symbol only if one with the same key isn't already declared.
    // Returns nullptr if symbol was declared (i.e. no previous symbol existed for that key),
    // otherwise returns the previous active node.
    ASTNode* declare_no_shadow(const chem::string_view& key, ASTNode* const node) {
        const auto d = declare_no_shadow_sym(key, node);
        return d ? d->activeNode : nullptr;
    }

    // check if given symbol is in current scope
    inline bool is_in_current_scope(const BucketSymbol* symbol) {
        return symbol->index >= scopeStack.back().start;
    }

    //-----------------------------------------------------------------
    // resolve: return the AST node pointer for the active symbol matching the given key.
    ASTNode* resolve(const chem::string_view& key) const {
        size_t hash = computeHash(key);
        size_t bucketIndex = hash & bucketMask;
        const Bucket &bucket = buckets[bucketIndex];
        if (bucket.index != -1 && bucket.hash == hash && bucket.key == key)
            return bucket.activeNode;
        // check if symbol exists in the collision chain
        auto sym = bucket.collision;
        while (sym) {
            const auto& ptr = *sym;
            if (ptr.hash == hash && ptr.key == key)
                return ptr.activeNode;
            sym = ptr.next;
        }
        return nullptr;
    }

    //-----------------------------------------------------------------
    // erase: remove the symbol for the given key.
    // Returns true if the erase succeeded.
    bool erase(const chem::string_view& key) {

        const auto hash = computeHash(key);
        const auto bucketIndex = hash & bucketMask;
        Bucket &bucket = buckets[bucketIndex];

        if (bucket.index != -1 && bucket.hash == hash && bucket.key == key) {
            if(bucket.next) {
                // bucket is a shadowing symbol, so we'll bring that symbol back
                const auto& current = *bucket.next;
                bucket.index = current.index;
                bucket.activeNode = current.activeNode;
                bucket.next = current.next;
            } else {
                // NOTE: we aren't going to touch the collision chain, because current symbol in the bucket and it's next chain
                // must always contain the same key, so we can guarantee that a symbol's key would match everything present in it's next (shadow chain)
                // even if collision chain is present, we'll make this key empty, so this means we must always check the collision chain even if the key is empty
                bucket.index = -1;
                bucket.activeNode = nullptr;
            }
            return true;
        } else if(bucket.collision) {
            // Since the hash matches, key is different and there's a collision chain, we must check the entire collision chain
            BucketSymbol** sym_ptr_ref = &bucket.collision;
            while (sym_ptr_ref) {
                BucketSymbol& sym = **sym_ptr_ref;
                if (sym.hash == hash && sym.key == key) {
                    // found it in the collision chain, we just need to relink the chain by removing this from in between
                    *sym_ptr_ref = sym.next;
                    return true;
                }
               sym_ptr_ref = &sym.next;
            }
            return false;
        }

        return false;

    }

    //-----------------------------------------------------------------
    // scope management
    inline void scope_start() {
        scopeStack.emplace_back(static_cast<int>(symbols.size()), 0);
    }

    inline void scope_start(int kind) {
        scopeStack.emplace_back(static_cast<int>(symbols.size()), kind);
    }

    inline int scope_start_index(int kind) {
        int scope_index = static_cast<int>(symbols.size());
        scopeStack.emplace_back(scope_index, kind);
        return scope_index;
    }

    inline int get_last_scope_kind() {
        return scopeStack.back().kind;
    }

    // scope_end: remove all symbols declared since the last scope_start.
    void scope_end() {
        assert(!scopeStack.empty());
        // get where the scope started (index)
        int marker = scopeStack.back().start;
        scopeStack.pop_back();

        // Roll back each symbol declared in the current scope.
        for (int i = static_cast<int>(symbols.size()) - 1; i >= marker; --i) {

            const SymbolEntry& entry = symbols[i];
            const auto bucketIndex = entry.hash & bucketMask;
            Bucket &bucket = buckets[bucketIndex];

            if (bucket.index == i) {
                if (bucket.next) {
                    // symbol has a shadow chain, so bring the shadowed chain symbol back
                    auto& current = *bucket.next;
                    bucket.key = current.key;
                    bucket.hash = current.hash;
                    bucket.index = current.index;
                    bucket.next = current.next;
                    bucket.activeNode = current.activeNode;
                } else {
                    bucket.index = -1;
                    bucket.activeNode = nullptr;
                }
            } else if(bucket.collision) {

                // check the collision chain for this symbol
                BucketSymbol** sym_ptr_ref = &bucket.collision;
                while (sym_ptr_ref) {
                    BucketSymbol& sym = **sym_ptr_ref;
                    if (sym.index == i) {
                        // found it in the collision chain, we just need to relink the chain by removing this from in between
                        *sym_ptr_ref = sym.next;
                        break;
                    }
                    sym_ptr_ref = &sym.next;
                }
            } else {
#ifdef DEBUG
                throw std::runtime_error("couldn't erase the symbol when scope ended");
#endif
            }

        }
        symbols.resize(marker);

    }

    ~SymbolTable() {
        for(const auto sym : extra_symbols) {
            ::operator delete(sym, std::align_val_t(alignof(BucketSymbol)));
        }
    }

};
