// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string_view>
#include <vector>
#include <functional>
#include <cassert>

// Dummy AST node struct for demonstration.
struct Node {
    // ... your AST node fields ...
};

struct SymbolEntry {
    std::string_view key;  // AST-owned key; no extra allocation.
    size_t hash;           // Precomputed hash.
    int prev;              // Index of previous declaration for shadowing (-1 if none).
};

struct Bucket {
    std::string_view key;  // Key stored for verification.
    size_t hash;           // Precomputed hash for this bucket.
    int index;             // Index into the symbol metadata & node pointer arrays (-1 if empty).
    Node* activeNode;      // Directly stores the active node pointer.
};

class SymbolResolver {
private:
    // Contiguous metadata for symbols.
    std::vector<SymbolEntry> symbols;
    // Parallel array storing the node pointers corresponding to symbols.
    std::vector<Node*> nodePtrs;
    // Open-addressing hash table for fast resolution.
    std::vector<Bucket> buckets;
    size_t bucketMask;  // Used for fast modulo assuming buckets.size() is a power of 2.
    // Scope stack stores the symbol count marker at each scope start.
    std::vector<int> scopeStack;

    // Compute the hash for a key.
    inline size_t computeHash(std::string_view key) const {
        return std::hash<std::string_view>{}(key);
    }

    // Find a bucket index given a precomputed hash and key using linear probing.
    inline int findBucketForHash(size_t hash, std::string_view key) const {
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
    inline int findBucket(std::string_view key) const {
        size_t hash = computeHash(key);
        return findBucketForHash(hash, key);
    }

    // Optional: rehash the buckets if the load factor exceeds a threshold.
    // Not the focus here since resizing is expected to be rare.
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
    // Constructor: preallocate bucket table and reserve typical symbol count.
    SymbolResolver(size_t initialBucketCount = 128) {
        // initialBucketCount must be a power of 2.
        buckets.resize(initialBucketCount, Bucket{"", 0, -1, nullptr});
        bucketMask = initialBucketCount - 1;
        symbols.reserve(100);
        nodePtrs.reserve(100);
    }

    // declare: add a new symbol (with key and node pointer) to the current scope.
    void declare(std::string_view key, Node* node) {
        size_t hash = computeHash(key);
        int bucketIndex = findBucketForHash(hash, key);
        Bucket& bucket = buckets[bucketIndex];

        // The current bucket holds the index of any previous declaration.
        int prevIndex = bucket.index;
        // Create the symbol metadata entry (without storing the node pointer here).
        SymbolEntry entry { key, hash, prevIndex };
        int newIndex = static_cast<int>(symbols.size());
        symbols.push_back(entry);
        nodePtrs.push_back(node);

        // Update the bucket with the new symbol.
        bucket.key = key;
        bucket.hash = hash;
        bucket.index = newIndex;
        bucket.activeNode = node;

        // Optionally: if (loadFactor() > threshold) then rehash();
    }

    // resolve: return the node pointer for the active symbol of the given key.
    Node* resolve(std::string_view key) const {
        int bucketIndex = findBucket(key);
        const Bucket& bucket = buckets[bucketIndex];
        return (bucket.index == -1) ? nullptr : bucket.activeNode;
    }

    // scope_start: record the current symbol count to mark the beginning of a new scope.
    void scope_start() {
        scopeStack.push_back(static_cast<int>(symbols.size()));
    }

    // scope_end: remove all symbols declared since the last scope_start.
    void scope_end() {
        assert(!scopeStack.empty());
        int marker = scopeStack.back();
        scopeStack.pop_back();

        // Roll back all symbols declared in the current scope.
        for (int i = static_cast<int>(symbols.size()) - 1; i >= marker; --i) {
            SymbolEntry& entry = symbols[i];
            int bucketIndex = findBucketForHash(entry.hash, entry.key);
            Bucket& bucket = buckets[bucketIndex];
            // If the bucket currently points to the symbol being removed, restore its previous value.
            if (bucket.index == i) {
                bucket.index = entry.prev;
                if (entry.prev != -1) {
                    bucket.activeNode = nodePtrs[entry.prev];
                } else {
                    bucket.activeNode = nullptr;
                    // Optionally clear bucket.key if desired.
                }
            }
        }
        // Remove symbols from the metadata and node pointer arrays.
        symbols.resize(marker);
        nodePtrs.resize(marker);
    }
};
