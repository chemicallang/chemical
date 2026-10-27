// Copyright (c) Chemical Language Foundation 2026.
//
// MIR → LLVM lowering. This is the MIR-driven replacement for the AST-based
// LLVM backend: it consumes the canonical MIR module (types + symbols +
// functions) and emits LLVM IR. No AST virtual dispatch is used.
//
// The emitter mirrors the C backend (`MIREmitter.cpp`) opcode-for-opcode, but
// produces LLVM values instead of C text. Type layout comes from the MIR type
// table (aggregate field lists and their byte offsets/size, filled by
// `MIRTypeBuilder`): the LLVM aggregate body is built to match MIR's recorded
// layout, so `sizeof`/`alignof`/`offsetof` and every field address agree with
// the C backend and the interpreter.

#include "Codegen.h"
#include "compiler/llvmimpl.h"

#include "compiler/mir/MIRModule.h"
#include "compiler/mir/MIRABI.h"
#include "compiler/mir/MIRInstruction.h"
#include "compiler/mir/MIRFunction.h"
#include "compiler/mir/MIRLayout.h"
#include "compiler/mir/MIRTraversal.h"
#include "compiler/mir/MIRDump.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/Alignment.h>

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <string>
#include <vector>

using namespace mir;

namespace {

// ── debug switches ─────────────────────────────────────────────────────────
// `MIR_LLVM_TRACE` / `MIR_LLVM_DUMP` are development aids. They are read once
// rather than per instruction: the traced loop runs once per MIR instruction
// (hundreds of thousands per suite compile), where a `getenv` per instruction —
// a linear scan of `environ` — is a real cost for a facility that is almost
// always off. Set them on the compiler command line, as every use does.
const bool kMirLlvmTrace = std::getenv("MIR_LLVM_TRACE") != nullptr;
const bool kMirLlvmDump = std::getenv("MIR_LLVM_DUMP") != nullptr;

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

            // Build the body from the access layout MIR publishes (see
            // `mir::compute_aggregate_plan`): each MIR field sits at its MIR byte
            // offset (padding where the offset exceeds the running end, so an
            // explicit `@align`/`@maxalign` is honored), union members and
            // variant cases share one storage element per overlap group (a
            // struct cannot have two fields at one offset; opaque pointers make
            // the group's field type irrelevant at the GEP), and the whole type
            // is sized to MIR's `r.size`. An aggregate with no explicit
            // alignment and no overlap produces an element sequence identical to
            // the field list, so the common case is unchanged.
            mir::MIRAggregatePlan plan;
            compute_aggregate_plan(m.types, id, plan);
            std::vector<llvm::Type*> elems;
            elems.reserve(plan.elements.size());
            for (const mir::MIRAggElement& e : plan.elements) {
                if (e.first_field == MIR_INVALID_ID) {
                    elems.push_back(
                        llvm::ArrayType::get(builder->getInt8Ty(), e.pad_bytes));
                } else {
                    elems.push_back(mir_type(m, e.rep_type));
                }
            }
            st->setBody(elems);
            mir_agg_plans_[id] = std::move(plan);
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

uint32_t Codegen::mir_field_elem(uint32_t agg, uint32_t field) const {
    auto it = mir_agg_plans_.find(agg);
    if (it == mir_agg_plans_.end() || field >= it->second.field_elem.size()) return field;
    return it->second.field_elem[field];
}

// ── Function declaration ────────────────────────────────────────────────────
llvm::FunctionType* Codegen::mir_fn_type(const mir::MIRModule& m, uint32_t ft) {
    if (ft == MIR_INVALID_ID || ft >= m.types.size() ||
        m.types.get(ft).kind != MIRTypeKind::Function) {
        return nullptr;
    }
    const MIRTypeRecord& fr = m.types.get(ft);
    const bool sret = abi::needs_sret(m, fr.element);
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
    const std::string name = m.symbols.mangled_name(rec);

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
    // The unwind policy the AST backend applied to every function it created
    // (`create_func`). A debug / debug_complete build keeps unwind tables so a
    // debugger can walk the stack (that is what the `-bt` test flag needs),
    // unless the user passed `-fno-unwind-tables`; every other build asserts the
    // function cannot unwind — Chemical has no exceptions and MIR never emits
    // `invoke` / `landingpad`.
    if (is_debug_or_compl(mode) && !options.fno_unwind_tables) {
        fn->addFnAttr(llvm::Attribute::UWTable);
    } else {
        fn->addFnAttr(llvm::Attribute::NoUnwind);
    }
    mir_fn_cache_[symbol] = fn;
    return fn;
}

void Codegen::mir_declare_globals(const mir::MIRModule& m) {
    for (uint32_t i = 0; i < m.symbols.size(); ++i) {
        const MIRSymbolRecord& rec = m.symbols.get(i);
        if (rec.kind != MIRSymbolKind::Global && rec.kind != MIRSymbolKind::External) continue;
        if (rec.type == MIR_INVALID_ID || rec.type >= m.types.size()) continue;
        const std::string name = m.symbols.mangled_name(rec);
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
        const uint32_t galign = (rec.type < m.types.size())
                                    ? m.types.get(rec.type).alignment : 0;
        if (galign > 1) gv->setAlignment(llvm::Align(galign));
        mir_global_cache_[i] = gv;
    }
}

// ── Debug info types ────────────────────────────────────────────────────────
llvm::DIType* Codegen::mir_di_type(const mir::MIRModule& m, uint32_t tid) {
    if (!di.isEnabled || !diBuilder || tid >= m.types.size()) return nullptr;
    auto cached = mir_di_types_.find(tid);
    if (cached != mir_di_types_.end()) return cached->second;
    // Insert a sentinel so a self-referential aggregate cannot recurse forever;
    // a recursive lookup resolves to nullptr and the member is skipped.
    mir_di_types_[tid] = nullptr;
    const mir::MIRTypeRecord& r = m.types.get(tid);
    llvm::DIFile* file = mir_di_file_;
    llvm::DIType* result = nullptr;
    auto unspecified = [&]() -> llvm::DIType* {
        return diBuilder->createUnspecifiedType("void");
    };
    switch (r.kind) {
        case mir::MIRTypeKind::Bool:
            result = diBuilder->createBasicType("bool", 8, llvm::dwarf::DW_ATE_boolean);
            break;
        case mir::MIRTypeKind::Int: {
            const bool sig = (r.flags & mir::TF_SIGNED) != 0;
            std::string name = m.types.name_of(tid);
            if (name.empty()) name = sig ? "int" : "uint";
            result = diBuilder->createBasicType(
                name, r.size ? r.size * 8 : 32,
                sig ? llvm::dwarf::DW_ATE_signed : llvm::dwarf::DW_ATE_unsigned);
            break;
        }
        case mir::MIRTypeKind::Float:
            result = diBuilder->createBasicType(r.size == 8 ? "double" : "float",
                                                r.size ? r.size * 8 : 32,
                                                llvm::dwarf::DW_ATE_float);
            break;
        case mir::MIRTypeKind::Pointer:
        case mir::MIRTypeKind::Reference: {
            llvm::DIType* pointee = mir_di_type(m, r.element);
            result = diBuilder->createPointerType(pointee ? pointee : unspecified(),
                                                  r.size ? r.size * 8 : 64);
            break;
        }
        case mir::MIRTypeKind::Array: {
            llvm::DIType* elem = mir_di_type(m, r.element);
            if (elem) {
                std::vector<llvm::Metadata*> subs;
                subs.push_back(diBuilder->getOrCreateSubrange(0, r.data_count));
                result = diBuilder->createArrayType(r.size * 8, r.alignment * 8, elem,
                                                    diBuilder->getOrCreateArray(subs));
            }
            break;
        }
        case mir::MIRTypeKind::Struct:
        case mir::MIRTypeKind::Union:
        case mir::MIRTypeKind::Variant: {
            std::string nm = m.types.name_of(tid);
            if (nm.empty()) nm = "__chx_anon";
            std::vector<llvm::Metadata*> elems;
            if (r.field_index != mir::MIR_INVALID_ID) {
                for (uint32_t i = 0; i < r.data_count; ++i) {
                    const uint32_t fi = r.field_index + i;
                    if (fi >= m.types.fields.size()) break;
                    const mir::MIRFieldInfo& f = m.types.fields[fi];
                    llvm::DIType* ft = mir_di_type(m, f.type);
                    if (!ft) continue;
                    const uint64_t fsize =
                        (f.type < m.types.size() ? m.types.get(f.type).size : 0) * 8;
                    std::string fname(m.types.field_names.data_at(f.name_offset),
                                      f.name_length);
                    elems.push_back(diBuilder->createMemberType(
                        file, fname, file, 0, fsize, f.align * 8, f.offset * 8,
                        llvm::DINode::FlagZero, ft));
                }
            }
            result = diBuilder->createStructType(
                file, nm, file, 0, r.size * 8, r.alignment * 8, llvm::DINode::FlagZero,
                nullptr, diBuilder->getOrCreateArray(elems));
            break;
        }
        default:
            result = unspecified();
            break;
    }
    mir_di_types_[tid] = result;
    return result;
}

// ── Function body ───────────────────────────────────────────────────────────
bool Codegen::mir_emit_function(const mir::MIRModule& m, const mir::MIRFunction& fn) {
    llvm::Function* lfn = mir_function(m, fn.symbol);
    if (!lfn) return false;
    if (!lfn->empty()) return true;
    if (kMirLlvmTrace)
        std::cerr << "[MIR-LLVM] emit " << std::string(lfn->getName()) << "\n";
    if (kMirLlvmDump) {
        std::cerr << "==== " << std::string(lfn->getName()) << " ====\n";
        mir::dump_function(fn, m, std::cerr);
    }

    const bool sret = abi::needs_sret(m, m.types.get(fn.function_type).element);

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

    // The coroutine frame pointer place (`__chx__af`). A `FrameField` place has
    // no stack slot: it is a GEP from this pointer, resolved lazily by
    // `place_ptr` below.
    int frame_place = -1;
    llvm::Value* frame_ptr_value = nullptr;
    // Places that carry a source name, for the `llvm.dbg.declare` calls below.
    // Populated only in a debug build (the lowerer records `debug_name` only
    // when `options->debug_info` is on).
    std::vector<std::pair<uint32_t, llvm::AllocaInst*>> named_places;
    // Where the function's real body starts: right after the allocas. The
    // `dbg.declare` intrinsics go here (a `dbg.declare` may not precede the slot
    // it describes, and must dominate every use of it).
    llvm::BasicBlock::iterator body_begin = entry->end();
    // Spill every non-frame-field place as an alloca in the entry block. A
    // `FramePtr` place *is* a real slot (the frame pointer is stored into it at
    // entry), so it must be allocated too.
    {
        llvm::IRBuilder<> top(entry, entry->begin());
        for (uint32_t p = 0; p < fn.places.size(); ++p) {
            const uint8_t sc = fn.places[p].storage_class;
            if (sc == static_cast<uint8_t>(MIRStorageClass::FrameField)) continue;
            if (sc == static_cast<uint8_t>(MIRStorageClass::FramePtr) && frame_place < 0) {
                frame_place = static_cast<int>(p);
            }
            llvm::AllocaInst* slot = top.CreateAlloca(mir_type(m, fn.places[p].type));
            // MIR owns the alignment: honor an explicit `@align`/`@maxalign` the
            // LLVM type cannot express (a struct's ABI alignment is derived from
            // its members and cannot be raised).
            const uint32_t palign = (fn.places[p].type < m.types.size())
                                        ? m.types.get(fn.places[p].type).alignment
                                        : 0;
            if (palign > 1) slot->setAlignment(llvm::Align(palign));
            places[p] = slot;
            if (fn.places[p].debug_name != mir::MIR_INVALID_ID) {
                named_places.emplace_back(p, slot);
            }
        }
        body_begin = top.GetInsertPoint();
    }
    builder->SetInsertPoint(entry);

    // Debug info: a DISubprogram per function, and a default DILocation for the
    // prologue (each MIR instruction refines it below). Emission is fully
    // MIR-driven — the legacy AST path set these via `DebugInfoBuilder`.
    mir_di_sp_ = nullptr;
    if (di.isEnabled && diBuilder) {
        llvm::DIFile* file = mir_di_file_;
        if (fn.has_decl_loc && fn.decl_loc.file < m.source_files.size()) {
            const std::string& path = m.source_files[fn.decl_loc.file];
            auto it = mir_di_files_.find(path);
            if (it != mir_di_files_.end()) {
                file = it->second;
            } else {
                const std::size_t slash = path.find_last_of("/\\");
                const std::string dir =
                    slash == std::string::npos ? std::string() : path.substr(0, slash);
                const std::string base =
                    slash == std::string::npos ? path : path.substr(slash + 1);
                file = diBuilder->createFile(base, dir);
                mir_di_files_[path] = file;
            }
        }
        if (file) {
            const unsigned line = fn.has_decl_loc ? fn.decl_loc.line + 1 : 1;
            // subroutine type: return type first, then the parameter types
            std::vector<llvm::Metadata*> di_params;
            if (fn.function_type < m.types.size() &&
                m.types.get(fn.function_type).kind == mir::MIRTypeKind::Function) {
                const mir::MIRTypeRecord& ftr = m.types.get(fn.function_type);
                llvm::DIType* rt = mir_di_type(m, ftr.element);
                di_params.push_back(rt ? rt : diBuilder->createUnspecifiedType("void"));
                for (uint32_t i = 0; i < ftr.data_count; ++i) {
                    llvm::DIType* pt = mir_di_type(m, m.types.data[ftr.data_offset + i]);
                    di_params.push_back(pt ? pt : diBuilder->createUnspecifiedType("void"));
                }
            } else {
                di_params.push_back(diBuilder->createUnspecifiedType("void"));
            }
            llvm::DISubroutineType* sty =
                diBuilder->createSubroutineType(diBuilder->getOrCreateTypeArray(di_params));
            llvm::DISubprogram* sp = diBuilder->createFunction(
                file, std::string(lfn->getName()), std::string(lfn->getName()), file, line,
                sty, line, llvm::DINode::FlagPrototyped,
                llvm::DISubprogram::SPFlagDefinition);
            lfn->setSubprogram(sp);
            mir_di_sp_ = sp;
            builder->SetCurrentDebugLocation(llvm::DILocation::get(*ctx, line, 1, sp));
        }
    }

    // Debug info: one `DILexicalBlock` per MIR lexical scope, so DWARF scopes a
    // variable's extent the way the source did instead of flattening every local
    // onto the function. A scope is pushed before its children (`lexical_scopes`
    // is a LIFO stack), so a forward pass creates parents first; the function
    // scope is the `DISubprogram` itself.
    std::vector<llvm::DILocalScope*> di_lexical(fn.lexical_scopes.size(), nullptr);
    if (mir_di_sp_) {
        llvm::DIFile* file = mir_di_sp_->getFile();
        for (uint32_t s = 0; s < fn.lexical_scopes.size(); ++s) {
            const mir::MIRLexicalScope& sc = fn.lexical_scopes[s];
            if (sc.inst_begin == mir::MIR_INVALID_ID || !file) break;
            llvm::DILocalScope* parent = mir_di_sp_;
            if (sc.parent != mir::MIR_INVALID_ID && sc.parent < di_lexical.size() &&
                di_lexical[sc.parent]) {
                parent = di_lexical[sc.parent];
            }
            // The scope's own source location; the function's declaration line
            // when the scope opens before any source text is known.
            unsigned line = mir_di_sp_->getLine();
            unsigned column = 1;
            if (sc.has_loc) {
                line = sc.loc.line + 1;
                column = sc.loc.column + 1;
            }
            di_lexical[s] = diBuilder->createLexicalBlock(parent, file, line, column);
        }
    }

    // Debug info: a `DILocalVariable` for every named place, `dbg.declare`d on
    // its stack slot. MIR keeps every local in memory (an `alloca` plus explicit
    // load/store — there is no SSA-local promotion in either backend), so
    // `dbg.declare` is the correct intrinsic here: it describes a memory
    // location, exactly like the legacy AST backend's `DebugInfoBuilder::declare`.
    // `AlwaysPreserve` keeps them through `DIBuilder::finalize`.
    if (mir_di_sp_ && !named_places.empty()) {
        llvm::DIFile* file = mir_di_sp_->getFile();
        const unsigned fn_line = mir_di_sp_->getLine();
        llvm::DIExpression* expr = diBuilder->createExpression();
        for (const auto& np : named_places) {
            const mir::MIRPlaceDef& pd = fn.places[np.first];
            const uint32_t cid = pd.debug_name;
            if (cid >= m.constants.size()) continue;
            const MIRConstant& c = m.constants.get(cid);
            if (c.kind != MIRConstantKind::String || c.data_count == 0) continue;
            std::string name(m.constants.data.text(c.data_offset));
            llvm::DIType* dt = mir_di_type(m, pd.type);
            // The scope the place was declared in (a lexical block), so the
            // variable scopes the way the source did and LLVM keeps the block.
            llvm::DILocalScope* var_scope = mir_di_sp_;
            if (pd.debug_scope != mir::MIR_INVALID_ID &&
                pd.debug_scope < di_lexical.size() && di_lexical[pd.debug_scope]) {
                var_scope = di_lexical[pd.debug_scope];
            }
            llvm::DILocalVariable* var = diBuilder->createAutoVariable(
                var_scope, name, file, fn_line,
                dt ? dt : diBuilder->createUnspecifiedType("void"),
                /*AlwaysPreserve=*/true);
            diBuilder->insertDeclare(np.second, var, expr,
                                     llvm::DILocation::get(*ctx, fn_line, 1, var_scope),
                                     llvm::InsertPosition(body_begin));
        }
    }

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
                std::string s(m.constants.data.text(c.data_offset));
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
        out.assign(m.constants.data.text(c.data_offset));
        return true;
    };

    auto field_index = [&](TypeId agg, const std::string& name) -> int {
        if (agg >= m.types.size()) return -1;
        const MIRTypeRecord& r = m.types.get(agg);
        for (uint32_t i = 0; i < r.data_count; ++i) {
            const MIRFieldInfo* f = m.types.field_of(agg, i);
            if (!f) continue;
            if (f->name_length != name.size()) continue;
            if (std::memcmp(m.types.field_names.data_at(f->name_offset), name.data(),
                            name.size()) == 0) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };
    // Translate a MIR field index into the LLVM struct-element index. They
    // differ whenever the aggregate's access layout has a padding element
    // (alignment or explicit `@align`), which shifts every later field. `mir_type`
    // must publish the plan first; argument evaluation order is unspecified, so
    // do it here rather than relying on the caller's other arguments.
    auto field_elem = [&](TypeId agg, int fi) -> uint32_t {
        if (fi < 0) return 0;
        (void)mir_type(m, agg);
        return mir_field_elem(agg, static_cast<uint32_t>(fi));
    };

    // Resolve a (possibly dotted) field path into a chain of struct field
    // indices. Variant payload fields are spelled `Case.param`; an inherited
    // member is spelled with the full base chain (`Base.Mid.Leaf.field`, see
    // `MIRLowerer::inherited_field_path`), so the walk is over every segment.
    // Each segment names a field of the type reached so far, which is exactly
    // how the C emitter spells the access (`base.Base.Mid.Leaf.field`).
    auto field_path = [&](TypeId agg, const std::string& name,
                          std::vector<uint32_t>& indices, TypeId& out_type) -> bool {
        TypeId cur = agg;
        size_t pos = 0;
        while (true) {
            if (cur == MIR_INVALID_ID || cur >= m.types.size()) return false;
            const size_t dot = name.find('.', pos);
            const std::string seg = (dot == std::string::npos)
                                        ? name.substr(pos)
                                        : name.substr(pos, dot - pos);
            if (seg.empty()) return false;
            const int fi = field_index(cur, seg);
            if (fi < 0) return false;
            indices.push_back(static_cast<uint32_t>(fi));
            cur = m.types.field_of(cur, fi)->type;
            if (dot == std::string::npos) break;
            pos = dot + 1;
        }
        out_type = cur;
        return true;
    };
    // The index of a variant's discriminator field (`__chx__vt_621827`).
    auto variant_tag_index = [&](TypeId agg) -> int {
        if (agg >= m.types.size()) return -1;
        const MIRTypeRecord& r = m.types.get(agg);
        for (uint32_t i = 0; i < r.data_count; ++i) {
            const MIRFieldInfo* f = m.types.field_of(agg, i);
            if (f && f->kind == static_cast<uint8_t>(MIRFieldKind::VariantTag)) {
                return static_cast<int>(i);
            }
        }
        return field_index(agg, "__chx__vt_621827");
    };

    auto gep_path = [&](llvm::Value* base_ptr, TypeId agg,
                        const std::vector<uint32_t>& indices) -> llvm::Value* {
        llvm::Value* ptr = base_ptr;
        TypeId cur = agg;
        for (uint32_t idx : indices) {
            // Sequence explicitly: `mir_type` publishes the field->element plan
            // that `mir_field_elem` reads, so it must run first (argument
            // evaluation order is unspecified).
            llvm::Type* agg_ty = mir_type(m, cur);
            const uint32_t elem = mir_field_elem(cur, idx);
            ptr = builder->CreateStructGEP(agg_ty, ptr, elem);
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

    // A `FrameField` place (`__chx__af-><field>`) is a GEP from the coroutine
    // frame pointer, not a stack slot. Resolve it lazily at first use so the GEP
    // lands at that use's insertion point (which the frame-pointer store
    // dominates).
    // `&<name>` for an async function's `FutureTable<T>` global. The vtable is
    // emitted as C text for the C backend, so the LLVM emitter has to build it
    // here: `{ &<stem>__poll, &<stem>__drop }`.
    auto async_vtable = [&](const std::string& name, TypeId table_ty) -> llvm::Value* {
        if (llvm::GlobalVariable* existing = module->getGlobalVariable(name)) {
            return existing;
        }
        if (table_ty == MIR_INVALID_ID || table_ty >= m.types.size()) return nullptr;
        auto* st = llvm::dyn_cast<llvm::StructType>(mir_type(m, table_ty));
        if (!st) return nullptr;
        std::string stem = name;
        if (stem.size() >= 6 && stem.compare(stem.size() - 6, 6, "__vtbl") == 0) {
            stem.erase(stem.size() - 6);
        }
        std::vector<llvm::Constant*> init;
        const MIRTypeRecord& tr = m.types.get(table_ty);
        for (uint32_t i = 0; i < tr.data_count; ++i) {
            const MIRFieldInfo* f = m.types.field_of(table_ty, i);
            const std::string fname = f ? m.types.field_name_of(*f) : std::string();
            llvm::Constant* c = nullptr;
            if (fname == "poll") {
                c = module->getFunction(stem + "__poll");
            } else if (fname == "drop") {
                c = module->getFunction(stem + "__drop");
            }
            init.push_back(c ? c : llvm::Constant::getNullValue(builder->getPtrTy()));
        }
        if (init.size() != st->getNumElements()) return nullptr;
        auto* gv = new llvm::GlobalVariable(*module, st, /*isConstant=*/true,
                                            llvm::GlobalValue::InternalLinkage,
                                            llvm::ConstantStruct::get(st, init), name);
        return gv;
    };

    auto place_ptr = [&](PlaceId p) -> llvm::Value* {
        if (p >= places.size()) return nullptr;
        if (places[p]) return places[p];
        if (p >= fn.places.size()) return nullptr;
        const MIRPlaceDef& pd = fn.places[p];
        if (pd.storage_class != static_cast<uint8_t>(MIRStorageClass::FrameField)) {
            return nullptr;
        }
        std::string fname;
        if (frame_place < 0 || pd.frame_field == MIR_INVALID_ID ||
            !const_string(pd.frame_field, fname)) {
            return nullptr;
        }
        const TypeId fptr_ty = fn.places[frame_place].type;
        if (fptr_ty >= m.types.size()) return nullptr;
        const TypeId agg = m.types.get(fptr_ty).element;
        if (agg >= m.types.size()) return nullptr;
        const int fi = field_index(agg, fname);
        if (fi < 0) {
            if (kMirLlvmTrace)
                std::cerr << "[MIR-LLVM] in " << std::string(lfn->getName())
                          << ": frame field lookup failed agg=" << agg << " name="
                          << m.types.name_of(agg) << " field=" << fname << "\n";
            return nullptr;
        }
        // Insert the frame-pointer load and the GEP in the *entry* block: a frame
        // field is used from several sibling blocks of the state machine, so a
        // use-site GEP would not dominate all uses. The entry block's terminator
        // already follows the frame-pointer store by the time the first field is
        // touched; if it does not exist yet we are still emitting into the entry
        // block, so the current insertion point is equivalent.
        llvm::Instruction* term = entry ? entry->getTerminator() : nullptr;
        if (!frame_ptr_value) {
            if (term) {
                llvm::IRBuilder<> top(term);
                frame_ptr_value =
                    top.CreateLoad(builder->getPtrTy(), places[frame_place]);
            } else {
                frame_ptr_value =
                    builder->CreateLoad(builder->getPtrTy(), places[frame_place]);
            }
        }
        llvm::Value* gep = nullptr;
        if (term) {
            llvm::IRBuilder<> top(term);
            gep = top.CreateStructGEP(mir_type(m, agg), frame_ptr_value,
                                      field_elem(agg, fi));
        } else {
            gep = builder->CreateStructGEP(mir_type(m, agg), frame_ptr_value,
                                           field_elem(agg, fi));
        }
        places[p] = gep;
        return gep;
    };

    std::function<llvm::Value*(const MIROperand&)> op_val =
        [&](const MIROperand& op) -> llvm::Value* {
        switch (op.kind()) {
            case MIROperandKind::Value: return vals[op.id];
            case MIROperandKind::Place: return place_ptr(op.id);
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
            return is_address_value(op.id) || abi::is_pointer_like(m, fn.values[op.id].type);
        }
        return abi::is_pointer_like(m, op.type());
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
        if (!abi::is_pointer_like(m, t)) return MIR_INVALID_ID;
        return m.types.get(t).element;
    };

    // Load a pointer operand: an SSA pointer value, or the pointer held in a
    // place (the lowerer normalizes to a value, but be defensive).
    auto atomic_ptr = [&](const MIROperand& op) -> llvm::Value* {
        if (op.kind() == MIROperandKind::Place) {
            const TypeId pt = fn.places[op.id].type;
            return builder->CreateLoad(mir_type(m, pt), place_ptr(op.id));
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
        if (!kMirLlvmTrace) return;
        const TypeId agg = (bt < m.types.size() && abi::is_pointer_like(m, bt))
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
            llvm::Value* bp = place_ptr(base.id);
            TypeId bt = fn.places[base.id].type;
            if (!bp) {
                // Frame places have no entry-block alloca (they live in the
                // coroutine frame and are reached through the frame pointer); a
                // selector on one is a lowering bug. Report it instead of
                // building a GEP on null.
                if (kMirLlvmTrace) {
                    std::cerr << "[MIR-LLVM] addr base place " << base.id
                              << " has no alloca (storage="
                              << (int)fn.places[base.id].storage_class << ") in "
                              << std::string(lfn->getName()) << "\n";
                }
                return nullptr;
            }
            if (!selector) {
                if (out_pointee) *out_pointee = bt;
                return bp;
            }
            // a pointer/reference place holds the pointer; load it before GEP
            const bool ptr_like = abi::is_pointer_like(m, bt);
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
                if (!idx) return nullptr;
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
                if (kMirLlvmTrace) {
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
                if (kMirLlvmTrace) {
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
            // C converts any scalar to `_Bool` as `!= 0`, not with a truncation.
            if (want->isIntegerTy(1)) {
                args[i] = as_i1(args[i]);
                continue;
            }
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
        const mir::MIRBlockRange range = mir::block_range(fn, blk);
        for (uint32_t ii = range.begin; ii < range.end; ++ii) {
            const MIRInstruction& inst = fn.instructions[ii];
            auto arg = [&](uint32_t n) -> const MIROperand* {
                return mir::operand_at(fn, inst, n);
            };
            const uint32_t v = inst.result_or_place;
            // Attach the instruction's MIR source location (when present),
            // scoped to the innermost lexical block covering it.
            if (mir_di_sp_) {
                llvm::DILocalScope* scope = mir_di_sp_;
                const uint32_t ls = fn.lexical_scope_of_inst(ii);
                if (ls != mir::MIR_INVALID_ID && ls < di_lexical.size() && di_lexical[ls]) {
                    scope = di_lexical[ls];
                }
                if (inst.source_index < fn.sources.size()) {
                    const mir::MIRSourceLoc& sl = fn.sources[inst.source_index];
                    builder->SetCurrentDebugLocation(
                        llvm::DILocation::get(*ctx, sl.line + 1, sl.column + 1, scope));
                } else {
                    builder->SetCurrentDebugLocation(
                        llvm::DILocation::get(*ctx, mir_di_sp_->getLine(), 1, scope));
                }
            }
            if (kMirLlvmTrace)
                std::cerr << "  bi=" << bi << " ii=" << ii << " "
                          << mir::opcode_name(inst.opcode()) << " res=" << v
                          << " n=" << inst.operand_count << "\n";
            switch (inst.opcode()) {
                case MIROpcode::Nop:
                    break;
                case MIROpcode::Alloca: {
                    // No unconditional zeroing here: the lowerer emits an
                    // explicit `MemSet` (`zero_init`) at exactly the sites that
                    // need well-defined tail bytes (a partial aggregate / array
                    // initializer, a struct temp with unset members, a struct's
                    // defaulted fields). Zeroing every aggregate alloca in
                    // addition duplicated that work (see evaluation §1.2); the
                    // C emitter was updated to match.
                    break;
                }
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
                            const std::string nm = m.symbols.mangled_name(rec);
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
                        if (kMirLlvmTrace)
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
                                     ? builder->CreateLoad(mir_type(m, sr.element), place_ptr(src->id))
                                     : builder->CreateLoad(mir_type(m, fn.places[src->id].type),
                                                           place_ptr(src->id));
                        }
                        // MIR keeps the source type on the value (C assigns with
                        // an implicit conversion); LLVM needs an exact match, so
                        // a wider value stored into a narrower place would
                        // otherwise clobber adjacent stack slots.
                        if (sv && dst_ty != MIR_INVALID_ID && dst_ty < m.types.size() &&
                            !abi::is_pointer_like(m, dst_ty)) {
                            llvm::Type* want = mir_type(m, dst_ty);
                            if (sv->getType() != want && !want->isAggregateType() &&
                                !sv->getType()->isAggregateType()) {
                                // C converts any scalar to `_Bool` as `!= 0`; a
                                // `trunc` would keep only the low bit (and drop
                                // e.g. `isdigit()`'s 4).
                                if (want->isIntegerTy(1))
                                    sv = as_i1(sv);
                                else if (sv->getType()->isIntegerTy() && want->isIntegerTy())
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
                    } else if (kMirLlvmTrace) {
                        TypeId bt = (b->kind() == MIROperandKind::Place)
                                        ? fn.places[b->id].type
                                        : fn.values[b->id].type;
                        TypeId agg = abi::is_pointer_like(m, bt) ? m.types.get(bt).element : bt;
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
                        if (rt->isIntegerTy() && rt != vals[v]->getType()) {
                            // `!x` is C's `int`, so the lowered i1 result is
                            // *widened* to the MIR value's type — never truncated.
                            if (vals[v]->getType()->getIntegerBitWidth() >
                                rt->getIntegerBitWidth()) {
                                vals[v] = builder->CreateTrunc(vals[v], rt);
                            } else {
                                vals[v] = builder->CreateZExt(vals[v], rt);
                            }
                        }
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
                    // `x as bool` is C's `x != 0` (not a truncation): the low bit
                    // of a non-0/1 integer is not its truthiness.
                    if (to->isIntegerTy(1)) {
                        vals[v] = as_i1(av);
                    } else if (av->getType()->isIntegerTy() && to->isIntegerTy()) {
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
                        // MIR's recorded size, not LLVM's: the C backend spells
                        // `sizeof(T)` and MIR's layout is verified against it, so
                        // this keeps the two backends and the interpreter in
                        // agreement (including `@align`/`@maxalign` padding).
                        const uint64_t sz =
                            (t->type() < m.types.size()) ? m.types.get(t->type()).size : 0;
                        vals[v] = llvm::ConstantInt::get(mir_type(m, fn.values[v].type), sz);
                    }
                    break;
                }
                case MIROpcode::AlignOf: {
                    const MIROperand* t = arg(0);
                    if (t) {
                        const uint64_t al =
                            (t->type() < m.types.size()) ? m.types.get(t->type()).alignment : 1;
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
                    // MIR's recorded field offset (what the C `offsetof(T, f)`
                    // the declaration compiles to produces).
                    const MIRFieldInfo* fi_rec =
                        (fi >= 0) ? m.types.field_of(t->type(), static_cast<uint32_t>(fi))
                                  : nullptr;
                    const uint64_t off = fi_rec ? fi_rec->offset : 0;
                    vals[v] = llvm::ConstantInt::get(mir_type(m, fn.values[v].type), off);
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
                        args.push_back(place_ptr(maybe_place->id));
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
                            if (pr.kind == MIRTypeKind::Array || abi::is_aggregate(m, fn.places[o->id].type)) {
                                av = place_ptr(o->id);
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
                    if (!call_sret && v < fn.values.size() && !abi::is_void(m, fn.values[v].type)) {
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
                    args.push_back(place_ptr(dest->id));
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
                    llvm::Value* dv = place_ptr(dest->id);
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
                    // Mirror C's implicit conversion to `_Bool` (`!= 0`).
                    if (sv && dest->id < fn.places.size() &&
                        fn.places[dest->id].type < m.types.size() &&
                        m.types.get(fn.places[dest->id].type).kind == MIRTypeKind::Bool &&
                        sv->getType()->isIntegerTy() && !sv->getType()->isIntegerTy(1))
                        sv = as_i1(sv);
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
                            place_ptr(p->id));
                    }
                    break;
                }
                case MIROpcode::MoveOut:
                    // dataflow marker only: no runtime effect
                    break;
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
                        llvm::Value* base = place_ptr(p->id);
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
                    if (d->kind() == MIROperandKind::Place) dv = place_ptr(d->id);
                    if (s->kind() == MIROperandKind::Place) sv = place_ptr(s->id);
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
                    llvm::Value* dv = (d->kind() == MIROperandKind::Place) ? place_ptr(d->id)
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
                case MIROpcode::AsyncFrameAlloc: {
                    const MIROperand* frame_ty = arg(0);
                    if (!frame_ty) break;
                    llvm::Type* ft = mir_type(m, frame_ty->type());
                    if (!ft->isSized()) break;
                    llvm::FunctionCallee alloc = module->getOrInsertFunction(
                        "chemical_async_frame_alloc",
                        llvm::FunctionType::get(
                            builder->getPtrTy(),
                            {builder->getInt64Ty(), builder->getInt64Ty()}, false));
                    const llvm::DataLayout& dl = module->getDataLayout();
                    llvm::Value* sz = builder->getInt64(dl.getTypeAllocSize(ft));
                    llvm::Value* al =
                        builder->getInt64(dl.getABITypeAlign(ft).value());
                    vals[v] = builder->CreateCall(alloc, {sz, al});
                    break;
                }
                case MIROpcode::AsyncFinish: {
                    // `frame->__state = DONE; *ret = Poll{ Ready(frame->__result) };`
                    const MIROperand* fr = arg(0);
                    const MIROperand* ret = arg(1);
                    const MIROperand* poll_ty_op = arg(2);
                    if (!fr || !ret || !poll_ty_op) break;
                    llvm::Value* fslot = place_ptr(fr->id);
                    llvm::Value* rslot = place_ptr(ret->id);
                    if (!fslot || !rslot) break;
                    llvm::Value* frp = builder->CreateLoad(builder->getPtrTy(), fslot);
                    const TypeId fpty = fn.places[fr->id].type;
                    if (fpty >= m.types.size()) break;
                    const TypeId frame_ty = m.types.get(fpty).element;
                    if (frame_ty >= m.types.size()) break;
                    const int st_fi = field_index(frame_ty, "__state");
                    if (st_fi >= 0) {
                        llvm::Value* stp = builder->CreateStructGEP(
                            mir_type(m, frame_ty), frp, field_elem(frame_ty, st_fi));
                        builder->CreateStore(builder->getInt32(-1), stp);
                    }
                    const TypeId poll_ty = poll_ty_op->type();
                    llvm::Type* poll_ll = mir_type(m, poll_ty);
                    llvm::Value* poll_out =
                        builder->CreateLoad(builder->getPtrTy(), rslot);
                    const int tag_i0 = variant_tag_index(poll_ty);
                    if (tag_i0 < 0) break;
                    llvm::Value* tagp = builder->CreateStructGEP(
                        poll_ll, poll_out,
                        mir_field_elem(poll_ty, static_cast<uint32_t>(tag_i0)));
                    builder->CreateStore(builder->getInt32(0), tagp);
                    std::vector<uint32_t> rt_idx;
                    TypeId ready_value_ty = MIR_INVALID_ID;
                    if (field_path(poll_ty, "Ready.value", rt_idx, ready_value_ty)) {
                        llvm::Value* slot = gep_path(poll_out, poll_ty, rt_idx);
                        const int res_fi = field_index(frame_ty, "__result");
                        if (res_fi >= 0 && ready_value_ty < m.types.size()) {
                            llvm::Value* resp = builder->CreateStructGEP(
                                mir_type(m, frame_ty), frp, field_elem(frame_ty, res_fi));
                            llvm::Value* rv = builder->CreateLoad(
                                mir_type(m, ready_value_ty), resp);
                            builder->CreateStore(rv, slot);
                        }
                    }
                    builder->CreateRetVoid();
                    break;
                }
                case MIROpcode::AsyncRampFinish: {
                    // `frame->__state = st; *sret = Handle{ frame, &vtbl };`
                    const MIROperand* fr = arg(0);
                    const MIROperand* sret = arg(1);
                    const MIROperand* state = arg(2);
                    const MIROperand* vtbl = arg(3);
                    const MIROperand* handle_ty_op = arg(4);
                    if (!fr || !sret || !state || !vtbl || !handle_ty_op) break;
                    llvm::Value* fslot = place_ptr(fr->id);
                    llvm::Value* sslot = place_ptr(sret->id);
                    std::string vtbl_name;
                    if (!fslot || !sslot || !const_string(vtbl->id, vtbl_name)) break;
                    llvm::Value* frp = builder->CreateLoad(builder->getPtrTy(), fslot);
                    llvm::Value* sp = builder->CreateLoad(builder->getPtrTy(), sslot);
                    const TypeId fpty = fn.places[fr->id].type;
                    if (fpty >= m.types.size()) break;
                    const TypeId frame_ty = m.types.get(fpty).element;
                    const uint64_t st = constant_bits(*state);
                    const int st_fi = field_index(frame_ty, "__state");
                    if (st_fi >= 0) {
                        llvm::Value* stp = builder->CreateStructGEP(
                            mir_type(m, frame_ty), frp, field_elem(frame_ty, st_fi));
                        builder->CreateStore(builder->getInt32(static_cast<uint32_t>(st)), stp);
                    }
                    const TypeId handle_ty = handle_ty_op->type();
                    llvm::Type* handle_ll = mir_type(m, handle_ty);
                    const int frame_fi = field_index(handle_ty, "frame");
                    if (frame_fi >= 0) {
                        llvm::Value* fp_slot = builder->CreateStructGEP(
                            handle_ll, sp, field_elem(handle_ty, frame_fi));
                        builder->CreateStore(frp, fp_slot);
                    }
                    const int vtbl_fi = field_index(handle_ty, "vtbl");
                    if (vtbl_fi >= 0) {
                        llvm::Value* vtbl_slot = builder->CreateStructGEP(
                            handle_ll, sp, field_elem(handle_ty, vtbl_fi));
                        const TypeId vtbl_field_ty = m.types.field_of(handle_ty, vtbl_fi)->type;
                        const TypeId table_ty =
                            (vtbl_field_ty < m.types.size())
                                ? m.types.get(vtbl_field_ty).element
                                : MIR_INVALID_ID;
                        llvm::Value* g = async_vtable(vtbl_name, table_ty);
                        if (g) builder->CreateStore(g, vtbl_slot);
                    }
                    builder->CreateRetVoid();
                    break;
                }
                case MIROpcode::AsyncAwait: {
                    // `Poll<T> p = child.vtbl->poll(&tmp, child.frame, frame->__cx);
                    //  if (p.__chx__vt != 0) { frame->__state = N; <spills>;
                    //                        *ret = Pending; return; }
                    //  result = p.Ready.value; child.vtbl->drop(child.frame);
                    //  child.frame = 0;`
                    const MIROperand* fr = arg(0);
                    const MIROperand* child = arg(1);
                    const MIROperand* result = arg(2);
                    const MIROperand* poll_ty_op = arg(3);
                    const MIROperand* state = arg(4);
                    const MIROperand* ret = arg(5);
                    const MIROperand* fn_poll_ty_op = arg(6);
                    const MIROperand* spill_n = arg(7);
                    if (!fr || !child || !result || !poll_ty_op || !state || !ret ||
                        !fn_poll_ty_op || !spill_n) {
                        break;
                    }
                    llvm::Value* fslot = place_ptr(fr->id);
                    llvm::Value* cslot = place_ptr(child->id);
                    if (!fslot || !cslot) break;
                    llvm::Value* frp = builder->CreateLoad(builder->getPtrTy(), fslot);
                    const TypeId fpty = fn.places[fr->id].type;
                    const TypeId child_ty = fn.places[child->id].type;
                    if (fpty >= m.types.size() || child_ty >= m.types.size()) break;
                    const TypeId frame_ty = m.types.get(fpty).element;
                    // child handle: { frame, vtbl }
                    const int c_frame_fi = field_index(child_ty, "frame");
                    const int c_vtbl_fi = field_index(child_ty, "vtbl");
                    if (c_frame_fi < 0 || c_vtbl_fi < 0) break;
                    llvm::Type* child_ll = mir_type(m, child_ty);
                    llvm::Value* cframe_slot = builder->CreateStructGEP(
                        child_ll, cslot, field_elem(child_ty, c_frame_fi));
                    llvm::Value* cframe =
                        builder->CreateLoad(builder->getPtrTy(), cframe_slot);
                    llvm::Value* cvtbl_slot = builder->CreateStructGEP(
                        child_ll, cslot, field_elem(child_ty, c_vtbl_fi));
                    llvm::Value* cvtbl =
                        builder->CreateLoad(builder->getPtrTy(), cvtbl_slot);
                    const TypeId vtbl_ptr_ty = m.types.field_of(child_ty, c_vtbl_fi)->type;
                    const TypeId table_ty = m.types.get(vtbl_ptr_ty).element;
                    const int t_poll_fi = field_index(table_ty, "poll");
                    if (t_poll_fi < 0) break;
                    llvm::Type* table_ll = mir_type(m, table_ty);
                    llvm::Value* poll_slot = builder->CreateStructGEP(
                        table_ll, cvtbl, field_elem(table_ty, t_poll_fi));
                    llvm::Value* poll_fn =
                        builder->CreateLoad(builder->getPtrTy(), poll_slot);
                    llvm::FunctionType* poll_ft =
                        mir_fn_type(m, m.types.field_of(table_ty, t_poll_fi)->type);
                    if (!poll_ft) break;
                    // Poll<T> temp (entry-block home: this runs inside the poll
                    // state machine, which must not grow the stack per resume)
                    const TypeId poll_ty = poll_ty_op->type();
                    llvm::Type* poll_ll = mir_type(m, poll_ty);
                    llvm::AllocaInst* tmp = nullptr;
                    {
                        llvm::BasicBlock& ent = lfn->getEntryBlock();
                        llvm::IRBuilder<> top(&ent, ent.begin());
                        tmp = top.CreateAlloca(poll_ll);
                    }
                    // the waker, stashed in the frame at poll entry
                    const int cx_fi = field_index(frame_ty, "__cx");
                    if (cx_fi < 0) break;
                    llvm::Value* cx_slot = builder->CreateStructGEP(
                        mir_type(m, frame_ty), frp, field_elem(frame_ty, cx_fi));
                    llvm::Value* cx =
                        builder->CreateLoad(builder->getPtrTy(), cx_slot);
                    builder->CreateCall(poll_ft, poll_fn, {tmp, cframe, cx});
                    llvm::BasicBlock* pend_b =
                        llvm::BasicBlock::Create(*ctx, "await.pending", lfn);
                    llvm::BasicBlock* ready_b =
                        llvm::BasicBlock::Create(*ctx, "await.ready", lfn);
                    const int tag_i = variant_tag_index(poll_ty);
                    if (tag_i < 0) break;
                    llvm::Value* tagp = builder->CreateStructGEP(
                        poll_ll, tmp, field_elem(poll_ty, tag_i));
                    llvm::Value* tag = builder->CreateLoad(builder->getInt32Ty(), tagp);
                    builder->CreateCondBr(
                        builder->CreateICmpNE(tag, builder->getInt32(0)), pend_b, ready_b);
                    // ── pending: remember the resume state, spill, return ────
                    builder->SetInsertPoint(pend_b);
                    const uint64_t resume_state = constant_bits(*state);
                    const int st_fi = field_index(frame_ty, "__state");
                    if (resume_state != 0 && st_fi >= 0) {
                        llvm::Value* stp = builder->CreateStructGEP(
                            mir_type(m, frame_ty), frp, field_elem(frame_ty, st_fi));
                        builder->CreateStore(
                            builder->getInt32(static_cast<uint32_t>(resume_state)), stp);
                    }
                    const uint32_t nspills = static_cast<uint32_t>(constant_bits(*spill_n));
                    for (uint32_t k = 0; k < nspills; ++k) {
                        const MIROperand* loc = arg(8 + k * 2);
                        const MIROperand* fld = arg(9 + k * 2);
                        if (!loc || !fld) continue;
                        std::string fname;
                        if (!const_string(fld->id, fname)) continue;
                        const int sfi = field_index(frame_ty, fname);
                        if (sfi < 0) continue;
                        llvm::Value* dst = builder->CreateStructGEP(
                            mir_type(m, frame_ty), frp, field_elem(frame_ty, sfi));
                        const TypeId lpt = fn.places[loc->id].type;
                        llvm::Value* src = place_ptr(loc->id);
                        if (!src) continue;
                        if (lpt < m.types.size() &&
                            (abi::is_aggregate(m, lpt) ||
                             m.types.get(lpt).kind == MIRTypeKind::Array)) {
                            const uint64_t sz =
                                module->getDataLayout().getTypeAllocSize(mir_type(m, lpt));
                            builder->CreateMemCpy(dst, llvm::MaybeAlign(), src,
                                                  llvm::MaybeAlign(),
                                                  builder->getInt64(sz));
                        } else {
                            builder->CreateStore(
                                builder->CreateLoad(mir_type(m, lpt), src), dst);
                        }
                    }
                    llvm::Value* rslot = place_ptr(ret->id);
                    if (rslot) {
                        llvm::Value* poll_out =
                            builder->CreateLoad(builder->getPtrTy(), rslot);
                        llvm::Value* outp = builder->CreateStructGEP(
                            poll_ll, poll_out, field_elem(poll_ty, tag_i));
                        builder->CreateStore(builder->getInt32(1), outp);
                    }
                    builder->CreateRetVoid();
                    // ── ready: take the value and release the child ──────────
                    builder->SetInsertPoint(ready_b);
                    std::vector<uint32_t> rt_idx;
                    TypeId ready_value_ty = MIR_INVALID_ID;
                    if (field_path(poll_ty, "Ready.value", rt_idx, ready_value_ty)) {
                        llvm::Value* vslot = gep_path(tmp, poll_ty, rt_idx);
                        llvm::Value* rplace = place_ptr(result->id);
                        if (rplace && ready_value_ty < m.types.size()) {
                            builder->CreateStore(
                                builder->CreateLoad(mir_type(m, ready_value_ty), vslot),
                                rplace);
                        }
                    }
                    const int t_drop_fi = field_index(table_ty, "drop");
                    if (t_drop_fi >= 0) {
                        llvm::Value* drop_slot = builder->CreateStructGEP(
                            table_ll, cvtbl, field_elem(table_ty, t_drop_fi));
                        llvm::Value* drop_fn =
                            builder->CreateLoad(builder->getPtrTy(), drop_slot);
                        llvm::FunctionType* drop_ft = mir_fn_type(
                            m, m.types.field_of(table_ty, t_drop_fi)->type);
                        if (drop_ft) builder->CreateCall(drop_ft, drop_fn, {cframe});
                    }
                    builder->CreateStore(
                        llvm::ConstantPointerNull::get(builder->getPtrTy()), cframe_slot);
                    break;
                }
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
                    const bool ft_is_ptr = abi::is_pointer_like(m, ft);
                    llvm::Type* fat_ty = mir_type(m, ft_is_ptr ? m.types.get(ft).element : ft);
                    llvm::Value* fat_ptr = nullptr;
                    if (fat->kind() == MIROperandKind::Place) {
                        fat_ptr = place_ptr(fat->id);
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
                    if (v < fn.values.size() && !abi::is_void(m, fn.values[v].type)) {
                        vals[v] = ci;
                    }
                    break;
                }
                default:
                    // Not yet lowered by the MIR LLVM emitter. Emit poison so the
                    // function still verifies; the driver reports the gap.
                    if (kMirLlvmTrace)
                        std::cerr << "[MIR-LLVM] UNHANDLED "
                                  << mir::opcode_name(inst.opcode()) << " in "
                                  << std::string(lfn->getName()) << "\n";
                    if (v < fn.values.size() && fn.values[v].type != MIR_INVALID_ID &&
                        !abi::is_void(m, fn.values[v].type)) {
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
    if (mir_di_sp_) builder->SetCurrentDebugLocation(nullptr);
    mir_di_sp_ = nullptr;
    return true;
}

// ── Global initializers from MIR constant expressions (action A3) ────────────
//
// `MIRLowerer` lowers a top-level global's initializer into a target-neutral
// constant-expression tree (`MIRConstExpr.h`) once; this builds the LLVM
// `Constant` from it. The AST-walking `GlobalConstantLowerer` it replaces lived
// in ASTCompiler.cpp. No AST is read here.

namespace {

/** Builds an `llvm::Constant` from a `MIRConstExpr` tree. */
class MIRConstBuilder {
public:
    MIRConstBuilder(Codegen& gen, const mir::MIRModule& m)
        : gen_(gen), m_(m), ctx_(gen.module->getContext()) {}

    llvm::Constant* build(mir::ConstExprId id, llvm::Type* want) {
        if (++depth_ > 128) {
            --depth_;
            return llvm::Constant::getNullValue(want);
        }
        llvm::Constant* c = build_impl(id, want);
        --depth_;
        return c;
    }

private:
    Codegen& gen_;
    const mir::MIRModule& m_;
    llvm::LLVMContext& ctx_;
    int depth_ = 0;
    unsigned str_counter_ = 0;

    bool valid(mir::TypeId t) const {
        return t != mir::MIR_INVALID_ID && t < m_.types.size();
    }

    static llvm::Constant* null(llvm::Type* want) {
        return llvm::Constant::getNullValue(want);
    }

    /** The tree child of aggregate `id` whose designator equals `name`. */
    const mir::MIRConstChild* find_child(mir::ConstExprId id, std::string_view name) const {
        const mir::MIRConstExpr& e = m_.const_exprs.get(id);
        for (uint32_t i = 0; i < e.child_count; ++i) {
            const mir::MIRConstChild* c = m_.const_exprs.child(id, i);
            if (c && m_.const_exprs.child_name_view(*c) == name) return c;
        }
        return nullptr;
    }

    llvm::Constant* build_impl(mir::ConstExprId id, llvm::Type* want) {
        if (!want) want = llvm::Type::getInt8Ty(ctx_);
        if (id == mir::MIR_INVALID_ID || id >= m_.const_exprs.size()) return null(want);
        const mir::MIRConstExpr& e = m_.const_exprs.get(id);
        switch (e.kind) {
            case mir::MIRConstKind::Zero:
                return null(want);
            case mir::MIRConstKind::Int:
                return int_constant(e.bits, want);
            case mir::MIRConstKind::Bool:
                return bool_constant(e.bits != 0, want);
            case mir::MIRConstKind::Float: {
                const uint32_t b = static_cast<uint32_t>(e.bits);
                float f = 0.0f;
                std::memcpy(&f, &b, sizeof(f));
                return fp_constant(static_cast<double>(f), want);
            }
            case mir::MIRConstKind::Double: {
                double d = 0.0;
                std::memcpy(&d, &e.bits, sizeof(d));
                return fp_constant(d, want);
            }
            case mir::MIRConstKind::Null:
                return null(want);
            case mir::MIRConstKind::String:
                return string_constant(e, want);
            case mir::MIRConstKind::Negate: {
                const mir::MIRConstChild* c = m_.const_exprs.child(id, 0);
                const mir::ConstExprId child = c ? c->expr : mir::MIR_INVALID_ID;
                if (want->isFloatingPointTy()) {
                    llvm::Constant* inner = build(child, want);
                    if (auto* cf = llvm::dyn_cast<llvm::ConstantFP>(inner)) {
                        return llvm::ConstantFP::get(want, -cf->getValueAPF());
                    }
                    return null(want);
                }
                llvm::Constant* inner = build(child, want);
                if (inner && inner->getType()->isIntegerTy()) {
                    return llvm::ConstantExpr::getNeg(inner);
                }
                return inner;
            }
            case mir::MIRConstKind::FunctionRef:
                return function_ref(e.symbol, want);
            case mir::MIRConstKind::Aggregate:
                return aggregate_constant(id, e, want);
            case mir::MIRConstKind::Variant:
                return variant_constant(id, e, want);
        }
        return null(want);
    }

    llvm::Constant* int_constant(uint64_t num, llvm::Type* want) {
        if (want->isPointerTy()) return int_constant_ptr(num, want);
        if (want->isIntegerTy(1)) return llvm::ConstantInt::get(want, num ? 1 : 0);
        if (want->isIntegerTy()) return llvm::ConstantInt::get(want, num, /*isSigned=*/true);
        if (want->isFloatTy()) {
            return llvm::ConstantFP::get(want, static_cast<float>(static_cast<int64_t>(num)));
        }
        if (want->isDoubleTy()) {
            return llvm::ConstantFP::get(want, static_cast<double>(static_cast<int64_t>(num)));
        }
        return null(want);
    }

    llvm::Constant* bool_constant(bool b, llvm::Type* want) {
        if (want->isIntegerTy()) return llvm::ConstantInt::get(want, b ? 1 : 0);
        if (want->isPointerTy()) return int_constant_ptr(b ? 1 : 0, want);
        if (want->isFloatingPointTy()) return llvm::ConstantFP::get(want, b ? 1.0 : 0.0);
        return null(want);
    }

    llvm::Constant* fp_constant(double d, llvm::Type* want) {
        if (want->isFloatTy()) return llvm::ConstantFP::get(want, static_cast<float>(d));
        if (want->isDoubleTy()) return llvm::ConstantFP::get(want, d);
        if (want->isIntegerTy()) {
            return llvm::ConstantInt::get(want, static_cast<uint64_t>(static_cast<int64_t>(d)));
        }
        return null(want);
    }

    llvm::Constant* int_constant_ptr(uint64_t num, llvm::Type* want) {
        return llvm::ConstantExpr::getIntToPtr(
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx_), num), want);
    }

    llvm::Constant* function_ref(uint32_t symbol, llvm::Type* want) {
        (void)want;
        if (symbol >= m_.symbols.size()) return nullptr;
        return gen_.mir_function(m_, symbol); // opaque pointer
    }

    llvm::Constant* string_constant(const mir::MIRConstExpr& e, llvm::Type* want) {
        std::string s(m_.const_exprs.data.data_at(e.data_offset), e.data_count);
        if (want->isPointerTy()) {
            llvm::Constant* arr = llvm::ConstantDataArray::getString(ctx_, s, true);
            return new llvm::GlobalVariable(*gen_.module, arr->getType(), true,
                                            llvm::GlobalValue::PrivateLinkage, arr,
                                            ".mir.str." + std::to_string(str_counter_++));
        }
        if (auto* at = llvm::dyn_cast<llvm::ArrayType>(want)) {
            llvm::Type* et = at->getElementType();
            const uint64_t n = at->getNumElements() ? at->getNumElements() : s.size() + 1;
            llvm::ArrayType* rt = llvm::ArrayType::get(et, n);
            std::vector<llvm::Constant*> elems;
            for (unsigned char ch : s) elems.push_back(llvm::ConstantInt::get(et, ch));
            while (elems.size() < n) elems.push_back(null(et));
            if (elems.size() != n) return null(want);
            return llvm::ConstantArray::get(rt, elems);
        }
        return null(want);
    }

    /** The MIR-owned aggregate access plan for `t` (see `mir_type`). */
    const mir::MIRAggregatePlan* plan_for(mir::TypeId t) const {
        auto it = gen_.mir_agg_plans_.find(t);
        if (it == gen_.mir_agg_plans_.end()) return nullptr;
        return &it->second;
    }

    /**
     * Fit `c` (the active union member / variant case payload) into the shared
     * storage element `want`, whose byte size is the overlap region. An equal
     * size is a bitcast; a smaller member is padded to the region (a packed
     * wrapper keeps the size exact) and then bitcast. This is how a union or
     * variant initializer is written into MIR's overlapping layout from LLVM,
     * which has no union.
     */
    llvm::Constant* fit_region(llvm::Constant* c, llvm::Type* want) {
        if (!c) return nullptr;
        if (c->getType() == want) return c;
        const llvm::DataLayout& dl = gen_.module->getDataLayout();
        const uint64_t want_sz = dl.getTypeAllocSize(want);
        const uint64_t have_sz = dl.getTypeAllocSize(c->getType());
        if (have_sz == want_sz) return llvm::ConstantExpr::getBitCast(c, want);
        if (have_sz > want_sz) return null(want);
        llvm::ArrayType* pad =
            llvm::ArrayType::get(llvm::Type::getInt8Ty(ctx_), want_sz - have_sz);
        llvm::StructType* wrapped =
            llvm::StructType::get(ctx_, {c->getType(), pad}, /*isPacked=*/true);
        llvm::Constant* padded = llvm::ConstantStruct::get(wrapped, {c, null(pad)});
        return llvm::ConstantExpr::getBitCast(padded, want);
    }

    llvm::Constant* aggregate_constant(mir::ConstExprId id, const mir::MIRConstExpr& e,
                                       llvm::Type* want) {
        mir::MIRTypeRecord rec_storage;
        const mir::MIRTypeRecord* rec = nullptr;
        if (valid(e.type)) {
            rec_storage = m_.types.get(e.type);
            rec = &rec_storage;
        }
        if (auto* at = llvm::dyn_cast<llvm::ArrayType>(want)) {
            llvm::Type* et = at->getElementType();
            std::vector<llvm::Constant*> elems;
            elems.reserve(e.child_count);
            for (uint32_t i = 0; i < e.child_count; ++i) {
                const mir::MIRConstChild* c = m_.const_exprs.child(id, i);
                elems.push_back(build(c ? c->expr : mir::MIR_INVALID_ID, et));
            }
            while (elems.size() < at->getNumElements()) elems.push_back(null(et));
            if (elems.size() != at->getNumElements()) return null(want);
            return llvm::ConstantArray::get(at, elems);
        }
        auto* st = llvm::dyn_cast<llvm::StructType>(want);
        if (!st) return null(want);
        // Make sure `mir_type` has published the element plan (padding and
        // overlap elements) before we walk the struct body.
        if (valid(e.type)) gen_.mir_type(m_, e.type);
        const mir::MIRAggregatePlan* plan = plan_for(e.type);
        std::vector<llvm::Constant*> elems;
        elems.reserve(st->getNumElements());
        for (uint32_t i = 0; i < st->getNumElements(); ++i) {
            llvm::Type* ft = st->getElementType(i);
            const mir::MIRAggElement* info =
                (plan && i < plan->elements.size()) ? &plan->elements[i] : nullptr;
            if (!info || info->field_count == 0 || !rec) {
                elems.push_back(null(ft));  // padding / unknown
                continue;
            }
            if (info->field_count == 1) {
                const mir::MIRFieldInfo* f = m_.types.field_of(e.type, info->first_field);
                const mir::MIRConstChild* c =
                    f ? find_child(id, m_.types.field_name_view(*f)) : nullptr;
                elems.push_back(build(c ? c->expr : mir::MIR_INVALID_ID, ft));
            } else {
                // A union: at most one member is initialized. Build it in its
                // own type and fit it into the shared storage element.
                llvm::Constant* c = nullptr;
                for (uint32_t k = 0; k < info->field_count && !c; ++k) {
                    const mir::MIRFieldInfo* f = m_.types.field_of(e.type, info->first_field + k);
                    if (!f) continue;
                    const mir::MIRConstChild* child =
                        find_child(id, m_.types.field_name_view(*f));
                    if (!child) continue;
                    llvm::Constant* m = build(child->expr, gen_.mir_type(m_, f->type));
                    c = fit_region(m, ft);
                }
                elems.push_back(c ? c : null(ft));
            }
        }
        return llvm::ConstantStruct::get(st, elems);
    }

    llvm::Constant* variant_constant(mir::ConstExprId id, const mir::MIRConstExpr& e,
                                     llvm::Type* want) {
        mir::MIRTypeRecord rec_storage;
        const mir::MIRTypeRecord* rec = nullptr;
        if (valid(e.type)) {
            rec_storage = m_.types.get(e.type);
            rec = &rec_storage;
        }
        auto* st = llvm::dyn_cast<llvm::StructType>(want);
        if (!st) return null(want);
        if (valid(e.type)) gen_.mir_type(m_, e.type);
        const mir::MIRAggregatePlan* plan = plan_for(e.type);
        std::vector<llvm::Constant*> elems;
        elems.reserve(st->getNumElements());
        for (uint32_t i = 0; i < st->getNumElements(); ++i) {
            llvm::Type* ft = st->getElementType(i);
            const mir::MIRAggElement* info =
                (plan && i < plan->elements.size()) ? &plan->elements[i] : nullptr;
            if (!info || info->field_count == 0 || !rec) {
                elems.push_back(null(ft));  // padding / unknown
                continue;
            }
            const mir::MIRFieldInfo* f = m_.types.field_of(e.type, info->first_field);
            if (!f) {
                elems.push_back(null(ft));
                continue;
            }
            const bool overlap_case =
                info->field_count > 1 ||
                static_cast<mir::MIRFieldKind>(f->kind) == mir::MIRFieldKind::VariantCase;
            if (overlap_case) {
                // One shared element holds all case payloads. Exactly one case is
                // active (its `Case.param` children exist); build and fit it.
                llvm::Constant* c = nullptr;
                for (uint32_t k = 0; k < info->field_count && !c; ++k) {
                    const mir::MIRFieldInfo* cf = m_.types.field_of(e.type, info->first_field + k);
                    if (!cf) continue;
                    auto* cst = llvm::dyn_cast<llvm::StructType>(gen_.mir_type(m_, cf->type));
                    if (!cst) continue;
                    const std::string cname(m_.types.field_name_view(*cf));
                    std::vector<llvm::Constant*> case_elems;
                    case_elems.reserve(cst->getNumElements());
                    bool any = false;
                    for (uint32_t j = 0; j < cst->getNumElements(); ++j) {
                        llvm::Type* cft = cst->getElementType(j);
                        const mir::MIRFieldInfo* pcf = m_.types.field_of(cf->type, j);
                        const mir::MIRConstChild* child = nullptr;
                        if (pcf) {
                            std::string path(cname);
                            path += '.';
                            path += m_.types.field_name_view(*pcf);
                            child = find_child(id, path);
                        }
                        if (child) any = true;
                        case_elems.push_back(
                            build(child ? child->expr : mir::MIR_INVALID_ID, cft));
                    }
                    if (any) {
                        llvm::Constant* cs = llvm::ConstantStruct::get(cst, case_elems);
                        c = fit_region(cs, ft);
                    }
                }
                elems.push_back(c ? c : null(ft));
                continue;
            }
            switch (static_cast<mir::MIRFieldKind>(f->kind)) {
                case mir::MIRFieldKind::Base: {
                    const mir::MIRConstChild* c = find_child(id, m_.types.field_name_view(*f));
                    elems.push_back(build(c ? c->expr : mir::MIR_INVALID_ID, ft));
                    break;
                }
                case mir::MIRFieldKind::VariantTag:
                    elems.push_back(ft->isIntegerTy()
                                        ? static_cast<llvm::Constant*>(llvm::ConstantInt::get(
                                              ft, static_cast<uint64_t>(e.case_index)))
                                        : null(ft));
                    break;
                default:
                    elems.push_back(null(ft));
                    break;
            }
        }
        return llvm::ConstantStruct::get(st, elems);
    }
};

} // namespace

void Codegen::mir_apply_global_initializers(const mir::MIRModule& m) {
    MIRConstBuilder builder(*this, m);
    for (const mir::MIRGlobal& g : m.globals) {
        if (g.init == mir::MIR_INVALID_ID) continue;
        if (g.symbol >= m.symbols.size()) continue;
        const mir::MIRSymbolRecord& rec = m.symbols.get(g.symbol);
        const std::string name = m.symbols.mangled_name(rec);
        llvm::GlobalVariable* gv = module->getNamedGlobal(name);
        if (!gv) {
            auto it = mir_global_cache_.find(g.symbol);
            if (it != mir_global_cache_.end()) gv = it->second;
        }
        // An `@extern` global is a declaration (no initializer); never turn it
        // into a definition — that would interpose the real symbol (libc's
        // `stdout`). `mir_declare_globals` gave a definition a null initializer.
        if (!gv || !gv->hasInitializer()) continue;
        // Another pass may already have defined this symbol.
        if (gv->getInitializer() && !gv->getInitializer()->isNullValue() &&
            !llvm::isa<llvm::ConstantAggregateZero>(gv->getInitializer())) {
            continue;
        }
        llvm::Constant* c = builder.build(g.init, gv->getValueType());
        if (!c) continue;
        if (c->getType() == gv->getValueType()) {
            gv->setInitializer(c);
            continue;
        }
        // An unsized `[0 x T]` global whose initializer carries the real extent
        // (`const x : char[] = "..."`): rebuild it with the inferred type so
        // indexing/GEP sees the data, as C's `char x[] = "..."` does.
        auto* old_at = llvm::dyn_cast<llvm::ArrayType>(gv->getValueType());
        auto* new_at = llvm::dyn_cast<llvm::ArrayType>(c->getType());
        if (old_at && new_at && old_at->getNumElements() == 0 && new_at->getNumElements() > 0) {
            auto* ng = new llvm::GlobalVariable(*module, c->getType(), gv->isConstant(),
                                                gv->getLinkage(), c, name);
            ng->setAlignment(gv->getAlign());
            mir_global_cache_[g.symbol] = ng;
            gv->eraseFromParent();
        }
    }
}
