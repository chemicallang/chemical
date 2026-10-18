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
#include <vector>

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

/**
 * One element of an aggregate's *access* layout: the sequence of storage slots
 * a backend indexes into with GEP/`[i]` when it must address a field, together
 * with the MIR field indices that live in each slot.
 *
 * `compute_aggregate_layout` above records each field's byte offset, which is
 * all a backend that lets a C compiler lay the type out (the C target) needs.
 * A backend that builds the aggregate itself (LLVM) needs something more: an
 * explicit ordered list of elements, one per padding run, and one *shared*
 * element for each overlap group (union members / variant case payloads overlap
 * at the same offset), because a struct type cannot have two fields at one
 * offset. That mapping is a pure function of the type table — identical for
 * every such backend — so it is derived once here instead of re-derived in each
 * backend.
 */
struct MIRAggElement {
    /** MIR field index of this element's first field, or `MIR_INVALID_ID` for a padding run. */
    uint32_t first_field = MIR_INVALID_ID;
    /** Number of overlapping fields in this element: 1 for a normal field, >1 for a union/variant group, 0 for padding. */
    uint32_t field_count = 0;
    /** Byte count of a padding element (`first_field == MIR_INVALID_ID`); 0 otherwise. */
    uint32_t pad_bytes = 0;
    /**
     * The MIR type a backend should spell as this element's storage type: for a
     * field group, the member with the largest aligned size (ties prefer the
     * larger alignment) so every member of the group fits; `MIR_INVALID_ID` for
     * a padding run, where the backend spells `[pad_bytes x i8]` instead.
     */
    TypeId rep_type = MIR_INVALID_ID;
};

/** The ordered access layout of one aggregate type. See `compute_aggregate_plan`. */
struct MIRAggregatePlan {
    /** Storage elements in byte-offset order (padding runs included). */
    std::vector<MIRAggElement> elements;
    /** MIR field index -> index into `elements`. */
    std::vector<uint32_t> field_elem;
    /** Aggregate byte size (mirrors `MIRTypeRecord::size`). */
    uint32_t size = 0;
    /** Aggregate alignment (mirrors `MIRTypeRecord::alignment`). */
    uint32_t alignment = 0;
};

/**
 * Build the access plan for aggregate `agg`. Returns false (and leaves `out`
 * cleared) for any non-aggregate kind. `types` must already carry the aggregate
 * layout (`compute_aggregate_layout`), which `MIRTypeBuilder` guarantees before
 * a backend runs.
 */
inline bool compute_aggregate_plan(const MIRTypeTable& types, TypeId agg,
                                   MIRAggregatePlan& out) {
    out.elements.clear();
    out.field_elem.clear();
    out.size = 0;
    out.alignment = 0;
    if (agg >= types.types.size()) return false;
    const MIRTypeRecord& rec = types.types[agg];
    if (rec.kind != MIRTypeKind::Struct && rec.kind != MIRTypeKind::Union &&
        rec.kind != MIRTypeKind::Variant) {
        return false;
    }
    out.size = rec.size;
    out.alignment = rec.alignment;
    out.field_elem.assign(rec.data_count, 0);

    const bool union_all = rec.kind == MIRTypeKind::Union;
    const uint8_t case_kind = static_cast<uint8_t>(MIRFieldKind::VariantCase);

    uint64_t cur = 0;
    uint32_t i = 0;
    while (i < rec.data_count) {
        const MIRFieldInfo* f = types.field_of(agg, i);
        if (!f) {
            ++i;
            continue;
        }
        MIRAggElement elem;
        const bool overlap = union_all || f->kind == case_kind;
        TypeId rep = f->type;
        if (overlap) {
            // union member or variant case: gather the whole run of overlapping
            // fields and pick the member with the largest aligned size as the
            // element's storage type (ties prefer the larger alignment)
            uint32_t best = i;
            uint32_t best_align = 0;
            uint64_t best_aligned = 0;
            uint32_t j = i;
            while (j < rec.data_count) {
                const MIRFieldInfo* g = types.field_of(agg, j);
                if (!g) break;
                if (!(union_all || g->kind == case_kind)) break;
                const uint32_t ga = mir_field_alignment(types, *g);
                const uint64_t gs = mir_align_up(mir_field_size(types, *g), ga);
                if (gs > best_aligned || (gs == best_aligned && ga > best_align)) {
                    best = j;
                    best_align = ga;
                    best_aligned = gs;
                }
                ++j;
            }
            const MIRFieldInfo* rb = types.field_of(agg, best);
            rep = rb ? rb->type : f->type;
            elem.first_field = i;
            elem.field_count = j - i;
            i = j;
        } else {
            elem.first_field = i;
            elem.field_count = 1;
            ++i;
        }

        const uint32_t offset = f->offset;
        if (offset > cur) {
            MIRAggElement pad;
            pad.pad_bytes = static_cast<uint32_t>(offset - cur);
            out.elements.push_back(pad);
            cur = offset;
        }
        elem.rep_type = rep;
        const uint32_t idx = static_cast<uint32_t>(out.elements.size());
        out.elements.push_back(elem);
        for (uint32_t k = 0; k < elem.field_count; ++k) {
            const uint32_t fld = elem.first_field + k;
            if (fld < out.field_elem.size()) out.field_elem[fld] = idx;
        }
        cur = offset + (rep < types.types.size() ? types.types[rep].size : 0);
    }
    if (rec.size > cur) {
        MIRAggElement pad;
        pad.pad_bytes = static_cast<uint32_t>(rec.size - cur);
        out.elements.push_back(pad);
    }
    return true;
}

} // namespace mir
