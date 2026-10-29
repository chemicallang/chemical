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
 * One reference-carrying field of an aggregate parameter (S4.2b).
 *
 * `field_name` is the interned name constant the field-access instructions
 * use, so the borrow checker can key its field facts exactly like a source
 * `p.field` access. `region` is the declared lifetime argument that field's
 * reference uses (resolved through the parameter type's lifetime args), and
 * `variance` records whether that use is covariant/invariant.
 */
struct MIRParamFieldRegion {
    uint32_t field_index = MIR_INVALID_ID;
    ConstantId field_name = MIR_INVALID_ID;
    MIRRegionId region = MIR_INVALID_ID;
    MIRVariance variance = MIRVariance::Bivariant;
};

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
    /**
     * Per parameter, the reference fields of an aggregate type and the
     * declared region each one carries (S4.2b). Empty for non-aggregate
     * parameters and for aggregates without lifetime-parameterised fields.
     */
    std::vector<std::vector<MIRParamFieldRegion>> param_field_regions;
    /** HRTB binders recorded for a declaration's own lifetime parameters. */
    std::vector<MIRRegionId> hrtb_binders;

    bool has_regions() const {
        return lifetime_param_count != 0 || return_region != MIR_INVALID_ID ||
               !param_regions.empty() || !outlives.empty() ||
               !param_field_regions.empty() || !hrtb_binders.empty();
    }
};

/**
 * Resolve one lifetime *name* to a region id in `lifetime_params`.
 * Returns `MIR_INVALID_ID` for an unknown name. `"static"` is
 * `MIR_REGION_STATIC`; `"elided"`/`"self"` are `MIR_REGION_ELIDED`.
 */
MIRRegionId region_id_for_lifetime(
    const std::vector<std::string_view>& lifetime_params,
    std::string_view name);

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
