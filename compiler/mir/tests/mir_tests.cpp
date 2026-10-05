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
#include <cstdint>
#include <iostream>

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
    CHECK(out.find("__chx_bb0:;") != std::string::npos);
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
    pr.size = 8;
    pr.alignment = 8;
    pr.element = s;
    const TypeId ptr = module.types.intern(pr);
    CHECK(c_type_of(module, ptr) == "struct main_Point*");
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
    if (!ok) {
        std::cerr << "mir_tests: FAILED\n";
        return 1;
    }
    std::cout << "mir_tests: OK (" << g_checks << " checks)\n";
    return 0;
}
