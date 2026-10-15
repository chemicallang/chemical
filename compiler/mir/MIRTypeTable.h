// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level canonical type table. Built once, serially, before parallel
// lowering, then read-only. See mir-implementation-plan.md §2.6.

#pragma once

#include "MIRTypes.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace mir {

enum class MIRTypeKind : uint8_t {
    Void = 0,
    Bool,
    Int,
    Float,
    Pointer,
    Reference,
    Array,
    Struct,
    Union,
    Variant,
    Function,
    Opaque,
};

enum MIRTypeFlags : uint8_t {
    TF_NONE = 0x00,
    TF_SIGNED = 0x01,
    TF_MUTABLE = 0x02,
    TF_HAS_DESTRUCTOR = 0x04,
    TF_HAS_CONSTRUCTOR = 0x08,
    TF_TRIVIAL_BITWISE = 0x10,
    TF_TYPEDEF = 0x20, // spelled by name without a `struct`/`union` prefix
    TF_VARIADIC = 0x40, // function type: trailing `...` (last param is the tail marker)
};

/**
 * Exact C spelling category for integer types, mirroring the legacy 2c backend
 * (see the legacy 2c backend's IntN spelling). 0 = unspecified (spell by width/signedness
 * as intN_t/uintN_t). Keeping the source-level spelling lets MIR-emitted
 * definitions match the prototypes emitted by the legacy declaration pass.
 */
enum MIRIntKind : uint8_t {
    IK_UNSPECIFIED = 0,
    IK_I8, IK_I16, IK_I32, IK_I64, IK_I128,
    IK_U8, IK_U16, IK_U32, IK_U64, IK_U128,
    IK_CHAR, IK_SHORT, IK_INT, IK_LONG, IK_LONGLONG,
    IK_UCHAR, IK_USHORT, IK_UINT, IK_ULONG, IK_ULONGLONG,
};

/**
 * A canonical type record. Portable semantic identity only; target-dependent
 * size/alignment live here as a cache but do not affect interning (interning
 * is per module/target).
 */
struct MIRTypeRecord {
    MIRTypeKind kind = MIRTypeKind::Opaque;
    uint8_t flags = TF_NONE;
    uint8_t int_kind = IK_UNSPECIFIED; // exact C spelling for MIRTypeKind::Int
    uint32_t size = 0;
    uint32_t alignment = 0;
    uint32_t data_offset = 0; // index into MIRTypeTable::data
    uint32_t data_count = 0;  // field/element type count
    TypeId element = MIR_INVALID_ID; // pointee / element / return type
    uint32_t decl = MIR_INVALID_ID;  // source declaration id (struct/variant)
    uint32_t name_offset = 0;        // index into MIRTypeTable::names (aggregates)
    uint32_t name_length = 0;
    // Aggregates only: start index into MIRTypeTable::fields (ordered runtime
    // fields, including inherited bases first). MIR_INVALID_ID = no fields.
    uint32_t field_index = MIR_INVALID_ID;
    /**
     * An aggregate's explicit `__chx_align(N)` request (0 = natural). It is an
     * *input* to `compute_aggregate_layout`, not a result, so it lives on the
     * record: the C declaration printer has to write the attribute out or the
     * C compiler would lay the aggregate out with less alignment than MIR
     * recorded. Source spelling is compared by `equal`/`hash_of`.
     */
    uint32_t explicit_alignment = 0;
};

/**
 * Spelling facts of a field that the source carries but layout does not, kept
 * in the field pool (never interned) so a declaration printer can spell a field
 * from MIR alone — action A3 of mir-migration-evaluation.md.
 */
enum MIRFieldFlags : uint8_t {
    MFF_NONE = 0x00,
    MFF_VOLATILE = 0x01,         // the member is `volatile`
    MFF_FAT_POINTER = 0x02,      // capturing-function member: `__chemical_fat_pointer__*`
    MFF_ANONYMOUS_STRUCT = 0x04, // declared as an inline `struct { ... } name;`
    MFF_ANONYMOUS_UNION = 0x08,  // declared as an inline `union { ... } name;`
};

/** One ordered runtime field of an aggregate (struct/union/variant). */
struct MIRFieldInfo {
    TypeId type = MIR_INVALID_ID;
    uint32_t name_offset = 0; // index into MIRTypeTable::field_names
    uint32_t name_length = 0;
    uint32_t index = 0;       // field index within the LLVM aggregate (for GEP)
    uint8_t kind = 0;         // MIRFieldKind
    uint8_t flags = MFF_NONE; // MIRFieldFlags (spelling, not layout)
    /**
     * Byte offset of the field inside its aggregate, and the explicit
     * alignment the source asked for (`__chx_align(N)`, 0 = natural). Both are
     * filled by `compute_aggregate_layout` (see MIRLayout.h), never interned:
     * interned records only carry `field_index` into this pool.
     */
    uint32_t offset = 0;
    uint32_t align = 0;
};

/** Distinguishes the structural role of a field for emitters. */
enum class MIRFieldKind : uint8_t {
    Direct = 0,   // a normal member
    Base = 1,     // an embedded inherited base struct
    VariantTag = 2, // the variant discriminator (`__chx__vt_621827`)
    VariantCase = 3, // a variant case payload struct (inside the union)
    VariantParam = 4, // a parameter of the active variant case payload
};


struct MIRTypeTable {
    std::vector<MIRTypeRecord> types;
    std::vector<TypeId> data;
    std::vector<char> names;
    std::vector<MIRFieldInfo> fields;
    std::vector<char> field_names;

    /** Attach a C-emittable name to a (named aggregate) type. */
    void set_name(TypeId id, const char* name, uint32_t len) {
        if (id >= types.size()) return;
        types[id].name_offset = static_cast<uint32_t>(names.size());
        types[id].name_length = len;
        if (len) names.insert(names.end(), name, name + len);
    }
    const char* name_data(const MIRTypeRecord& r) const {
        return names.data() + r.name_offset;
    }
    std::string name_of(TypeId id) const {
        if (id >= types.size()) return {};
        const MIRTypeRecord& r = types[id];
        if (r.name_length == 0) return {};
        return std::string(names.data() + r.name_offset, r.name_length);
    }

    /** Append a range of fields, returning the start index. */
    uint32_t append_fields(const std::vector<MIRFieldInfo>& fs) {
        uint32_t off = static_cast<uint32_t>(fields.size());
        fields.insert(fields.end(), fs.begin(), fs.end());
        return off;
    }

    std::string field_name_of(const MIRFieldInfo& f) const {
        if (f.name_length == 0) return {};
        return std::string(field_names.data() + f.name_offset, f.name_length);
    }

    MIRFieldInfo* field_of(TypeId agg, uint32_t i) {
        if (agg >= types.size()) return nullptr;
        const MIRTypeRecord& r = types[agg];
        if (r.field_index == MIR_INVALID_ID || i >= r.data_count) return nullptr;
        const uint32_t idx = r.field_index + i;
        if (idx >= fields.size()) return nullptr;
        return &fields[idx];
    }

    const MIRFieldInfo* field_of(TypeId agg, uint32_t i) const {
        if (agg >= types.size()) return nullptr;
        const MIRTypeRecord& r = types[agg];
        if (r.field_index == MIR_INVALID_ID || i >= r.data_count) return nullptr;
        const uint32_t idx = r.field_index + i;
        if (idx >= fields.size()) return nullptr;
        return &fields[idx];
    }

    const MIRTypeRecord& get(TypeId id) const { return types[id]; }
    uint32_t size() const { return static_cast<uint32_t>(types.size()); }

    /** Append a range of TypeIds to the shared data pool, returning its offset. */
    uint32_t append_data(const TypeId* ids, uint32_t count) {
        uint32_t off = static_cast<uint32_t>(data.size());
        for (uint32_t i = 0; i < count; ++i) data.push_back(ids[i]);
        return off;
    }

    /**
     * Intern a type record.
     *
     * `intern_index_` is a *hint*, not the source of truth: every hit is
     * re-checked with `equal` and every miss falls back to the linear scan, so
     * the result is identical to a pure scan even when a record is mutated after
     * it was interned (e.g. `fat_pointer_type` fills its fields, frame/aggregate
     * records get theirs, all of which push directly instead of interning). The
     * index exists because most `intern` calls re-look-up a handful of types: on
     * the main suite it turns ~101M `equal` calls into 186k hash lookups plus
     * ~7M (the fallback scans for the ~1k genuinely new records).
     */
    TypeId intern(const MIRTypeRecord& rec) {
        const uint64_t h = hash_of(rec);
        auto hit = intern_index_.find(h);
        if (hit != intern_index_.end() && hit->second < types.size() &&
            equal(types[hit->second], rec)) {
            return hit->second;
        }
        for (uint32_t i = 0; i < types.size(); ++i) {
            if (equal(types[i], rec)) {
                intern_index_[h] = i;
                return i;
            }
        }
        types.push_back(rec);
        const TypeId id = static_cast<TypeId>(types.size() - 1);
        intern_index_[h] = id;
        return id;
    }

    /**
     * Hash of exactly the fields `equal` compares (never `name_offset`,
     * `data_offset` or the pools' positions, which can differ between equal
     * records). Function parameter ids live in the shared `data` pool, so they
     * are mixed in by value.
     */
    uint64_t hash_of(const MIRTypeRecord& r) const {
        uint64_t h = 1469598103934665603ull;
        auto mix = [&h](uint64_t v) {
            h ^= v;
            h *= 1099511628211ull;
        };
        mix(static_cast<uint64_t>(r.kind));
        mix(r.flags);
        mix(r.size);
        mix(r.alignment);
        mix(r.element);
        mix(r.decl);
        mix(r.data_count);
        mix(r.int_kind);
        mix(r.field_index);
        mix(r.explicit_alignment);
        if (r.kind == MIRTypeKind::Function) {
            for (uint32_t i = 0; i < r.data_count; ++i) {
                const uint32_t idx = r.data_offset + i;
                mix(idx < data.size() ? data[idx] : 0xFFFFFFFFu);
            }
        }
        return h;
    }

    /**
     * Fast interning index: hash of an interned record -> a representative id.
     * Only ids whose current record was verified against the querying record are
     * returned, so a stale or mutated entry can never be handed out.
     */
    std::unordered_map<uint64_t, TypeId> intern_index_;

    bool equal(const MIRTypeRecord& a, const MIRTypeRecord& b) const {
        if (a.kind != b.kind || a.flags != b.flags || a.size != b.size ||
            a.alignment != b.alignment || a.element != b.element || a.decl != b.decl ||
            a.data_count != b.data_count || a.int_kind != b.int_kind ||
            a.field_index != b.field_index ||
            a.explicit_alignment != b.explicit_alignment) {
            return false;
        }
        // Only function types store parameter types in the shared data pool;
        // for arrays `data_count` is the element count (element holds the type).
        if (a.kind != MIRTypeKind::Function) return true;
        for (uint32_t i = 0; i < a.data_count; ++i) {
            const uint32_t ia = a.data_offset + i;
            const uint32_t ib = b.data_offset + i;
            if (ia >= data.size() || ib >= data.size()) return false;
            if (data[ia] != data[ib]) return false;
        }
        return true;
    }
};

} // namespace mir
