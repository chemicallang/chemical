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

/**
 * @brief Holds metadata for a declared symbol.
 *
 * The SymbolEntry structure stores the symbol's key (as a string view),
 * its precomputed hash, and a pointer to its associated AST node.
 */
struct SymbolEntry {
    chem::string_view key;  // The symbol name (AST-owned; no extra allocation).
    size_t hash;            // Precomputed hash value for fast lookup.
    ASTNode* node;          // Pointer to the associated AST node.
};

/**
 * @brief Represents an entry in a bucket chain for shadowed symbols.
 *
 * BucketSymbol is used in two cases:
 *  - When a symbol is shadowed (i.e. a later declaration of the same key),
 *    the previous active symbol is stored in the chain via the `next` pointer.
 *  - When a collision occurs (two different keys hash to the same bucket),
 *    the collision chain is maintained via the `collision` pointer in Bucket.
 */
struct BucketSymbol {
    chem::string_view key = "";   // The symbol key.
    size_t hash = 0;              // Precomputed hash for the symbol.
    ASTNode* activeNode = nullptr;// Direct pointer to the AST node.
    int index = -1;               // Index into the SymbolEntry vector (-1 if empty).
    BucketSymbol* next = nullptr; // Pointer to the next symbol in the chain.
};

/**
 * @brief Represents a hash table bucket.
 *
 * Inherits from BucketSymbol to directly store the "active" symbol
 * in the bucket. In addition, it maintains a separate collision chain
 * (via the collision pointer) for symbols with different keys that
 * hash to the same index.
 */
struct Bucket : BucketSymbol {
    BucketSymbol* collision = nullptr; // Chain of symbols that collided (different keys).
};

/**
 * @brief Represents a lexical scope.
 *
 * Each SymbolScope records the index in the SymbolEntry vector at which
 * the scope began, as well as a kind identifier.
 */
struct SymbolScope {
    int start;  // The index in the symbols vector where this scope began.
    int kind;   // An integer identifier for the scope type (0 if not specified).
};

/**
 * @brief A high-performance symbol table with custom chaining for shadowing and collisions.
 *
 * This SymbolTable maintains a contiguous array of SymbolEntry metadata and
 * an array of Bucket entries for fast resolution. Shadowing (multiple declarations
 * of the same key) is handled via a chain linked by the 'next' pointer, while hash
 * collisions (different keys with the same hash bucket) are stored in a separate chain
 * via the 'collision' pointer. Note that the collision chain is always checked,
 * even if the bucket's active entry is removed.
 */
class SymbolTable {
public:
    // Computes the hash for a given key.
    static inline size_t computeHash(const chem::string_view& key) {
        return std::hash<chem::string_view>{}(key);
    }

private:
    std::vector<SymbolEntry> symbols;    // Contiguous metadata for declared symbols.
    std::vector<Bucket> buckets;           // Hash table: an array of buckets.
    size_t bucketMask;                     // Mask used for fast modulo (buckets.size() is a power of two).
    std::vector<SymbolScope> scopeStack;   // Stack of scopes for managing declarations.
    std::vector<BucketSymbol*> extra_symbols; // Dynamically allocated BucketSymbol objects for shadowing and collisions.

    /**
     * @brief Dummy batch allocator for BucketSymbol objects.
     *
     * Replace this with your actual memory pool allocator if needed.
     *
     * @return Pointer to a newly allocated BucketSymbol.
     */
    inline BucketSymbol* allocateBucketSymbol() {
        auto sym = reinterpret_cast<BucketSymbol*>(
                ::operator new(sizeof(BucketSymbol), std::align_val_t(alignof(BucketSymbol)))
        );
        extra_symbols.emplace_back(sym);
        return sym;
    }

    /**
     * @brief Allocates and constructs a BucketSymbol by copying an existing one.
     *
     * @param bucket The BucketSymbol to copy.
     * @return Pointer to the newly allocated BucketSymbol.
     */
    inline BucketSymbol* allocateBucketSymbol(const BucketSymbol& bucket) {
        return new (allocateBucketSymbol()) BucketSymbol(bucket);
    }

    /**
     * @brief Adds a new symbol entry to the symbols vector.
     *
     * @param key The symbol key.
     * @param hash The precomputed hash.
     * @param node Pointer to the associated AST node.
     * @return The index of the new entry.
     */
    inline int put_entry(const chem::string_view& key, const size_t hash, ASTNode* const node) noexcept {
        int newIndex = static_cast<int>(symbols.size());
        symbols.emplace_back(key, hash, node);
        return newIndex;
    }

    /**
     * @brief Sets the bucket's active fields.
     *
     * @param bucket The bucket to update.
     * @param key The symbol key.
     * @param hash The symbol hash.
     * @param node Pointer to the AST node.
     * @param index Index into the symbols vector.
     * @param next Pointer to the next symbol in the shadow chain.
     */
    inline void set_to_bucket(Bucket& bucket, const chem::string_view& key, const size_t hash,
                              ASTNode* const node, int index, BucketSymbol* const next) noexcept {
        bucket.key = key;
        bucket.hash = hash;
        bucket.index = index;
        bucket.activeNode = node;
        bucket.next = next;
    }

    /**
     * @brief Allocates and constructs a BucketSymbol with the given parameters.
     *
     * @param key The symbol key.
     * @param hash The symbol hash.
     * @param node Pointer to the AST node.
     * @param index Index into the symbols vector.
     * @param next Pointer to the next symbol in the chain.
     * @return Pointer to the new BucketSymbol.
     */
    inline BucketSymbol* allocateBucketSymbol(const chem::string_view& key, const size_t hash,
                                              ASTNode* const node, int index, BucketSymbol* const next) noexcept {
        return new (allocateBucketSymbol()) BucketSymbol{key, hash, node, index, next};
    }

    /**
     * @brief Inserts a symbol entry into a new bucket array during rehashing.
     *
     * @param index Index of the symbol entry.
     * @param entry The symbol entry.
     * @param targetBuckets The new bucket array.
     * @param mask The new bucket mask.
     */
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
                // Same key (shadowing): save the current active symbol in the shadow chain.
                const auto extra = allocateBucketSymbol(bucket);
                // Update bucket's active entry with the new symbol.
                bucket.key = entry.key;
                bucket.hash = entry.hash;
                bucket.index = index;
                bucket.activeNode = entry.node;
                bucket.next = extra;
            } else {
                // Collision: keys differ but hashed to the same bucket.
                // Allocate a new symbol for the collision chain.
                const auto extra = allocateBucketSymbol(entry.key, entry.hash, entry.node, index, bucket.collision);
                bucket.collision = extra;
            }
        }
    }

    /**
     * @brief Rehashes the bucket array when load factor exceeds threshold.
     *
     * Doubles the bucket count (rounded to the next power of two) and reinserts all symbols.
     */
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
    /**
     * @brief Computes the next power of two for a given size.
     *
     * @param x The input size.
     * @return The next power of two.
     */
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
     * @brief Default constructor.
     *
     * Initializes the symbol table with an initial bucket count of 128 (a power of two).
     */
    SymbolTable() {
        const auto initialBucketCount = 128; // Must be a power of two.
        buckets.resize(initialBucketCount, Bucket());
        bucketMask = initialBucketCount - 1;
        symbols.reserve(initialBucketCount);
        scopeStack.reserve(20);
    }

    /**
     * @brief Constructs a SymbolTable with a specified initial bucket count.
     *
     * The bucket count is rounded up to the next power of two.
     *
     * @param initialBucketCount The requested initial bucket count.
     */
    SymbolTable(size_t initialBucketCount) {
        initialBucketCount = next_power_of_two(initialBucketCount);
        buckets.resize(initialBucketCount, Bucket());
        bucketMask = initialBucketCount - 1;
        symbols.reserve(initialBucketCount);
        scopeStack.reserve(20);
    }

    /**
     * @brief Declares a new symbol in the current scope.
     *
     * Inserts the symbol into the hash table. If the bucket is empty,
     * the symbol is placed directly; otherwise, if the same key exists,
     * the new symbol shadows the current one via the shadow chain (next pointer).
     * If the bucket is occupied by a different key (hash collision), the new
     * symbol is added to the collision chain.
     *
     * @param key The symbol key.
     * @param node Pointer to the associated AST node.
     */
    void declare(const chem::string_view& key, ASTNode* const node) {
        // Rehash if load factor exceeds 90%.
        if (symbols.size() >= static_cast<size_t>(buckets.size() * 0.9)) {
            rehash();
        }

        const auto hash = computeHash(key);
        const auto bucketIndex = hash & bucketMask;
        Bucket &bucket = buckets[bucketIndex];
        const auto index = put_entry(key, hash, node);

        if (bucket.index == -1) {
            // Bucket is empty; insert directly.
            set_to_bucket(bucket, key, hash, node, index, nullptr);
        } else {
            if (bucket.hash == hash && bucket.key == key) {
                // Same key: shadow the current active symbol.
                const auto next = allocateBucketSymbol(bucket);
                set_to_bucket(bucket, key, hash, node, index, next);
            } else {
                // Collision: different key but same bucket.
                const auto extra = allocateBucketSymbol(key, hash, node, index, bucket.collision);
                bucket.collision = extra;
            }
        }
    }

    /**
     * @brief Declares a symbol without allowing shadowing.
     *
     * If a symbol with the same key is already declared (either in the bucket or in the collision chain),
     * returns a pointer to the existing BucketSymbol; otherwise, declares the new symbol.
     *
     * @param key The symbol key.
     * @param node Pointer to the associated AST node.
     * @return Pointer to an existing BucketSymbol if already declared, or nullptr otherwise.
     */
    const BucketSymbol* declare_no_shadow_sym(const chem::string_view& key, ASTNode* const node) {
        // Rehash if needed.
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
                // Check collision chain for the key.
                BucketSymbol* sym = bucket.collision;
                while(sym) {
                    const auto& ptr = *sym;
                    if(ptr.hash == hash && ptr.key == key) {
                        return &ptr;
                    }
                    sym = ptr.next;
                }
                // Not found; add to collision chain.
                const auto new_coll_chain = allocateBucketSymbol(key, hash, node, put_entry(key, hash, node), bucket.collision);
                bucket.collision = new_coll_chain;
                return nullptr;
            }
        }
    }

    /**
     * @brief Declares a symbol without allowing shadowing.
     *
     * If a symbol with the same key already exists, returns the active node of that symbol;
     * otherwise, declares the symbol and returns nullptr.
     *
     * @param key The symbol key.
     * @param node Pointer to the associated AST node.
     * @return Pointer to the active AST node of an existing symbol if found, or nullptr otherwise.
     */
    ASTNode* declare_no_shadow(const chem::string_view& key, ASTNode* const node) {
        const auto d = declare_no_shadow_sym(key, node);
        return d ? d->activeNode : nullptr;
    }

    /**
     * @brief Checks if the given symbol is declared in the current scope.
     *
     * @param symbol Pointer to the BucketSymbol.
     * @return True if the symbol's index is within the current scope; false otherwise.
     */
    inline bool is_in_current_scope(const BucketSymbol* symbol) {
        return symbol->index >= scopeStack.back().start;
    }

    /**
     * @brief Resolves a symbol by its key.
     *
     * Returns the active AST node pointer for the symbol that matches the given key.
     * It first checks the active bucket entry, then scans the collision chain if necessary.
     *
     * @param key The symbol key.
     * @return Pointer to the associated AST node if found, or nullptr if not found.
     */
    ASTNode* resolve(const chem::string_view& key) const {
        size_t hash = computeHash(key);
        size_t bucketIndex = hash & bucketMask;
        const Bucket &bucket = buckets[bucketIndex];
        if (bucket.index != -1 && bucket.hash == hash && bucket.key == key)
            return bucket.activeNode;
        // Scan collision chain even if bucket index is -1.
        auto sym = bucket.collision;
        while (sym) {
            const auto& ptr = *sym;
            if (ptr.hash == hash && ptr.key == key)
                return ptr.activeNode;
            sym = ptr.next;
        }
        return nullptr;
    }

    /**
     * @brief Erases a symbol with the given key.
     *
     * If the active bucket entry matches the key, it is removed.
     * If a shadow chain exists (via the 'next' pointer), the next symbol is promoted.
     * Otherwise, if the symbol exists in the collision chain, it is removed from that chain.
     *
     * @param key The symbol key to remove.
     * @return True if the symbol was found and erased; false otherwise.
     */
    bool erase(const chem::string_view& key) {
        const auto hash = computeHash(key);
        const auto bucketIndex = hash & bucketMask;
        Bucket &bucket = buckets[bucketIndex];

        if (bucket.index != -1 && bucket.hash == hash && bucket.key == key) {
            if (bucket.next) {
                // If a shadow chain exists, promote the next symbol.
                const auto& current = *bucket.next;
                bucket.index = current.index;
                bucket.activeNode = current.activeNode;
                bucket.next = current.next;
            } else {
                // No shadow chain: mark the bucket as empty.
                bucket.index = -1;
                bucket.activeNode = nullptr;
            }
            return true;
        } else if (bucket.collision) {
            // Check the collision chain for the symbol.
            BucketSymbol** sym_ptr_ref = &bucket.collision;
            while (*sym_ptr_ref) {
                BucketSymbol& sym = **sym_ptr_ref;
                if (sym.hash == hash && sym.key == key) {
                    // Remove the symbol from the collision chain.
                    *sym_ptr_ref = sym.next;
                    return true;
                }
                sym_ptr_ref = &sym.next;
            }
            return false;
        }
        return false;
    }

    /**
     * @brief Begins a new scope.
     *
     * Records the current number of symbols to mark the beginning of a new scope.
     */
    inline void scope_start() {
        scopeStack.emplace_back(static_cast<int>(symbols.size()), 0);
    }

    /**
     * @brief Begins a new scope with a specified kind.
     *
     * @param kind The kind identifier for the scope.
     */
    inline void scope_start(int kind) {
        scopeStack.emplace_back(static_cast<int>(symbols.size()), kind);
    }

    /**
     * @brief Begins a new scope and returns the starting index.
     *
     * @param kind The kind identifier for the scope.
     * @return The starting index in the symbols vector for the new scope.
     */
    inline int scope_start_index(int kind) {
        int scope_index = static_cast<int>(symbols.size());
        scopeStack.emplace_back(scope_index, kind);
        return scope_index;
    }

    /**
     * @brief Retrieves the kind of the most recent scope.
     *
     * @return The kind identifier of the last scope.
     */
    inline int get_last_scope_kind() {
        return scopeStack.back().kind;
    }

    /**
     * @brief Returns the symbols declared in the current scope.
     *
     * @return A span over the SymbolEntry objects for the current scope.
     */
    inline std::span<SymbolEntry> last_scope() {
        int start = scopeStack.back().start;
        return { &symbols[start], symbols.size() - start };
    }

    /**
     * @brief Ends the current scope.
     *
     * Removes all symbols declared since the last scope_start(). For each symbol
     * removed, if it was shadowing another symbol (via the 'next' chain), that
     * shadowed symbol is restored. Additionally, symbols in the collision chain
     * are removed if they were declared in the ending scope.
     */
    void scope_end() {
        assert(!scopeStack.empty());
        int marker = scopeStack.back().start;
        scopeStack.pop_back();

        // Roll back each symbol declared in the current scope.
        for (int i = static_cast<int>(symbols.size()) - 1; i >= marker; --i) {
            const SymbolEntry& entry = symbols[i];
            const auto bucketIndex = entry.hash & bucketMask;
            Bucket &bucket = buckets[bucketIndex];

            if (bucket.index == i) {
                if (bucket.next) {
                    // If a shadow chain exists, promote the next symbol.
                    auto& current = *bucket.next;
                    bucket.key = current.key;
                    bucket.hash = current.hash;
                    bucket.index = current.index;
                    bucket.activeNode = current.activeNode;
                    bucket.next = current.next;
                } else {
                    // No shadow chain: mark the bucket as empty.
                    bucket.index = -1;
                    bucket.activeNode = nullptr;
                }
            } else if (bucket.collision) {
                // Remove the symbol from the collision chain.
                BucketSymbol** sym_ptr_ref = &bucket.collision;
                while (*sym_ptr_ref) {
                    BucketSymbol& sym = **sym_ptr_ref;
                    if (sym.index == i) {
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

    /**
     * @brief Destructor.
     *
     * Frees all BucketSymbol objects allocated via the dummy batch allocator.
     */
    ~SymbolTable() {
        for (const auto sym : extra_symbols) {
            ::operator delete(sym, std::align_val_t(alignof(BucketSymbol)));
        }
    }
};
