// Copyright (c) Chemical Language Foundation 2025.
//
// Standalone unit tests for the MIR core (PR 1): arena, instruction encoding,
// module tables, textual dump and structural verifier. Built as the `MIRTests`
// target; independent of the compiler pipeline. See
// mir-implementation-plan.md §9 Stage 1.

#include "compiler/mir/MIR.h"
#include "compiler/mir/MIRBuilder.h"
#include "compiler/mir/MIREmitter.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
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
    // Every block a terminator jumps to carries a label (`then_b` 1, `else_b` 2,
    // `join` 3) ...
    CHECK(out.find("__chx_bb1:;") != std::string::npos);
    CHECK(out.find("__chx_bb2:;") != std::string::npos);
    CHECK(out.find("__chx_bb3:;") != std::string::npos);
    // ... and a block nothing jumps to (the entry block) carries none: a dead
    // label is not emitted (`emit_function_c`).
    CHECK(out.find("__chx_bb0:;") == std::string::npos);
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
MIR_TEST_ALIGNED(16) struct MirrorOverAligned { int i; };

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
    r.data_count = static_cast<uint32_t>(fields.size());
    std::vector<MIRFieldInfo> fs;
    for (const FieldSpec& f : fields) {
        MIRFieldInfo fi;
        fi.type = f.type;
        fi.kind = static_cast<uint8_t>(f.kind);
        fi.align = f.align;
        fi.index = static_cast<uint32_t>(fs.size());
        fi.name_offset = static_cast<uint32_t>(m.types.field_names.size());
        fi.name_length = static_cast<uint32_t>(std::string(f.name).size());
        m.types.field_names.insert(m.types.field_names.end(), f.name,
                                   f.name + fi.name_length);
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

int main() {
    bool ok = true;
    ok &= test_arena();
    ok &= test_instruction_encoding();
    ok &= test_function_build_dump();
    ok &= test_builder();
    ok &= test_emitter();
    ok &= test_control_flow_emitter();
    ok &= test_type_names();
    ok &= test_aggregate_layout();
    if (!ok) {
        std::cerr << "mir_tests: FAILED\n";
        return 1;
    }
    std::cout << "mir_tests: OK (" << g_checks << " checks)\n";
    return 0;
}
