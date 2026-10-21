// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level canonical type table. Built once, serially, before parallel
// lowering, then read-only. See mir-implementation-plan.md §2.6.

#pragma once

#include "MIRTypes.h"
#include "MIRBytePool.h"
#include "MIRKeyMap.h"
#include "MIRStableArray.h"
#include "MIRTableLock.h"

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <string_view>
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
    /**
     * Declare that one thread performs every mutation of this table (the
     * shipping pipeline). Readers then skip the shared lock; concurrent readers
     * stay safe because there is no concurrent writer. Leave this off when
     * several lowering workers intern into the table (see MIRTableLock.h).
     */
    void set_single_threaded(bool v) noexcept {
        lock_policy_.set_single_threaded(v);
        names.set_single_threaded(v);
        field_names.set_single_threaded(v);
    }
    bool single_threaded() const noexcept { return lock_policy_.single_threaded(); }
    /**
     * Three storage classes, chosen by how the container is *read* (action E3):
     *
     *  - `types`: a stable append-only array, because the hot `equal`/`hash_of`
     *    intern scan indexes it constantly and its records are read lock-free by
     *    every worker (action E3d).
     *  - `data`/`fields`: the same stable array, because they hand out element
     *    pointers (`field_of`) and are indexed by emitters concurrently, so they
     *    must not realloc under a reader.
     *  - `names`/`field_names`: `MIRBytePool`, because readers hold a raw
     *    `const char*` + length *into* them (see MIRBytePool.h).
     */
    MIRStableArray<MIRTypeRecord> types;
    MIRStableArray<TypeId> data;
    MIRBytePool names;
    MIRStableArray<MIRFieldInfo> fields;
    MIRBytePool field_names;

    /** Attach a C-emittable name to a (named aggregate) type. */
    void set_name(TypeId id, const char* name, uint32_t len) {
        MIRWriteLock lock(mtx_, single_threaded());
        if (id >= types.size()) return;
        types[id].name_offset = names.append(name, len);
        types[id].name_length = len;
    }
    const char* name_data(const MIRTypeRecord& r) const {
        return names.data_at(r.name_offset);
    }
    std::string name_of(TypeId id) const {
        if (id >= types.size()) return {};
        const MIRTypeRecord& r = types[id];
        if (r.name_length == 0) return {};
        return std::string(names.data_at(r.name_offset), r.name_length);
    }

    /**
     * The interned name of an aggregate, without the `std::string` copy
     * `name_of` makes. The bytes live in `names` (stable on append), so the
     * view stays valid as long as this table's pool is not cleared — emitters
     * consume it immediately, which is the contract the pool documents.
     */
    std::string_view name_view(TypeId id) const {
        if (id >= types.size()) return {};
        const MIRTypeRecord& r = types[id];
        if (r.name_length == 0) return {};
        return std::string_view(names.data_at(r.name_offset), r.name_length);
    }

    /** True when `id` is an aggregate with a C-emittable name. */
    bool has_name(TypeId id) const { return !name_view(id).empty(); }

    /** Append a range of fields, returning the start index. */
    uint32_t append_fields(const std::vector<MIRFieldInfo>& fs) {
        MIRWriteLock lock(mtx_, single_threaded());
        return fields.append(fs.data(), static_cast<uint32_t>(fs.size()));
    }

    /**
     * Append a non-interned record (variant case payloads, synthesized inline
     * structs), returning its id. Locked like `intern` (action E3d).
     */
    TypeId push(const MIRTypeRecord& rec) {
        MIRWriteLock lock(mtx_, single_threaded());
        types.push_back(rec);
        const TypeId id = static_cast<TypeId>(types.size() - 1);
        // Register the record's hash even though it is not interned: `intern`
        // relies on "an absent hash means no equal record exists" (see there),
        // which only holds if every record in the table is discoverable by its
        // current hash. A record pushed empty and filled by `update` is
        // re-registered by `update`.
        intern_index_.set(hash_of(types[id]), id);
        return id;
    }

    /**
     * Mutate an interned record under the exclusive lock (build-phase fill:
     * data_count/field_index/explicit_alignment). `fn` must not call back into
     * the table (no `intern`/`append_fields`/`update` inside it).
     */
    template <class F>
    void update(TypeId id, F&& fn) {
        MIRWriteLock lock(mtx_, single_threaded());
        if (id >= types.size()) return;
        fn(types[id]);
        // Re-register under the record's *new* hash: `intern` treats an absent
        // hash as proof that no equal record exists, which requires the index to
        // follow a mutation of a hashed field (data_count/field_index/…). The
        // stale entry under the old hash stays; `intern` treats it as a
        // present-but-unequal hint and falls back to the verified scan.
        intern_index_.set(hash_of(types[id]), id);
    }

    /**
     * Intern a field name, returning its stable offset (action E3). Guarded like
     * the other mutators, so workers can append field names concurrently without
     * reallocating a `const char*` a reader still holds.
     */
    uint32_t append_field_name(const char* n, uint32_t len) {
        MIRWriteLock lock(mtx_, single_threaded());
        return field_names.append(n, len);
    }

    std::string field_name_of(const MIRFieldInfo& f) const {
        if (f.name_length == 0) return {};
        return std::string(field_names.data_at(f.name_offset), f.name_length);
    }

    /** `field_name_of` without the allocation — use it for comparisons and for
     * appending a field name straight into an emitter's output buffer. */
    std::string_view field_name_view(const MIRFieldInfo& f) const {
        if (f.name_length == 0) return {};
        return std::string_view(field_names.data_at(f.name_offset), f.name_length);
    }

    MIRFieldInfo* field_of(TypeId agg, uint32_t i) {
        if (agg >= types.size()) return nullptr;
        const MIRTypeRecord& r = types[agg];
        if (r.field_index == MIR_INVALID_ID || i >= r.data_count) return nullptr;
        const uint32_t idx = r.field_index + i;
        if (idx >= fields.size()) return nullptr;
        // `fields` is a stable array, so the returned pointer stays valid
        return &fields[idx];
    }

    const MIRFieldInfo* field_of(TypeId agg, uint32_t i) const {
        if (agg >= types.size()) return nullptr;
        const MIRTypeRecord& r = types[agg];
        if (r.field_index == MIR_INVALID_ID || i >= r.data_count) return nullptr;
        const uint32_t idx = r.field_index + i;
        if (idx >= fields.size()) return nullptr;
        // `fields` is a stable array, so the returned pointer stays valid
        return &fields[idx];
    }

    /**
     * Lock-free read of the record (action E3d: `types` is an append-only stable
     * array, so another worker's append never moves the record, and a reader's
     * reference stays valid). A reference, not a copy: this is the single most
     * frequent table read in the compiler (every operand's type, on every
     * lowered expression and every emitted instruction), and the record is 44
     * bytes.
     */
    const MIRTypeRecord& get(TypeId id) const {
        return types[id];
    }
    uint32_t size() const {
        return types.size();
    }

    /** Append a range of TypeIds to the shared data pool, returning its offset. */
    uint32_t append_data(const TypeId* ids, uint32_t count) {
        MIRWriteLock lock(mtx_, single_threaded());
        return data.append(ids, count);
    }

    /** Lock-free read of the shared `data` pool (action E3d). */
    TypeId data_at(uint32_t index) const {
        return data[index];
    }
    uint32_t data_size() const {
        return data.size();
    }

    /**
     * Intern a type record.
     *
     * `intern_index_` is a *hint*, not the source of truth: every hit is
     * re-checked with `equal`, and a hash that is present but names a different
     * record falls back to the verified scan, so a record mutated after it was
     * interned (e.g. `fat_pointer_type` fills its fields, frame/aggregate
     * records get theirs) can never be aliased. An *absent* hash skips the scan
     * entirely — see the comment at the insert below.
     *
     * The index exists because most `intern` calls re-look-up a handful of
     * types: on the main suite it turns ~101M `equal` calls into 116k hash
     * lookups.
     */
    TypeId intern(const MIRTypeRecord& rec) {
        const uint64_t h = hash_of(rec);
        if (!single_threaded()) {
            // Read-mostly: on the main suite 106,337 of 116,157 `intern` calls
            // are hits, and an *exclusive* lock per hit serialises every
            // lowering worker (the parallel phase is the only time the lock is
            // taken at all). Verify the hint under the shared lock and only fall
            // through to the writer path when the record is not already here.
            MIRReadLock rl(mtx_, false);
            const uint32_t hit = intern_index_.find(h);
            if (hit != MIRKeyMap::kNotFound && hit < types.size() && equal(types[hit], rec)) {
                return hit;
            }
        }
        MIRWriteLock lock(mtx_, single_threaded());
        const uint32_t n = types.size();
        const uint32_t hit = intern_index_.find(h);
        if (hit != MIRKeyMap::kNotFound) {
            // re-checked under the writer lock: another worker may have interned
            // this record between the shared fast path above and here
            if (hit < n && equal(types[hit], rec)) return hit;
            // The hash is present but names a different record: an exact 64-bit
            // collision, or an entry left stale by a mutation. Only this case
            // needs the verified scan.
            const uint32_t found = types.find_writer([this, &rec](const MIRTypeRecord& r) {
                return equal(r, rec);
            });
            if (found != MIRStableArray<MIRTypeRecord>::kNotFound) {
                intern_index_.set(h, found);
                return found;
            }
        }
        // An *absent* hash proves no equal record exists, so the scan above is
        // skipped: `hash_of` is a pure function of exactly the fields `equal`
        // compares, and every record's current hash is registered (`intern`,
        // `push` and `update` all set it). Measured on the main suite: 9,820
        // misses performed 3,898,451 `equal` calls and found **nothing**, so the
        // scan was pure cost on every genuinely new type.
        types.push_back(rec);
        const TypeId id = static_cast<TypeId>(n);
        intern_index_.set(h, id);
        return id;
    }

    /** Drop every type, pool and the interning index (reused `MIRModule`). */
    void clear() {
        MIRWriteLock lock(mtx_, single_threaded());
        types.clear();
        data.clear();
        names.clear();
        fields.clear();
        field_names.clear();
        intern_index_.clear();
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
    MIRKeyMap intern_index_;

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

private:
    MIRTableLockPolicy lock_policy_;
    /** Guards the mutators only (action E1/E3d): readers are lock-free against
     *  the stable arrays/byte pool, so the exclusive lock is only needed to
     *  serialize appends when more than one lowering worker runs. */
    mutable std::shared_mutex mtx_;
};

} // namespace mir
