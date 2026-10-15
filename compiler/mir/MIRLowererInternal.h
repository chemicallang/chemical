// Copyright (c) Chemical Language Foundation 2025.
//
// Small helpers shared by the split MIRLowerer translation units
// (`MIRLowerer.cpp`, `MIRLowerAsync.cpp`, `MIRLowerLambda.cpp`, …). The bulk of
// the lowering lives in `MIRLowerer.cpp`; the per-construct TUs include this for
// the few free helpers that do not belong to the class. Keep it minimal — a
// helper only moves here when a second TU needs it.

#pragma once

#include "MIRModule.h"

namespace mir {

/** True when `type` is void (or an unknown id, which behaves as void). */
inline bool is_void_type(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    return module.types.get(type).kind == MIRTypeKind::Void;
}

} // namespace mir
