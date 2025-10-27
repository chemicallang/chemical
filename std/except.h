// Copyright (c) Chemical Language Foundation 2025.

#pragma once

// Detect whether exceptions are enabled without pulling in heavy headers.
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#define CHEM_EXCEPTIONS_ENABLED 1
#else
#define CHEM_EXCEPTIONS_ENABLED 0
#endif

// Forward-declare helper functions implemented in a single .cpp file.
// These are intentionally minimal and take only C-style strings / ints so the header
// doesn't need <string>, <iostream>, or <system_error>.
namespace chem::detail {

    [[noreturn]] void abort_runtime_msg(const char* msg) noexcept;
// err: platform errno-style integer (e.g. EOVERFLOW) when applicable.
// cat_name: optional category name (may be nullptr).
// cat_message: optional category message (may be nullptr).
    [[noreturn]] void abort_system_msg(int err, const char* cat_name, const char* cat_message, const char* msg) noexcept;

} // namespace chem::detail

// Public macros:
// - CHEM_THROW_RUNTIME("...")                 => throws std::runtime_error(...) or aborts with message
// - CHEM_THROW_SYSTEM(INT_ERR, CAT_NAME, CAT_MSG, "msg")
//     where CAT_NAME/CAT_MSG can be nullptr if not available.
//
// Note: keep these macros expression-safe like a throw used to be.
#if CHEM_EXCEPTIONS_ENABLED

#include <stdexcept>

#define CHEM_THROW_RUNTIME(MSG) \
    do { throw std::runtime_error(MSG); } while (0)

// When exceptions enabled we throw system_error. The macro keeps the same fields as the abort path.
// Caller typically uses: CHEM_THROW_SYSTEM(EOVERFLOW, "generic", "overflow", "file too large")
#define CHEM_THROW_SYSTEM(ERR, CAT_NAME, CAT_MSG, MSG) \
    do { throw std::system_error((ERR), std::generic_category(), (MSG)); } while (0)

#else // exceptions disabled

#define CHEM_THROW_RUNTIME(MSG) \
    do { ::chem::detail::abort_runtime_msg((MSG)); } while (0)

  #define CHEM_THROW_SYSTEM(ERR, CAT_NAME, CAT_MSG, MSG) \
    do { ::chem::detail::abort_system_msg((ERR), (CAT_NAME), (CAT_MSG), (MSG)); } while (0)

#endif // CHEM_EXCEPTIONS_ENABLED
