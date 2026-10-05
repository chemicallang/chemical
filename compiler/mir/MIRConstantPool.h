// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level interned constants (integers, floats, booleans, null, strings,
// byte blobs). A string literal is a constant plus its addressable global
// representation. See mir-design.md §5.2.

#pragma once

#include "MIRTypes.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace mir {

enum class MIRConstantKind : uint8_t {
    Int = 0,
    Float,
    Double,
    Bool,
    Null,
    String,
    Bytes,
};

struct MIRConstant {
    MIRConstantKind kind = MIRConstantKind::Int;
    TypeId type = MIR_INVALID_ID;
    uint64_t bits = 0;        // integer/float bit pattern, bool, or null
    uint32_t data_offset = 0; // index into MIRConstantPool::data
    uint32_t data_count = 0;  // payload length (string/bytes)
};

struct MIRConstantPool {
    std::vector<MIRConstant> constants;
    std::vector<char> data;

    const MIRConstant& get(ConstantId id) const { return constants[id]; }
    uint32_t size() const { return static_cast<uint32_t>(constants.size()); }

    ConstantId add(const MIRConstant& c) {
        for (uint32_t i = 0; i < constants.size(); ++i) {
            const MIRConstant& o = constants[i];
            if (o.kind != c.kind || o.bits != c.bits || o.data_count != c.data_count) continue;
            bool same = true;
            for (uint32_t j = 0; j < c.data_count; ++j) {
                if (data[o.data_offset + j] != data[c.data_offset + j]) {
                    same = false;
                    break;
                }
            }
            if (same) return i;
        }
        constants.push_back(c);
        return static_cast<ConstantId>(constants.size() - 1);
    }

    ConstantId add_int(TypeId t, uint64_t bits) {
        MIRConstant c;
        c.kind = MIRConstantKind::Int;
        c.type = t;
        c.bits = bits;
        return add(c);
    }

    ConstantId add_float(TypeId t, float v) {
        uint32_t b = 0;
        std::memcpy(&b, &v, sizeof(b));
        MIRConstant c;
        c.kind = MIRConstantKind::Float;
        c.type = t;
        c.bits = b;
        return add(c);
    }

    ConstantId add_double(TypeId t, double v) {
        uint64_t b = 0;
        std::memcpy(&b, &v, sizeof(b));
        MIRConstant c;
        c.kind = MIRConstantKind::Double;
        c.type = t;
        c.bits = b;
        return add(c);
    }

    ConstantId add_bool(TypeId t, bool v) {
        MIRConstant c;
        c.kind = MIRConstantKind::Bool;
        c.type = t;
        c.bits = v ? 1u : 0u;
        return add(c);
    }

    ConstantId add_null(TypeId t) {
        MIRConstant c;
        c.kind = MIRConstantKind::Null;
        c.type = t;
        return add(c);
    }

    ConstantId add_string(TypeId t, const char* s, uint32_t len) {
        MIRConstant c;
        c.kind = MIRConstantKind::String;
        c.type = t;
        c.data_offset = static_cast<uint32_t>(data.size());
        c.data_count = len;
        if (len) data.insert(data.end(), s, s + len);
        return add(c);
    }
};

} // namespace mir
