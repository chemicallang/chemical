// Copyright (c) Chemical Language Foundation 2025.
//
// The MIR interpreter: a single-threaded execution engine over a fully
// populated `MIRModule`. It is the interpretation job's substitute for the
// (soon deleted) AST interpreter, so that `--arg-interpret` executes the *same*
// MIR every code generator consumes.
//
// It is AST-free (it reads only MIR tables) and single-threaded (no locks on the
// hot path). Everything the loop needs — type facts, field offsets, symbol
// targets, constant bits, global storage — is resolved once by `prepare()` into
// flat, dense side tables indexed by the same ids MIR already uses. See
// `lang/docs/mir-interpretation-plan.md` §5.

#pragma once

#include "MIRModule.h"

#include <memory>
#include <string>

namespace mir {

class MIRInterpreter {
public:
    explicit MIRInterpreter(const MIRModule& module);
    ~MIRInterpreter();

    MIRInterpreter(const MIRInterpreter&) = delete;
    MIRInterpreter& operator=(const MIRInterpreter&) = delete;

    /**
     * Run the function whose symbol name is `entry` (mangled or plain). Returns
     * true if a function was found and ran to completion; `out_result` receives
     * its scalar return value (0 for a void function). On failure `error` is set.
     */
    bool run(const std::string& entry, int argc, const char* const* argv,
             long long& out_result, std::string& error);

    /** True if a function with symbol name `entry` exists in the module. */
    bool has_function(const std::string& entry) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mir
