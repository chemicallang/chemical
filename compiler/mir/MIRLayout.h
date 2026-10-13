// Copyright (c) Chemical Language Foundation 2025.
//
// Aggregate layout: size, alignment and per-field byte offset, computed in MIR
// from the canonical type table. This is action A1 of
// mir-migration-evaluation.md: layout is no longer re-derived per backend from
// the AST (or by whichever C compiler happens to compile the declaration) —
// `MIRTypeBuilder` fills it once, here, and the table is the single answer. The
// steps that consume it (A2's one C type printer, A3's declaration table, and a
// shared ABI/layout service for backend B) build on this. See
// mir-implementation-plan.md §2.5/§2.6.
//
// It is deliberately AST-free (it only needs the type/field tables) so it lives
// in MIR core and is covered by the standalone MIRTests target.

#pragma once

#include "MIRTypes.h"
#include "MIRTypeTable.h"

#include <cstdint>

namespace mir {

/** target-independent layout facts the emitters need */
struct MIRModuleLayout {
    bool is64Bit = true;
    /** Windows target: `long`/`ulong` are 32-bit (LLP64) */
    bool windows = false;
    uint32_t pointer_size = 8;
    uint32_t pointer_alignment = 8;
};

/** Round `value` up to the next multiple of `align` (0/1 rounds nothing). */
inline uint32_t mir_align_up(uint32_t value, uint32_t align) {
    if (align <= 1) return value;
    return value + ((align - (value % align)) % align);
}

/** Natural alignment of a field, raised by an explicit `@align` requirement. */
inline uint32_t mir_field_alignment(const MIRTypeTable& types, const MIRFieldInfo& f) {
    uint32_t a = f.type < types.types.size() ? types.types[f.type].alignment : 1;
    if (a == 0) a = 1;
    return f.align > a ? f.align : a;
}

/** Byte size the field's type occupies. */
inline uint32_t mir_field_size(const MIRTypeTable& types, const MIRFieldInfo& f) {
    return f.type < types.types.size() ? types.types[f.type].size : 0;
}

/**
 * Compute `size`, `alignment` and every field's byte `offset` of aggregate `id`
 * from the field types already interned in `types`, writing the result back
 * into `types`. `MIRTypeBuilder` calls this immediately after it fills an
 * aggregate's field list; it is the only place aggregate layout is derived.
 *
 * Rules — the C target's, i.e. what the declaration the emitter writes compiles
 * to (the emitter spells the fields out and lets the C compiler lay them out, so
 * MIR has to agree with it):
 *  - a struct places its fields in order, each at its own alignment;
 *  - a union places every field at offset 0; the region is the largest field's
 *    size rounded up to the largest field's alignment;
 *  - a variant is a struct of its bases, then the `int` discriminator, then a
 *    union of the case payload structs — so `MIRFieldKind::VariantCase` fields
 *    overlap, while the bases and the tag are laid out normally;
 *  - the aggregate's alignment is the largest field (or `explicit_alignment`)
 *    alignment, and its size is rounded up to that alignment.
 *
 * Target dependence enters only through the field records: pointer and integer
 * sizes/alignments were built from `MIRModuleLayout` by `MIRTypeBuilder`, so
 * nothing host-specific is read here. A nested aggregate is laid out when it is
 * mapped, which happens before its container's layout runs; a field of an
 * opaque (or not-yet-filled) type contributes the size/alignment recorded for
 * it.
 */
inline void compute_aggregate_layout(MIRTypeTable& types, TypeId id,
                                     uint32_t explicit_alignment = 0) {
    if (id >= types.types.size()) return;
    MIRTypeRecord& rec = types.types[id];
    if (rec.kind != MIRTypeKind::Struct && rec.kind != MIRTypeKind::Union &&
        rec.kind != MIRTypeKind::Variant) {
        return;
    }

    const bool union_all = rec.kind == MIRTypeKind::Union;
    const uint8_t case_kind = static_cast<uint8_t>(MIRFieldKind::VariantCase);

    uint32_t max_align = explicit_alignment > 0 ? explicit_alignment : 1;
    uint32_t offset = 0;
    uint32_t overlap_size = 0;  // union region: largest member size, aligned
    uint32_t overlap_align = 0; // union region: largest member alignment

    for (uint32_t i = 0; i < rec.data_count; ++i) {
        MIRFieldInfo* f = types.field_of(id, i);
        if (!f) continue;
        const uint32_t fa = mir_field_alignment(types, *f);
        const uint32_t fsz = mir_field_size(types, *f);
        if (union_all || f->kind == case_kind) {
            f->offset = 0;
            const uint32_t aligned = mir_align_up(fsz, fa);
            if (aligned > overlap_size) overlap_size = aligned;
            if (fa > overlap_align) overlap_align = fa;
            continue;
        }
        offset = mir_align_up(offset, fa);
        f->offset = offset;
        offset += fsz;
        if (fa > max_align) max_align = fa;
    }

    if (overlap_align > 0) {
        // the union region follows the fields laid out above; every overlapping
        // field starts there (they share the bytes)
        offset = mir_align_up(offset, overlap_align);
        for (uint32_t i = 0; i < rec.data_count; ++i) {
            MIRFieldInfo* f = types.field_of(id, i);
            if (!f) continue;
            if (!union_all && f->kind != case_kind) continue;
            f->offset = offset;
        }
        offset += overlap_size;
        if (overlap_align > max_align) max_align = overlap_align;
    }

    rec.alignment = max_align;
    rec.size = mir_align_up(offset, max_align);
}

} // namespace mir
