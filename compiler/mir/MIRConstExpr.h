// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level constant expressions (action A3 of mir-migration-evaluation.md):
// the target-neutral representation of a top-level global's initializer.
//
// Before this table the initializer was walked from the AST twice, once per
// backend: the C emitter (`MIRDeclEmitter::const_value_expr`) spelled C text and
// the LLVM emitter (`GlobalConstantLowerer`) built an `llvm::Constant`. Both
// read the same `Value*` tree with the same rules, so the rules now live in one
// place — the front-end `MIRLowerer` lowers a `Value*` into this AST-free tree
// once, and each backend renders the tree it already knows how to consume.
//
// AST-free on purpose (it stores only ids and bytes), so it lives in MIR core
// and is covered by the standalone MIRTests target.
//
// A designator name is a *dotted field path* (`Base.Mid.Leaf.field` for an
// inherited member, `Case.param` for a variant payload) exactly as the type
// table already spells field names — see MIRTypeTable. The C printer emits the
// path verbatim (`.__chx__vt_621827 = 1, .Case.param = 3`); the LLVM printer
// resolves it positionally against the type record. Array elements carry no
// name; their position is the index.

#pragma once

#include "MIRTypes.h"
#include "MIRBytePool.h"

#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace mir {

enum class MIRConstKind : uint8_t {
    Zero = 0,     // zero / absent initializer; C spells `0`
    Int,          // `bits` holds the value; signedness from the type record
    Float,        // `bits` holds the 32-bit float bit pattern
    Double,       // `bits` holds the 64-bit double bit pattern
    Bool,         // `bits` == 0/1
    Null,         // C spells `NULL`
    String,       // `data`/`data_count` hold the bytes (no NUL)
    Negate,       // unary minus of child 0 (C spells `-<child>`)
    Aggregate,    // struct/union/array: children (named for a struct/union,
    //               positional for an array)
    Variant,      // children carry full designator paths; `case_index` is the
    //               discriminator written to the variant tag
    FunctionRef,  // `symbol` holds the function symbol (C spells its mangled name)
};

/** One child of an aggregate/variant constant. `name` is empty for arrays. */
struct MIRConstChild {
    ConstExprId expr = MIR_INVALID_ID;
    uint32_t name_offset = 0;
    uint32_t name_length = 0;
};

/** Node flags (spelling hints that layout does not carry). */
enum MIRConstFlags : uint8_t {
    MCF_NONE = 0x00,
    // a default-constructed aggregate keeps its `(Type)` cast prefix, matching
    // `MIRDeclEmitter::struct_default_expr` (`(Base_t) { .field = ... }`)
    MCF_CAST_PREFIX = 0x01,
    // an integer literal spelled without a sign (the source's own unsignedness,
    // so the C printer does not depend on the mapped type's signedness)
    MCF_UNSIGNED = 0x02,
};

/** A node of the initializer tree. Kept small; children live in the pool. */
struct MIRConstExpr {
    MIRConstKind kind = MIRConstKind::Zero;
    uint8_t flags = MCF_NONE;
    TypeId type = MIR_INVALID_ID;
    uint64_t bits = 0;         // Int/Float/Double/Bool
    uint32_t data_offset = 0;  // String payload index into `data`
    uint32_t data_count = 0;
    uint32_t child_offset = 0; // Aggregate/Variant/Negate, index into `children`
    uint32_t child_count = 0;
    uint32_t case_index = 0;   // Variant active case (discriminator value)
    uint32_t symbol = MIR_INVALID_ID; // FunctionRef
};

/**
 * Append-only pool of constant expressions. Like the type/constant pools this
 * is module-level and cold, so a plain `std::vector` is fine; ids are dense and
 * stable.
 */
struct MIRConstExprTable {
    /** `exprs` is a vector (ids, not references, escape it — see MIRTypeTable's
     * storage note); `children` is a `std::deque` because `child` hands out an
     * element pointer and it is cold; `data`/`names` are `MIRBytePool`
     * (reference-stable on append, see MIRBytePool.h). */
    std::vector<MIRConstExpr> exprs;
    std::deque<MIRConstChild> children;
    MIRBytePool data;
    MIRBytePool names;

    ConstExprId add(const MIRConstExpr& e) {
        exprs.push_back(e);
        return static_cast<ConstExprId>(exprs.size() - 1);
    }
    const MIRConstExpr& get(ConstExprId id) const { return exprs[id]; }
    uint32_t size() const { return static_cast<uint32_t>(exprs.size()); }

    const char* name_data(const MIRConstChild& c) const { return names.data_at(c.name_offset); }

    /** The child's designator without an allocation (see `child_name`). */
    std::string_view child_name_view(const MIRConstChild& c) const {
        if (c.name_length == 0) return {};
        return std::string_view(name_data(c), c.name_length);
    }
    std::string child_name(const MIRConstChild& c) const {
        return c.name_length ? std::string(name_data(c), c.name_length) : std::string();
    }
    uint32_t intern_name(const char* s, uint32_t n) {
        return names.append(s, n);
    }
    const MIRConstChild* child(ConstExprId id, uint32_t i) const {
        const MIRConstExpr& e = exprs[id];
        if (i >= e.child_count) return nullptr;
        return &children[e.child_offset + i];
    }

    ConstExprId add_zero(TypeId t) {
        MIRConstExpr e;
        e.kind = MIRConstKind::Zero;
        e.type = t;
        return add(e);
    }
    ConstExprId add_int(TypeId t, uint64_t bits) {
        MIRConstExpr e;
        e.kind = MIRConstKind::Int;
        e.type = t;
        e.bits = bits;
        return add(e);
    }
    ConstExprId add_bool(TypeId t, bool v) {
        MIRConstExpr e;
        e.kind = MIRConstKind::Bool;
        e.type = t;
        e.bits = v ? 1u : 0u;
        return add(e);
    }
    ConstExprId add_float(TypeId t, float v) {
        MIRConstExpr e;
        e.kind = MIRConstKind::Float;
        e.type = t;
        uint32_t b = 0;
        std::memcpy(&b, &v, sizeof(b));
        e.bits = b;
        return add(e);
    }
    ConstExprId add_double(TypeId t, double v) {
        MIRConstExpr e;
        e.kind = MIRConstKind::Double;
        e.type = t;
        uint64_t b = 0;
        std::memcpy(&b, &v, sizeof(b));
        e.bits = b;
        return add(e);
    }
    ConstExprId add_null(TypeId t) {
        MIRConstExpr e;
        e.kind = MIRConstKind::Null;
        e.type = t;
        return add(e);
    }
    ConstExprId add_string(TypeId t, const char* s, uint32_t n) {
        MIRConstExpr e;
        e.kind = MIRConstKind::String;
        e.type = t;
        e.data_offset = data.append(s, n);
        e.data_count = n;
        return add(e);
    }
    ConstExprId add_symbol(TypeId t, SymbolId sym) {
        MIRConstExpr e;
        e.kind = MIRConstKind::FunctionRef;
        e.type = t;
        e.symbol = sym;
        return add(e);
    }
    ConstExprId add_negate(TypeId t, ConstExprId inner) {
        MIRConstChild c;
        c.expr = inner;
        MIRConstExpr e;
        e.kind = MIRConstKind::Negate;
        e.type = t;
        e.child_offset = static_cast<uint32_t>(children.size());
        e.child_count = 1;
        children.push_back(c);
        return add(e);
    }
    ConstExprId add_with_children(MIRConstKind kind, TypeId t,
                                  const std::vector<MIRConstChild>& kids,
                                  uint32_t case_index = 0, uint8_t flags = MCF_NONE) {
        MIRConstExpr e;
        e.kind = kind;
        e.type = t;
        e.flags = flags;
        e.child_offset = static_cast<uint32_t>(children.size());
        e.child_count = static_cast<uint32_t>(kids.size());
        e.case_index = case_index;
        children.insert(children.end(), kids.begin(), kids.end());
        return add(e);
    }

    void clear() {
        exprs.clear();
        children.clear();
        data.clear();
        names.clear();
    }
};

} // namespace mir
