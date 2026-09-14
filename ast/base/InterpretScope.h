// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "ASTNode.h"
#include "ast/base/ASTAllocator.h"
#include "std/chem_string_view.h"
#include "ast/utils/Operation.h"
#include "ast/base/ValueKind.h"
#include "core/diag/Diagnostic.h"

class Value;

class BaseType;

class GlobalInterpretScope;

class Scope;

class ASTNode;

class LocationManager;

class ASTDiagnoser;

using node_map = std::unordered_map<chem::string_view, ASTNode*>;
using node_iterator = node_map::iterator;
using value_map = std::unordered_map<chem::string_view, Value*>;
using value_iterator = value_map::iterator;

class FunctionType;

class InterpretScope {
public:

    /**
      * This contains a map between identifiers and its values, of the current scope
      */
    std::unordered_map<chem::string_view, Value*> values;

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
     * Return value for the current function being interpreted.
     * This is set by set_return() and retrieved by call() after
     * body interpretation completes. Stored here instead of on
     * AST nodes to avoid storing comptime state in the AST.
     */
    Value* returnValue = nullptr;

    /**
     * Implicit arguments map — populated by `provide` statements.
     * When a function with implicit parameters is called, the interpreter
     * looks up the parameter name in this map from the caller's scope.
     *
     * Lazily allocated: most scopes never contain `provide` statements, so the
     * map is only created on first use. Access via implicit_args_ref() /
     * implicit_args_if_any().
     */
    std::unique_ptr<std::unordered_map<chem::string_view, Value*>> implicit_args;

    /**
     * Returns the implicit-args map, allocating it on first use.
     */
    std::unordered_map<chem::string_view, Value*>& implicit_args_ref() {
        if(!implicit_args) {
            implicit_args = std::make_unique<std::unordered_map<chem::string_view, Value*>>();
        }
        return *implicit_args;
    }

    /**
     * Returns the implicit-args map, or nullptr if none was ever created.
     */
    const std::unordered_map<chem::string_view, Value*>* implicit_args_if_any() const {
        return implicit_args.get();
    }

    /**
     * When set to false (e.g. global scope), values in this scope
     * are NOT destructed when the scope ends.
     */
    bool should_destruct_values = true;

    /**
     * When a return statement is interpreted inside this scope,
     * this flag is set to stop further interpretation of sibling
     * nodes in all parent scopes up to the function scope.
     * Prevents non-loop scopes (like if-blocks) from continuing
     * after a return has been processed.
     */
    bool stopInterpretation = false;

    /**
     * When a break A with a value (break i) is interpreted inside a loop,
     * the evaluated value is stored here so that LoopValue::evaluated_value()
     * can retrieve it. Stored on the scope rather than on AST nodes to avoid
     * polluting the AST with interpretation state.
     */
    Value* loop_break_value = nullptr;

    /**
     * Marks this scope as the evaluation frame of a `loop { }` value expression.
     * `break value` statements walk the scope chain upward to the nearest scope
     * with this flag set and store their value there. This keeps break values
     * per-execution (no shared global slot) and survives intermediate child
     * scope destruction.
     */
    bool is_loop_value_scope = false;

    /**
     * Marks this scope as the execution frame of a loop (for / while / do-while /
     * loop / for-in). `break` and `continue` statements walk the scope chain
     * upward to the nearest scope with this flag set and raise `loop_signal`
     * there. This replaces the previous per-AST-node `stoppedInterpretation`
     * flags, making loop control per-execution (reentrancy-safe).
     */
    bool is_loop_scope = false;

    /**
     * Pending loop control signal for this loop frame:
     *   0 = none, 1 = break, 2 = continue
     * Set by raise_loop_break() / raise_loop_continue() and consumed by the loop
     * interpreter after each iteration.
     */
    int loop_signal = 0;

    /**
     * constructor
     */
    explicit InterpretScope(
        InterpretScope* parent,
        ASTAllocator& allocator,
        GlobalInterpretScope* global
    ) : parent(parent), allocator(allocator), global(global) {

    }

    /**
     * use default move constructor
     */
    InterpretScope(InterpretScope&& scope) noexcept = default;

    /**
     * deleted copy constructor
     */
    InterpretScope(const InterpretScope& copy) = delete;

    /**
     * a helper function to allocate objects so they are destroyed when the scope dies
     * instead of when the allocator dies
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

    /**
     * erases a value by the key name from the value map safely
     */
    void erase_value(const chem::string_view& name);

    /**
     * return value with name, or nullptr
     */
    Value* find_value(const chem::string_view& name);

    /**
     * @return iterator for found value, the map that it was found in
     */
    std::pair<value_iterator, InterpretScope&> find_value_iterator(const chem::string_view& name);

    /**
     * perform an operation between two values
     */
    Value* evaluate(Operation operation, Value* fEvl, Value* sEvl, SourceLocation location, Value* debugValue);

    /**
     * this node will be interpreted in this interpret scope
     */
    void interpret(ASTNode* node);

    /**
     * print all values
     */
    void print_values();

    /**
     * get if it's 64bit
     */
    inline constexpr bool isInterpret64Bit() {
        return sizeof(void*) == 8;
    }

    /**
     *  emit an error for given node
     */
    template <typename GS = GlobalInterpretScope>
    inline void error(const chem::string_view& err, ASTNode* node) {
        const auto diagnoser = static_cast<GS*>(global)->getASTDiagnoser();
        diagnoser.error(err, node);
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
     * Iterates over all values in this scope and calls destructors
     * for struct values that have destructor functions defined.
     * The returnValue is skipped (it has been moved to the caller).
     */
    void destroy_values();

    /**
     * Runs the user-defined `@delete` destructor for a single value, if it is a
     * struct/variant value whose type has a destructor with a body. The destructor
     * body runs in a temporary scope with `self` bound to the value. `self` is
     * removed before the temporary scope is destroyed so the value is not
     * destructed a second time.
     *
     * This is the single place that invokes user destructors for temporary values
     * (bare expression results, access-chain intermediates, index parents, etc.).
     * It does NOT recursively destruct member values — that is handled by
     * destroy_values() during scope teardown.
     */
    void destroy_value(Value* val);

    /**
     * Move semantics helper: after declaring a new variable that holds a destructible
     * struct, scan the scope chain for any existing variable pointing to the same
     * StructValue pointer and clear it (set to nullptr). This prevents double-
     * destruction when a struct is moved from one variable to another.
     * Works correctly regardless of whether the AST node is a VariableIdentifier
     * or has been replaced by the compiler during resolution.
     */
    void move_clear_source(Value* initializer, const chem::string_view& new_name);

    /**
     * Coerces an already-evaluated value to the given target type.
     *
     * Currently this normalizes integer values to the target's bit width
     * (truncation for narrower types, sign extension for signed types), so the
     * interpreter matches the C and LLVM backends for narrow integer types
     * (u8/i8/u16/i16/...). Without this, an out-of-range intermediate (e.g. the
     * literal 300 stored into a `u8`) would keep its full 64-bit value.
     *
     * Returns the (possibly newly allocated) coerced value, or `value` unchanged
     * when the target type is null or no coercion applies.
     */
    Value* coerce_to_type(Value* value, BaseType* type);

    /**
     * Values that want to be deleted when the scope ends
     * must be deleted in this destructor
     */
    virtual ~InterpretScope();

};