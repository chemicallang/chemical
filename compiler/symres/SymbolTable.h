// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
#include <vector>
#include <functional>
#include <span>
#include <cassert>

class ASTNode;

struct SymbolEntry {
    chem::string_view key;  // AST-owned key; no extra allocation.
    size_t hash;           // Precomputed hash.
    int prev;              // Index of previous declaration for shadowing (-1 if none).
    ASTNode* node;            // Pointer to the associated AST node.
};

struct Bucket {
    chem::string_view key;  // Key stored for verification.
    size_t hash;           // Precomputed hash for this bucket.
    int index;             // Index into the symbol metadata array (-1 if empty).
    ASTNode* activeNode;      // Directly stores the active node pointer.
};

struct SymbolScope {

    /**
     * the start index at where this scope started
     * this means the first symbol in this scope is at this index
     */
    int start;

    /**
     * this kind is provided by the user, if not then 0 is stored
     * the kind tells us which kind of scope this is
     */
    int kind;

};

class SymbolTable {
public:

    // Compute the hash for a key.
    static inline size_t computeHash(const chem::string_view& key) {
        return std::hash<chem::string_view>{}(key);
    }

private:

    // Contiguous metadata for symbols.
    std::vector<SymbolEntry> symbols;
    // Open-addressing hash table for fast resolution.
    std::vector<Bucket> buckets;
    size_t bucketMask;  // Used for fast modulo assuming buckets.size() is a power of 2.
    // Scope stack stores the symbol count marker at each scope start.
    std::vector<SymbolScope> scopeStack;

    // Find a bucket index given a precomputed hash and key using linear probing.
    int findBucketForHash(size_t hash, const chem::string_view& key) const {
        size_t i = hash & bucketMask;
        while (true) {
            const Bucket& b = buckets[i];
            // If empty bucket or matching key (and hash) is found, return the index.
            if (b.index == -1 || (b.hash == hash && b.key == key)) {
                return static_cast<int>(i);
            }
            i = (i + 1) & bucketMask;
        }
    }

    // Compute hash and find bucket for a key.
    inline int findBucket(const chem::string_view& key) const {
        return findBucketForHash(computeHash(key), key);
    }

    // Optional: rehash the buckets if the load factor exceeds a threshold.
    // Resizing is rare, so details are omitted.
    void rehash() {
        size_t newBucketCount = buckets.size() * 2;
        std::vector<Bucket> newBuckets(newBucketCount, Bucket{"", 0, -1, nullptr});
        size_t newMask = newBucketCount - 1;

        for (const Bucket& bucket : buckets) {
            if (bucket.index != -1) {
                size_t i = bucket.hash & newMask;
                while (true) {
                    Bucket& newBucket = newBuckets[i];
                    if (newBucket.index == -1) {
                        newBucket = bucket;
                        break;
                    }
                    i = (i + 1) & newMask;
                }
            }
        }
        buckets = std::move(newBuckets);
        bucketMask = newMask;
    }

public:

    // Helper function to compute the next power of two for a given size.
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

    /**
     * Constructor: preallocate bucket table and reserve typical symbol count.
     */
    SymbolTable() {
        // Ensure initialBucketCount is a power of two.
        const auto initialBucketCount = 128;
        buckets.resize(initialBucketCount, Bucket{"", 0, -1, nullptr});
        bucketMask = initialBucketCount - 1;
        symbols.reserve(initialBucketCount);  // Typical number of symbols per function.
        scopeStack.reserve(20);
    }

    /**
     * Constructor: preallocate bucket table and reserve typical symbol count.
     */
    SymbolTable(size_t initialBucketCount) {
        // Ensure initialBucketCount is a power of two.
        initialBucketCount = next_power_of_two(initialBucketCount);
        buckets.resize(initialBucketCount, Bucket{"", 0, -1, nullptr});
        bucketMask = initialBucketCount - 1;
        symbols.reserve(initialBucketCount);  // Typical number of symbols per function.
        scopeStack.reserve(20);
    }

    // declare: add a new symbol (with key and node pointer) to the current scope.
    void declare(chem::string_view key, ASTNode* node) {

        // Check load factor: if symbols exceed 90% of bucket capacity, rehash.
        if (symbols.size() >= static_cast<size_t>(buckets.size() * 0.9)) {
            rehash();
        }

        size_t hash = computeHash(key);
        int bucketIndex = findBucketForHash(hash, key);
        Bucket& bucket = buckets[bucketIndex];

        // Save previous declaration index.
        int prevIndex = bucket.index;
        // Create a new symbol entry with the node pointer.
        SymbolEntry entry { key, hash, prevIndex, node };
        int newIndex = static_cast<int>(symbols.size());
        symbols.push_back(entry);

        // Update the bucket with the new symbol.
        bucket.key = key;
        bucket.hash = hash;
        bucket.index = newIndex;
        bucket.activeNode = node;

    }

    // declare: add a new symbol (with key and node pointer) to the current scope. without shadowing
    // returns nullptr if symbol was declared, otherwise the previous symbol
    ASTNode* declare_no_shadow(chem::string_view key, ASTNode* node) {

        // Check load factor: if symbols exceed 90% of bucket capacity, rehash.
        if (symbols.size() >= static_cast<size_t>(buckets.size() * 0.9)) {
            rehash();
        }

        size_t hash = computeHash(key);
        int bucketIndex = findBucketForHash(hash, key);
        Bucket& bucket = buckets[bucketIndex];

        // Save previous declaration index.
        int prevIndex = bucket.index;
        if(prevIndex == -1) {

            // Create a new symbol entry with the node pointer.
            SymbolEntry entry{key, hash, prevIndex, node};
            int newIndex = static_cast<int>(symbols.size());
            symbols.push_back(entry);

            // Update the bucket with the new symbol.
            bucket.key = key;
            bucket.hash = hash;
            bucket.index = newIndex;
            bucket.activeNode = node;

            return nullptr;

        } else {
            return bucket.activeNode;
        }
    }

    // resolve: return the node pointer for the active symbol of the given key.
    ASTNode* resolve(const chem::string_view& key) const {
        int bucketIndex = findBucket(key);
        const Bucket& bucket = buckets[bucketIndex];
        return (bucket.index == -1) ? nullptr : bucket.activeNode;
    }

    // returns true if erase succeeds
    bool erase(const chem::string_view& key) {
        int bucketIndex = findBucket(key);
        Bucket& bucket = buckets[bucketIndex];
        bucket.index = -1;
        return true;
    }

    // scope_start: record the current symbol count to mark the beginning of a new scope.
    inline void scope_start() {
        scopeStack.emplace_back(static_cast<int>(symbols.size()), 0);
    }

    inline void scope_start(int kind) {
        scopeStack.emplace_back(static_cast<int>(symbols.size()), kind);
    }

    // apart from starting the scope, get an index of the scope
    inline int scope_start_index(int kind) {
        const auto scope_index = static_cast<int>(symbols.size());
        scopeStack.emplace_back(scope_index, kind);
        return scope_index;
    }

    // get the kind of the last (current) scope
    inline int get_last_scope_kind() {
        return scopeStack.back().kind;
    }

    inline std::span<SymbolEntry> last_scope() {
        const auto start = scopeStack.back().start;
        return {&symbols[start], symbols.size() - start};
    }

    // scope_end: remove all symbols declared since the last scope_start.
    void scope_end() {
        assert(!scopeStack.empty());
        int marker = scopeStack.back().start;
        scopeStack.pop_back();
        // Roll back all symbols declared in the current scope.
        for (int i = static_cast<int>(symbols.size()) - 1; i >= marker; --i) {
            SymbolEntry& entry = symbols[i];
            int bucketIndex = findBucketForHash(entry.hash, entry.key);
            Bucket& bucket = buckets[bucketIndex];
            // If the bucket currently points to the symbol being removed, restore its previous value.
            if (bucket.index == i) {
                bucket.index = entry.prev;
                bucket.activeNode = (entry.prev != -1) ? symbols[entry.prev].node : nullptr;
                // Optionally clear bucket.key if bucket.index becomes -1.
            }
        }
        // Remove symbols from the metadata array.
        symbols.resize(marker);
    }
};