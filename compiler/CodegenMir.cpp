// Copyright (c) Chemical Language Foundation 2026.
//
// MIR → LLVM lowering. This is the MIR-driven replacement for the AST-based
// LLVM backend: it consumes the canonical MIR module (types + symbols +
// functions) and emits LLVM IR. No AST virtual dispatch is used.
//
// The emitter mirrors the C backend (`MIREmitter.cpp`) opcode-for-opcode, but
// produces LLVM values instead of C text. Type layout comes from the MIR type
// table (aggregate field lists filled by `MIRTypeBuilder`); LLVM's DataLayout is
// authoritative for sizes/offsets.

#include "Codegen.h"
#include "compiler/llvmimpl.h"

#include "compiler/mir/MIRModule.h"
#include "compiler/mir/MIRInstruction.h"
#include "compiler/mir/MIRFunction.h"
#include "compiler/mir/MIRDump.h"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <string>
#include <vector>

using namespace mir;

namespace {

bool mir_is_void(const MIRModule& m, TypeId t) {
    return t == MIR_INVALID_ID || t >= m.types.size() ||
           m.types.get(t).kind == MIRTypeKind::Void;
}

// An aggregate passed *by pointer* (mirrors MIRDeclEmitter::needs_aggregate /
// MIRLowerer::needs_aggregate_path: typedef structs such as the dynamic fat
// pointer are passed by value, so they are excluded here).
bool mir_is_aggregate(const MIRModule& m, TypeId t) {
    if (t == MIR_INVALID_ID || t >= m.types.size()) return false;
    const MIRTypeRecord& r = m.types.get(t);
    if (r.flags & TF_TYPEDEF) return false;
    return r.kind == MIRTypeKind::Struct || r.kind == MIRTypeKind::Union ||
           r.kind == MIRTypeKind::Variant || r.kind == MIRTypeKind::Array;
}

// A struct-like (or array) return goes through a hidden sret pointer. Must match
// MIRDeclEmitter::needs_sret and MIRLowerer::needs_sret_return *exactly* — those
// include typedef structs such as the dynamic fat pointer.
bool mir_needs_sret(const MIRModule& m, TypeId t) {
    if (t == MIR_INVALID_ID || t >= m.types.size()) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union ||
           k == MIRTypeKind::Variant || k == MIRTypeKind::Array;
}

bool mir_is_ptr_like(const MIRModule& m, TypeId t) {
    if (t == MIR_INVALID_ID || t >= m.types.size()) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    return k == MIRTypeKind::Pointer || k == MIRTypeKind::Reference;
}

// ── atomic selectors ────────────────────────────────────────────────────────
// MIR encodes the LLVM atomic order/scope/op as the same integers the
// `BackendAtomicMemoryOrder` / `BackendAtomicSyncScope` / `BackendAtomicOp`
// enums use (which the atomic library produces through `llvm_mem_order` /
// `scope_to_int` / `llvm_atomic_op_to_int`).
llvm::AtomicOrdering mir_atomic_ordering(uint64_t order) {
    switch (order) {
        case 0: return llvm::AtomicOrdering::NotAtomic;
        case 1: return llvm::AtomicOrdering::Unordered;
        case 2: return llvm::AtomicOrdering::Monotonic;
        case 4: return llvm::AtomicOrdering::Acquire;
        case 5: return llvm::AtomicOrdering::Release;
        case 6: return llvm::AtomicOrdering::AcquireRelease;
        case 7: return llvm::AtomicOrdering::SequentiallyConsistent;
        default: return llvm::AtomicOrdering::SequentiallyConsistent;
    }
}

uint8_t mir_atomic_scope(uint64_t scope) {
    return scope == 1 ? llvm::SyncScope::SingleThread : llvm::SyncScope::System;
}

llvm::AtomicRMWInst::BinOp mir_atomic_rmw_op(uint64_t op) {
    switch (op) {
        case 0: return llvm::AtomicRMWInst::Xchg;
        case 1: return llvm::AtomicRMWInst::Add;
        case 2: return llvm::AtomicRMWInst::Sub;
        case 3: return llvm::AtomicRMWInst::And;
        case 4: return llvm::AtomicRMWInst::Nand;
        case 5: return llvm::AtomicRMWInst::Or;
        case 6: return llvm::AtomicRMWInst::Xor;
        case 7: return llvm::AtomicRMWInst::Max;
        case 8: return llvm::AtomicRMWInst::Min;
        case 9: return llvm::AtomicRMWInst::UMax;
        case 10: return llvm::AtomicRMWInst::UMin;
        case 11: return llvm::AtomicRMWInst::FAdd;
        case 12: return llvm::AtomicRMWInst::FSub;
        case 13: return llvm::AtomicRMWInst::FMax;
        case 14: return llvm::AtomicRMWInst::FMin;
        case 15: return llvm::AtomicRMWInst::UIncWrap;
        case 16: return llvm::AtomicRMWInst::UDecWrap;
        default: return llvm::AtomicRMWInst::Add;
    }
}

} // namespace

// ── MIR type → LLVM type ────────────────────────────────────────────────────
llvm::Type* Codegen::mir_type(const mir::MIRModule& m, uint32_t id) {
    auto it = mir_type_cache_.find(id);
    if (it != mir_type_cache_.end()) return it->second;

    if (id == MIR_INVALID_ID || id >= m.types.size()) return builder->getInt8Ty();
    const MIRTypeRecord& r = m.types.get(id);
    llvm::Type* t = builder->getInt8Ty();
    switch (r.kind) {
        case MIRTypeKind::Void: t = builder->getVoidTy(); break;
        case MIRTypeKind::Bool: t = builder->getInt1Ty(); break;
        case MIRTypeKind::Int: t = builder->getIntNTy(r.size ? r.size * 8 : 32); break;
        case MIRTypeKind::Float:
            t = (r.size == 4) ? static_cast<llvm::Type*>(builder->getFloatTy())
                              : static_cast<llvm::Type*>(builder->getDoubleTy());
            break;
        case MIRTypeKind::Pointer:
        case MIRTypeKind::Reference:
            t = builder->getPtrTy();
            break;
        case MIRTypeKind::Array:
            t = llvm::ArrayType::get(mir_type(m, r.element), r.data_count);
            break;
        case MIRTypeKind::Struct:
        case MIRTypeKind::Union:
        case MIRTypeKind::Variant: {
            std::string name = m.types.name_of(id);
            llvm::StructType* st = name.empty()
                                       ? llvm::StructType::create(*ctx)
                                       : llvm::StructType::create(*ctx, name);
            mir_type_cache_[id] = st;
            std::vector<llvm::Type*> elems;
            elems.reserve(r.data_count);
            for (uint32_t i = 0; i < r.data_count; ++i) {
                const MIRFieldInfo* f = m.types.field_of(id, i);
                if (!f) continue;
                elems.push_back(mir_type(m, f->type));
            }
            if (elems.empty()) elems.push_back(builder->getInt8Ty());
            st->setBody(elems);
            t = st;
            break;
        }
        case MIRTypeKind::Function: {
            std::vector<llvm::Type*> params;
            for (uint32_t i = 0; i < r.data_count; ++i) {
                params.push_back(mir_type(m, m.types.data[r.data_offset + i]));
            }
            llvm::Type* ret = mir_type(m, r.element);
            llvm::FunctionType* ft =
                llvm::FunctionType::get(ret, params, (r.flags & TF_VARIADIC) != 0);
            (void)ft;
            t = llvm::PointerType::get(*ctx, 0);
            break;
        }
        case MIRTypeKind::Opaque:
        default:
            t = builder->getInt8Ty();
            break;
    }
    mir_type_cache_[id] = t;
    return t;
}

// ── Function declaration ────────────────────────────────────────────────────
llvm::FunctionType* Codegen::mir_fn_type(const mir::MIRModule& m, uint32_t ft) {
    if (ft == MIR_INVALID_ID || ft >= m.types.size() ||
        m.types.get(ft).kind != MIRTypeKind::Function) {
        return nullptr;
    }
    const MIRTypeRecord& fr = m.types.get(ft);
    const bool sret = mir_needs_sret(m, fr.element);
    std::vector<llvm::Type*> params;
    if (sret) params.push_back(builder->getPtrTy());
    for (uint32_t i = 0; i < fr.data_count; ++i) {
        params.push_back(mir_type(m, m.types.data[fr.data_offset + i]));
    }
    llvm::Type* ret = sret ? builder->getVoidTy() : mir_type(m, fr.element);
    return llvm::FunctionType::get(ret, params, (fr.flags & TF_VARIADIC) != 0);
}

llvm::StructType* Codegen::mir_vtable_struct(const std::string& name,
                                             const std::vector<llvm::FunctionType*>& slots) {
    auto it = mir_vtable_types_.find(name);
    if (it != mir_vtable_types_.end()) return it->second;
    llvm::StructType* st = llvm::StructType::create(*ctx, name);
    std::vector<llvm::Type*> body;
    for (size_t i = 0; i < slots.size(); ++i) body.push_back(llvm::PointerType::get(*ctx, 0));
    if (body.empty()) body.push_back(builder->getInt8Ty());
    st->setBody(body);
    mir_vtable_types_[name] = st;
    mir_vtable_slot_types_[name] = slots;
    return st;
}

llvm::FunctionType* Codegen::mir_vtable_slot_type(const std::string& type_name,
                                                  uint32_t index) const {
    auto it = mir_vtable_slot_types_.find(type_name);
    if (it == mir_vtable_slot_types_.end() || index >= it->second.size()) return nullptr;
    return it->second[index];
}

bool Codegen::mir_vtable_method_index(const std::string& type_name, const std::string& method,
                                      uint32_t& out) const {
    auto it = mir_vtable_methods_.find(type_name);
    if (it == mir_vtable_methods_.end()) return false;
    const auto& v = it->second;
    for (uint32_t i = 0; i < v.size(); ++i) {
        if (v[i] == method) { out = i; return true; }
    }
    return false;
}

llvm::Function* Codegen::mir_function(const mir::MIRModule& m, uint32_t symbol) {
    auto it = mir_fn_cache_.find(symbol);
    if (it != mir_fn_cache_.end()) return it->second;
    if (symbol >= m.symbols.size()) return nullptr;
    const MIRSymbolRecord& rec = m.symbols.get(symbol);
    std::string name(rec.mangled_length ? m.symbols.mangled_data(rec)
                                        : m.symbols.name_data(rec),
                     rec.mangled_length ? rec.mangled_length : rec.name_length);

    const MIRFunction* body = nullptr;
    for (const MIRFunction& f : m.functions) {
        if (f.symbol != symbol) continue;
        // Prefer a real definition: a static-interface stub is appended last and
        // must never win over an implementation of the same symbol (they mangle
        // to one name, so whichever is found first would also decide linkage).
        if (!body) body = &f;
        if ((f.flags & MF_WEAK_STUB) == 0) { body = &f; break; }
    }
    TypeId ftype = MIR_INVALID_ID;
    if (body) {
        ftype = body->function_type;
    } else if (rec.type != MIR_INVALID_ID && rec.type < m.types.size() &&
               m.types.get(rec.type).kind == MIRTypeKind::Function) {
        ftype = rec.type;
    }
    if (ftype == MIR_INVALID_ID) return nullptr;
    // Symbol ids are not name-unique (the lowerer may intern the same function
    // twice, e.g. by declaration node and by mangled name). LLVM globals are
    // name-unique, so reuse an existing declaration/definition by name.
    if (llvm::Function* existing = module->getFunction(name)) {
        mir_fn_cache_[symbol] = existing;
        return existing;
    }
    llvm::FunctionType* fty = mir_fn_type(m, ftype);
    if (!fty) return nullptr;
    // Every MIR function is external in the LLVM module, definitions included.
    //
    // The C backend emits the whole program as ONE translation unit, so its
    // `MIRLinkage::Internal` spells C `static` and a reference from another
    // module still resolves inside that single TU. The LLVM backend emits one
    // object per module, so an internal definition cannot satisfy another
    // module's reference (`test_report_TestLogmake` / `_TestLogdelete` were
    // emitted `t` in the `test_report` object and `U` in the `test` object).
    // Giving every definition external linkage is safe because mangled names are
    // unique program-wide: the single-TU C build would otherwise reject the
    // duplicate definition of the same symbol.
    //
    // The exception is a static-interface stub (`MF_WEAK_STUB`): the interface's
    // method and its impls share one mangled name, so the stub is weak and a
    // real implementation — in this or any other module's object — overrides it.
    const bool weak_stub = body && (body->flags & MF_WEAK_STUB) != 0;
    llvm::Function* fn = llvm::Function::Create(
        fty, weak_stub ? llvm::GlobalValue::WeakAnyLinkage
                       : llvm::GlobalValue::ExternalLinkage, name, module.get());
    mir_fn_cache_[symbol] = fn;
    return fn;
}

void Codegen::mir_declare_globals(const mir::MIRModule& m) {
    for (uint32_t i = 0; i < m.symbols.size(); ++i) {
        const MIRSymbolRecord& rec = m.symbols.get(i);
        if (rec.kind != MIRSymbolKind::Global && rec.kind != MIRSymbolKind::External) continue;
        if (rec.type == MIR_INVALID_ID || rec.type >= m.types.size()) continue;
        std::string name(rec.mangled_length ? m.symbols.mangled_data(rec)
                                            : m.symbols.name_data(rec),
                         rec.mangled_length ? rec.mangled_length : rec.name_length);
        if (llvm::GlobalVariable* existing = module->getGlobalVariable(name)) {
            mir_global_cache_[i] = existing;
            continue;
        }
        llvm::Type* t = mir_type(m, rec.type);
        // `MIRLinkage::External` marks an *extern* declaration: the symbol is
        // defined by someone else (e.g. libc's `stdout`, declared `@extern` in
        // cstd). It must stay uninitialized — attaching an initializer turns it
        // into a definition that interposes the real symbol, and a null-pointer
        // `stdout` in the executable makes every libc `printf` segfault. A
        // non-extern symbol is this module's own global: define it (external
        // linkage, see `mir_function` for why) with a null initializer that the
        // literal-initializer pass may overwrite.
        const bool is_extern = rec.linkage != MIRLinkage::Internal;
        llvm::GlobalVariable* gv = new llvm::GlobalVariable(
            *module, t, false, llvm::GlobalValue::ExternalLinkage,
            is_extern ? nullptr : llvm::Constant::getNullValue(t), name);
        mir_global_cache_[i] = gv;
    }
}

// ── Function body ───────────────────────────────────────────────────────────
bool Codegen::mir_emit_function(const mir::MIRModule& m, const mir::MIRFunction& fn) {
    llvm::Function* lfn = mir_function(m, fn.symbol);
    if (!lfn) return false;
    if (!lfn->empty()) return true;
    if (getenv("MIR_LLVM_TRACE"))
        std::cerr << "[MIR-LLVM] emit " << std::string(lfn->getName()) << "\n";
    if (getenv("MIR_LLVM_DUMP")) {
        std::cerr << "==== " << std::string(lfn->getName()) << " ====\n";
        mir::dump_function(fn, m, std::cerr);
    }

    const bool sret = mir_needs_sret(m, m.types.get(fn.function_type).element);

    std::vector<llvm::BasicBlock*> blocks(fn.blocks.size(), nullptr);
    for (uint32_t i = 0; i < fn.blocks.size(); ++i) {
        blocks[i] = llvm::BasicBlock::Create(*ctx, "bb" + std::to_string(i), lfn);
    }
    llvm::BasicBlock* entry = nullptr;
    if (fn.entry_block != MIR_NULL && fn.entry_block < blocks.size()) {
        entry = blocks[fn.entry_block];
    } else if (!blocks.empty()) {
        entry = blocks[0];
    } else {
        entry = llvm::BasicBlock::Create(*ctx, "entry", lfn);
        builder->SetInsertPoint(entry);
        builder->CreateRetVoid();
        return true;
    }

    std::vector<llvm::Value*> vals(fn.values.size(), nullptr);
    std::vector<llvm::Value*> places(fn.places.size(), nullptr);

    // spill every non-frame place as an alloca in the entry block
    {
        llvm::IRBuilder<> top(entry, entry->begin());
        for (uint32_t p = 0; p < fn.places.size(); ++p) {
            const uint8_t sc = fn.places[p].storage_class;
            if (sc == static_cast<uint8_t>(MIRStorageClass::FrameField) ||
                sc == static_cast<uint8_t>(MIRStorageClass::FramePtr)) {
                continue;
            }
            places[p] = top.CreateAlloca(mir_type(m, fn.places[p].type));
        }
    }
    builder->SetInsertPoint(entry);

    // `vtype` is the type the *use site* expects (the MIR value/operand type).
    // The constant pool interns by (kind, bits, payload) and ignores the type,
    // so `c.type` may name a different-but-equal-valued constant's type. The
    // value type is authoritative (the C emitter uses it too via
    // `const_literal(module, id, function.values[v].type, lit)`).
    auto const_of = [&](ConstantId cid, TypeId vtype) -> llvm::Constant* {
        if (cid >= m.constants.size())
            return llvm::Constant::getNullValue(builder->getInt32Ty());
        const MIRConstant& c = m.constants.get(cid);
        const TypeId cty = (vtype != MIR_INVALID_ID && vtype < m.types.size()) ? vtype : c.type;
        switch (c.kind) {
            case MIRConstantKind::Int: {
                llvm::Type* t = (cty < m.types.size()) ? mir_type(m, cty)
                                                       : builder->getInt32Ty();
                if (t->isPointerTy()) {
                    return llvm::ConstantExpr::getIntToPtr(
                        llvm::ConstantInt::get(builder->getInt64Ty(), c.bits), t);
                }
                return llvm::ConstantInt::get(t, c.bits, /*signed=*/true);
            }
            case MIRConstantKind::Bool:
                return llvm::ConstantInt::get(builder->getInt1Ty(), c.bits ? 1 : 0);
            case MIRConstantKind::Null:
                return llvm::ConstantPointerNull::get(builder->getPtrTy());
            case MIRConstantKind::Float: {
                uint32_t b = static_cast<uint32_t>(c.bits);
                float f;
                std::memcpy(&f, &b, sizeof(f));
                return llvm::ConstantFP::get(builder->getFloatTy(), f);
            }
            case MIRConstantKind::Double: {
                double d;
                std::memcpy(&d, &c.bits, sizeof(d));
                return llvm::ConstantFP::get(builder->getDoubleTy(), d);
            }
            case MIRConstantKind::String: {
                std::string s(m.constants.data.data() + c.data_offset, c.data_count);
                return llvm::ConstantDataArray::getString(*ctx, s, true);
            }
            default:
                return llvm::Constant::getNullValue(builder->getInt8Ty());
        }
    };

    auto const_string = [&](ConstantId cid, std::string& out) -> bool {
        if (cid >= m.constants.size()) return false;
        const MIRConstant& c = m.constants.get(cid);
        if (c.kind != MIRConstantKind::String) return false;
        out.assign(m.constants.data.data() + c.data_offset, c.data_count);
        return true;
    };

    auto field_index = [&](TypeId agg, const std::string& name) -> int {
        if (agg >= m.types.size()) return -1;
        const MIRTypeRecord& r = m.types.get(agg);
        for (uint32_t i = 0; i < r.data_count; ++i) {
            const MIRFieldInfo* f = m.types.field_of(agg, i);
            if (!f) continue;
            if (f->name_length != name.size()) continue;
            if (std::memcmp(m.types.field_names.data() + f->name_offset, name.data(),
                            name.size()) == 0) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };

    // Resolve a (possibly dotted) field path into a chain of struct field
    // indices. Variant payload fields are spelled `Case.param`.
    auto field_path = [&](TypeId agg, const std::string& name,
                          std::vector<uint32_t>& indices, TypeId& out_type) -> bool {
        const size_t dot = name.find('.');
        if (dot == std::string::npos) {
            const int fi = field_index(agg, name);
            if (fi < 0) return false;
            indices.push_back(static_cast<uint32_t>(fi));
            out_type = m.types.field_of(agg, fi)->type;
            return true;
        }
        const std::string case_name = name.substr(0, dot);
        const std::string param_name = name.substr(dot + 1);
        const int ci = field_index(agg, case_name);
        if (ci < 0) return false;
        const TypeId case_type = m.types.field_of(agg, ci)->type;
        const int pi = field_index(case_type, param_name);
        if (pi < 0) return false;
        indices.push_back(static_cast<uint32_t>(ci));
        indices.push_back(static_cast<uint32_t>(pi));
        out_type = m.types.field_of(case_type, pi)->type;
        return true;
    };
    auto gep_path = [&](llvm::Value* base_ptr, TypeId agg,
                        const std::vector<uint32_t>& indices) -> llvm::Value* {
        llvm::Value* ptr = base_ptr;
        TypeId cur = agg;
        for (uint32_t idx : indices) {
            ptr = builder->CreateStructGEP(mir_type(m, cur), ptr, idx);
            cur = m.types.field_of(cur, idx)->type;
        }
        return ptr;
    };

    // Is this MIR type a signed integer (or a non-int, where C promotes signed)?
    auto is_signed_ty = [&](TypeId t) {
        if (t == MIR_INVALID_ID || t >= m.types.size()) return true;
        const MIRTypeRecord& r = m.types.get(t);
        if (r.kind == MIRTypeKind::Bool) return false;
        if (r.kind != MIRTypeKind::Int) return true;
        return (r.flags & TF_SIGNED) != 0;
    };

    // Coerce a binary/compare operand pair to a single LLVM type, mirroring C's
    // integer promotions + usual arithmetic conversions (sub-int operands
    // become `int` first; then the wider integer/float wins; int<->pointer
    // becomes a pointer). Necessary because MIR operands keep their MIR types
    // and need not be identical. `out_unsigned` receives the signedness of the
    // common type — which decides the icmp/udiv predicate — and is only set
    // for integer operands.
    auto coerce_pair = [&](llvm::Value*& av, llvm::Value*& bv, TypeId at_id, TypeId bt_id,
                           bool allow_ptr_int, bool* out_unsigned) {
        if (out_unsigned) *out_unsigned = false;
        llvm::Type* at = av->getType();
        llvm::Type* bt = bv->getType();
        if (at->isIntegerTy() && bt->isIntegerTy()) {
            bool as = is_signed_ty(at_id);
            bool bs = is_signed_ty(bt_id);
            // C integer promotions: anything narrower than `int` becomes
            // `int` first, so `char < u8` compares 32 with 128 (as int),
            // not the raw i8 bit patterns.
            llvm::Type* i32 = builder->getInt32Ty();
            if (at->getIntegerBitWidth() < 32) {
                av = builder->CreateIntCast(av, i32, as);
                as = true;
            }
            if (bt->getIntegerBitWidth() < 32) {
                bv = builder->CreateIntCast(bv, i32, bs);
                bs = true;
            }
            at = av->getType();
            bt = bv->getType();
            const unsigned aw = at->getIntegerBitWidth();
            const unsigned bw = bt->getIntegerBitWidth();
            if (aw == bw) {
                if (out_unsigned) *out_unsigned = !(as && bs);
                return;
            }
            if (aw < bw) {
                av = as ? builder->CreateSExt(av, bt) : builder->CreateZExt(av, bt);
                if (out_unsigned) *out_unsigned = !bs;
            } else {
                bv = bs ? builder->CreateSExt(bv, at) : builder->CreateZExt(bv, at);
                if (out_unsigned) *out_unsigned = !as;
            }
            return;
        }
        if (at == bt) return;
        if (at->isPointerTy() || bt->isPointerTy()) {
            if (at->isPointerTy() && bt->isPointerTy()) {
                av = builder->CreateBitCast(av, builder->getPtrTy());
                bv = builder->CreateBitCast(bv, builder->getPtrTy());
            } else if (allow_ptr_int) {
                if (at->isPointerTy()) {
                    bv = builder->CreateIntToPtr(bv, at);
                } else {
                    av = builder->CreateIntToPtr(av, bt);
                }
            }
            return;
        }
        if (at->isFloatingPointTy() && bt->isFloatingPointTy()) {
            if (at->getPrimitiveSizeInBits() >= bt->getPrimitiveSizeInBits())
                bv = builder->CreateFPCast(bv, at);
            else
                av = builder->CreateFPCast(av, bt);
            return;
        }
        if (at->isFloatingPointTy() && bt->isIntegerTy()) {
            bv = is_signed_ty(bt_id) ? builder->CreateSIToFP(bv, at) : builder->CreateUIToFP(bv, at);
            return;
        }
        if (bt->isFloatingPointTy() && at->isIntegerTy()) {
            av = is_signed_ty(at_id) ? builder->CreateSIToFP(av, bt) : builder->CreateUIToFP(av, bt);
            return;
        }
    };

    std::function<llvm::Value*(const MIROperand&)> op_val =
        [&](const MIROperand& op) -> llvm::Value* {
        switch (op.kind()) {
            case MIROperandKind::Value: return vals[op.id];
            case MIROperandKind::Place: return places[op.id];
            case MIROperandKind::Constant: return const_of(op.id, op.type());
            case MIROperandKind::Symbol: return mir_function(m, op.id);
            default: return nullptr;
        }
    };

    // MIR records the *pointee* type for the address values produced by
    // FieldAddr/IndexAddr (`T` for a `T*` C variable declared as
    // `__typeof__(access)* v`). Every other address value (AddressOf,
    // GlobalAddr, a loaded pointer) records a real pointer type. Emitters must
    // therefore consult the defining opcode, not just the MIR type kind.
    auto is_address_value = [&](ValueId id) -> bool {
        if (id >= fn.values.size()) return false;
        const uint32_t di = fn.values[id].def_inst;
        if (di >= fn.instructions.size()) return false;
        const MIROpcode op = fn.instructions[di].opcode();
        return op == MIROpcode::FieldAddr || op == MIROpcode::IndexAddr;
    };
    auto value_pointee = [&](ValueId id) -> TypeId {
        if (id >= fn.values.size()) return MIR_INVALID_ID;
        const TypeId vt = fn.values[id].type;
        if (is_address_value(id)) return vt;
        if (vt == MIR_INVALID_ID || vt >= m.types.size()) return MIR_INVALID_ID;
        return m.types.get(vt).element;
    };
    auto value_is_pointer = [&](const MIROperand& op) -> bool {
        if (op.kind() == MIROperandKind::Value) {
            return is_address_value(op.id) || mir_is_ptr_like(m, fn.values[op.id].type);
        }
        return mir_is_ptr_like(m, op.type());
    };

    // The type an atomic memory operand points at (the atomic width).
    auto pointee_of = [&](const MIROperand& op) -> TypeId {
        TypeId t = MIR_INVALID_ID;
        if (op.kind() == MIROperandKind::Place) {
            t = fn.places[op.id].type;
        } else if (op.kind() == MIROperandKind::Value) {
            t = fn.values[op.id].type;
            if (is_address_value(op.id)) return t; // records the pointee already
        } else {
            return MIR_INVALID_ID;
        }
        if (t == MIR_INVALID_ID || t >= m.types.size()) return MIR_INVALID_ID;
        if (!mir_is_ptr_like(m, t)) return MIR_INVALID_ID;
        return m.types.get(t).element;
    };

    // Load a pointer operand: an SSA pointer value, or the pointer held in a
    // place (the lowerer normalizes to a value, but be defensive).
    auto atomic_ptr = [&](const MIROperand& op) -> llvm::Value* {
        if (op.kind() == MIROperandKind::Place) {
            const TypeId pt = fn.places[op.id].type;
            return builder->CreateLoad(mir_type(m, pt), places[op.id]);
        }
        return op_val(op);
    };

    auto constant_bits = [&](const MIROperand& op) -> uint64_t {
        if (op.kind() != MIROperandKind::Constant || op.id >= m.constants.size()) return 0;
        return m.constants.get(op.id).bits;
    };

    // Coerce an atomic operand to the atomic width (the library passes `%maybe_runtime<T>`
    // values whose MIR type already matches, but a cast in the source may not).
    auto coerce_atomic = [&](llvm::Value* v, llvm::Type* ty) -> llvm::Value* {
        if (!v || v->getType() == ty) return v;
        if (v->getType()->isIntegerTy() && ty->isIntegerTy())
            return builder->CreateIntCast(v, ty, true);
        if (v->getType()->isFloatingPointTy() && ty->isFloatingPointTy())
            return builder->CreateFPCast(v, ty);
        if (v->getType()->isPointerTy() && ty->isPointerTy())
            return builder->CreateBitCast(v, ty);
        return v;
    };

    // A MIR condition is any scalar (C truthiness): an `int`, a `char`, a
    // pointer, a float. LLVM's `br`/`select` require an `i1`, so compare against
    // zero ("Branch condition is not 'i1' type!" otherwise).
    auto as_i1 = [&](llvm::Value* c) -> llvm::Value* {
        if (!c) return nullptr;
        llvm::Type* t = c->getType();
        if (t->isIntegerTy(1)) return c;
        if (t->isIntegerTy()) return builder->CreateICmpNE(c, llvm::ConstantInt::get(t, 0));
        if (t->isPointerTy()) {
            return builder->CreateICmpNE(
                c, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(t)));
        }
        if (t->isFloatingPointTy())
            return builder->CreateFCmpONE(c, llvm::ConstantFP::get(t, 0.0));
        return c;
    };

    // Resolve a memory address from a base operand plus an optional selector
    // (field name / index), mirroring the C emitter's Load/Store shapes.
    // `selector` is nullptr for a whole-place / deref access.
    auto trace_field_fail = [&](const char* where, TypeId bt, const std::string& fname) {
        if (!getenv("MIR_LLVM_TRACE")) return;
        const TypeId agg = (bt < m.types.size() && mir_is_ptr_like(m, bt))
                               ? m.types.get(bt).element
                               : bt;
        std::string fields;
        if (agg < m.types.size()) {
            for (uint32_t i = 0; i < m.types.get(agg).data_count; ++i) {
                const MIRFieldInfo* f = m.types.field_of(agg, i);
                if (f) { fields += ","; fields += m.types.field_name_of(*f); }
            }
        }
        std::cerr << "[MIR-LLVM] in " << std::string(lfn->getName()) << ": " << where
                  << " field lookup failed bt=" << bt << " agg=" << agg
                  << " name=" << m.types.name_of(agg) << " field=" << fname
                  << " have=" << fields << "\n";
    };
    auto resolve_addr = [&](const MIROperand& base, const MIROperand* selector,
                            TypeId* out_pointee) -> llvm::Value* {
        if (base.kind() == MIROperandKind::Place) {
            llvm::Value* bp = places[base.id];
            TypeId bt = fn.places[base.id].type;
            if (!selector) {
                if (out_pointee) *out_pointee = bt;
                return bp;
            }
            // a pointer/reference place holds the pointer; load it before GEP
            const bool ptr_like = mir_is_ptr_like(m, bt);
            llvm::Value* base_ptr = ptr_like ? builder->CreateLoad(builder->getPtrTy(), bp) : bp;
            if (selector->kind() == MIROperandKind::Constant) {
                std::string fname;
                if (!const_string(selector->id, fname)) return nullptr;
                TypeId agg = ptr_like ? m.types.get(bt).element : bt;
                std::vector<uint32_t> indices;
                TypeId ft = MIR_INVALID_ID;
                if (!field_path(agg, fname, indices, ft)) {
                    trace_field_fail("place-sel", bt, fname);
                    return nullptr;
                }
                if (out_pointee) *out_pointee = ft;
                return gep_path(base_ptr, agg, indices);
            }
            if (selector->kind() == MIROperandKind::Value) {
                llvm::Value* idx = vals[selector->id];
                if (ptr_like) {
                    TypeId elem = m.types.get(bt).element;
                    if (out_pointee) *out_pointee = elem;
                    return builder->CreateInBoundsGEP(mir_type(m, elem), base_ptr, {idx});
                }
                const MIRTypeRecord& br = m.types.get(bt);
                if (out_pointee) *out_pointee = br.element;
                if (br.kind == MIRTypeKind::Array) {
                    return builder->CreateInBoundsGEP(
                        mir_type(m, bt), base_ptr, {builder->getInt32(0), idx});
                }
                return builder->CreateInBoundsGEP(mir_type(m, br.element), base_ptr, {idx});
            }
            return nullptr;
        }
        if (base.kind() == MIROperandKind::Value) {
            llvm::Value* bp = vals[base.id];
            if (!bp) {
                if (getenv("MIR_LLVM_TRACE")) {
                    const uint32_t di = fn.values[base.id].def_inst;
                    std::cerr << "[MIR-LLVM] addr base value " << base.id
                              << " is null; defined by "
                              << (di < fn.instructions.size()
                                      ? mir::opcode_name(fn.instructions[di].opcode())
                                      : "<none>")
                              << " in " << std::string(lfn->getName()) << "\n";
                }
                return nullptr;
            }
            TypeId bt = fn.values[base.id].type;
            if (bt == MIR_INVALID_ID || bt >= m.types.size()) {
                if (getenv("MIR_LLVM_TRACE")) {
                    const uint32_t di = fn.values[base.id].def_inst;
                    std::cerr << "[MIR-LLVM] invalid base type for value " << base.id
                              << " (ty=" << bt << ") defined by "
                              << (di < fn.instructions.size()
                                      ? mir::opcode_name(fn.instructions[di].opcode())
                                      : "<none>")
                              << " in " << std::string(lfn->getName()) << "\n";
                }
                return nullptr;
            }
            // the pointee of an address value (FieldAddr/IndexAddr record the
            // pointee; other address values record a pointer type)
            TypeId base_pointee = value_pointee(base.id);
            if (!selector) {
                if (out_pointee) *out_pointee = base_pointee;
                return bp;
            }
            if (selector->kind() == MIROperandKind::Constant) {
                std::string fname;
                if (!const_string(selector->id, fname)) return nullptr;
                TypeId agg = base_pointee;
                std::vector<uint32_t> indices;
                TypeId ft = MIR_INVALID_ID;
                if (!field_path(agg, fname, indices, ft)) {
                    trace_field_fail("value-sel", bt, fname);
                    return nullptr;
                }
                if (out_pointee) *out_pointee = ft;
                return gep_path(bp, agg, indices);
            }
            if (selector->kind() == MIROperandKind::Value) {
                llvm::Value* idx = vals[selector->id];
                const MIRTypeRecord& br = m.types.get(bt);
                if (br.kind == MIRTypeKind::Array) {
                    if (out_pointee) *out_pointee = br.element;
                    return builder->CreateInBoundsGEP(
                        mir_type(m, bt), bp, {builder->getInt32(0), idx});
                }
                if (out_pointee) *out_pointee = br.element;
                return builder->CreateInBoundsGEP(mir_type(m, br.element), bp, {idx});
            }
            return nullptr;
        }
        return nullptr;
    };

    // Coerce call arguments to the callee's parameter types. MIR arguments
    // keep their source types and rely on C's implicit conversions; LLVM
    // requires an exact match. Extra (variadic) arguments get the C default
    // argument promotions (float -> double, small ints -> i32). `arg_types`
    // (when given) carries each argument's MIR type so the cast uses the
    // right signedness — a `bool`/`u8` must zero-extend, not sign-extend.
    auto coerce_args = [&](std::vector<llvm::Value*>& args, llvm::FunctionType* fty,
                           const std::vector<TypeId>* arg_types = nullptr) {
        const unsigned nfixed = fty->getNumParams();
        for (size_t i = 0; i < args.size(); ++i) {
            if (!args[i]) continue;
            llvm::Type* have = args[i]->getType();
            llvm::Type* want = nullptr;
            if (i < nfixed) {
                want = fty->getParamType(i);
            } else if (fty->isVarArg()) {
                if (have->isFloatTy()) want = builder->getDoubleTy();
                else if (have->isIntegerTy() && have->getIntegerBitWidth() < 32)
                    want = builder->getInt32Ty();
                else
                    continue;
            } else {
                continue;
            }
            if (have == want) continue;
            if (want->isIntegerTy() && have->isIntegerTy()) {
                bool sig = have->isIntegerTy(1) ? false : true;
                if (arg_types && i < arg_types->size() && (*arg_types)[i] != MIR_INVALID_ID)
                    sig = is_signed_ty((*arg_types)[i]);
                args[i] = builder->CreateIntCast(args[i], want, sig);
            } else if (want->isPointerTy() && have->isIntegerTy())
                args[i] = builder->CreateIntToPtr(args[i], want);
            else if (want->isIntegerTy() && have->isPointerTy())
                args[i] = builder->CreatePtrToInt(args[i], want);
            else if (want->isPointerTy() && have->isPointerTy())
                args[i] = builder->CreateBitCast(args[i], want);
            else if (want->isFloatingPointTy() && have->isFloatingPointTy())
                args[i] = builder->CreateFPCast(args[i], want);
            else if (want->isFloatingPointTy() && have->isIntegerTy())
                args[i] = builder->CreateSIToFP(args[i], want);
            else if (want->isIntegerTy() && have->isFloatingPointTy())
                args[i] = builder->CreateFPToSI(args[i], want);
        }
    };

    uint32_t arg_index = 0;
    if (sret) arg_index = 1;

    for (uint32_t bi = 0; bi < fn.blocks.size(); ++bi) {
        const MIRBlock& blk = fn.blocks[bi];
        builder->SetInsertPoint(blocks[bi]);
        const uint32_t end = blk.inst_start + blk.inst_count + 1;
        for (uint32_t ii = blk.inst_start; ii < end && ii < fn.instructions.size(); ++ii) {
            const MIRInstruction& inst = fn.instructions[ii];
            const uint32_t base = inst.operand_offset;
            auto arg = [&](uint32_t n) -> const MIROperand* {
                if (n >= inst.operand_count) return nullptr;
                return &fn.operands[base + n];
            };
            const uint32_t v = inst.result_or_place;
            if (getenv("MIR_LLVM_TRACE"))
                std::cerr << "  bi=" << bi << " ii=" << ii << " "
                          << mir::opcode_name(inst.opcode()) << " res=" << v
                          << " n=" << inst.operand_count << "\n";
            switch (inst.opcode()) {
                case MIROpcode::Nop:
                case MIROpcode::Alloca:
                    break;
                case MIROpcode::ConstInt:
                case MIROpcode::ConstBool:
                case MIROpcode::ConstFloat:
                case MIROpcode::ConstDouble:
                case MIROpcode::ConstNull: {
                    const MIROperand* c = arg(0);
                    if (c) {
                        const TypeId vt = (v < fn.values.size()) ? fn.values[v].type : c->type();
                        vals[v] = const_of(c->id, vt);
                    }
                    break;
                }
                case MIROpcode::ConstString: {
                    const MIROperand* c = arg(0);
                    if (!c) break;
                    llvm::Constant* cv = const_of(c->id, c->type());
                    auto* gv = new llvm::GlobalVariable(
                        *module, cv->getType(), true,
                        llvm::GlobalValue::PrivateLinkage, cv, ".str");
                    vals[v] = llvm::ConstantExpr::getBitCast(gv, builder->getPtrTy());
                    break;
                }
                case MIROpcode::Param: {
                    vals[v] = lfn->getArg(arg_index++);
                    break;
                }
                case MIROpcode::GlobalAddr: {
                    const MIROperand* s = arg(0);
                    if (s) {
                        auto git = mir_global_cache_.find(s->id);
                        if (git != mir_global_cache_.end() && git->second) {
                            vals[v] = git->second;
                        } else if (s->id < m.symbols.size()) {
                            const MIRSymbolRecord& rec = m.symbols.get(s->id);
                            std::string nm(rec.mangled_length ? m.symbols.mangled_data(rec)
                                                              : m.symbols.name_data(rec),
                                           rec.mangled_length ? rec.mangled_length
                                                              : rec.name_length);
                            auto nit = mir_named_globals_.find(nm);
                            if (nit != mir_named_globals_.end()) {
                                vals[v] = nit->second;
                            } else {
                                // A named global owned by *another* module — an
                                // interface vtable, for instance, is emitted in
                                // the module that hosts the `impl`. The C backend
                                // emits the whole program as one translation unit
                                // so the reference just resolves; per-module LLVM
                                // objects need an external declaration, otherwise
                                // the address is missing and the store that uses
                                // it (a `dyn<I>` fat pointer's vtable slot) is
                                // dropped.
                                llvm::Type* gt = (rec.type != MIR_INVALID_ID &&
                                                  rec.type < m.types.size())
                                                     ? mir_type(m, rec.type)
                                                     : nullptr;
                                if (!gt || !gt->isSized())
                                    gt = llvm::Type::getInt8Ty(module->getContext());
                                llvm::GlobalVariable* gv = module->getGlobalVariable(nm);
                                if (!gv) {
                                    gv = new llvm::GlobalVariable(
                                        *module, gt, /*isConstant=*/false,
                                        llvm::GlobalValue::ExternalLinkage, nullptr, nm);
                                }
                                mir_global_cache_[s->id] = gv;
                                vals[v] = gv;
                            }
                        }
                    }
                    break;
                }
                case MIROpcode::FunctionAddr: {
                    const MIROperand* s = arg(0);
                    if (s) vals[v] = mir_function(m, s->id);
                    break;
                }
                case MIROpcode::Load: {
                    const MIROperand* b = arg(0);
                    if (!b) break;
                    // {base, Type} | {base, selector(Constant|Value), Type}
                    const MIROperand* sel = (inst.operand_count >= 3) ? arg(1) : nullptr;
                    TypeId pointee = fn.values[v].type;
                    llvm::Value* ptr = resolve_addr(*b, sel, &pointee);
                    if (!ptr) {
                        if (getenv("MIR_LLVM_TRACE"))
                            std::cerr << "[MIR-LLVM] null-load-base in "
                                      << std::string(lfn->getName()) << "\n";
                        vals[v] = llvm::PoisonValue::get(mir_type(m, fn.values[v].type));
                        break;
                    }
                    vals[v] = builder->CreateLoad(mir_type(m, fn.values[v].type), ptr);
                    break;
                }
                case MIROpcode::Store: {
                    const MIROperand* b = arg(0);
                    if (!b) break;
                    const MIROperand* sel = nullptr;
                    const MIROperand* src = nullptr;
                    if (inst.operand_count == 2) {
                        src = arg(1); // {base, value}
                    } else {
                        sel = arg(1); // {base, selector, value}
                        src = arg(2);
                    }
                    TypeId dst_ty = MIR_INVALID_ID;
                    llvm::Value* dst = resolve_addr(*b, sel, &dst_ty);
                    llvm::Value* sv = nullptr;
                    if (src) {
                        sv = op_val(*src);
                        if (src->kind() == MIROperandKind::Place) {
                            const MIRTypeRecord& sr = m.types.get(fn.places[src->id].type);
                            sv = (sr.kind == MIRTypeKind::Pointer ||
                                  sr.kind == MIRTypeKind::Reference)
                                     ? builder->CreateLoad(mir_type(m, sr.element), places[src->id])
                                     : builder->CreateLoad(mir_type(m, fn.places[src->id].type),
                                                           places[src->id]);
                        }
                        // MIR keeps the source type on the value (C assigns with
                        // an implicit conversion); LLVM needs an exact match, so
                        // a wider value stored into a narrower place would
                        // otherwise clobber adjacent stack slots.
                        if (sv && dst_ty != MIR_INVALID_ID && dst_ty < m.types.size() &&
                            !mir_is_ptr_like(m, dst_ty)) {
                            llvm::Type* want = mir_type(m, dst_ty);
                            if (sv->getType() != want && !want->isAggregateType() &&
                                !sv->getType()->isAggregateType()) {
                                if (sv->getType()->isIntegerTy() && want->isIntegerTy())
                                    sv = builder->CreateIntCast(sv, want, is_signed_ty(dst_ty));
                                else if (sv->getType()->isFloatingPointTy() &&
                                         want->isFloatingPointTy())
                                    sv = builder->CreateFPCast(sv, want);
                                else if (sv->getType()->isPointerTy() && want->isPointerTy())
                                    sv = builder->CreateBitCast(sv, want);
                            }
                        }
                    }
                    if (dst && sv) builder->CreateStore(sv, dst);
                    break;
                }
                case MIROpcode::AddressOf: {
                    const MIROperand* p = arg(0);
                    if (p) vals[v] = op_val(*p);
                    break;
                }
                case MIROpcode::FieldAddr:
                case MIROpcode::IndexAddr: {
                    const MIROperand* b = arg(0);
                    const MIROperand* sel = arg(1);
                    if (!b || !sel) break;
                    TypeId pointee = MIR_INVALID_ID;
                    llvm::Value* ptr = resolve_addr(*b, sel, &pointee);
                    if (ptr) {
                        vals[v] = ptr;
                    } else if (getenv("MIR_LLVM_TRACE")) {
                        TypeId bt = (b->kind() == MIROperandKind::Place)
                                        ? fn.places[b->id].type
                                        : fn.values[b->id].type;
                        TypeId agg = mir_is_ptr_like(m, bt) ? m.types.get(bt).element : bt;
                        std::string fname, fields;
                        const_string(sel->id, fname);
                        if (agg < m.types.size()) {
                            for (uint32_t i = 0; i < m.types.get(agg).data_count; ++i) {
                                const MIRFieldInfo* f = m.types.field_of(agg, i);
                                if (f) { fields += ","; fields += m.types.field_name_of(*f); }
                            }
                        }
                        std::cerr << "[MIR-LLVM] "
                                  << mir::opcode_name(inst.opcode()) << " failed in "
                                  << std::string(lfn->getName()) << " base.kind="
                                  << static_cast<int>(b->kind()) << " base.ty=" << bt
                                  << " agg=" << agg << " aggname=" << m.types.name_of(agg)
                                  << " field=" << fname << " have=" << fields << "\n";
                    }
                    break;
                }
                case MIROpcode::Unary: {
                    const MIROperand* a = arg(0);
                    const MIROperand* opc = arg(1);
                    if (!a || !opc) break;
                    const uint64_t code =
                        (opc->id < m.constants.size()) ? m.constants.get(opc->id).bits : 0;
                    llvm::Value* av = op_val(*a);
                    if (!av) break;
                    switch (static_cast<MIRUnaryOp>(code)) {
                        case MIRUnaryOp::Neg:
                            vals[v] = av->getType()->isFloatingPointTy()
                                          ? builder->CreateFNeg(av)
                                          : builder->CreateNeg(av);
                            break;
                        case MIRUnaryOp::Not: {
                            // logical not: `!x` is `x == 0` (or `x == null`), never
                            // a bitwise complement of the operand's own type
                            llvm::Type* t = av->getType();
                            if (t->isIntegerTy(1)) {
                                vals[v] = builder->CreateNot(av);
                            } else if (t->isFloatingPointTy()) {
                                vals[v] = builder->CreateFCmpOEQ(
                                    av, llvm::ConstantFP::get(t, 0.0));
                            } else if (t->isPointerTy()) {
                                vals[v] = builder->CreateICmpEQ(
                                    av, llvm::ConstantPointerNull::get(
                                            llvm::cast<llvm::PointerType>(t)));
                            } else if (t->isIntegerTy()) {
                                vals[v] = builder->CreateICmpEQ(
                                    av, llvm::ConstantInt::get(t, 0));
                            } else {
                                vals[v] = builder->CreateNot(av);
                            }
                            break;
                        }
                        case MIRUnaryOp::BitNot:
                            if (av->getType()->isIntegerTy()) vals[v] = builder->CreateNot(av);
                            break;
                        case MIRUnaryOp::Plus: vals[v] = av; break;
                    }
                    // C spells the result as `int8_t v = -x;` — the promoted-width
                    // result is narrowed back to the MIR value's type. LLVM has no
                    // implicit conversion, so `-(i8 100)` must be `trunc i32` of the
                    // negation (otherwise the wrap-around is lost).
                    if (vals[v] && vals[v]->getType()->isIntegerTy() &&
                        fn.values[v].type < m.types.size()) {
                        llvm::Type* rt = mir_type(m, fn.values[v].type);
                        if (rt->isIntegerTy() && rt != vals[v]->getType())
                            vals[v] = builder->CreateTrunc(vals[v], rt);
                    }
                    break;
                }
                case MIROpcode::Binary:
                case MIROpcode::Compare: {
                    const MIROperand* a = arg(0);
                    const MIROperand* b = arg(1);
                    const MIROperand* opc = arg(2);
                    if (!a || !b || !opc) break;
                    llvm::Value* av = op_val(*a);
                    llvm::Value* bv = op_val(*b);
                    if (!av || !bv) break;
                    // MIR (like C) relies on the usual arithmetic conversions, so
                    // the two operands may carry different-but-compatible types
                    // (e.g. `ptr == 0`, `int32 < int64`, `char < int`). LLVM
                    // requires identical operand types: coerce the pair first.
                    const uint64_t code =
                        (opc->id < m.constants.size()) ? m.constants.get(opc->id).bits : 0;
                    const auto op = static_cast<MIRBinaryOp>(code);
                    // signedness of the common integer type after coercion
                    // (set by coerce_pair; decides icmp/udiv/udiv predicates)
                    bool uns = false;
                    // pointer arithmetic: `p +/- n` (and `p - p`) must become a
                    // GEP/PtrDiff over the pointee type, never an integer op.
                    const bool a_ptr = value_is_pointer(*a);
                    const bool b_ptr = value_is_pointer(*b);
                    if ((op == MIRBinaryOp::Add || op == MIRBinaryOp::Sub) &&
                        (a_ptr || b_ptr)) {
                        auto pointee = [&](const MIROperand& op) -> llvm::Type* {
                            TypeId e = MIR_INVALID_ID;
                            if (op.kind() == MIROperandKind::Value) {
                                e = value_pointee(op.id);
                            } else if (op.type() < m.types.size()) {
                                e = m.types.get(op.type()).element;
                            }
                            if (e == MIR_INVALID_ID || e >= m.types.size() ||
                                m.types.get(e).kind == MIRTypeKind::Void) {
                                return builder->getInt8Ty();
                            }
                            return mir_type(m, e);
                        };
                        if (a_ptr && b_ptr) {
                            if (op == MIRBinaryOp::Sub) {
                                llvm::Value* d = builder->CreatePtrDiff(pointee(*a), av, bv);
                                llvm::Type* rt = mir_type(m, fn.values[v].type);
                                if (rt->isIntegerTy() && d->getType() != rt)
                                    d = builder->CreateIntCast(d, rt, true);
                                vals[v] = d;
                            } else {
                                vals[v] = av;
                            }
                        } else if (a_ptr) {
                            llvm::Value* idx = builder->CreateIntCast(
                                bv, builder->getInt64Ty(), b->type() < m.types.size() &&
                                    (m.types.get(b->type()).flags & TF_SIGNED) != 0);
                            if (op == MIRBinaryOp::Sub) idx = builder->CreateNeg(idx);
                            vals[v] = builder->CreateGEP(pointee(*a), av, {idx});
                        } else {
                            llvm::Value* idx = builder->CreateIntCast(
                                av, builder->getInt64Ty(), a->type() < m.types.size() &&
                                    (m.types.get(a->type()).flags & TF_SIGNED) != 0);
                            vals[v] = builder->CreateGEP(pointee(*b), bv, {idx});
                        }
                        break;
                    }
                    coerce_pair(av, bv, a->type(), b->type(),
                                inst.opcode() == MIROpcode::Compare, &uns);
                    if (!av || !bv) break;
                    const bool fp = av->getType()->isFloatingPointTy();
                    if (fp) {
                        switch (op) {
                            case MIRBinaryOp::Add: vals[v] = builder->CreateFAdd(av, bv); break;
                            case MIRBinaryOp::Sub: vals[v] = builder->CreateFSub(av, bv); break;
                            case MIRBinaryOp::Mul: vals[v] = builder->CreateFMul(av, bv); break;
                            case MIRBinaryOp::Div: vals[v] = builder->CreateFDiv(av, bv); break;
                            case MIRBinaryOp::Rem: vals[v] = builder->CreateFRem(av, bv); break;
                            case MIRBinaryOp::Lt: vals[v] = builder->CreateFCmpOLT(av, bv); break;
                            case MIRBinaryOp::Le: vals[v] = builder->CreateFCmpOLE(av, bv); break;
                            case MIRBinaryOp::Gt: vals[v] = builder->CreateFCmpOGT(av, bv); break;
                            case MIRBinaryOp::Ge: vals[v] = builder->CreateFCmpOGE(av, bv); break;
                            case MIRBinaryOp::Eq: vals[v] = builder->CreateFCmpOEQ(av, bv); break;
                            case MIRBinaryOp::Ne: vals[v] = builder->CreateFCmpONE(av, bv); break;
                            default: vals[v] = av; break;
                        }
                    } else {
                        switch (op) {
                            case MIRBinaryOp::Add: vals[v] = builder->CreateAdd(av, bv); break;
                            case MIRBinaryOp::Sub: vals[v] = builder->CreateSub(av, bv); break;
                            case MIRBinaryOp::Mul: vals[v] = builder->CreateMul(av, bv); break;
                            case MIRBinaryOp::Div:
                                vals[v] = uns ? builder->CreateUDiv(av, bv)
                                              : builder->CreateSDiv(av, bv);
                                break;
                            case MIRBinaryOp::Rem:
                                vals[v] = uns ? builder->CreateURem(av, bv)
                                              : builder->CreateSRem(av, bv);
                                break;
                            case MIRBinaryOp::Shl: vals[v] = builder->CreateShl(av, bv); break;
                            case MIRBinaryOp::Shr:
                                vals[v] = uns ? builder->CreateLShr(av, bv)
                                              : builder->CreateAShr(av, bv);
                                break;
                            case MIRBinaryOp::BitAnd: vals[v] = builder->CreateAnd(av, bv); break;
                            case MIRBinaryOp::BitOr: vals[v] = builder->CreateOr(av, bv); break;
                            case MIRBinaryOp::BitXor: vals[v] = builder->CreateXor(av, bv); break;
                            case MIRBinaryOp::Lt:
                                vals[v] = uns ? builder->CreateICmpULT(av, bv)
                                              : builder->CreateICmpSLT(av, bv);
                                break;
                            case MIRBinaryOp::Le:
                                vals[v] = uns ? builder->CreateICmpULE(av, bv)
                                              : builder->CreateICmpSLE(av, bv);
                                break;
                            case MIRBinaryOp::Gt:
                                vals[v] = uns ? builder->CreateICmpUGT(av, bv)
                                              : builder->CreateICmpSGT(av, bv);
                                break;
                            case MIRBinaryOp::Ge:
                                vals[v] = uns ? builder->CreateICmpUGE(av, bv)
                                              : builder->CreateICmpSGE(av, bv);
                                break;
                            case MIRBinaryOp::Eq: vals[v] = builder->CreateICmpEQ(av, bv); break;
                            case MIRBinaryOp::Ne: vals[v] = builder->CreateICmpNE(av, bv); break;
                        }
                    }
                    // Narrow the promoted-width result back to the MIR value's
                    // type, exactly as C does when it assigns to `int8_t v = a+b`.
                    // Integer promotions widen i8/i16 operands to i32, so without
                    // this the wrap-around of `(x + y) == -56i8` is lost. A
                    // comparison already yields i1, which is the value's type.
                    if (vals[v] && vals[v]->getType()->isIntegerTy() &&
                        fn.values[v].type < m.types.size()) {
                        llvm::Type* rt = mir_type(m, fn.values[v].type);
                        if (rt->isIntegerTy() && rt != vals[v]->getType() &&
                            vals[v]->getType()->getIntegerBitWidth() > rt->getIntegerBitWidth())
                            vals[v] = builder->CreateTrunc(vals[v], rt);
                    }
                    break;
                }
                case MIROpcode::Cast: {
                    const MIROperand* a = arg(0);
                    if (!a) break;
                    llvm::Value* av = op_val(*a);
                    if (!av) break;
                    llvm::Type* to = mir_type(m, fn.values[v].type);
                    if (av->getType() == to) { vals[v] = av; break; }
                    // Casting to an `Opaque` MIR type (`value as StaticInterface`)
                    // is a pointer reinterpretation; `mir_type` spells Opaque `i8`,
                    // so `ptrtoint ptr %p to i8` would *truncate* the address to 8
                    // bits. Keep the pointer — the erased-self ABI is a pointer.
                    if (av->getType()->isPointerTy() &&
                        fn.values[v].type < m.types.size() &&
                        m.types.get(fn.values[v].type).kind == MIRTypeKind::Opaque) {
                        vals[v] = av;
                        break;
                    }
                    if (av->getType()->isIntegerTy() && to->isIntegerTy()) {
                        bool src_signed = false;
                        if (a->type() < m.types.size())
                            src_signed = (m.types.get(a->type()).flags & TF_SIGNED) != 0;
                        vals[v] = builder->CreateIntCast(av, to, src_signed);
                    } else if (av->getType()->isPointerTy() && to->isPointerTy()) {
                        vals[v] = builder->CreateBitCast(av, to);
                    } else if (av->getType()->isIntegerTy() && to->isPointerTy()) {
                        // C converts the integer to the pointer width first with
                        // the *source* signedness: `(void*)-1` is all-ones, not
                        // `0xFFFFFFFF`. `CreateIntToPtr` only zero-extends.
                        bool src_signed = false;
                        if (a->type() < m.types.size())
                            src_signed = (m.types.get(a->type()).flags & TF_SIGNED) != 0;
                        const unsigned ptr_bits =
                            module->getDataLayout().getPointerSizeInBits(0);
                        llvm::Value* iv = av;
                        llvm::Type* ipt = builder->getIntNTy(ptr_bits);
                        if (iv->getType()->getIntegerBitWidth() < ptr_bits)
                            iv = src_signed ? builder->CreateSExt(iv, ipt)
                                            : builder->CreateZExt(iv, ipt);
                        else if (iv->getType()->getIntegerBitWidth() > ptr_bits)
                            iv = builder->CreateTrunc(iv, ipt);
                        vals[v] = builder->CreateIntToPtr(iv, to);
                    } else if (av->getType()->isPointerTy() && to->isIntegerTy()) {
                        vals[v] = builder->CreatePtrToInt(av, to);
                    } else if (av->getType()->isFloatingPointTy() && to->isIntegerTy()) {
                        vals[v] = builder->CreateFPToSI(av, to);
                    } else if (av->getType()->isIntegerTy() && to->isFloatingPointTy()) {
                        vals[v] = builder->CreateSIToFP(av, to);
                    } else if (av->getType()->isFloatingPointTy() && to->isFloatingPointTy()) {
                        vals[v] = builder->CreateFPCast(av, to);
                    } else {
                        vals[v] = av;
                    }
                    break;
                }
                case MIROpcode::Select: {
                    const MIROperand* c = arg(0);
                    const MIROperand* t = arg(1);
                    const MIROperand* f = arg(2);
                    if (c && t && f) {
                        llvm::Value* tv = op_val(*t);
                        llvm::Value* fv = op_val(*f);
                        // the two arms must have one type (MIR keeps the source
                        // types and relies on C's implicit conversions)
                        if (tv && fv && tv->getType() != fv->getType()) {
                            if (tv->getType()->isIntegerTy() && fv->getType()->isIntegerTy())
                                fv = builder->CreateIntCast(fv, tv->getType(), true);
                            else if (tv->getType()->isPointerTy() && fv->getType()->isPointerTy())
                                fv = builder->CreateBitCast(fv, tv->getType());
                            else if (tv->getType()->isFloatingPointTy() &&
                                     fv->getType()->isFloatingPointTy())
                                fv = builder->CreateFPCast(fv, tv->getType());
                        }
                        llvm::Value* cv = as_i1(op_val(*c));
                        if (cv && tv && fv) vals[v] = builder->CreateSelect(cv, tv, fv);
                    }
                    break;
                }
                case MIROpcode::SizeOf: {
                    const MIROperand* t = arg(0);
                    if (t) {
                        uint64_t sz = module->getDataLayout().getTypeAllocSize(
                            mir_type(m, t->type()));
                        vals[v] = llvm::ConstantInt::get(mir_type(m, fn.values[v].type), sz);
                    }
                    break;
                }
                case MIROpcode::AlignOf: {
                    const MIROperand* t = arg(0);
                    if (t) {
                        uint64_t al = module->getDataLayout()
                                          .getABITypeAlign(mir_type(m, t->type()))
                                          .value();
                        vals[v] = llvm::ConstantInt::get(mir_type(m, fn.values[v].type), al);
                    }
                    break;
                }
                case MIROpcode::OffsetOf: {
                    const MIROperand* t = arg(0);
                    const MIROperand* f = arg(1);
                    if (!t || !f) break;
                    std::string fname;
                    if (!const_string(f->id, fname)) break;
                    const int fi = field_index(t->type(), fname);
                    if (fi < 0) break;
                    const llvm::StructLayout* sl =
                        module->getDataLayout().getStructLayout(
                            llvm::cast<llvm::StructType>(mir_type(m, t->type())));
                    vals[v] = llvm::ConstantInt::get(mir_type(m, fn.values[v].type),
                                                     sl->getElementOffset(fi));
                    break;
                }
                case MIROpcode::Call:
                case MIROpcode::CallIndirect: {
                    llvm::Value* callee = nullptr;
                    uint32_t a = 0;
                    if (inst.opcode() == MIROpcode::Call) {
                        const MIROperand* s = arg(0);
                        if (!s || s->kind() != MIROperandKind::Symbol) break;
                        callee = mir_function(m, s->id);
                        a = 1;
                    } else {
                        const MIROperand* f = arg(0);
                        if (!f) break;
                        callee = op_val(*f);
                        a = 1;
                    }
                    if (!callee) break;
                    const MIROperand* maybe_place = arg(a);
                    const bool call_sret =
                        maybe_place && maybe_place->kind() == MIROperandKind::Place;
                    std::vector<llvm::Value*> args;
                    std::vector<TypeId> arg_types;
                    if (call_sret) {
                        args.push_back(places[maybe_place->id]);
                        arg_types.push_back(MIR_INVALID_ID);
                        a++;
                    }
                    for (; a < inst.operand_count; ++a) {
                        const MIROperand* o = arg(a);
                        if (!o) continue;
                        llvm::Value* av = op_val(*o);
                        if (!av) continue;
                        arg_types.push_back(o->type());
                        if (o->kind() == MIROperandKind::Place) {
                            const MIRTypeRecord& pr = m.types.get(fn.places[o->id].type);
                            if (pr.kind == MIRTypeKind::Array || mir_is_aggregate(m, fn.places[o->id].type)) {
                                av = places[o->id];
                            } else {
                                av = builder->CreateLoad(mir_type(m, fn.places[o->id].type), av);
                            }
                        }
                        args.push_back(av);
                    }
                    llvm::CallInst* ci;
                    if (inst.opcode() == MIROpcode::Call) {
                        llvm::FunctionType* fty =
                            llvm::cast<llvm::Function>(callee)->getFunctionType();
                        coerce_args(args, fty, &arg_types);
                        ci = builder->CreateCall(llvm::cast<llvm::Function>(callee), args);
                    } else {
                        const MIROperand* f = arg(0);
                        TypeId ft = fn.values[f->id].type;
                        llvm::FunctionType* fty = mir_fn_type(m, ft);
                        if (!fty) break;
                        coerce_args(args, fty, &arg_types);
                        ci = builder->CreateCall(fty, callee, args);
                    }
                    if (!call_sret && v < fn.values.size() && !mir_is_void(m, fn.values[v].type)) {
                        vals[v] = ci;
                    }
                    break;
                }
                case MIROpcode::Init: {
                    const MIROperand* dest = arg(0);
                    const MIROperand* ctor = arg(1);
                    if (!dest || !ctor || ctor->kind() != MIROperandKind::Symbol) break;
                    llvm::Function* cf = mir_function(m, ctor->id);
                    if (!cf) break;
                    std::vector<llvm::Value*> args;
                    std::vector<TypeId> arg_types;
                    args.push_back(places[dest->id]);
                    arg_types.push_back(MIR_INVALID_ID);
                    for (uint32_t a = 2; a < inst.operand_count; ++a) {
                        const MIROperand* o = arg(a);
                        if (o) {
                            args.push_back(op_val(*o));
                            arg_types.push_back(o->type());
                        }
                    }
                    // MIR arguments keep their source types and rely on C's
                    // implicit conversions; LLVM requires an exact match. A
                    // default `usize` literal is `i32` in MIR but `i64` in the
                    // constructor's signature.
                    coerce_args(args, cf->getFunctionType(), &arg_types);
                    builder->CreateCall(cf, args);
                    break;
                }
                case MIROpcode::CopyInit:
                case MIROpcode::MoveInit:
                case MIROpcode::Assign: {
                    const MIROperand* dest = arg(0);
                    const MIROperand* src = arg(1);
                    if (!dest || !src) break;
                    llvm::Value* dv = places[dest->id];
                    llvm::Value* sv = op_val(*src);
                    if (src->kind() == MIROperandKind::Place) {
                        // Mirror the C emitter: a pointer/reference-typed source
                        // place copies the *pointee* (`var x = *j`), not the
                        // pointer. Storing the pointer bits into a struct slot
                        // left every field but the first zero.
                        const MIRTypeRecord& srt = m.types.get(fn.places[src->id].type);
                        if (srt.kind == MIRTypeKind::Pointer || srt.kind == MIRTypeKind::Reference) {
                            llvm::Value* p = builder->CreateLoad(builder->getPtrTy(), sv);
                            sv = builder->CreateLoad(mir_type(m, fn.places[dest->id].type), p);
                        } else {
                            sv = builder->CreateLoad(mir_type(m, fn.places[src->id].type), sv);
                        }
                    }
                    if (dv && sv) builder->CreateStore(sv, dv);
                    break;
                }
                case MIROpcode::SetDrop: {
                    const MIROperand* p = arg(0);
                    const MIROperand* c = arg(1);
                    if (p && c) {
                        uint64_t b =
                            (c->id < m.constants.size()) ? m.constants.get(c->id).bits : 0;
                        builder->CreateStore(
                            llvm::ConstantInt::get(builder->getInt1Ty(), b ? 1 : 0),
                            places[p->id]);
                    }
                    break;
                }
                case MIROpcode::Drop:
                case MIROpcode::Destroy: {
                    const MIROperand* p = arg(0);
                    const MIROperand* d = arg(1);
                    if (!p || !d || d->kind() != MIROperandKind::Symbol) break;
                    llvm::Function* df = mir_function(m, d->id);
                    if (!df) break;
                    // Mirror `MIREmitter`'s Drop/Destroy: an aggregate-param place
                    // is *pointer-typed* (`&param` names the object), and an array
                    // place drops each element. Passing the alloca of a
                    // pointer-typed place would hand the destructor its own stack
                    // slot (`self.lamb` in the callee reads garbage).
                    const TypeId pt = fn.places[p->id].type;
                    const MIRTypeRecord& dpr =
                        (pt < m.types.size()) ? m.types.get(pt) : MIRTypeRecord{};
                    const bool ptr_place = dpr.kind == MIRTypeKind::Pointer ||
                                           dpr.kind == MIRTypeKind::Reference;
                    const bool arr_place = dpr.kind == MIRTypeKind::Array;
                    auto emit_drop = [&]() {
                        llvm::Value* base = places[p->id];
                        if (ptr_place) {
                            base = builder->CreateLoad(builder->getPtrTy(), base);
                            builder->CreateCall(df, {base});
                        } else if (arr_place) {
                            // last-to-first, matching the C emitter (and the
                            // language's reverse destruction order)
                            llvm::Type* at = mir_type(m, pt);
                            for (uint32_t i = dpr.data_count; i-- > 0;) {
                                llvm::Value* ep = builder->CreateInBoundsGEP(
                                    at, base, {builder->getInt32(0), builder->getInt32(i)});
                                builder->CreateCall(df, {ep});
                            }
                        } else {
                            builder->CreateCall(df, {base});
                        }
                    };
                    if (inst.opcode() == MIROpcode::Drop && inst.operand_count > 2) {
                        const MIROperand* flag = arg(2);
                        llvm::Value* fv = flag ? op_val(*flag) : nullptr;
                        if (!fv) break;
                        llvm::BasicBlock* do_b =
                            llvm::BasicBlock::Create(*ctx, "drop", lfn);
                        llvm::BasicBlock* end_b =
                            llvm::BasicBlock::Create(*ctx, "drop.end", lfn);
                        builder->CreateCondBr(as_i1(fv), do_b, end_b);
                        builder->SetInsertPoint(do_b);
                        emit_drop();
                        builder->CreateBr(end_b);
                        builder->SetInsertPoint(end_b);
                    } else {
                        emit_drop();
                    }
                    break;
                }
                case MIROpcode::MemCpy: {
                    const MIROperand* d = arg(0);
                    const MIROperand* s = arg(1);
                    const MIROperand* n = arg(2);
                    if (!d || !s) break;
                    llvm::Value* dv = op_val(*d);
                    llvm::Value* sv = op_val(*s);
                    if (d->kind() == MIROperandKind::Place) dv = places[d->id];
                    if (s->kind() == MIROperandKind::Place) sv = places[s->id];
                    llvm::Value* nv = n ? op_val(*n) : nullptr;
                    if (!nv && d->kind() == MIROperandKind::Place) {
                        // the C emitter defaults the size to `sizeof(dest place)`
                        // (`var x = *j` lowers to `memcpy p1, ptr` with no size)
                        const TypeId dt = fn.places[d->id].type;
                        if (dt >= m.types.size()) break;
                        nv = builder->getInt64(
                            module->getDataLayout().getTypeAllocSize(mir_type(m, dt)));
                    }
                    if (dv && sv && nv)
                        builder->CreateMemCpy(dv, llvm::MaybeAlign(), sv, llvm::MaybeAlign(), nv);
                    break;
                }
                case MIROpcode::MemSet: {
                    const MIROperand* d = arg(0);
                    if (!d) break;
                    llvm::Value* dv = (d->kind() == MIROperandKind::Place) ? places[d->id]
                                                                           : op_val(*d);
                    if (!dv) break;
                    const MIROperand* byte = arg(1);
                    const MIROperand* n = arg(2);
                    llvm::Value* bv = byte ? op_val(*byte) : nullptr;
                    llvm::Value* nv = n ? op_val(*n) : nullptr;
                    if (!bv || !nv) {
                        // `zero_init(place)` carries only the destination; the C
                        // emitter zeroes `sizeof(place)` bytes.
                        if (d->kind() != MIROperandKind::Place) break;
                        const TypeId dt = fn.places[d->id].type;
                        if (dt >= m.types.size()) break;
                        const uint64_t sz =
                            module->getDataLayout().getTypeAllocSize(mir_type(m, dt));
                        bv = builder->getInt8(0);
                        nv = builder->getInt64(sz);
                    }
                    builder->CreateMemSet(dv, bv, nv, llvm::MaybeAlign());
                    break;
                }
                case MIROpcode::AtomicLoad: {
                    const MIROperand* ptr = arg(0);
                    const MIROperand* order = arg(1);
                    const MIROperand* scope = arg(2);
                    llvm::Value* pv = ptr ? atomic_ptr(*ptr) : nullptr;
                    if (!pv || !order) break;
                    llvm::LoadInst* li = builder->CreateLoad(mir_type(m, fn.values[v].type), pv);
                    li->setAtomic(mir_atomic_ordering(constant_bits(*order)),
                                  mir_atomic_scope(scope ? constant_bits(*scope) : 0));
                    vals[v] = li;
                    break;
                }
                case MIROpcode::AtomicStore: {
                    const MIROperand* ptr = arg(0);
                    const MIROperand* value = arg(1);
                    const MIROperand* order = arg(2);
                    const MIROperand* scope = arg(3);
                    llvm::Value* pv = ptr ? atomic_ptr(*ptr) : nullptr;
                    llvm::Value* vv = value ? op_val(*value) : nullptr;
                    if (!pv || !vv || !order) break;
                    const TypeId pointee = pointee_of(*ptr);
                    if (pointee == MIR_INVALID_ID) break;
                    vv = coerce_atomic(vv, mir_type(m, pointee));
                    llvm::StoreInst* si = builder->CreateStore(vv, pv);
                    si->setAtomic(mir_atomic_ordering(constant_bits(*order)),
                                  mir_atomic_scope(scope ? constant_bits(*scope) : 0));
                    break;
                }
                case MIROpcode::AtomicCmpXchg: {
                    const MIROperand* ptr = arg(0);
                    const MIROperand* expected = arg(1);
                    const MIROperand* value = arg(2);
                    const MIROperand* success = arg(3);
                    const MIROperand* failure = arg(4);
                    const MIROperand* scope = arg(5);
                    const MIROperand* weak = arg(6);
                    llvm::Value* pv = ptr ? atomic_ptr(*ptr) : nullptr;
                    llvm::Value* ev = expected ? atomic_ptr(*expected) : nullptr;
                    llvm::Value* nv = value ? op_val(*value) : nullptr;
                    if (!pv || !ev || !nv || !success || !failure) break;
                    const TypeId pointee = ptr ? pointee_of(*ptr) : MIR_INVALID_ID;
                    if (pointee == MIR_INVALID_ID) break;
                    llvm::Type* ty = mir_type(m, pointee);
                    nv = coerce_atomic(nv, ty);
                    // `expected` is a pointer to the value to compare against; on
                    // failure LLVM leaves the loaded value in it, so it is written
                    // back (this is what C `atomic_compare_exchange` does).
                    llvm::Value* ev_loaded = builder->CreateLoad(ty, ev);
                    llvm::AtomicCmpXchgInst* ci = builder->CreateAtomicCmpXchg(
                        pv, ev_loaded, nv, llvm::MaybeAlign(),
                        mir_atomic_ordering(constant_bits(*success)),
                        mir_atomic_ordering(constant_bits(*failure)),
                        mir_atomic_scope(scope ? constant_bits(*scope) : 0));
                    if (weak && constant_bits(*weak)) ci->setWeak(true);
                    builder->CreateStore(builder->CreateExtractValue(ci, 0), ev);
                    vals[v] = builder->CreateExtractValue(ci, 1);
                    break;
                }
                case MIROpcode::AtomicRMW: {
                    const MIROperand* op = arg(0);
                    const MIROperand* ptr = arg(1);
                    const MIROperand* value = arg(2);
                    const MIROperand* order = arg(3);
                    const MIROperand* scope = arg(4);
                    llvm::Value* pv = ptr ? atomic_ptr(*ptr) : nullptr;
                    llvm::Value* vv = value ? op_val(*value) : nullptr;
                    if (!op || !pv || !vv || !order) break;
                    const TypeId pointee = pointee_of(*ptr);
                    if (pointee == MIR_INVALID_ID) break;
                    llvm::Type* ty = mir_type(m, pointee);
                    vv = coerce_atomic(vv, ty);
                    vals[v] = builder->CreateAtomicRMW(
                        mir_atomic_rmw_op(constant_bits(*op)), pv, vv, llvm::MaybeAlign(),
                        mir_atomic_ordering(constant_bits(*order)),
                        mir_atomic_scope(scope ? constant_bits(*scope) : 0));
                    break;
                }
                case MIROpcode::AtomicFence: {
                    const MIROperand* order = arg(0);
                    const MIROperand* scope = arg(1);
                    const MIROperand* signal = arg(2);
                    if (!order) break;
                    const bool signal_only = signal && constant_bits(*signal) != 0;
                    const uint64_t bits = constant_bits(*order);
                    if (signal_only) {
                        // a compiler-only fence constrains nothing at relaxed
                        // ordering (matches clang's __atomic_signal_fence)
                        if (bits == 0 || bits == 1 || bits == 2) break;
                        builder->CreateFence(mir_atomic_ordering(bits),
                                             llvm::SyncScope::SingleThread);
                        break;
                    }
                    const llvm::AtomicOrdering mo = mir_atomic_ordering(bits);
                    // LLVM's fence rejects `monotonic`, so a relaxed fence is
                    // upgraded to acquire-release (a stronger, still valid order)
                    builder->CreateFence(
                        mo == llvm::AtomicOrdering::Monotonic
                            ? llvm::AtomicOrdering::AcquireRelease
                            : mo,
                        mir_atomic_scope(scope ? constant_bits(*scope) : 0));
                    break;
                }
                case MIROpcode::Br: {
                    const MIROperand* t = arg(0);
                    if (t) builder->CreateBr(blocks[t->id]);
                    break;
                }
                case MIROpcode::CondBr: {
                    const MIROperand* c = arg(0);
                    const MIROperand* t = arg(1);
                    const MIROperand* f = arg(2);
                    if (c && t && f) {
                        builder->CreateCondBr(as_i1(op_val(*c)), blocks[t->id], blocks[f->id]);
                    }
                    break;
                }
                case MIROpcode::Return: {
                    if (inst.operand_count == 0) {
                        // a fall-through return in a non-void function is
                        // unreachable (all paths returned)
                        if (lfn->getReturnType()->isVoidTy()) builder->CreateRetVoid();
                        else builder->CreateUnreachable();
                    } else {
                        const MIROperand* r = arg(0);
                        llvm::Value* rv = r ? op_val(*r) : nullptr;
                        if (r && r->kind() == MIROperandKind::Place) {
                            rv = builder->CreateLoad(mir_type(m, fn.places[r->id].type), rv);
                        }
                        // the MIR value type need not equal the declared return
                        // type (C converts implicitly); coerce before returning
                        if (rv) {
                            llvm::Type* rt = lfn->getReturnType();
                            llvm::Type* vt = rv->getType();
                            if (rt != vt) {
                                if (rt->isIntegerTy() && vt->isIntegerTy()) {
                                    const bool src_signed =
                                        r->type() < m.types.size() && is_signed_ty(r->type());
                                    rv = builder->CreateIntCast(rv, rt, src_signed);
                                } else if (rt->isFloatingPointTy() && vt->isFloatingPointTy())
                                    rv = builder->CreateFPCast(rv, rt);
                                else if (rt->isPointerTy() && vt->isIntegerTy())
                                    rv = builder->CreateIntToPtr(rv, rt);
                                else if (rt->isIntegerTy() && vt->isPointerTy())
                                    rv = builder->CreatePtrToInt(rv, rt);
                                else if (rt->isPointerTy() && vt->isPointerTy())
                                    rv = builder->CreateBitCast(rv, rt);
                                else if (rt->isIntegerTy() && vt->isFloatingPointTy())
                                    rv = builder->CreateFPToSI(rv, rt);
                                else if (rt->isFloatingPointTy() && vt->isIntegerTy())
                                    rv = builder->CreateSIToFP(rv, rt);
                            }
                        }
                        if (!rv || rv->getType()->isVoidTy()) {
                            if (lfn->getReturnType()->isVoidTy()) builder->CreateRetVoid();
                            else builder->CreateUnreachable();
                        } else {
                            builder->CreateRet(rv);
                        }
                    }
                    break;
                }
                case MIROpcode::Unreachable:
                    builder->CreateUnreachable();
                    break;
                case MIROpcode::DynamicCall: {
                    const MIROperand* fat = arg(0);
                    const MIROperand* vt = arg(1);
                    const MIROperand* meth = arg(2);
                    if (!fat || !vt || !meth) break;
                    std::string vt_name, m_name;
                    if (!const_string(vt->id, vt_name) ||
                        !const_string(meth->id, m_name)) {
                        break;
                    }
                    const TypeId ft = fat->type();
                    // A pointer-like operand type means the value is the *address*
                    // of the fat pointer (the lowerer types an lvalue receiver as
                    // `*dyn I`), so the struct type is the pointee and a place must
                    // be loaded once. This mirrors the C emitter's `arrow` flag
                    // (`base->first` vs `base.first`). GEPing the pointer type
                    // itself is invalid IR ("Invalid indices for GEP pointer
                    // type!") and aborts the module verifier.
                    const bool ft_is_ptr = mir_is_ptr_like(m, ft);
                    llvm::Type* fat_ty = mir_type(m, ft_is_ptr ? m.types.get(ft).element : ft);
                    llvm::Value* fat_ptr = nullptr;
                    if (fat->kind() == MIROperandKind::Place) {
                        fat_ptr = places[fat->id];
                        if (ft_is_ptr) fat_ptr = builder->CreateLoad(builder->getPtrTy(), fat_ptr);
                    } else {
                        fat_ptr = op_val(*fat);
                        // The fat pointer is a by-value struct (`{ first, second }`).
                        // An SSA aggregate is not addressable, so the receiver
                        // fields cannot be GEPed directly (an aggregate GEP base is
                        // invalid IR and crashes instruction selection). Give the
                        // value a stack home inside the entry block first.
                        if (fat_ptr && !fat_ptr->getType()->isPointerTy()) {
                            llvm::BasicBlock& ent = lfn->getEntryBlock();
                            llvm::IRBuilder<> top(&ent, ent.begin());
                            llvm::AllocaInst* slot = top.CreateAlloca(fat_ptr->getType());
                            builder->CreateStore(fat_ptr, slot);
                            fat_ptr = slot;
                        }
                    }
                    if (!fat_ptr) break;
                    llvm::Value* self_ptr = builder->CreateStructGEP(fat_ty, fat_ptr, 0);
                    llvm::Value* self = builder->CreateLoad(builder->getPtrTy(), self_ptr);
                    llvm::Value* vtbl_ptr = builder->CreateStructGEP(fat_ty, fat_ptr, 1);
                    llvm::Value* vtbl = builder->CreateLoad(builder->getPtrTy(), vtbl_ptr);
                    llvm::StructType* vts = mir_vtable_struct(vt_name, {});
                    uint32_t midx = 0;
                    if (!mir_vtable_method_index(vt_name, m_name, midx)) break;
                    llvm::FunctionType* slot_ty = mir_vtable_slot_type(vt_name, midx);
                    if (!slot_ty) break;
                    llvm::Value* slot = builder->CreateStructGEP(vts, vtbl, midx);
                    llvm::Value* fnp = builder->CreateLoad(builder->getPtrTy(), slot);
                    std::vector<llvm::Value*> args;
                    std::vector<TypeId> arg_types;
                    args.push_back(self);
                    arg_types.push_back(MIR_INVALID_ID);
                    for (uint32_t a = 3; a < inst.operand_count; ++a) {
                        const MIROperand* o = arg(a);
                        if (o) {
                            args.push_back(op_val(*o));
                            arg_types.push_back(o->type());
                        }
                    }
                    // the slot's signature is authoritative (see `Init`)
                    coerce_args(args, slot_ty, &arg_types);
                    llvm::CallInst* ci = builder->CreateCall(slot_ty, fnp, args);
                    if (v < fn.values.size() && !mir_is_void(m, fn.values[v].type)) {
                        vals[v] = ci;
                    }
                    break;
                }
                default:
                    // Not yet lowered by the MIR LLVM emitter. Emit poison so the
                    // function still verifies; the driver reports the gap.
                    if (getenv("MIR_LLVM_TRACE"))
                        std::cerr << "[MIR-LLVM] UNHANDLED "
                                  << mir::opcode_name(inst.opcode()) << " in "
                                  << std::string(lfn->getName()) << "\n";
                    if (v < fn.values.size() && fn.values[v].type != MIR_INVALID_ID &&
                        !mir_is_void(m, fn.values[v].type)) {
                        vals[v] = llvm::PoisonValue::get(mir_type(m, fn.values[v].type));
                    } else {
                        vals[v] = llvm::PoisonValue::get(builder->getInt8Ty());
                    }
                    break;
            }
        }
        // every LLVM block must end in a terminator; MIR blocks that fall
        // through to nothing (e.g. an unreachable switch merge) get `unreachable`
        if (blocks[bi] && !blocks[bi]->getTerminator()) {
            builder->SetInsertPoint(blocks[bi]);
            builder->CreateUnreachable();
        }
    }
    return true;
}
