// Copyright (c) Chemical Language Foundation 2025.
//
// Standalone unit tests for the MIR core (PR 1): arena, instruction encoding,
// module tables, textual dump and structural verifier. Built as the `MIRTests`
// target; independent of the compiler pipeline. See
// mir-implementation-plan.md §9 Stage 1.

#include "compiler/mir/MIR.h"
#include "compiler/mir/MIRBuilder.h"
#include "compiler/mir/MIRDeclPrinter.h"
#include "compiler/mir/MIREmitter.h"
#include "compiler/mir/MIRFacts.h"
#include "compiler/mir/MIRInterpreter.h"
#include "compiler/mir/MIRTraversal.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace mir;

static int g_checks = 0;
#define CHECK(x)                                                              \
    do {                                                                      \
        ++g_checks;                                                           \
        if (!(x)) {                                                           \
            std::cerr << "FAILED: " << #x << " (" << __FILE__ << ":"          \
                      << __LINE__ << ")\n";                                   \
            return false;                                                     \
        }                                                                     \
    } while (0)

// ── arena ──────────────────────────────────────────────────────────────────
static bool test_arena() {
    MIRArena arena(1024);
    void* a = arena.allocate(16);
    void* b = arena.allocate(64, 16);
    CHECK(a != nullptr);
    CHECK(b != nullptr);
    CHECK(reinterpret_cast<uintptr_t>(b) % 16 == 0);
    CHECK(arena.bytes_used() >= 80);

    // many small allocations reuse the inline chunk
    for (int i = 0; i < 1000; ++i) {
        CHECK(arena.allocate(8) != nullptr);
    }
    size_t used = arena.bytes_used();
    arena.reset();
    CHECK(arena.bytes_used() == 0);
    CHECK(arena.capacity() > 0);
    (void) used;
    return true;
}

// ── instruction encoding ───────────────────────────────────────────────────
static bool test_instruction_encoding() {
    MIRInstruction inst = MIRInstruction::make(MIROpcode::Binary);
    CHECK(inst.opcode() == MIROpcode::Binary);
    CHECK((inst.flags() & OF_Pure) != 0);
    CHECK((inst.flags() & OF_Terminator) == 0);
    CHECK(!is_terminator(MIROpcode::Binary));
    CHECK(is_terminator(MIROpcode::Return));
    CHECK(opcode_flags(MIROpcode::Call) & OF_Call);
    CHECK(opcode_portability(MIROpcode::SizeOf) == MIRPortability::TargetLayoutDependent);

    MIROperand op = MIROperand::value(7, 3);
    CHECK(op.kind() == MIROperandKind::Value);
    CHECK(op.id == 7);
    CHECK(op.type() == 3);
    static_assert(sizeof(MIRInstruction) == 16, "MIRInstruction must stay 16 bytes");
    return true;
}

// ── build a tiny function by hand and verify + dump it ─────────────────────
static bool test_function_build_dump() {
    MIRModule module;

    MIRTypeRecord ir;
    ir.kind = MIRTypeKind::Int;
    ir.flags = TF_SIGNED;
    ir.size = 4;
    ir.alignment = 4;
    const TypeId i32 = module.types.intern(ir);
    CHECK(module.types.intern(ir) == i32); // canonical interning

    MIRArena arena;
    MIRFunction fn;
    fn.symbol = 0;
    fn.function_type = i32;

    // %v0 = const.int c0
    MIRValueDef vd;
    vd.type = i32;
    vd.def_inst = 0;
    vd.flags = VF_CONSTANT | VF_PURE;
    fn.values.push_back(vd);

    const ConstantId c0 = module.constants.add_int(i32, 42);
    const uint32_t op_off = static_cast<uint32_t>(fn.operands.size());
    CHECK(fn.operands.push(arena, MIROperand::constant(c0, i32)));

    MIRInstruction ci = MIRInstruction::make(MIROpcode::ConstInt);
    ci.result_or_place = 0;
    ci.operand_offset = op_off;
    ci.operand_count = 1;
    CHECK(fn.instructions.push(arena, ci));

    // return %v0
    const uint32_t ret_op = static_cast<uint32_t>(fn.operands.size());
    CHECK(fn.operands.push(arena, MIROperand::value(0, i32)));
    MIRInstruction ret = MIRInstruction::make(MIROpcode::Return);
    ret.operand_offset = ret_op;
    ret.operand_count = 1;
    CHECK(fn.instructions.push(arena, ret));

    MIRBlock blk;
    blk.id = 0;
    blk.inst_start = 0;
    blk.inst_count = 1; // the const; terminator follows at index 1
    CHECK(fn.blocks.push(arena, blk));
    fn.entry_block = 0;

    module.functions.push_back(fn);

    MIRVerifyResult res = verify_function(module.functions[0]);
    if (!res.ok()) {
        for (const auto& d : res.diagnostics) std::cerr << "  diag: " << d.message << "\n";
    }
    CHECK(res.ok());

    std::string dump = dump_function_str(module.functions[0], module);
    CHECK(dump.find("const.int") != std::string::npos);
    CHECK(dump.find("return") != std::string::npos);

    // an intentionally malformed function must be rejected
    MIRFunction bad = module.functions[0];
    bad.blocks[0].inst_count = 99;
    CHECK(!verify_function(bad).ok());
    return true;
}

// ── builder: control flow + storage + calls ────────────────────────────────
static bool test_builder() {
    MIRModule module;
    MIRTypeRecord ir;
    ir.kind = MIRTypeKind::Int;
    ir.flags = TF_SIGNED;
    ir.size = 4;
    ir.alignment = 4;
    const TypeId i32 = module.types.intern(ir);

    MIRTypeRecord br;
    br.kind = MIRTypeKind::Bool;
    br.size = 1;
    br.alignment = 1;
    const TypeId bool_t = module.types.intern(br);

    MIRArena arena;
    MIRFunction fn;
    fn.function_type = i32;
    MIRBuilder b(arena, module, fn);

    const BlockId entry = b.create_block();
    const BlockId then_b = b.create_block();
    const BlockId else_b = b.create_block();
    const BlockId join = b.create_block();
    fn.entry_block = entry;

    b.set_block(entry);
    const ValueId ten = b.const_int(i32, module.constants.add_int(i32, 10));
    const ValueId zero = b.const_int(i32, module.constants.add_int(i32, 0));
    const ValueId cond = b.compare(ten, zero, module.constants.add_int(i32, 1), bool_t);
    const PlaceId p = b.alloca(i32, MIRStorageClass::Local);
    b.cond_br(cond, then_b, else_b);

    b.set_block(then_b);
    b.store(p, ten);
    b.br(join);

    b.set_block(else_b);
    b.store(p, zero);
    b.br(join);

    b.set_block(join);
    const ValueId r = b.load(p, i32);
    b.ret(r);

    if (!b.ok()) std::cerr << "builder error: " << (b.error() ? b.error() : "?") << "\n";
    CHECK(b.ok());
    CHECK(fn.blocks.size() == 4);
    CHECK(fn.instructions.size() > 0);
    // ten is used by compare + store
    CHECK(fn.values[ten].use_count == 2);
    CHECK(fn.blocks[entry].inst_count >= 3);

    MIRVerifyResult res = verify_function(fn);
    if (!res.ok()) {
        for (const auto& d : res.diagnostics) std::cerr << "  diag: " << d.message << "\n";
    }
    CHECK(res.ok());

    std::string dump = dump_function_str(fn, module);
    CHECK(dump.find("cond_br") != std::string::npos);
    CHECK(dump.find("alloca") != std::string::npos);
    CHECK(dump.find("return") != std::string::npos);
    return true;
}

// ── emitter: straight-line function -> C ───────────────────────────────────
static bool test_emitter() {
    MIRModule module;
    MIRTypeRecord ir;
    ir.kind = MIRTypeKind::Int;
    ir.flags = TF_SIGNED;
    ir.size = 4;
    ir.alignment = 4;
    const TypeId i32 = module.types.intern(ir);

    TypeId params[2] = {i32, i32};
    MIRTypeRecord fr;
    fr.kind = MIRTypeKind::Function;
    fr.element = i32;
    fr.data_offset = module.types.append_data(params, 2);
    fr.data_count = 2;
    fr.size = 8;
    fr.alignment = 8;
    const TypeId ftype = module.types.intern(fr);

    MIRSymbolRecord srec;
    srec.kind = MIRSymbolKind::Function;
    srec.linkage = MIRLinkage::Internal;
    srec.type = ftype;
    const SymbolId sym = module.symbols.add(srec, "add", 3, "add", 3);

    MIRArena arena;
    MIRFunction fn;
    fn.symbol = sym;
    fn.function_type = ftype;
    MIRBuilder b(arena, module, fn);
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;

    const ValueId pa = b.param(i32);
    const PlaceId ppa = b.alloca(i32, MIRStorageClass::Parameter);
    b.store(ppa, pa);
    const ValueId pb = b.param(i32);
    const PlaceId ppb = b.alloca(i32, MIRStorageClass::Parameter);
    b.store(ppb, pb);

    const ValueId la = b.load(ppa, i32);
    const ValueId lb = b.load(ppb, i32);
    const ConstantId addc = module.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Add));
    const ValueId sum = b.binary(la, lb, addc, i32);
    b.ret(sum);

    CHECK(b.ok());
    MIRVerifyResult vr = verify_function(fn);
    for (const auto& d : vr.diagnostics) std::cerr << "  emitter diag: " << d.message << "\n";
    CHECK(vr.ok());

    std::string out, err;
    CHECK(emit_function_c(fn, module, out, err));
    CHECK(out.find("add") != std::string::npos);
    CHECK(out.find("+") != std::string::npos);
    CHECK(out.find("return") != std::string::npos);
    CHECK(out.find("({") == std::string::npos); // no GNU statement expressions
    return true;
}

// ── emitter: control flow (labels + gotos) ─────────────────────────────────
static bool test_control_flow_emitter() {
    MIRModule module;
    MIRTypeRecord ir;
    ir.kind = MIRTypeKind::Int;
    ir.flags = TF_SIGNED;
    ir.size = 4;
    ir.alignment = 4;
    const TypeId i32 = module.types.intern(ir);
    MIRTypeRecord br;
    br.kind = MIRTypeKind::Bool;
    br.size = 1;
    br.alignment = 1;
    const TypeId bool_t = module.types.intern(br);

    MIRTypeRecord fr;
    fr.kind = MIRTypeKind::Function;
    fr.element = i32;
    fr.size = 8;
    fr.alignment = 8;
    const TypeId ftype = module.types.intern(fr);

    MIRSymbolRecord srec;
    srec.kind = MIRSymbolKind::Function;
    srec.linkage = MIRLinkage::Internal;
    srec.type = ftype;
    const SymbolId sym = module.symbols.add(srec, "pick", 4, "pick", 4);

    MIRArena arena;
    MIRFunction fn;
    fn.symbol = sym;
    fn.function_type = ftype;
    MIRBuilder b(arena, module, fn);
    const BlockId entry = b.create_block();
    const BlockId then_b = b.create_block();
    const BlockId else_b = b.create_block();
    const BlockId join = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;
    const ValueId ten = b.const_int(i32, module.constants.add_int(i32, 10));
    const ValueId zero = b.const_int(i32, module.constants.add_int(i32, 0));
    const ValueId cond = b.compare(ten, zero, module.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq)), bool_t);
    const PlaceId p = b.alloca(i32, MIRStorageClass::Local);
    b.cond_br(cond, then_b, else_b);
    b.set_block(then_b);
    b.store(p, ten);
    b.br(join);
    b.set_block(else_b);
    b.store(p, zero);
    b.br(join);
    b.set_block(join);
    b.ret(b.load(p, i32));

    CHECK(b.ok());
    std::string out, err;
    if (!emit_function_c(fn, module, out, err)) {
        std::cerr << "  cf emit error: " << err << "\n";
        return false;
    }
    CHECK(out.find("goto __chx_bb") != std::string::npos);
    CHECK(out.find("if (") != std::string::npos);
    // `then_b` (1) begins at the instruction right after the terminator, so the
    // true arm *falls through*: the emitter jumps only on the false arm, on the
    // negated condition, and emits no `else goto`.
    CHECK(out.find("if (!(") != std::string::npos);
    CHECK(out.find("else goto") == std::string::npos);
    // Every block a terminator really jumps to carries a label (`else_b` 2,
    // `join` 3) ...
    CHECK(out.find("__chx_bb2:;") != std::string::npos);
    CHECK(out.find("__chx_bb3:;") != std::string::npos);
    // ... and a block nothing jumps to carries none: a dead label is not emitted
    // (`emit_function_c`). That is the entry block, and now also `then_b`, whose
    // only reference was the fall-through arm above.
    CHECK(out.find("__chx_bb0:;") == std::string::npos);
    CHECK(out.find("__chx_bb1:;") == std::string::npos);
    // `join` is reached by exactly one jump: `then_b`'s `br` is not a
    // fall-through (the block after it is `else_b`), while `else_b`'s `br` jumps
    // to the block that immediately follows, so it emits no instruction at all.
    const std::size_t first_join = out.find("goto __chx_bb3;");
    CHECK(first_join != std::string::npos);
    CHECK(out.find("goto __chx_bb3;", first_join + 1) == std::string::npos);
    return true;
}

// ── emitter: named aggregate types ─────────────────────────────────────────
static bool test_type_names() {
    MIRModule module;
    MIRTypeRecord sr;
    sr.kind = MIRTypeKind::Struct;
    sr.size = 8;
    sr.alignment = 4;
    const TypeId s = module.types.intern(sr);
    module.types.set_name(s, "main_Point", 10);
    CHECK(c_type_of(module, s) == "struct main_Point");

    MIRTypeRecord pr;
    pr.kind = MIRTypeKind::Pointer;
    pr.flags = TF_MUTABLE;
    pr.size = 8;
    pr.alignment = 8;
    pr.element = s;
    const TypeId ptr = module.types.intern(pr);
    CHECK(c_type_of(module, ptr) == "struct main_Point*");

    MIRTypeRecord cpr;
    cpr.kind = MIRTypeKind::Pointer;
    cpr.size = 8;
    cpr.alignment = 8;
    cpr.element = s;
    const TypeId cptr = module.types.intern(cpr);
    CHECK(c_type_of(module, cptr) == "const struct main_Point*");
    return true;
}

// ── layout: MIR-computed aggregate layout vs. what a C compiler computes ────
//
// C mirrors of the aggregates built below. `CHECK_AGG`/`CHECK_FIELD` compare
// the MIR-computed size/alignment/offsets against the C compiler's
// `sizeof`/`offsetof`, so the layout pass is pinned to the real ABI it must
// agree with: the emitter writes the fields out and lets the C compiler lay
// them out, so MIR's numbers have to match. MIRTests is built by whatever C++
// compiler is configured, so the mirrors avoid anything with a non-obvious
// spelling (long double, empty aggregates, bitfields).
#if defined(_MSC_VER)
#define MIR_TEST_ALIGNED(n) __declspec(align(n))
#else
#define MIR_TEST_ALIGNED(n) __attribute__((aligned(n)))
#endif

/** The C compiler's alignment of `T`, derived from `offsetof` (portable across
 *  MSVC, whose C mode has no `_Alignof`). */
template <typename T> struct MirrorAlignProbe { char c; T t; };
template <typename T> static uint32_t mir_c_align() {
    return static_cast<uint32_t>(offsetof(MirrorAlignProbe<T>, t));
}

struct MirrorCharInt { char c; int i; };
struct MirrorIntChar { int i; char c; };
struct MirrorCharDoubleInt { char c; double d; int i; };
struct MirrorPtrChar { void* p; char c; };
struct MirrorIntArr3Char { int a[3]; char c; };
struct MirrorInner { int i; char c; };
struct MirrorOuter { char c; struct MirrorInner in; char d; };
union MirrorUnion { int i; char c; double d; };
struct MirrorVariantCaseA { char c; };
struct MirrorVariantCaseB { double d; int i; };
struct MirrorVariant {
    int __chx__vt_621827;
    union {
        struct MirrorVariantCaseA A;
        struct MirrorVariantCaseB B;
    };
};
struct MIR_TEST_ALIGNED(16) MirrorOverAligned { int i; };

#define CHECK_AGG(module, id, ctype)                                           \
    do {                                                                       \
        CHECK((module).types.get(id).size == sizeof(ctype));                    \
        CHECK((module).types.get(id).alignment == mir_c_align<ctype>());        \
    } while (0)
#define CHECK_FIELD(module, id, fname, expected)                               \
    CHECK(mir_field_offset_named(module, (id), (fname)) ==                     \
          static_cast<uint32_t>(expected))

/** `offset` of the field of `agg` named `name`, or MIR_INVALID_ID. */
static uint32_t mir_field_offset_named(const MIRModule& m, TypeId agg,
                                       const char* name) {
    const MIRTypeRecord& r = m.types.get(agg);
    for (uint32_t i = 0; i < r.data_count; ++i) {
        const MIRFieldInfo* f = m.types.field_of(agg, i);
        if (f && m.types.field_name_of(*f) == name) return f->offset;
    }
    return MIR_INVALID_ID;
}

struct FieldSpec {
    const char* name;
    TypeId type;
    uint32_t align = 0;
    MIRFieldKind kind = MIRFieldKind::Direct;
    uint8_t flags = MFF_NONE;
};

static TypeId mir_layout_int(MIRModule& m, uint32_t size, MIRIntKind ik) {
    MIRTypeRecord r;
    r.kind = MIRTypeKind::Int;
    r.flags = TF_SIGNED;
    r.size = size;
    r.alignment = size;
    r.int_kind = ik;
    return m.types.intern(r);
}

static TypeId mir_layout_float(MIRModule& m, uint32_t size) {
    MIRTypeRecord r;
    r.kind = MIRTypeKind::Float;
    r.size = size;
    r.alignment = size;
    return m.types.intern(r);
}

static TypeId mir_layout_ptr(MIRModule& m, TypeId pointee) {
    MIRTypeRecord r;
    r.kind = MIRTypeKind::Pointer;
    r.flags = TF_MUTABLE;
    r.size = 8;
    r.alignment = 8;
    r.element = pointee;
    return m.types.intern(r);
}

static TypeId mir_layout_array(MIRModule& m, TypeId elem, uint32_t count) {
    MIRTypeRecord r;
    r.kind = MIRTypeKind::Array;
    r.element = elem;
    r.data_count = count;
    r.size = m.types.get(elem).size * count;
    r.alignment = m.types.get(elem).alignment;
    return m.types.intern(r);
}

/** Build an aggregate record with fields, then lay it out (as the type
 *  builder does) and return its id. */
static TypeId mir_layout_agg(MIRModule& m, MIRTypeKind kind,
                             const std::vector<FieldSpec>& fields,
                             uint32_t explicit_align = 0) {
    MIRTypeRecord r;
    r.kind = kind;
    r.explicit_alignment = explicit_align;
    r.data_count = static_cast<uint32_t>(fields.size());
    std::vector<MIRFieldInfo> fs;
    for (const FieldSpec& f : fields) {
        MIRFieldInfo fi;
        fi.type = f.type;
        fi.kind = static_cast<uint8_t>(f.kind);
        fi.flags = f.flags;
        fi.align = f.align;
        fi.index = static_cast<uint32_t>(fs.size());
        fi.name_length = static_cast<uint32_t>(std::string(f.name).size());
        fi.name_offset = m.types.append_field_name(f.name, fi.name_length);
        fs.push_back(fi);
    }
    r.field_index = fs.empty() ? MIR_INVALID_ID : m.types.append_fields(fs);
    const TypeId id = static_cast<TypeId>(m.types.types.size());
    r.decl = id + 0x1000u; // distinct identity per aggregate
    m.types.types.push_back(r);
    compute_aggregate_layout(m.types, id, explicit_align);
    return id;
}

static bool test_aggregate_layout() {
    MIRModule m;
    const TypeId ch = mir_layout_int(m, 1, IK_CHAR);
    const TypeId i32 = mir_layout_int(m, 4, IK_INT);
    const TypeId i64 = mir_layout_int(m, 8, IK_I64);
    const TypeId dbl = mir_layout_float(m, 8);
    const TypeId ptr = mir_layout_ptr(m, ch);

    // {char, int}: the int needs 3 bytes of padding after the char
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct, {{"c", ch}, {"i", i32}});
        CHECK_AGG(m, t, MirrorCharInt);
        CHECK_FIELD(m, t, "c", offsetof(MirrorCharInt, c));
        CHECK_FIELD(m, t, "i", offsetof(MirrorCharInt, i));
    }
    // {int, char}: trailing padding to the alignment
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct, {{"i", i32}, {"c", ch}});
        CHECK_AGG(m, t, MirrorIntChar);
        CHECK_FIELD(m, t, "i", offsetof(MirrorIntChar, i));
        CHECK_FIELD(m, t, "c", offsetof(MirrorIntChar, c));
    }
    // {char, double, int}: 8-byte alignment, interior and trailing padding
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct,
                                        {{"c", ch}, {"d", dbl}, {"i", i32}});
        CHECK_AGG(m, t, MirrorCharDoubleInt);
        CHECK_FIELD(m, t, "c", offsetof(MirrorCharDoubleInt, c));
        CHECK_FIELD(m, t, "d", offsetof(MirrorCharDoubleInt, d));
        CHECK_FIELD(m, t, "i", offsetof(MirrorCharDoubleInt, i));
    }
    // {void*, char}
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct, {{"p", ptr}, {"c", ch}});
        CHECK_AGG(m, t, MirrorPtrChar);
        CHECK_FIELD(m, t, "p", offsetof(MirrorPtrChar, p));
        CHECK_FIELD(m, t, "c", offsetof(MirrorPtrChar, c));
    }
    // {int[3], char}: the array's stride and alignment come from its element
    {
        const TypeId arr = mir_layout_array(m, i32, 3);
        CHECK(m.types.get(arr).size == sizeof(((struct MirrorIntArr3Char*)0)->a));
        CHECK(m.types.get(arr).alignment == mir_c_align<int>());
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct, {{"a", arr}, {"c", ch}});
        CHECK_AGG(m, t, MirrorIntArr3Char);
        CHECK_FIELD(m, t, "a", offsetof(MirrorIntArr3Char, a));
        CHECK_FIELD(m, t, "c", offsetof(MirrorIntArr3Char, c));
    }
    // nested aggregate by value
    {
        const TypeId inner = mir_layout_agg(m, MIRTypeKind::Struct, {{"i", i32}, {"c", ch}});
        CHECK_AGG(m, inner, MirrorInner);
        const TypeId outer = mir_layout_agg(m, MIRTypeKind::Struct,
                                            {{"c", ch}, {"in", inner}, {"d", ch}});
        CHECK_AGG(m, outer, MirrorOuter);
        CHECK_FIELD(m, outer, "c", offsetof(MirrorOuter, c));
        CHECK_FIELD(m, outer, "in", offsetof(MirrorOuter, in));
        CHECK_FIELD(m, outer, "d", offsetof(MirrorOuter, d));
        // a field that does not exist
        CHECK(mir_field_offset_named(m, outer, "nope") == MIR_INVALID_ID);
    }
    // union: every field at offset 0, sized by the largest member
    {
        const TypeId u = mir_layout_agg(m, MIRTypeKind::Union,
                                        {{"i", i32}, {"c", ch}, {"d", dbl}});
        CHECK_AGG(m, u, MirrorUnion);
        CHECK_FIELD(m, u, "i", offsetof(MirrorUnion, i));
        CHECK_FIELD(m, u, "c", offsetof(MirrorUnion, c));
        CHECK_FIELD(m, u, "d", offsetof(MirrorUnion, d));
    }
    // variant: bases, then the `int` tag, then a union of the case payloads
    {
        const TypeId case_a = mir_layout_agg(m, MIRTypeKind::Struct,
                                             {{{"c", ch}}});
        const TypeId case_b = mir_layout_agg(m, MIRTypeKind::Struct,
                                             {{"d", dbl}, {"i", i32}});
        CHECK_AGG(m, case_a, MirrorVariantCaseA);
        CHECK_AGG(m, case_b, MirrorVariantCaseB);
        const TypeId v = mir_layout_agg(m, MIRTypeKind::Variant,
                                        {{"__chx__vt_621827", i32, 0, MIRFieldKind::VariantTag},
                                         {"A", case_a, 0, MIRFieldKind::VariantCase},
                                         {"B", case_b, 0, MIRFieldKind::VariantCase}});
        CHECK_AGG(m, v, MirrorVariant);
        CHECK_FIELD(m, v, "__chx__vt_621827", offsetof(MirrorVariant, __chx__vt_621827));
        CHECK_FIELD(m, v, "A", offsetof(MirrorVariant, A));
        CHECK_FIELD(m, v, "B", offsetof(MirrorVariant, B));
    }
    // a variant with an inherited base: the base is laid out first
    {
        const TypeId base = mir_layout_agg(m, MIRTypeKind::Struct, {{"i", i32}});
        const TypeId case_a = mir_layout_agg(m, MIRTypeKind::Struct, {{"c", ch}});
        const TypeId v = mir_layout_agg(m, MIRTypeKind::Variant,
                                        {{"Base", base, 0, MIRFieldKind::Base},
                                         {"__chx__vt_621827", i32, 0, MIRFieldKind::VariantTag},
                                         {"A", case_a, 0, MIRFieldKind::VariantCase}});
        CHECK_FIELD(m, v, "Base", 0u);
        CHECK_FIELD(m, v, "__chx__vt_621827", 4u);
        CHECK_FIELD(m, v, "A", 8u);
        CHECK(m.types.get(v).size == 12u);
        CHECK(m.types.get(v).alignment == 4u);
    }
    // an explicit over-alignment (`__chx_align_struct(N)`) raises the alignment
    // and pads the size
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct, {{"i", i32}}, 16);
        CHECK_AGG(m, t, MirrorOverAligned);
        CHECK_FIELD(m, t, "i", offsetof(MirrorOverAligned, i));
    }
    // an explicit alignment on a member raises only that member's alignment
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct,
                                        {{"a", i64}, {"b", ch, 32}});
        CHECK_FIELD(m, t, "a", 0u);
        CHECK_FIELD(m, t, "b", 32u);
        CHECK(m.types.get(t).size == 64u);
        CHECK(m.types.get(t).alignment == 32u);
    }
    // an empty aggregate stays empty rather than gaining a phantom byte
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct, {});
        CHECK(m.types.get(t).size == 0u);
        CHECK(m.types.get(t).alignment == 1u);
    }
    return true;
}

// ── aggregate access plan (core service for backends that build the type) ──
// `compute_aggregate_plan` turns an aggregate's field/offset layout into the
// ordered element list a backend indexes with GEP — one element per padding run
// and one shared element per union/variant overlap group. It is a pure function
// of the type table, so it is pinned here; the LLVM backend (the only consumer
// today) reads it instead of re-deriving one, and a future JVM/JS backend gets
// the same sequence.
static bool test_aggregate_plan() {
    MIRModule m;
    const TypeId ch = mir_layout_int(m, 1, IK_CHAR);
    const TypeId i32 = mir_layout_int(m, 4, IK_INT);
    const TypeId i64 = mir_layout_int(m, 8, IK_I64);
    const TypeId dbl = mir_layout_float(m, 8);

    // a non-aggregate has no plan
    {
        MIRAggregatePlan p;
        CHECK(!compute_aggregate_plan(m.types, i32, p));
        CHECK(p.elements.empty());
    }
    // {char, int}: a padding element precedes the int
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct, {{"c", ch}, {"i", i32}});
        MIRAggregatePlan p;
        CHECK(compute_aggregate_plan(m.types, t, p));
        CHECK(p.size == m.types.get(t).size);
        CHECK(p.alignment == m.types.get(t).alignment);
        CHECK(p.elements.size() == 3u);
        CHECK(p.elements[0].first_field == 0u && p.elements[0].field_count == 1u &&
              p.elements[0].rep_type == ch);
        CHECK(p.elements[1].first_field == MIR_INVALID_ID && p.elements[1].pad_bytes == 3u);
        CHECK(p.elements[2].first_field == 1u && p.elements[2].field_count == 1u &&
              p.elements[2].rep_type == i32);
        CHECK(p.field_elem.size() == 2u);
        CHECK(p.field_elem[0] == 0u);
        CHECK(p.field_elem[1] == 2u);
    }
    // {int, char}: trailing padding, no interior padding
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct, {{"i", i32}, {"c", ch}});
        MIRAggregatePlan p;
        CHECK(compute_aggregate_plan(m.types, t, p));
        CHECK(p.elements.size() == 3u);
        CHECK(p.elements[0].rep_type == i32);
        CHECK(p.elements[1].rep_type == ch);
        CHECK(p.elements[2].first_field == MIR_INVALID_ID && p.elements[2].pad_bytes == 3u);
        CHECK(p.field_elem[0] == 0u && p.field_elem[1] == 1u);
    }
    // union: one shared element, represented by the largest aligned member
    {
        const TypeId u = mir_layout_agg(m, MIRTypeKind::Union,
                                        {{"i", i32}, {"c", ch}, {"d", dbl}});
        MIRAggregatePlan p;
        CHECK(compute_aggregate_plan(m.types, u, p));
        CHECK(p.elements.size() == 1u);
        CHECK(p.elements[0].first_field == 0u);
        CHECK(p.elements[0].field_count == 3u);
        CHECK(p.elements[0].rep_type == dbl);
        CHECK(p.field_elem.size() == 3u);
        CHECK(p.field_elem[0] == 0u && p.field_elem[1] == 0u && p.field_elem[2] == 0u);
    }
    // variant: tag, padding, then one shared element for the case payloads
    {
        const TypeId case_a = mir_layout_agg(m, MIRTypeKind::Struct, {{"c", ch}});
        const TypeId case_b = mir_layout_agg(m, MIRTypeKind::Struct, {{"d", dbl}, {"i", i32}});
        const TypeId v = mir_layout_agg(m, MIRTypeKind::Variant,
                                        {{"__chx__vt_621827", i32, 0, MIRFieldKind::VariantTag},
                                         {"A", case_a, 0, MIRFieldKind::VariantCase},
                                         {"B", case_b, 0, MIRFieldKind::VariantCase}});
        MIRAggregatePlan p;
        CHECK(compute_aggregate_plan(m.types, v, p));
        CHECK(p.elements.size() == 3u);
        CHECK(p.elements[0].rep_type == i32);               // tag
        CHECK(p.elements[1].first_field == MIR_INVALID_ID); // padding to the overlap
        CHECK(p.elements[1].pad_bytes == 4u);
        CHECK(p.elements[2].first_field == 1u);             // A and B share it
        CHECK(p.elements[2].field_count == 2u);
        CHECK(p.elements[2].rep_type == case_b);            // largest aligned case
        CHECK(p.field_elem[0] == 0u && p.field_elem[1] == 2u && p.field_elem[2] == 2u);
    }
    // over-alignment pads the tail
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct, {{"i", i32}}, 16);
        MIRAggregatePlan p;
        CHECK(compute_aggregate_plan(m.types, t, p));
        CHECK(p.elements.size() == 2u);
        CHECK(p.elements[0].rep_type == i32);
        CHECK(p.elements[1].pad_bytes == 12u);
    }
    // an explicitly aligned member creates interior padding
    {
        const TypeId t = mir_layout_agg(m, MIRTypeKind::Struct, {{"a", i64}, {"b", ch, 32}});
        MIRAggregatePlan p;
        CHECK(compute_aggregate_plan(m.types, t, p));
        CHECK(p.elements.size() == 4u);
        CHECK(p.elements[0].rep_type == i64);
        CHECK(p.elements[1].pad_bytes == 24u);
        CHECK(p.elements[2].rep_type == ch);
        CHECK(p.elements[3].pad_bytes == 31u);
        CHECK(p.field_elem[0] == 0u && p.field_elem[1] == 2u);
    }
    return true;
}

// ── per-function facts (action B) ──────────────────────────────────────────
// `compute_function_facts` is the analysis the C emitter used to run inline;
// these pin the answers both the emitter and any future backend read.
static bool test_function_facts() {
    MIRModule module;
    MIRTypeRecord ir;
    ir.kind = MIRTypeKind::Int;
    ir.flags = TF_SIGNED;
    ir.size = 4;
    ir.alignment = 4;
    const TypeId i32 = module.types.intern(ir);
    MIRTypeRecord br;
    br.kind = MIRTypeKind::Bool;
    br.size = 1;
    br.alignment = 1;
    const TypeId bool_t = module.types.intern(br);

    // a) a scalar parameter written once by its own spill is its argument
    {
        TypeId params[2] = {i32, i32};
        MIRTypeRecord fr;
        fr.kind = MIRTypeKind::Function;
        fr.element = i32;
        fr.data_offset = module.types.append_data(params, 2);
        fr.data_count = 2;
        fr.size = 8;
        fr.alignment = 8;
        const TypeId ftype = module.types.intern(fr);

        MIRArena arena;
        MIRFunction fn;
        fn.function_type = ftype;
        MIRBuilder b(arena, module, fn);
        const BlockId entry = b.create_block();
        b.set_block(entry);
        fn.entry_block = entry;

        const ValueId pa = b.param(i32);
        const PlaceId ppa = b.alloca(i32, MIRStorageClass::Parameter);
        b.store(ppa, pa);
        const ValueId pb = b.param(i32);
        const PlaceId ppb = b.alloca(i32, MIRStorageClass::Parameter);
        b.store(ppb, pb);
        const ValueId la = b.load(ppa, i32);
        const ValueId lb = b.load(ppb, i32);
        const ValueId sum = b.binary(
            la, lb, module.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Add)), i32);
        b.ret(sum);
        CHECK(b.ok());

        MIRFunctionFacts f;
        compute_function_facts(module, fn, f);
        // both parameter places are aliased to their argument, in order
        CHECK(f.param_alias.size() == fn.places.size());
        CHECK(f.param_alias[ppa] == 0u);
        CHECK(f.param_alias[ppb] == 1u);
        CHECK(f.place_writes[ppa] == 1u);
        CHECK(f.place_writes[ppb] == 1u);
        CHECK(f.place_addressed[ppa] == 0u);
        // every `Param` instruction carries its argument index
        uint32_t params_seen = 0;
        for (uint32_t ii = 0; ii < fn.instructions.size(); ++ii) {
            if (fn.instructions[ii].opcode() != MIROpcode::Param) continue;
            CHECK(f.param_index_at_inst[ii] == params_seen);
            ++params_seen;
        }
        CHECK(params_seen == 2u);
        // one block, nothing jumps to it, nothing falls through
        CHECK(f.block_start[entry] == 0u);
        CHECK(f.block_starts_inst[0] == 1u);
        CHECK(f.block_referenced[entry] == 0u);
        CHECK(f.label_at_inst[0] == MIR_INVALID_ID);
    }

    // b) a two-arm branch: the fall-through arm is not a reference and gets no
    //    label; the other arm does
    {
        MIRTypeRecord fr;
        fr.kind = MIRTypeKind::Function;
        fr.element = i32;
        fr.size = 8;
        fr.alignment = 8;
        const TypeId ftype = module.types.intern(fr);

        MIRArena arena;
        MIRFunction fn;
        fn.function_type = ftype;
        MIRBuilder b(arena, module, fn);
        const BlockId entry = b.create_block();
        const BlockId then_b = b.create_block();
        const BlockId else_b = b.create_block();
        const BlockId join = b.create_block();
        b.set_block(entry);
        fn.entry_block = entry;
        const ValueId ten = b.const_int(i32, module.constants.add_int(i32, 10));
        const ValueId zero = b.const_int(i32, module.constants.add_int(i32, 0));
        const ValueId cond = b.compare(
            ten, zero, module.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Eq)),
            bool_t);
        const PlaceId p = b.alloca(i32, MIRStorageClass::Local);
        b.cond_br(cond, then_b, else_b);
        b.set_block(then_b);
        b.store(p, ten);
        b.br(join);
        b.set_block(else_b);
        b.store(p, zero);
        b.br(join);
        b.set_block(join);
        b.ret(b.load(p, i32));
        CHECK(b.ok());

        MIRFunctionFacts f;
        compute_function_facts(module, fn, f);
        CHECK(f.block_start[entry] == 0u);
        // the arm that starts right after the terminator falls through: no
        // reference, hence no label
        const uint32_t then_start = f.block_start[then_b];
        CHECK(f.block_starts_inst[then_start] == 1u);
        CHECK(f.is_fallthrough_inst(fn, then_start - 1, then_b));
        CHECK(f.block_referenced[then_b] == 0u);
        CHECK(f.label_at_inst[then_start] == MIR_INVALID_ID);
        // the other arm and the join are jumped to and labelled
        const uint32_t else_start = f.block_start[else_b];
        const uint32_t join_start = f.block_start[join];
        CHECK(f.block_referenced[else_b] == 1u);
        CHECK(f.block_referenced[join] == 1u);
        CHECK(f.label_at_inst[else_start] == else_b);
        CHECK(f.label_at_inst[join_start] == join);
        // `then_b`'s branch to `join` is a real jump; `else_b`'s is a fall-through
        CHECK(!f.is_fallthrough_inst(fn, else_start - 1, join));
        CHECK(f.is_fallthrough_inst(fn, join_start - 1, join));
        // the local is written twice: never a parameter alias
        CHECK(f.place_writes[p] == 2u);
        CHECK(f.param_alias[p] == MIR_INVALID_ID);
    }
    return true;
}

// ── per-value facts (action B) ──────────────────────────────────────────────
// `MIRValueFacts` publishes the single-use / pure classification the C emitter's
// inline compaction and any other backend read, instead of each re-deriving it
// from `MIRValueDef::use_count` and the opcode flags.
static bool test_value_facts() {
    MIRModule module;
    MIRTypeRecord ir;
    ir.kind = MIRTypeKind::Int;
    ir.flags = TF_SIGNED;
    ir.size = 4;
    ir.alignment = 4;
    const TypeId i32 = module.types.intern(ir);

    TypeId params[1] = {i32};
    MIRTypeRecord fr;
    fr.kind = MIRTypeKind::Function;
    fr.element = i32;
    fr.data_offset = module.types.append_data(params, 1);
    fr.data_count = 1;
    fr.size = 8;
    fr.alignment = 8;
    const TypeId ftype = module.types.intern(fr);

    MIRArena arena;
    MIRFunction fn;
    fn.function_type = ftype;
    MIRBuilder b(arena, module, fn);
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;

    const ValueId pa = b.param(i32);
    const PlaceId ppa = b.alloca(i32, MIRStorageClass::Parameter);
    b.store(ppa, pa);
    const ValueId la = b.load(ppa, i32);
    // `la` is read twice, so it is not a single-use candidate
    const ValueId sum = b.binary(
        la, la, module.constants.add_int(MIR_INVALID_ID, static_cast<uint64_t>(MIRBinaryOp::Add)), i32);
    b.ret(sum);
    CHECK(b.ok());

    MIRValueFacts vf;
    compute_value_facts(module, fn, vf);
    CHECK(vf.def_opcode.size() == fn.values.size());
    CHECK(vf.def_opcode[pa] == static_cast<uint16_t>(MIROpcode::Param));
    CHECK(vf.def_opcode[la] == static_cast<uint16_t>(MIROpcode::Load));
    CHECK(vf.def_opcode[sum] == static_cast<uint16_t>(MIROpcode::Binary));
    // a `Param` is used once and is pure
    CHECK(vf.single_use[pa] == 1u);
    CHECK(vf.single_use_pure[pa] == 1u);
    // a twice-read load is neither
    CHECK(vf.single_use[la] == 0u);
    CHECK(vf.single_use_pure[la] == 0u);
    // a single-use `Binary` is pure; a single-use `Load` would be single_use but
    // not single_use_pure (covered below)
    CHECK(vf.single_use[sum] == 1u);
    CHECK(vf.single_use_pure[sum] == 1u);

    // a value with no defining instruction has the sentinel opcode
    MIRValueFacts empty;
    compute_value_facts(module, fn, empty);
    CHECK(empty.def_opcode.size() == vf.def_opcode.size());
    return true;
}

// ── traversal / operand-decode / naming (action B) ──────────────────────────
static bool test_traversal_helpers() {
    MIRModule module;
    MIRTypeRecord ir;
    ir.kind = MIRTypeKind::Int;
    ir.flags = TF_SIGNED;
    ir.size = 4;
    ir.alignment = 4;
    const TypeId i32 = module.types.intern(ir);

    // symbol naming: the mangled spelling wins, the plain name is the fallback
    MIRSymbolRecord rec;
    rec.kind = MIRSymbolKind::Function;
    rec.type = MIR_INVALID_ID;
    const SymbolId sym = module.symbols.add(rec, "plain", 5, "mangled", 7);
    CHECK(symbol_name_view(module, sym) == "mangled");
    const SymbolId sym2 = module.symbols.add(rec, "plain2", 6, "", 0);
    CHECK(symbol_name_view(module, sym2) == "plain2");
    CHECK(symbol_name_view(module, MIR_INVALID_ID).empty());

    // constant payload: only String constants carry text
    const ConstantId cid = module.constants.add_string(i32, "hello", 5);
    CHECK(constant_string_view(module, cid) == "hello");
    const ConstantId cid_int = module.constants.add_int(i32, 7);
    CHECK(constant_string_view(module, cid_int).empty());

    TypeId params[1] = {i32};
    MIRTypeRecord fr;
    fr.kind = MIRTypeKind::Function;
    fr.element = i32;
    fr.data_offset = module.types.append_data(params, 1);
    fr.data_count = 1;
    fr.size = 8;
    fr.alignment = 8;
    const TypeId ftype = module.types.intern(fr);

    MIRArena arena;
    MIRFunction fn;
    fn.function_type = ftype;
    MIRBuilder b(arena, module, fn);
    const BlockId entry = b.create_block();
    b.set_block(entry);
    fn.entry_block = entry;
    const PlaceId p = b.alloca(i32, MIRStorageClass::Local);
    const ValueId n = b.const_int(i32, module.constants.add_int(i32, 1));
    b.store(p, n);
    b.ret(b.load(p, i32));
    CHECK(b.ok());

    // block iteration covers both non-terminators and the terminator
    CHECK(fn.blocks.size() == 1u);
    const MIRBlockRange r = block_range(fn, fn.blocks[0]);
    CHECK(r.begin == 0u);
    CHECK(r.end == fn.instructions.size());

    uint32_t blocks_seen = 0;
    uint32_t insts_seen = 0;
    bool block_index_ok = true;
    for_each_block(fn, [&](uint32_t bi, const MIRBlock& blk) -> void {
        if (bi != 0u) block_index_ok = false;
        ++blocks_seen;
        for_each_block_inst(fn, blk,
                            [&](uint32_t, const MIRInstruction&) -> void { ++insts_seen; });
    });
    CHECK(block_index_ok);
    CHECK(blocks_seen == 1u);
    CHECK(insts_seen == fn.instructions.size());

    // operand decode through the one accessor
    const MIRInstruction& last = fn.instructions[fn.instructions.size() - 1];
    CHECK(operand_at(fn, last, 0) != nullptr);
    CHECK(operand_at(fn, last, last.operand_count) == nullptr);

    // a block with no valid start owns an empty range
    MIRBlock none;
    none.inst_start = MIR_INVALID_ID;
    const MIRBlockRange er = block_range(fn, none);
    CHECK(er.begin == er.end);
    return true;
}

// ── declaration printer (action A3) ────────────────────────────────────────
// The C text of a declaration is MIR's business (`MIRDeclPrinter`); these pin
// the exact spelling the AST-walking declaration emitter used to write, since
// the emitted C is compared against it byte for byte.
static bool test_decl_printer() {
    MIRModule m;
    const TypeId ch = mir_layout_int(m, 1, IK_CHAR);
    const TypeId i32 = mir_layout_int(m, 4, IK_INT);
    const TypeId dbl = mir_layout_float(m, 8);

    // struct
    {
        const TypeId s = mir_layout_agg(m, MIRTypeKind::Struct, {{"c", ch}, {"i", i32}});
        m.types.set_name(s, "S", 1);
        CHECK(aggregate_body_c(m, s) == "\nstruct S {\n\tchar c;\n\tint i;\n};");
        CHECK(inline_aggregate_c(m, s) == "struct S { char c; int i; };");
    }
    // struct with an explicit `__chx_align_struct(N)`
    {
        const TypeId s = mir_layout_agg(m, MIRTypeKind::Struct, {{"i", i32}}, 16);
        m.types.set_name(s, "Aligned", 7);
        CHECK(aggregate_body_c(m, s) ==
              "\n__chx_align_struct(16) Aligned {\n\tint i;\n};");
    }
    // union
    {
        const TypeId u = mir_layout_agg(m, MIRTypeKind::Union, {{"i", i32}, {"c", ch}});
        m.types.set_name(u, "U", 1);
        CHECK(aggregate_body_c(m, u) == "\nunion U {\n\tint i;\n\tchar c;\n};");
    }
    // variant: inherited base, tag, then an inline union of the case payloads
    {
        const TypeId base = mir_layout_agg(m, MIRTypeKind::Struct, {{"i", i32}});
        m.types.set_name(base, "Base", 4);
        const TypeId case_a =
            mir_layout_agg(m, MIRTypeKind::Struct,
                           {{"x", i32, 0, MIRFieldKind::VariantParam}});
        const TypeId case_b =
            mir_layout_agg(m, MIRTypeKind::Struct,
                           {{"c", ch, 0, MIRFieldKind::VariantParam},
                            {"d", dbl, 0, MIRFieldKind::VariantParam}});
        const TypeId v = mir_layout_agg(
            m, MIRTypeKind::Variant,
            {{"Base", base, 0, MIRFieldKind::Base},
             {"__chx__vt_621827", i32, 0, MIRFieldKind::VariantTag},
             {"A", case_a, 0, MIRFieldKind::VariantCase},
             {"B", case_b, 0, MIRFieldKind::VariantCase}});
        m.types.set_name(v, "V", 1);
        const std::string expected =
            "\nstruct V {\n\tstruct Base Base;\n\tint __chx__vt_621827;\n\tunion {"
            // two tabs on a case parameter: the walk this replaces wrote
            // `append("\n\t\t\t", 3)`, so the count cut the third tab off
            "\n\t\tstruct {\n\t\tint x;\n\t\t} A;"
            "\n\t\tstruct {\n\t\tchar c;\n\t\tdouble d;\n\t\t} B;"
            "\n\t};\n};";
        CHECK(aggregate_body_c(m, v) == expected);
        // a variant has no one-line form
        CHECK(inline_aggregate_c(m, v).empty());
    }
    // member spelling: alignment, volatile, capturing-function, anonymous form
    {
        const TypeId s = mir_layout_agg(
            m, MIRTypeKind::Struct,
            {{"a", ch, 8, MIRFieldKind::Direct},
             {"v", i32, 0, MIRFieldKind::Direct, MFF_VOLATILE},
             {"cb", i32, 0, MIRFieldKind::Direct, MFF_FAT_POINTER}});
        m.types.set_name(s, "M", 1);
        CHECK(aggregate_body_c(m, s) ==
              "\nstruct M {\n\t__chx_align(8) char a;\n\tvolatile int v;\n"
              "\t__chemical_fat_pointer__* cb;\n};");
    }
    // an anonymous member keeps its inline body, nested one level deeper
    {
        const TypeId inner = mir_layout_agg(m, MIRTypeKind::Struct, {{"i", i32}});
        const TypeId s = mir_layout_agg(
            m, MIRTypeKind::Struct,
            {{"u", inner, 0, MIRFieldKind::Direct, MFF_ANONYMOUS_UNION}});
        m.types.set_name(s, "Anon", 4);
        CHECK(aggregate_body_c(m, s) ==
              "\nstruct Anon {\n\tunion {\n\t\tint i;\n\t} u;\n};");
    }
    // unnamed / unsupported ids are reported as nothing rather than misprinted
    {
        const TypeId void_t = m.types.intern(MIRTypeRecord{});
        CHECK(aggregate_body_c(m, void_t).empty());
        CHECK(aggregate_body_c(m, MIR_INVALID_ID).empty());
        CHECK(inline_aggregate_c(m, MIR_INVALID_ID).empty());
    }
    return true;
}

// ── constant-expression printer (action A3) ─────────────────────────────────
// A global initializer is now a target-neutral MIR constant-expression tree
// (`MIRConstExpr.h`) that both backends render. These pin the C spelling
// `const_expr_c` produces, which replaces the AST-walking
// `MIRDeclEmitter::const_value_expr`.
static bool test_const_expr_printer() {
    MIRModule m;
    MIRConstExprTable& ce = m.const_exprs;
    const TypeId i32 = mir_layout_int(m, 4, IK_INT);
    const TypeId ch = mir_layout_int(m, 1, IK_CHAR);
    const TypeId ptr = mir_layout_ptr(m, ch);
    const TypeId arr = mir_layout_array(m, i32, 3);
    const TypeId s = mir_layout_agg(m, MIRTypeKind::Struct, {{"a", i32}, {"b", i32}});
    m.types.set_name(s, "S", 1);

    auto mk_child = [&](const char* n, ConstExprId e) {
        MIRConstChild c;
        c.expr = e;
        const std::string name(n);
        c.name_offset = ce.intern_name(name.data(), static_cast<uint32_t>(name.size()));
        c.name_length = static_cast<uint32_t>(name.size());
        return c;
    };

    // scalars
    CHECK(const_expr_c(m, ce.add_int(i32, 42)) == "42");
    CHECK(const_expr_c(m, ce.add_negate(i32, ce.add_int(i32, 5))) == "-5");
    CHECK(const_expr_c(m, ce.add_float(mir_layout_float(m, 4), 2.0f)) == "2.0f");
    CHECK(const_expr_c(m, ce.add_double(mir_layout_float(m, 8), 2.0)) == "2.0");
    CHECK(const_expr_c(m, ce.add_bool(i32, true)) == "1");
    CHECK(const_expr_c(m, ce.add_null(ptr)) == "NULL");
    CHECK(const_expr_c(m, ce.add_string(ptr, "hi", 2)) == "\"hi\"");
    CHECK(const_expr_c(m, MIR_INVALID_ID) == "0");
    CHECK(const_expr_c(m, ce.add_zero(i32)) == "0");
    // an unsigned literal prints without a sign
    {
        MIRTypeRecord ur;
        ur.kind = MIRTypeKind::Int;
        ur.size = 4;
        ur.alignment = 4;
        ur.int_kind = IK_UINT;
        const TypeId u32 = m.types.intern(ur);
        CHECK(const_expr_c(m, ce.add_int(u32, 4000000000u)) == "4000000000");
    }

    // array: positional elements, `{0}` when empty
    {
        std::vector<MIRConstChild> elems;
        MIRConstChild c0;
        c0.expr = ce.add_int(i32, 10);
        elems.push_back(c0);
        MIRConstChild c1;
        c1.expr = ce.add_int(i32, 20);
        elems.push_back(c1);
        CHECK(const_expr_c(m, ce.add_with_children(MIRConstKind::Aggregate, arr, elems)) ==
              "{10, 20}");
        CHECK(const_expr_c(m, ce.add_with_children(MIRConstKind::Aggregate, arr, {})) == "{0}");
    }

    // struct: designated initializers; a default construction keeps its cast
    {
        std::vector<MIRConstChild> kids;
        kids.push_back(mk_child("a", ce.add_int(i32, 1)));
        kids.push_back(mk_child("b", ce.add_int(i32, 2)));
        CHECK(const_expr_c(m, ce.add_with_children(MIRConstKind::Aggregate, s, kids)) ==
              "{.a = 1, .b = 2}");
        CHECK(const_expr_c(m, ce.add_with_children(MIRConstKind::Aggregate, s, kids, 0,
                                                   MCF_CAST_PREFIX)) ==
              "(struct S) {.a = 1, .b = 2}");
    }

    // variant: the tag plus the active case's payload (a dotted designator)
    {
        const TypeId case_a = mir_layout_agg(
            m, MIRTypeKind::Struct, {{"x", i32, 0, MIRFieldKind::VariantParam}});
        const TypeId v = mir_layout_agg(
            m, MIRTypeKind::Variant,
            {{"__chx__vt_621827", i32, 0, MIRFieldKind::VariantTag},
             {"A", case_a, 0, MIRFieldKind::VariantCase}});
        m.types.set_name(v, "V", 1);
        std::vector<MIRConstChild> kids;
        kids.push_back(mk_child("A.x", ce.add_int(i32, 7)));
        CHECK(const_expr_c(m, ce.add_with_children(MIRConstKind::Variant, v, kids, 0)) ==
              "(struct V) { .__chx__vt_621827 = 0, .A.x = 7 }");
    }

    // function reference: its mangled symbol name
    {
        MIRSymbolRecord rec;
        rec.kind = MIRSymbolKind::Function;
        const std::string fn = "main_f";
        const SymbolId sym = m.symbols.add(rec, fn.data(), static_cast<uint32_t>(fn.size()),
                                           fn.data(), static_cast<uint32_t>(fn.size()));
        CHECK(const_expr_c(m, ce.add_symbol(ptr, sym)) == "main_f");
    }
    return true;
}

// ── declaration-list printer (action A3) ───────────────────────────────────
// The C declaration pass now records an ordered `MIRDeclList` and this AST-free
// printer renders it; these pin the item spellings.
static bool test_decl_list_printer() {
    MIRModule m;
    const TypeId i32 = mir_layout_int(m, 4, IK_INT);
    const TypeId ch = mir_layout_int(m, 1, IK_CHAR);
    const TypeId s = mir_layout_agg(m, MIRTypeKind::Struct, {{"i", i32}});
    m.types.set_name(s, "S", 1);

    // an `int f(void)` function type for the prototype / vtable-slot items
    MIRTypeRecord fr;
    fr.kind = MIRTypeKind::Function;
    fr.element = i32;
    const TypeId ft = m.types.intern(fr);

    MIRDeclList list;
    auto push = [&](MIRDeclItem it) { list.items.push_back(std::move(it)); };

    MIRDeclItem fwd;
    fwd.kind = MIRDeclKind::ForwardStruct;
    fwd.name = "S";
    push(fwd);
    MIRDeclItem fwu;
    fwu.kind = MIRDeclKind::ForwardUnion;
    fwu.name = "U";
    push(fwu);
    MIRDeclItem ab;
    ab.kind = MIRDeclKind::AggregateBody;
    ab.type = s;
    ab.what = "struct";
    push(ab);
    MIRDeclItem td;
    td.kind = MIRDeclKind::Typedef;
    td.name = "Alias";
    td.type = i32;
    push(td);
    MIRDeclItem tda;
    tda.kind = MIRDeclKind::Typedef;
    tda.name = "Buf";
    tda.unsized_array = true;
    tda.elem = ch;
    push(tda);
    MIRDeclItem proto;
    proto.kind = MIRDeclKind::Prototype;
    proto.proto.name = "f";
    proto.proto.type = ft;
    proto.proto.flags = MPF_EXTERN;
    push(proto);
    MIRDeclItem g;
    g.kind = MIRDeclKind::Global;
    g.global.name = "g";
    g.global.type = i32;
    g.global.flags = MGD_STATIC;
    push(g);
    MIRDeclItem vt;
    vt.kind = MIRDeclKind::VtableType;
    vt.name = "__chx_I_vt_t";
    {
        MIRVtableSlot sl;
        sl.name = "go";
        sl.type = ft;
        vt.slots.push_back(sl);
    }
    push(vt);
    MIRDeclItem ve;
    ve.kind = MIRDeclKind::VtableExtern;
    ve.type_name = "__chx_I_vt_t";
    ve.name = "I_impl";
    push(ve);

    const std::string out = decl_list_c(m, list);
    CHECK(out.find("\nstruct S;") != std::string::npos);
    CHECK(out.find("\nunion U;") != std::string::npos);
    CHECK(out.find("\nstruct S {\n\tint i;\n};") != std::string::npos);
    CHECK(out.find("\ntypedef int Alias;") != std::string::npos);
    CHECK(out.find("\ntypedef char Buf[];") != std::string::npos);
    CHECK(out.find("\nextern int f(void);") != std::string::npos);
    CHECK(out.find("\nstatic int g;") != std::string::npos);
    CHECK(out.find("\ntypedef struct __chx_I_vt_t {") != std::string::npos);
    CHECK(out.find("\n} __chx_I_vt_t;") != std::string::npos);
    CHECK(out.find("\nextern const __chx_I_vt_t I_impl;") != std::string::npos);
    return true;
}

// ── vtable-definition printer (action A3) ──────────────────────────────────
// The C definition emitter no longer resolves vtable callees by hand: the shared
// `mir::build_vtable_def_record` resolves them and this AST-free printer renders
// the record. These pin the two spellings — a container impl casts each slot to
// its self-erased function type, a primitive impl does not and writes `0` for a
// slot with no overrider (an unresolved container slot falls back to
// `__chx_unresolved`).
static bool test_vtable_def_printer() {
    MIRModule m;
    const TypeId i32 = mir_layout_int(m, 4, IK_INT);
    MIRTypeRecord fr;
    fr.kind = MIRTypeKind::Function;
    fr.element = i32;
    const TypeId ft = m.types.intern(fr);

    MIRVtableRecord container;
    container.kind = 1;
    container.type_name = "__chx_I_vt_t";
    container.name = "I_C";
    {
        MIRVtableSlot s;
        s.name = "go";
        s.type = ft;
        s.target = "I_C_go";
        container.slots.push_back(std::move(s));
    }
    {
        MIRVtableSlot s;
        s.name = "gone";
        s.type = ft;
        container.slots.push_back(std::move(s));
    }
    const std::string c = vtable_def_c(m, container);
    CHECK(c.find("\nconst __chx_I_vt_t I_C = {\n(") != std::string::npos);
    CHECK(c.find(") I_C_go,\n") != std::string::npos);
    CHECK(c.find(") __chx_unresolved,\n") != std::string::npos);
    CHECK(c.rfind("\n};") == c.size() - 3);

    MIRVtableRecord prim;
    prim.kind = 2;
    prim.type_name = "__chx_I_vt_t";
    prim.name = "I_int";
    {
        MIRVtableSlot s;
        s.name = "go";
        s.type = ft;
        s.target = "I_int_go";
        prim.slots.push_back(std::move(s));
    }
    {
        MIRVtableSlot s;
        s.name = "gone";
        s.type = ft;
        prim.slots.push_back(std::move(s));
    }
    const std::string p = vtable_def_c(m, prim);
    CHECK(p.find("\nconst __chx_I_vt_t I_int = {\nI_int_go,\n0,\n};") != std::string::npos);
    CHECK(p.find('(') == std::string::npos); // a primitive impl adds no cast
    return true;
}

// ── E1: concurrent interning into the shared module tables ─────────────────
// Several lowering workers intern into one `MIRModule`; the type/constant pools
// must hand back the same id for the same content from any thread, and the
// symbol table must never lose or tear a record. See mir-migration-evaluation.md
// §2E (stage E1).
static bool test_table_thread_safety() {
    constexpr int kThreads = 8;
    constexpr int kPer = 200;

    // Types: content-based interning must be order-independent under contention.
    MIRTypeTable types;
    std::vector<TypeId> expect_type(kPer, MIR_INVALID_ID);
    for (int i = 0; i < kPer; ++i) {
        MIRTypeRecord r;
        r.kind = MIRTypeKind::Int;
        r.size = static_cast<uint32_t>(i + 1);
        r.alignment = 1;
        expect_type[i] = types.intern(r);
    }
    std::vector<std::thread> workers;
    std::vector<char> type_ok(kThreads, 0);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&types, &expect_type, &type_ok, t]() {
            bool ok = true;
            for (int i = 0; i < kPer; ++i) {
                const int j = (i * 7 + t) % kPer; // same set, different order
                MIRTypeRecord r;
                r.kind = MIRTypeKind::Int;
                r.size = static_cast<uint32_t>(j + 1);
                r.alignment = 1;
                if (types.intern(r) != expect_type[j]) ok = false;
            }
            type_ok[t] = ok ? 1 : 0;
        });
    }
    for (auto& w : workers) w.join();
    CHECK(types.size() == static_cast<uint32_t>(kPer));
    for (int t = 0; t < kThreads; ++t) CHECK(type_ok[t]);

    // Constants: `add_int` with equal content -> the same id from any thread.
    MIRTypeRecord i32r;
    i32r.kind = MIRTypeKind::Int;
    i32r.size = 4;
    i32r.alignment = 1; // matches expect_type[3], so interning adds no record
    const TypeId i32 = types.intern(i32r);
    CHECK(i32 == expect_type[3]);

    MIRConstantPool pool;
    std::vector<ConstantId> expect_const(kPer, MIR_INVALID_ID);
    for (int i = 0; i < kPer; ++i) {
        expect_const[i] = pool.add_int(i32, static_cast<uint64_t>(i));
    }
    workers.clear();
    std::vector<char> const_ok(kThreads, 0);
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&pool, &expect_const, &const_ok, i32, t]() {
            bool ok = true;
            for (int i = 0; i < kPer; ++i) {
                const int j = (i * 13 + t) % kPer;
                if (pool.add_int(i32, static_cast<uint64_t>(j)) != expect_const[j]) ok = false;
            }
            const_ok[t] = ok ? 1 : 0;
        });
    }
    for (auto& w : workers) w.join();
    CHECK(pool.size() == static_cast<uint32_t>(kPer));
    for (int t = 0; t < kThreads; ++t) CHECK(const_ok[t]);

    // Symbols are deliberately NOT deduplicated (backends match by mangled name),
    // so assert the guard keeps every record whole and every id readable.
    MIRSymbolTable symbols;
    std::vector<SymbolId> ids(static_cast<size_t>(kThreads) * kPer, MIR_INVALID_ID);
    workers.clear();
    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&symbols, &ids, t]() {
            for (int i = 0; i < kPer; ++i) {
                const std::string nm = "fn_" + std::to_string(t) + "_" + std::to_string(i);
                MIRSymbolRecord rec;
                rec.kind = MIRSymbolKind::Function;
                ids[static_cast<size_t>(t) * kPer + i] =
                    symbols.add(rec, nm.data(), static_cast<uint32_t>(nm.size()),
                                nm.data(), static_cast<uint32_t>(nm.size()));
            }
        });
    }
    for (auto& w : workers) w.join();
    CHECK(symbols.size() == static_cast<uint32_t>(kThreads) * kPer);
    for (int t = 0; t < kThreads; ++t) {
        for (int i = 0; i < kPer; ++i) {
            const SymbolId id = ids[static_cast<size_t>(t) * kPer + i];
            CHECK(id < symbols.size());
            const std::string nm = "fn_" + std::to_string(t) + "_" + std::to_string(i);
            CHECK(symbols.mangled_name(symbols.get(id)) == nm);
        }
    }
    return true;
}

// ── interpreter round-trip: run a hand-built function ───────────────────────
static bool test_interpreter_roundtrip() {
    MIRModule module;

    MIRTypeRecord ir;
    ir.kind = MIRTypeKind::Int;
    ir.flags = TF_SIGNED;
    ir.size = 4;
    ir.alignment = 4;
    const TypeId i32 = module.types.intern(ir);

    MIRTypeRecord fr;
    fr.kind = MIRTypeKind::Function;
    fr.size = 8;
    fr.alignment = 8;
    fr.element = i32; // return type
    const TypeId fty = module.types.intern(fr);

    MIRSymbolRecord srec;
    srec.kind = MIRSymbolKind::Function;
    srec.linkage = MIRLinkage::External;
    srec.type = fty;
    const SymbolId sym = module.symbols.add(srec, "add", 3, "add", 3);

    MIRArena arena;
    MIRFunction fn;
    fn.symbol = sym;
    fn.function_type = fty;
    for (int i = 0; i < 3; ++i) {
        MIRValueDef vd;
        vd.type = i32;
        fn.values.push_back(vd);
    }

    auto emit_param = [&](ValueId v) -> bool {
        const uint32_t off = static_cast<uint32_t>(fn.operands.size());
        CHECK(fn.operands.push(arena, MIROperand::type(i32)));
        MIRInstruction p = MIRInstruction::make(MIROpcode::Param);
        p.result_or_place = v;
        p.operand_offset = off;
        p.operand_count = 1;
        CHECK(fn.instructions.push(arena, p));
        return true;
    };
    CHECK(emit_param(0));
    CHECK(emit_param(1));

    // %v2 = %v0 + %v1
    const ConstantId add_op =
        module.constants.add_int(i32, static_cast<uint64_t>(MIRBinaryOp::Add));
    {
        const uint32_t off = static_cast<uint32_t>(fn.operands.size());
        CHECK(fn.operands.push(arena, MIROperand::value(0, i32)));
        CHECK(fn.operands.push(arena, MIROperand::value(1, i32)));
        CHECK(fn.operands.push(arena, MIROperand::constant(add_op, MIR_INVALID_ID)));
        CHECK(fn.operands.push(arena, MIROperand::type(i32)));
        MIRInstruction b = MIRInstruction::make(MIROpcode::Binary);
        b.result_or_place = 2;
        b.operand_offset = off;
        b.operand_count = 4;
        CHECK(fn.instructions.push(arena, b));
    }

    // return %v2
    {
        const uint32_t off = static_cast<uint32_t>(fn.operands.size());
        CHECK(fn.operands.push(arena, MIROperand::value(2, i32)));
        MIRInstruction r = MIRInstruction::make(MIROpcode::Return);
        r.operand_offset = off;
        r.operand_count = 1;
        CHECK(fn.instructions.push(arena, r));
    }

    MIRBlock blk;
    blk.id = 0;
    blk.inst_start = 0;
    blk.inst_count = 3; // the two params + the binary; the return terminates
    CHECK(fn.blocks.push(arena, blk));
    fn.entry_block = 0;

    module.functions.push_back(fn);

    MIRInterpreter interp(module);
    CHECK(interp.has_function("add"));
    uint64_t args[2] = {40, 2};
    uint64_t result = 0;
    std::string err;
    CHECK(interp.call("add", args, 2, result, err));
    CHECK(result == 42);
    return true;
}

int main() {
    bool ok = true;
    ok &= test_arena();
    ok &= test_instruction_encoding();
    ok &= test_table_thread_safety();
    ok &= test_function_build_dump();
    ok &= test_builder();
    ok &= test_emitter();
    ok &= test_interpreter_roundtrip();
    ok &= test_control_flow_emitter();
    ok &= test_type_names();
    ok &= test_aggregate_layout();
    ok &= test_aggregate_plan();
    ok &= test_function_facts();
    ok &= test_value_facts();
    ok &= test_traversal_helpers();
    ok &= test_decl_printer();
    ok &= test_const_expr_printer();
    ok &= test_decl_list_printer();
    ok &= test_vtable_def_printer();
    if (!ok) {
        std::cerr << "mir_tests: FAILED\n";
        return 1;
    }
    std::cout << "mir_tests: OK (" << g_checks << " checks)\n";
    return 0;
}
