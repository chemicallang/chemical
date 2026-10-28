// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <utility>
#include <cstdint>

#include "ASTNode.h"
#include "ast/base/ASTAllocator.h"
#include "ast/base/ScopeValueMap.h"
#include "std/chem_string_view.h"
#include "ast/utils/Operation.h"
#include "ast/base/ValueKind.h"
#include "core/diag/Diagnostic.h"

class Value;

class BaseType;

class GlobalInterpretScope;

class ASTNode;

class InterpretScope {
public:

    /**
      * This contains a map between identifiers and its values, of the current scope.
      * Backed by ScopeValueMap (flat vector for small scopes, lazily indexed for
      * large ones) to avoid a heap allocation per declared variable.
      */
    ScopeValueMap values;

    /**
     * a pointer to the parent scope, If this is a global scope, it will be a nullptr
     */
    InterpretScope* parent;

    /**
     * a pointer to global scope, If this is a global scope, it will be a pointer to itself
     */
    GlobalInterpretScope* global;

    /**
     * this is a very lightweight allocator, that allocates every value on heap
     * that's it
     */
    ASTAllocator& allocator;

    /**
     * constructor
     */
    explicit InterpretScope(
        InterpretScope* parent,
        ASTAllocator& allocator,
        GlobalInterpretScope* global
    ) : parent(parent), allocator(allocator), global(global), id(next_scope_id()) {

    }

    /**
     * A monotonically increasing identity for this scope object. The
     * interpreter's identifier cache validates it before trusting a cached
     * parent-hop depth: a stack-allocated InterpretScope can be reused at the
     * same address for a different call frame, so the address alone is not a
     * stable identity.
     */
    uint64_t id = 0;

    /**
     * use default move constructor
     */
    InterpretScope(InterpretScope&& scope) noexcept = default;

    /**
     * deleted copy constructor
     */
    InterpretScope(const InterpretScope& copy) = delete;

    /**
     * a helper function to allocate objects from this scope's (arena) allocator
     */
    template<typename T>
    FORCE_INLINE T* allocate() {
        return allocator.allocate<T>();
    }

    /**
     * get the null value
     */
    Value* getNullValue();

    /**
     * declares a value with this name in current scope
     */
    void declare(const chem::string_view& name, Value* value);

    static uint64_t next_scope_id();

    /**
     * return value with name, or nullptr.
     * If out_depth is non-null, it receives the number of parent hops taken to
     * reach the owning scope (0 = this scope). Used by identifier inline caching.
     */
    Value* find_value(const chem::string_view& name, unsigned* out_depth = nullptr);

    /**
     * Returns the ancestor scope `depth` hops up the parent chain, or nullptr if
     * the chain is shorter than `depth`.
     */
    InterpretScope* ancestor_at(unsigned depth) noexcept {
        auto* s = this;
        while(depth > 0) {
            if(!s->parent) return nullptr;
            s = s->parent;
            --depth;
        }
        return s;
    }

    /**
     * perform an operation between two values
     */
    Value* evaluate(Operation operation, Value* fEvl, Value* sEvl, SourceLocation location, Value* debugValue);

    /**
     *  emit an error for given node
     */
    template <typename GS = GlobalInterpretScope>
    inline void error(const chem::string_view& err, ASTNode* node) {
        static_cast<GS*>(global)->getASTDiagnoser().error(err, node);
    }

    /**
     *  emit an error for given value
     */
    template <typename GS = GlobalInterpretScope>
    inline void error(const chem::string_view& err, Value* value) {
        static_cast<GS*>(global)->getASTDiagnoser().error(err, value);
    }

    /**
     *  emit an error for given value
     */
    template <typename GS = GlobalInterpretScope>
    inline Diag& error(SourceLocation location) {
        return static_cast<GS*>(global)->getASTDiagnoser().error(location);
    }

    /**
     *  emit an error for given value
     */
    template <typename GS = GlobalInterpretScope>
    inline Diag& warn(SourceLocation location) {
        return static_cast<GS*>(global)->getASTDiagnoser().warn(location);
    }

    /**
     *  emit an error for given value
     */
    template <typename GS = GlobalInterpretScope>
    inline Diag& info(SourceLocation location) {
        return static_cast<GS*>(global)->getASTDiagnoser().info(location);
    }

    /**
     * create a info diagnostic
     */
    template <typename NodeT>
    inline Diag& info(NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        return info(node->encoded_location());
    }

    /**
     * create a warning diagnostic
     */
    template <typename NodeT>
    inline Diag& warn(NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        return warn(node->encoded_location());
    }

    /**
     * create a error diagnostic
     */
    template <typename NodeT>
    inline Diag& error(NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        return error(node->encoded_location());
    }

    /**
     * Coerces an already-evaluated value to the given target type.
     *
     * Currently this normalizes integer values to the target's bit width
     * (truncation for narrower types, sign extension for signed types), so a
     * folded comptime expression matches the C and LLVM backends for narrow
     * integer types (u8/i8/u16/i16/...). Without this, an out-of-range
     * intermediate (e.g. the literal 300 stored into a `u8`) would keep its
     * full 64-bit value.
     *
     * Returns the (possibly newly allocated) coerced value, or `value` unchanged
     * when the target type is null or no coercion applies.
     */
    Value* coerce_to_type(Value* value, BaseType* type);

    virtual ~InterpretScope();

};