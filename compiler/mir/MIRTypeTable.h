// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level canonical type table. Built once, serially, before parallel
// lowering, then read-only. See mir-implementation-plan.md §2.6.

#pragma once

#include "MIRTypes.h"

#include <cstdint>
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
};

/**
 * A canonical type record. Portable semantic identity only; target-dependent
 * size/alignment live here as a cache but do not affect interning (interning
 * is per module/target).
 */
struct MIRTypeRecord {
    MIRTypeKind kind = MIRTypeKind::Opaque;
    uint8_t flags = TF_NONE;
    uint32_t size = 0;
    uint32_t alignment = 0;
    uint32_t data_offset = 0; // index into MIRTypeTable::data
    uint32_t data_count = 0;  // field/element type count
    TypeId element = MIR_INVALID_ID; // pointee / element / return type
    uint32_t decl = MIR_INVALID_ID;  // source declaration id (struct/variant)
};

struct MIRTypeTable {
    std::vector<MIRTypeRecord> types;
    std::vector<TypeId> data;

    const MIRTypeRecord& get(TypeId id) const { return types[id]; }
    uint32_t size() const { return static_cast<uint32_t>(types.size()); }

    /** Append a range of TypeIds to the shared data pool, returning its offset. */
    uint32_t append_data(const TypeId* ids, uint32_t count) {
        uint32_t off = static_cast<uint32_t>(data.size());
        for (uint32_t i = 0; i < count; ++i) data.push_back(ids[i]);
        return off;
    }

    /**
     * Intern a type record. Serial-time only (linear scan); the type table is
     * sealed before parallel workers start, so this is never on the hot path.
     */
    TypeId intern(const MIRTypeRecord& rec) {
        for (uint32_t i = 0; i < types.size(); ++i) {
            if (equal(types[i], rec)) return i;
        }
        types.push_back(rec);
        return static_cast<TypeId>(types.size() - 1);
    }

    bool equal(const MIRTypeRecord& a, const MIRTypeRecord& b) const {
        if (a.kind != b.kind || a.flags != b.flags || a.size != b.size ||
            a.alignment != b.alignment || a.element != b.element || a.decl != b.decl ||
            a.data_count != b.data_count) {
            return false;
        }
        for (uint32_t i = 0; i < a.data_count; ++i) {
            if (data[a.data_offset + i] != data[b.data_offset + i]) return false;
        }
        return true;
    }
};

} // namespace mir
