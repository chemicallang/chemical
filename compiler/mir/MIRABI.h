// Copyright (c) Chemical Language Foundation 2025.
//
// Backend-neutral ABI classification. Every emitter (C, LLVM, JVM, JS) and the
// lowerer must agree on these *exactly*, or a definition and its callers stop
// matching; they are therefore defined once here rather than re-derived per
// backend (evaluation §1.4 and action B). Before this header the same rules were
// written out nine times across three translation units — `MIRLowerer`
// (`needs_aggregate_path`, `needs_sret_return`, `is_struct_like`),
// `MIRDeclEmitter` (`needs_aggregate`, `needs_sret`) and `CodegenMir`
// (`mir_is_aggregate`, `mir_needs_sret`, `mir_is_ptr_like`, `mir_is_void`) — and
// had to be kept in sync by hand.
//
// The rules are the legacy prototype ABI:
//
//   * a struct-like or array **return** goes through a hidden sret pointer, and
//     typedef structs are included — the `dyn<I>` fat pointer (`TF_TYPEDEF`) is
//     returned through sret like any other aggregate;
//   * a struct-like or array **parameter** is passed by pointer, but a typedef
//     struct is passed *by value* (it is two words: vtable + object);
//   * pointers and references share a C spelling (`T*`, differing only in the
//     const on the pointee), so `is_pointer_like` covers both.
//
// The prototype helpers above are pure functions of the interned type table —
// no AST, no target state. The by-value `@extern` classifier below is the one
// target-aware part: it switches on `MIRModuleLayout::c_abi`, which the front
// end records from the target triple, so the LLVM backend never infers the C
// convention from whichever host compiled it.

#pragma once

#include "MIRModule.h"

#include <cstdint>
#include <vector>

namespace mir {
namespace abi {

/** True when `t` is a valid, in-range type id. */
inline bool is_valid(const MIRModule& m, TypeId t) {
    return t != MIR_INVALID_ID && t < m.types.size();
}

/** Void (or an unknown type id, which behaves as void everywhere). */
inline bool is_void(const MIRModule& m, TypeId t) {
    return !is_valid(m, t) || m.types.get(t).kind == MIRTypeKind::Void;
}

/**
 * A struct-like or array value: the set that travels through memory rather than
 * in registers. A typedef struct is *not* included here for parameter purposes
 * — see `is_aggregate` versus `needs_sret`, which differ exactly on that.
 */
inline bool is_struct_like(const MIRModule& m, TypeId t) {
    if (!is_valid(m, t)) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union ||
           k == MIRTypeKind::Variant;
}

/**
 * Whether a value of `t` is returned through a hidden sret pointer. Every
 * struct-like type *including* typedef structs, plus arrays.
 */
inline bool needs_sret(const MIRModule& m, TypeId t) {
    if (!is_valid(m, t)) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union ||
           k == MIRTypeKind::Variant || k == MIRTypeKind::Array;
}

/**
 * Whether a parameter of type `t` is passed by pointer. Struct-like and arrays,
 * but **not** a typedef struct (the dynamic fat pointer is passed by value).
 */
inline bool is_aggregate(const MIRModule& m, TypeId t) {
    if (!is_valid(m, t)) return false;
    if (m.types.get(t).flags & TF_TYPEDEF) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union ||
           k == MIRTypeKind::Variant || k == MIRTypeKind::Array;
}

/** Pointer or reference. */
inline bool is_pointer_like(const MIRModule& m, TypeId t) {
    if (!is_valid(m, t)) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    return k == MIRTypeKind::Pointer || k == MIRTypeKind::Reference;
}

/**
 * One register-sized piece of a by-value C aggregate. `size` is the number of
 * bytes the C ABI passes in that register; `floating` selects an SSE/FP
 * register class. Backends map this to their own register type (LLVM: iN or
 * float/double).
 */
struct MIRABIPart {
    uint32_t size = 0;
    bool floating = false;
    /** Byte offset of this part inside the aggregate. */
    uint32_t offset = 0;
};

struct MIRABIInfo {
    bool aggregate = false;
    bool memory = false;
    std::vector<MIRABIPart> parts;
};

namespace detail {

inline int abi_scalar_class(const MIRModule& m, TypeId t) {
    if (!is_valid(m, t)) return 1;
    const MIRTypeKind k = m.types.get(t).kind;
    if (k == MIRTypeKind::Float) return 2;
    if (k == MIRTypeKind::Struct || k == MIRTypeKind::Union) return 0;
    if (k == MIRTypeKind::Array || k == MIRTypeKind::Variant) return -1;
    return 1;
}

/** x86-64 System V eightbyte classification (INTEGER/SSE/memory). */
inline bool classify_sysv_x86_64(const MIRModule& m, TypeId t, MIRABIInfo& out,
                                 uint32_t depth) {
    if (depth > 16) {
        out.memory = true;
        return true;
    }
    const MIRTypeRecord& r = m.types.get(t);
    if (r.size == 0 || r.size > 16 || r.alignment == 0) {
        out.memory = true;
        return true;
    }
    int classes[2] = {0, 0};
    for (uint32_t i = 0; i < r.data_count; ++i) {
        const MIRFieldInfo* f = m.types.field_of(t, i);
        if (!f || !is_valid(m, f->type)) {
            out.memory = true;
            return true;
        }
        const MIRTypeRecord& ft = m.types.get(f->type);
        const uint32_t fsize = ft.size;
        if (fsize == 0) continue;
        const uint32_t falign = f->align != 0 ? f->align : ft.alignment;
        if (falign != 0 && (f->offset % falign) != 0) {
            out.memory = true;
            return true;
        }
        if (f->offset >= 16 || f->offset + fsize > 16) {
            out.memory = true;
            return true;
        }
        const int cls = abi_scalar_class(m, f->type);
        if (cls < 0) {
            out.memory = true;
            return true;
        }
        if (cls == 0) {
            MIRABIInfo nested;
            if (!classify_sysv_x86_64(m, f->type, nested, depth + 1) ||
                nested.memory) {
                out.memory = true;
                return true;
            }
            if ((f->offset % 8) != 0 || nested.parts.size() > 2) {
                out.memory = true;
                return true;
            }
            for (uint32_t j = 0; j < nested.parts.size(); ++j) {
                const int nc = nested.parts[j].floating ? 2 : 1;
                classes[((f->offset / 8) + j) & 1] |= nc;
            }
            continue;
        }
        const uint32_t first = f->offset / 8;
        const uint32_t last = (f->offset + fsize - 1) / 8;
        if (last > 1) {
            out.memory = true;
            return true;
        }
        classes[first] |= cls;
        if (last != first) classes[last] |= cls;
    }
    const uint32_t chunks = (r.size + 7) / 8;
    for (uint32_t i = 0; i < chunks; ++i) {
        const uint32_t bytes = r.size - i * 8 > 8 ? 8 : r.size - i * 8;
        if (classes[i] & 1) {
            // Match Clang's C integer type for the register piece: a partial
            // register is rounded up to 1/2/4 bytes, a full one is 8.
            uint32_t size = 8;
            if (bytes <= 1) size = 1;
            else if (bytes <= 2) size = 2;
            else if (bytes <= 4) size = 4;
            out.parts.push_back(MIRABIPart{size, false, i * 8});
        } else if (classes[i] & 2) {
            out.parts.push_back(MIRABIPart{bytes > 4 ? 8u : 4u, true, i * 8});
        } else {
            out.parts.push_back(MIRABIPart{1, false, i * 8});
        }
    }
    return true;
}

/** Windows x64: 1/2/4/8-byte aggregates travel as an integer; all others by ref. */
inline bool classify_windows_x64(const MIRModule& m, TypeId t, MIRABIInfo& out) {
    const MIRTypeRecord& r = m.types.get(t);
    if (r.size == 1 || r.size == 2 || r.size == 4 || r.size == 8) {
        out.parts.push_back(MIRABIPart{r.size, false, 0});
        return true;
    }
    out.memory = true;
    return true;
}

/**
 * AArch64 AAPCS homogeneous floating-point aggregate (1-4 identical floats or
 * doubles, including nested arrays/structs, no padding).
 */
inline bool aapcs64_hfa_impl(const MIRModule& m, TypeId t, uint32_t& base_size,
                             uint32_t& count, uint32_t depth) {
    if (!is_valid(m, t) || depth > 8) return false;
    const MIRTypeRecord& r = m.types.get(t);
    if (r.kind == MIRTypeKind::Float) {
        if (r.size != 4 && r.size != 8) return false;
        base_size = r.size;
        count = 1;
        return true;
    }
    if (r.kind == MIRTypeKind::Array) {
        uint32_t elem_base = 0;
        uint32_t elem_count = 0;
        if (!aapcs64_hfa_impl(m, r.element, elem_base, elem_count, depth + 1)) {
            return false;
        }
        const uint64_t total = static_cast<uint64_t>(elem_count) * r.data_count;
        if (total == 0 || total > 4) return false;
        base_size = elem_base;
        count = static_cast<uint32_t>(total);
        return true;
    }
    if (r.kind != MIRTypeKind::Struct && r.kind != MIRTypeKind::Union) {
        return false;
    }
    const bool is_union = r.kind == MIRTypeKind::Union;
    bool any = false;
    uint32_t base = 0;
    uint32_t total = 0;
    for (uint32_t i = 0; i < r.data_count; ++i) {
        const MIRFieldInfo* f = m.types.field_of(t, i);
        if (!f || !is_valid(m, f->type)) return false;
        const MIRTypeRecord& ft = m.types.get(f->type);
        if (ft.size == 0) continue;
        uint32_t fbase = 0;
        uint32_t fcount = 0;
        if (!aapcs64_hfa_impl(m, f->type, fbase, fcount, depth + 1)) {
            return false;
        }
        if (any && fbase != base) return false;
        any = true;
        base = fbase;
        // Struct members are laid out side by side; union members overlap, so
        // the union's HFA width is the widest member, not their sum.
        if (is_union) {
            if (fcount > total) total = fcount;
        } else {
            total += fcount;
        }
        if (total > 4) return false;
    }
    if (!any) return false;
    // Padding or a partially covered final element disqualifies the HFA.
    if (r.size != total * base) return false;
    base_size = base;
    count = total;
    return true;
}

/** AArch64 AAPCS64: HFAs in FP registers; other <=16-byte composites in x regs. */
inline bool classify_aapcs64(const MIRModule& m, TypeId t, MIRABIInfo& out) {
    const MIRTypeRecord& r = m.types.get(t);
    if (r.size == 0 || r.alignment == 0 || r.alignment > 16) {
        out.memory = true;
        return true;
    }
    uint32_t base = 0;
    uint32_t count = 0;
    // HFAs may be larger than 16 bytes (up to four doubles) and still travel
    // in d0-d3, so test them before the non-HFA 16-byte memory cutoff.
    if (aapcs64_hfa_impl(m, t, base, count, 0)) {
        for (uint32_t i = 0; i < count; ++i) {
            out.parts.push_back(MIRABIPart{base, true, i * base});
        }
        return true;
    }
    if (r.size > 16) {
        out.memory = true;
        return true;
    }
    const uint32_t chunks = (r.size + 7) / 8;
    for (uint32_t i = 0; i < chunks; ++i) {
        uint32_t bytes = r.size - i * 8;
        if (bytes > 8) bytes = 8;
        uint32_t size = 8;
        if (bytes <= 1) size = 1;
        else if (bytes <= 2) size = 2;
        else if (bytes <= 4) size = 4;
        out.parts.push_back(MIRABIPart{size, false, i * 8});
    }
    return true;
}

} // namespace detail

/**
 * Classify a by-value struct/union for the target's C ABI. Returns false when
 * `t` is not a C aggregate (struct/union); variants are deliberately excluded
 * because their discriminator layout has no C counterpart. `memory` means the
 * caller passes a pointer (`byval`/`sret` in LLVM); `parts` describes the
 * register sequence otherwise.
 */
inline bool classify_c_abi(const MIRModule& m, TypeId t, MIRABIInfo& out) {
    out = MIRABIInfo{};
    if (!is_valid(m, t)) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    if (k != MIRTypeKind::Struct && k != MIRTypeKind::Union) return false;
    out.aggregate = true;
    switch (m.layout.c_abi) {
        case MIRABIKind::SysVX86_64:
            return detail::classify_sysv_x86_64(m, t, out, 0);
        case MIRABIKind::WindowsX64:
            return detail::classify_windows_x64(m, t, out);
        case MIRABIKind::SysVX86_32:
            // i386 (System V and Windows cdecl) passes every aggregate on the
            // stack and returns it through a hidden pointer.
            out.memory = true;
            return true;
        case MIRABIKind::AAPCS64:
            return detail::classify_aapcs64(m, t, out);
        case MIRABIKind::Unknown:
        default:
            // The caller must reject by-value extern aggregates before codegen.
            out.memory = true;
            return true;
    }
}

/** Does this target have a classifier that can emit by-value C aggregates? */
inline bool c_abi_supported(const MIRModule& m) {
    switch (m.layout.c_abi) {
        case MIRABIKind::SysVX86_64:
        case MIRABIKind::WindowsX64:
        case MIRABIKind::AAPCS64:
            return true;
        default:
            return false;
    }
}

} // namespace abi
} // namespace mir
