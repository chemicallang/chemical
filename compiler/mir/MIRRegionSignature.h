// Copyright (c) Chemical Language Foundation 2025.
//
// Region signatures (S4.2). These are built from declarations before body
// lowering and are read by MIRBorrowCheck. Region ids are per-function: they
// never enter MIRTypeRecord (types stay lifetime-erased).
//
// See lang/docs/safety-implementation-plan.md §6.4 and §7.

#pragma once

#include "MIRTypes.h"

#include <string_view>
#include <utility>
#include <vector>

namespace mir {

using MIRRegionId = uint32_t;

/** `'static`: outlives every function-local region. */
constexpr MIRRegionId MIR_REGION_STATIC = 0;
/** An elided `&self`/`&T` region; the borrow checker allocates a fresh var. */
constexpr MIRRegionId MIR_REGION_ELIDED = 0xFFFFFFFEu;

/**
 * One function's region signature.
 *
 * Region ids 1..lifetime_param_count are the declared lifetime parameters in
 * declaration order. `MIR_REGION_STATIC` and `MIR_REGION_ELIDED` are reserved.
 * `MIR_INVALID_ID` means "no region" (a by-value parameter, or a non-reference
 * return).
 */
struct MIRRegionSignature {
    uint32_t lifetime_param_count = 0;
    /** One entry per function parameter (the receiver is included when present). */
    std::vector<MIRRegionId> param_regions;
    MIRRegionId return_region = MIR_INVALID_ID;
    /** `first` outlives `second` (`where 'a: 'b`). */
    std::vector<std::pair<MIRRegionId, MIRRegionId>> outlives;
    /** HRTB binders (`for<'a> Fn(...)`); reserved for S4.2b. */
    std::vector<MIRRegionId> hrtb_binders;

    bool has_regions() const {
        return lifetime_param_count != 0 || return_region != MIR_INVALID_ID ||
               !param_regions.empty() || !outlives.empty() || !hrtb_binders.empty();
    }
};

/**
 * The pure mapping step: resolve lifetime *names* (the token spelling without
 * the leading quote) to per-function region ids. Unknown names map to
 * `MIR_INVALID_ID` (the front end has already rejected them; this is
 * defensive). `"static"` is `MIR_REGION_STATIC`; `"elided"`/`"self"` are
 * `MIR_REGION_ELIDED`.
 */
MIRRegionSignature make_region_signature(
    const std::vector<std::string_view>& lifetime_params,
    const std::vector<std::string_view>& param_lifetimes,
    std::string_view return_lifetime,
    const std::vector<std::pair<std::string_view, std::string_view>>& outlives);

} // namespace mir
