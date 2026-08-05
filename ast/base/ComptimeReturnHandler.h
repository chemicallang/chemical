// Copyright (c) Chemical Language Foundation 2025.

#pragma once

class InterpretScope;
class Value;

/**
 * Interface implemented by the backends (C translator / 2c, and the LLVM
 * backend). It is used to handle the return value of the top most comptime
 * function that is called from the runtime mode (codegen).
 *
 * When a comptime function returns a runtime value (`%runtime_value` /
 * `%runtime_block_value`), the returned expression references comptime
 * variables that were automatically captured (linked to a
 * CapturedComptimeVariable node). Those captured variables can only be
 * resolved inside the interpret scope where the return is being interpreted,
 * which dies right after the top most comptime function finishes execution.
 *
 * Instead of copying the returned AST and walking it to replace captured
 * variables with their evaluated values (the old evaluated_comptime
 * approach), the return statement calls this interface while the scope is
 * still alive. The implementation:
 *
 *  1. receives the evaluated runtime value, and
 *  2. receives the interpret scope where the return is being interpreted,
 *     and sets it as a pointer on the backend (so identifiers linked to a
 *     CapturedComptimeVariable can look up the captured variable's value
 *     with find_value and translate that value)
 *  3. visits / translates the returned value using the backend
 *
 * The only overhead is a single virtual function call per return.
 */
class ComptimeReturnHandler {
public:

    /**
     * destructor
     */
    virtual ~ComptimeReturnHandler() = default;

    /**
     * called by the return statement (set_return) of the top most comptime
     * function that is called from the runtime mode (codegen)
     *
     * @param scope the interpret scope where the return value is being
     *        interpreted, it is still alive at this point
     * @param value the evaluated runtime value
     */
    virtual void handle_return_value(InterpretScope& scope, Value* value) = 0;

};
