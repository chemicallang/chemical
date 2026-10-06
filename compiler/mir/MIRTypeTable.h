// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level canonical type table. Built once, serially, before parallel
// lowering, then read-only. See mir-implementation-plan.md §2.6.

#pragma once

#include "MIRTypes.h"

#include <cstdint>
#include <string>
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
 * Exact C spelling category for integer types, mirroring the legacy 2c backend
 * (see ToCAstVisitor::VisitIntNType). 0 = unspecified (spell by width/signedness
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
};

struct MIRTypeTable {
    std::vector<MIRTypeRecord> types;
    std::vector<TypeId> data;
    std::vector<char> names;

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
            a.data_count != b.data_count || a.int_kind != b.int_kind) {
            return false;
        }
        for (uint32_t i = 0; i < a.data_count; ++i) {
            if (data[a.data_offset + i] != data[b.data_offset + i]) return false;
        }
        return true;
    }
};

} // namespace mir
