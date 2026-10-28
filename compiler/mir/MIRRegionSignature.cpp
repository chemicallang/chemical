// Copyright (c) Chemical Language Foundation 2025.

#include "MIRRegionSignature.h"

namespace mir {

namespace {

MIRRegionId map_lifetime(const std::vector<std::string_view>& lifetime_params,
                         std::string_view name) {
    if (name.empty()) return MIR_INVALID_ID;
    // The lexer stores the token without its leading quote, but accept both
    // spellings so callers cannot lose a region to a stray quote.
    if (!name.empty() && name.front() == '\'') name.remove_prefix(1);
    if (name == "static") return MIR_REGION_STATIC;
    if (name == "elided" || name == "self") return MIR_REGION_ELIDED;
    for (size_t i = 0; i < lifetime_params.size(); ++i) {
        std::string_view declared = lifetime_params[i];
        if (!declared.empty() && declared.front() == '\'') declared.remove_prefix(1);
        if (declared == name) return static_cast<MIRRegionId>(i + 1);
    }
    return MIR_INVALID_ID;
}

} // namespace

MIRRegionId region_id_for_lifetime(
    const std::vector<std::string_view>& lifetime_params,
    std::string_view name) {
    return map_lifetime(lifetime_params, name);
}

MIRRegionSignature make_region_signature(
    const std::vector<std::string_view>& lifetime_params,
    const std::vector<std::string_view>& param_lifetimes,
    std::string_view return_lifetime,
    const std::vector<std::pair<std::string_view, std::string_view>>& outlives) {
    MIRRegionSignature sig;
    sig.lifetime_param_count = static_cast<uint32_t>(lifetime_params.size());
    sig.param_regions.reserve(param_lifetimes.size());
    for (std::string_view name : param_lifetimes) {
        sig.param_regions.push_back(map_lifetime(lifetime_params, name));
    }
    sig.return_region = map_lifetime(lifetime_params, return_lifetime);
    sig.outlives.reserve(outlives.size());
    for (const auto& bound : outlives) {
        const MIRRegionId from = map_lifetime(lifetime_params, bound.first);
        const MIRRegionId to = map_lifetime(lifetime_params, bound.second);
        if (from != MIR_INVALID_ID && to != MIR_INVALID_ID) {
            sig.outlives.emplace_back(from, to);
        }
    }
    return sig;
}

} // namespace mir
