// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstddef>

namespace chem {

    /**
     * number of bytes of stack still available to the calling thread, or 0 when it
     * cannot be determined on the current platform
     *
     * the stack grows downwards on every platform we support, so the available
     * space is measured between the thread's stack base and the address of the
     * calling frame. The thread's stack bounds are queried once per thread and
     * cached, so repeated calls are cheap
     */
    size_t remaining_stack_bytes();

    /**
     * true once too little stack is left to keep nesting comptime evaluation by
     * another level.
     *
     * A comptime function which calls itself never returns, so its evaluation —
     * and the MIR lowering it drives, and the MIR engine which runs it — all
     * recurse on the compiler's own stack. The space that remains before this
     * returns true is reserved to report the diagnostic and unwind the already
     * nested calls, so the compiler never overflows the stack.
     */
    bool comptime_stack_exhausted();

    /**
     * the diagnostic to report when comptime_stack_exhausted() is true
     */
    const char* comptime_stack_error_message();

    /**
     * true once a comptime evaluation in this thread has stopped for lack of
     * stack. Sticky for the rest of the compilation: the root cause is reported
     * once and every later evaluation bails instead of repeating the recursion.
     */
    bool comptime_stack_failed();

    /**
     * record that a comptime evaluation stopped for lack of stack
     */
    void mark_comptime_stack_failed();

    /**
     * true once the stack-exhaustion diagnostic has been emitted
     */
    bool comptime_stack_reported();

    /**
     * record that the stack-exhaustion diagnostic has been emitted
     */
    void mark_comptime_stack_reported();

}
