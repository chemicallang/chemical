// Copyright (c) Chemical Language Foundation 2025.
//
// The MIR interpreter: a single-threaded execution engine over a fully
// populated `MIRModule`. It replaces the deleted AST interpreter, so that
// `--arg-interpret` executes the *same* MIR every code generator consumes.
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
     * Re-derive the side tables after the module grew. The comptime driver
     * lowers each callee into the same `MIRModule` between calls, and every MIR
     * table is append-only with stable ids, so this only consumes the records
     * added since the last build instead of rebuilding every table from scratch
     * (that rebuild dominated comptime-heavy lowering).
     */
    void update();

    /**
     * Run the function whose symbol name is `entry` (mangled or plain). Returns
     * true if a function was found and ran to completion; `out_result` receives
     * its scalar return value (0 for a void function). On failure `error` is set.
     */
    bool run(const std::string& entry, int argc, const char* const* argv,
             long long& out_result, std::string& error);

    /** True if a function with symbol name `entry` exists in the module. */
    bool has_function(const std::string& entry) const;

    /**
     * Call a module function by symbol name with raw 64-bit argument cells,
     * returning its raw 64-bit scalar result. This is the entry point a comptime
     * driver will use (see lang/docs/mir-interpretation-plan.md Phase 5 step 1);
     * it is also exercised by MIRTests. Aggregate (sret) results are not
     * supported here yet.
     */
    bool call(const std::string& name, const uint64_t* args, uint32_t nargs,
              uint64_t& out_result, std::string& error);

    /**
     * Call a function whose return is an aggregate (sret): `out_buf` receives
     * the result bytes. `args` are the ordinary arguments; the hidden sret
     * pointer is prepended.
     */
    bool call_sret(const std::string& name, const uint64_t* args, uint32_t nargs,
                   void* out_buf, std::string& error);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mir
