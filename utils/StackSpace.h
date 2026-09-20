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

}
