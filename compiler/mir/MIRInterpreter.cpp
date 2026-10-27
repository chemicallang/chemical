// Copyright (c) Chemical Language Foundation 2025.
//
// MIR interpreter implementation. See MIRInterpreter.h and
// lang/docs/mir-interpretation-plan.md.

#include "MIRInterpreter.h"

#include "MIRABI.h"
#include "MIRInstruction.h"
#include "MIRTraversal.h"
#include "utils/StackSpace.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mir {

namespace {

// ── per-type facts, resolved once ─────────────────────────────────────────

struct TypeInfo {
    uint32_t size = 0;
    uint32_t align = 1;
    MIRTypeKind kind = MIRTypeKind::Void;
    bool is_signed = false;
    bool is_float = false;     // Float kind, 4 bytes
    bool is_double = false;    // Float kind, >= 8 bytes
    bool is_pointer = false;   // Pointer / Reference / Function (a machine word)
    bool is_aggregate = false; // Struct / Union / Variant / Array (lives in memory)
};

struct FieldSlot {
    uint32_t offset = 0;
    TypeId type = MIR_INVALID_ID;
    uint32_t dyn_slot = MIR_INVALID_ID; // DynamicCall: resolved vtable slot index
    bool valid = false;
};

struct Callee {
    enum class Kind : uint8_t { Mir, Host, Unresolved };
    Kind kind = Kind::Unresolved;
    size_t fn_index = 0; // index into module.functions when Mir
    uint64_t (*host)(void*, const uint64_t*, uint32_t) = nullptr;
};

// ── frame arena ────────────────────────────────────────────────────────────

struct FrameArena {
    std::vector<uint8_t*> blocks;
    size_t first_cap = 0;
    uint8_t* cur = nullptr;
    size_t remain = 0;

    uint8_t* alloc(size_t n) {
        n = (n + 15u) & ~size_t(15u);
        if (n == 0) n = 16;
        if (n > remain) {
            size_t cap = n > (32u * 1024u) ? n : (32u * 1024u);
            uint8_t* b = static_cast<uint8_t*>(std::malloc(cap));
            blocks.push_back(b);
            if (blocks.size() == 1) first_cap = cap;
            cur = b;
            remain = cap;
        }
        uint8_t* p = cur;
        cur += n;
        remain -= n;
        return p;
    }

    void reset() {
        if (!blocks.empty()) {
            cur = blocks[0];
            remain = first_cap;
        } else {
            cur = nullptr;
            remain = 0;
        }
    }

    ~FrameArena() {
        for (uint8_t* b : blocks) std::free(b);
    }

    FrameArena() = default;
    FrameArena(const FrameArena&) = delete;
    FrameArena& operator=(const FrameArena&) = delete;
};

struct Frame {
    const MIRFunction* fn = nullptr;
    std::vector<uint64_t> values;
    std::vector<uint8_t*> places;
    FrameArena arena;
    uint64_t ret = 0;

    void prepare(const MIRFunction* f, const std::vector<TypeInfo>& ti) {
        fn = f;
        values.assign(f->values.size(), 0);
        places.assign(f->places.size(), nullptr);
        for (size_t i = 0; i < f->places.size(); ++i) {
            uint32_t sz = (f->places[i].type < ti.size()) ? ti[f->places[i].type].size : 8;
            if (sz == 0) sz = 1;
            places[i] = arena.alloc(sz);
            std::memset(places[i], 0, sz);
        }
    }
};

} // namespace

// ── the engine ─────────────────────────────────────────────────────────────

struct MIRInterpreter::Impl {
    const MIRModule& module;

    std::vector<TypeInfo> types;
    std::vector<uint64_t> const_bits;
    std::vector<const char*> const_str;
    /**
     * NUL-terminated storage for every string constant, in fixed 4 KiB blocks.
     *
     * `const_str` is handed to interpreted code as a raw `*char` and read
     * linearly, so every entry must be one contiguous run of bytes. A
     * `std::deque<char>` looks like it offers that (its `push_back` never
     * invalidates an earlier element) but it does not: consecutive elements are
     * contiguous only *within* one internal chunk, and a string that straddles a
     * chunk boundary reads the unrelated heap that follows the first chunk —
     * 177 constants of one test-suite run were affected. Each block is a single
     * heap allocation that no entry ever straddles, the pointers stay valid as
     * the append-only module grows, and the module's whole string payload costs
     * one allocation per 4 KiB instead of one malloc per constant.
     */
    std::vector<std::unique_ptr<char[]>> const_blocks;
    static constexpr size_t kConstBlockSize = 4096;
    size_t const_block_used_ = 0;
    size_t const_block_cap_ = 0;
    std::vector<Callee> callees;       // one per symbol
    std::vector<uint8_t*> global_addr; // one per symbol
    std::vector<std::string> sym_name; // one per symbol
    std::unordered_map<std::string, size_t> fn_by_name;
    /**
     * Resolved field/index slots of one function, by operand index. Built on the
     * function's first call: a comptime driver lowers the whole callee graph
     * into the module but only ever runs a handful of those functions, so
     * pre-resolving every function at construction was the single largest cost
     * of a rebuild. `operand_count` detects an in-place body replacement.
     */
    struct FunctionSlots {
        std::vector<FieldSlot> slots;
        uint32_t operand_count = 0;
        bool built = false;
    };
    std::vector<FunctionSlots> field_slots;
    std::unordered_map<std::string, std::vector<std::string>> vtable_methods; // type name -> slots
    std::unordered_map<std::string, uint8_t*> vtable_addr_by_name;            // def name -> storage
    std::vector<std::vector<const Callee*>> vtable_storage;                   // owning storage

    /**
     * Incremental build cursors. Every MIR table is append-only and ids are
     * stable, so each `build_*` only consumes the records added since the last
     * pass; a shrinking table (a cleared/reused module) resets its table.
     */
    uint32_t built_types = 0;
    uint32_t built_constants = 0;
    uint32_t built_functions = 0;
    uint32_t built_globals = 0;
    uint32_t built_symbols = 0;
    uint32_t built_vtypes = 0;
    uint32_t built_vdefs = 0;
    /** Symbol name -> global storage (a symbol and its global share one store). */
    std::unordered_map<std::string, uint8_t*> global_by_name;
    /** Symbols whose callee was still unresolved: a MIR body may appear later. */
    std::vector<uint32_t> pending_symbols;
    bool vtables_changed_ = false;
    size_t wired_symbols_ = 0;

    std::vector<std::unique_ptr<Frame>> pool;
    size_t depth = 0;
    /**
     * set when `call_function` stopped descending because too little stack was
     * left (an unbounded recursion): every nested call returns immediately and
     * the top-level `call`/`call_sret`/`run` reports it instead of crashing.
     */
    bool stack_exhausted = false;
    std::string unsupported_error;

    explicit Impl(const MIRModule& m) : module(m) {}

    // ── type facts ─────────────────────────────────────────────────────────
    void build_types() {
        const uint32_t n = module.types.size();
        if (n < types.size()) {
            types.clear();
            built_types = 0;
        }
        if (n == types.size()) return;
        types.resize(n);
        for (uint32_t i = built_types; i < n; ++i) {
            const MIRTypeRecord r = module.types.get(i);
            TypeInfo& t = types[i];
            t.size = r.size;
            t.align = r.alignment ? r.alignment : 1;
            t.kind = r.kind;
            t.is_signed = (r.flags & TF_SIGNED) != 0;
            switch (r.kind) {
                case MIRTypeKind::Float:
                    t.is_float = r.size == 4;
                    t.is_double = r.size >= 8;
                    break;
                case MIRTypeKind::Pointer:
                case MIRTypeKind::Reference:
                case MIRTypeKind::Function:
                    t.is_pointer = true;
                    t.size = module.layout.pointer_size;
                    if (t.align < module.layout.pointer_alignment)
                        t.align = module.layout.pointer_alignment;
                    break;
                case MIRTypeKind::Struct:
                case MIRTypeKind::Union:
                case MIRTypeKind::Variant:
                case MIRTypeKind::Array:
                    t.is_aggregate = true;
                    break;
                case MIRTypeKind::Bool:
                    if (t.size == 0) t.size = 1;
                    break;
                default:
                    break;
            }
        }
        built_types = n;
    }

    const TypeInfo& ti(TypeId t) const {
        static const TypeInfo dummy{};
        return t < types.size() ? types[t] : dummy;
    }

    TypeId pointee(TypeId t) const {
        if (t >= types.size()) return MIR_INVALID_ID;
        const MIRTypeRecord r = module.types.get(t);
        if (r.kind == MIRTypeKind::Pointer || r.kind == MIRTypeKind::Reference)
            return r.element;
        return MIR_INVALID_ID;
    }

    // ── constants ──────────────────────────────────────────────────────────
    /**
     * Copy one string constant into the block arena, NUL-terminated. The entry
     * is kept whole: if it does not fit in the current block, a new block is
     * taken (a single constant longer than a block gets a block of its own), so
     * the returned pointer is a contiguous `len + 1` byte run.
     */
    char* intern_string_constant(const char* src, uint32_t len) {
        const size_t need = static_cast<size_t>(len) + 1;
        if (const_blocks.empty() || const_block_used_ + need > const_block_cap_) {
            const size_t cap = need > kConstBlockSize ? need : kConstBlockSize;
            const_blocks.emplace_back(std::make_unique<char[]>(cap));
            const_block_cap_ = cap;
            const_block_used_ = 0;
        }
        char* dst = const_blocks.back().get() + const_block_used_;
        if (len) std::memcpy(dst, src, len);
        dst[len] = '\0';
        const_block_used_ += need;
        return dst;
    }

    void build_constants() {
        const uint32_t n = module.constants.size();
        if (n < const_bits.size()) {
            // a cleared/reused module: drop the side tables. The blocks are kept
            // (not freed) so a `*char` already handed to interpreted code still
            // points at live, correctly terminated text.
            const_bits.clear();
            const_str.clear();
            built_constants = 0;
        }
        if (n == const_bits.size()) return;
        const_bits.resize(n, 0);
        const_str.resize(n, nullptr);
        for (uint32_t i = built_constants; i < n; ++i) {
            const MIRConstant c = module.constants.get(i);
            const_bits[i] = c.bits;
            if (c.kind == MIRConstantKind::String) {
                const std::string_view sv = constant_string_view(module, i);
                const_str[i] = intern_string_constant(sv.data(), static_cast<uint32_t>(sv.size()));
            }
        }
        built_constants = n;
    }

    std::string constant_text(ConstantId cid) const {
        return std::string(constant_string_view(module, cid));
    }

    /** `constant_text` without the copy — used on the field-lookup path, which
     * runs for every FieldAddr/IndexAddr instruction. The pool's bytes are
     * append-only and stable (MIRBytePool), so the view is valid for the lookup. */
    std::string_view constant_text_view(ConstantId cid) const {
        return constant_string_view(module, cid);
    }

    // ── field resolution ───────────────────────────────────────────────────
    bool resolve_field(TypeId agg, std::string_view dotted, FieldSlot& out) {
        TypeId cur = agg;
        uint32_t off = 0;
        size_t start = 0;
        while (true) {
            const size_t dot = dotted.find('.', start);
            // a view into the designator, not a `std::string` per segment:
            // this runs for every field access the interpreter executes
            const std::string_view seg =
                (dot == std::string_view::npos) ? dotted.substr(start)
                                                : dotted.substr(start, dot - start);
            if (cur >= types.size()) return false;
            const MIRTypeRecord rec = module.types.get(cur);
            bool found = false;
            for (uint32_t i = 0; i < rec.data_count; ++i) {
                const MIRFieldInfo* f = module.types.field_of(cur, i);
                if (!f) break;
                if (module.types.field_name_view(*f) == seg) {
                    off += f->offset;
                    cur = f->type;
                    found = true;
                    break;
                }
            }
            if (!found) return false;
            if (dot == std::string_view::npos) break;
            start = dot + 1;
        }
        out.offset = off;
        out.type = cur;
        out.valid = true;
        return true;
    }

    TypeId base_aggregate(const MIRFunction& fn, const MIROperand& base) const {
        const TypeId bt = (base.kind() == MIROperandKind::Place)
                              ? fn.places[base.id].type
                              : fn.values[base.id].type;
        const TypeId p = pointee(bt);
        return p != MIR_INVALID_ID ? p : bt;
    }

    // ── symbols / callees / globals ────────────────────────────────────────
    static uint64_t host_printf(void*, const uint64_t*, uint32_t);
    static uint64_t host_malloc(void*, const uint64_t*, uint32_t);
    static uint64_t host_free(void*, const uint64_t*, uint32_t);
    static uint64_t host_realloc(void*, const uint64_t*, uint32_t);
    static uint64_t host_memcpy(void*, const uint64_t*, uint32_t);
    static uint64_t host_memmove(void*, const uint64_t*, uint32_t);
    static uint64_t host_memset(void*, const uint64_t*, uint32_t);
    static uint64_t host_strlen(void*, const uint64_t*, uint32_t);
    static uint64_t host_strcmp(void*, const uint64_t*, uint32_t);
    static uint64_t host_abort(void*, const uint64_t*, uint32_t);
    static uint64_t host_exit(void*, const uint64_t*, uint32_t);
    static uint64_t host_putchar(void*, const uint64_t*, uint32_t);

    static uint64_t (*host_for(const char* name))(void*, const uint64_t*, uint32_t) {
        if (std::strcmp(name, "printf") == 0) return &host_printf;
        if (std::strcmp(name, "malloc") == 0) return &host_malloc;
        if (std::strcmp(name, "free") == 0) return &host_free;
        if (std::strcmp(name, "realloc") == 0) return &host_realloc;
        if (std::strcmp(name, "memcpy") == 0) return &host_memcpy;
        if (std::strcmp(name, "memmove") == 0) return &host_memmove;
        if (std::strcmp(name, "memset") == 0) return &host_memset;
        if (std::strcmp(name, "strlen") == 0) return &host_strlen;
        if (std::strcmp(name, "strcmp") == 0) return &host_strcmp;
        if (std::strcmp(name, "abort") == 0) return &host_abort;
        if (std::strcmp(name, "exit") == 0) return &host_exit;
        if (std::strcmp(name, "putchar") == 0) return &host_putchar;
        return nullptr;
    }

    void build_symbols_and_globals() {
        // a cleared/reused module invalidates every cursor and every side table
        if (module.functions.size() < built_functions || module.globals.size() < built_globals ||
            module.symbols.size() < callees.size()) {
            callees.clear();
            global_addr.clear();
            sym_name.clear();
            fn_by_name.clear();
            global_by_name.clear();
            pending_symbols.clear();
            built_functions = built_globals = built_symbols = 0;
        }

        auto glb_size = [&](TypeId t, ConstExprId init) -> uint32_t {
            uint32_t sz = (t < types.size()) ? types[t].size : 0;
            if (sz == 0 && init != MIR_INVALID_ID && init < module.const_exprs.size()) {
                const MIRConstExpr e = module.const_exprs.get(init);
                if (e.kind == MIRConstKind::String) sz = e.data_count + 1;
            }
            return sz ? sz : 8;
        };

        // 1. globals: storage keyed by symbol name (duplicate interned ids for one
        // global resolve to one store, matching the linker's name-based view).
        for (uint32_t gi = built_globals; gi < module.globals.size(); ++gi) {
            const MIRGlobal& g = module.globals[gi];
            const std::string name = symbol_name(g.symbol);
            if (name.empty()) continue;
            const MIRSymbolRecord r = module.symbols.get(g.symbol);
            const uint32_t sz = glb_size(r.type, g.init);
            auto it = global_by_name.find(name);
            uint8_t* storage;
            if (it != global_by_name.end()) {
                storage = it->second;
            } else {
                storage = static_cast<uint8_t*>(std::calloc(1, sz));
                global_by_name[name] = storage;
            }
            if (g.init != MIR_INVALID_ID) eval_const_into(storage, r.type, g.init);
        }
        built_globals = static_cast<uint32_t>(module.globals.size());

        // 2. callee name -> function index. A re-lowered callee is appended again
        // (call-site dependent MIR), so an existing name is re-pointed below.
        const uint32_t nfn = static_cast<uint32_t>(module.functions.size());
        std::vector<std::string> renamed;
        for (uint32_t i = built_functions; i < nfn; ++i) {
            std::string name = symbol_name(module.functions[i].symbol);
            auto it = fn_by_name.find(name);
            if (it != fn_by_name.end()) {
                it->second = i;
                renamed.push_back(std::move(name));
            } else {
                fn_by_name.emplace(std::move(name), i);
            }
        }
        built_functions = nfn;

        // 3. new symbols: classify the callee, and give a global its storage
        const uint32_t nsym = module.symbols.size();
        callees.resize(nsym, Callee{});
        global_addr.resize(nsym, nullptr);
        sym_name.resize(nsym);
        for (uint32_t s = built_symbols; s < nsym; ++s) {
            std::string name = symbol_name(s);
            auto it = fn_by_name.find(name);
            if (it != fn_by_name.end()) {
                callees[s].kind = Callee::Kind::Mir;
                callees[s].fn_index = it->second;
            } else if (auto* h = host_for(name.c_str())) {
                callees[s].kind = Callee::Kind::Host;
                callees[s].host = h;
            } else {
                pending_symbols.push_back(s);
            }
            const MIRSymbolRecord r = module.symbols.get(s);
            if (r.kind == MIRSymbolKind::Global) {
                auto g = global_by_name.find(name);
                if (g != global_by_name.end()) {
                    global_addr[s] = g->second;
                } else {
                    uint8_t* st =
                        static_cast<uint8_t*>(std::calloc(1, glb_size(r.type, MIR_INVALID_ID)));
                    global_by_name[name] = st;
                    global_addr[s] = st;
                }
            }
            sym_name[s] = std::move(name);
        }
        built_symbols = nsym;

        // 4. a callee that was unresolved before may have a MIR body now
        if (!pending_symbols.empty() && !fn_by_name.empty()) {
            size_t keep = 0;
            for (size_t i = 0; i < pending_symbols.size(); ++i) {
                const uint32_t s = pending_symbols[i];
                if (callees[s].kind == Callee::Kind::Unresolved) {
                    auto it = fn_by_name.find(sym_name[s]);
                    if (it != fn_by_name.end()) {
                        callees[s].kind = Callee::Kind::Mir;
                        callees[s].fn_index = it->second;
                        continue;
                    }
                }
                pending_symbols[keep++] = s;
            }
            pending_symbols.resize(keep);
        }

        // 5. re-point symbols of a re-lowered callee at its newest body (a full
        // rebuild would resolve the name the same way).
        if (!renamed.empty()) {
            for (const std::string& name : renamed) {
                const size_t idx = fn_by_name[name];
                for (uint32_t s = 0; s < nsym; ++s) {
                    if (callees[s].kind == Callee::Kind::Mir && sym_name[s] == name) {
                        callees[s].fn_index = idx;
                    } else if (callees[s].kind == Callee::Kind::Unresolved &&
                               sym_name[s] == name) {
                        callees[s].kind = Callee::Kind::Mir;
                        callees[s].fn_index = idx;
                    }
                }
            }
        }
    }

    std::string symbol_name(SymbolId s) const {
        if (s == MIR_INVALID_ID || s >= module.symbols.size()) return std::string();
        const MIRSymbolRecord r = module.symbols.get(s);
        const std::string mangled = module.symbols.mangled_name(r);
        return mangled.empty() ? std::string(module.symbols.name_data(r), r.name_length)
                               : mangled;
    }

    // ── interface vtables (for DynamicCall) ────────────────────────────────
    void build_vtables() {
        if (module.vtables.types.size() < built_vtypes ||
            module.vtables.defs.size() < built_vdefs) {
            vtable_methods.clear();
            vtable_addr_by_name.clear();
            vtable_storage.clear();
            built_vtypes = built_vdefs = 0;
            vtables_changed_ = true;
        }
        if (built_vtypes == module.vtables.types.size() &&
            built_vdefs == module.vtables.defs.size()) {
            return;
        }
        for (uint32_t i = built_vtypes; i < module.vtables.types.size(); ++i) {
            const MIRVtableRecord& t = module.vtables.types[i];
            std::vector<std::string> names;
            names.reserve(t.slots.size());
            for (const MIRVtableSlot& sl : t.slots) names.push_back(sl.name);
            vtable_methods[t.name] = std::move(names);
        }
        built_vtypes = static_cast<uint32_t>(module.vtables.types.size());
        if (built_vdefs < module.vtables.defs.size()) {
            std::unordered_map<std::string, const Callee*> callee_by_name;
            for (uint32_t s = 0; s < callees.size(); ++s) {
                if (callees[s].kind != Callee::Kind::Unresolved)
                    callee_by_name[sym_name[s]] = &callees[s];
            }
            for (uint32_t i = built_vdefs; i < module.vtables.defs.size(); ++i) {
                const MIRVtableRecord& d = module.vtables.defs[i];
                std::vector<const Callee*> row(d.slots.size(), nullptr);
                for (size_t k = 0; k < d.slots.size(); ++k) {
                    if (d.slots[k].target.empty()) continue;
                    auto it = callee_by_name.find(d.slots[k].target);
                    if (it != callee_by_name.end()) row[k] = it->second;
                }
                vtable_storage.push_back(std::move(row));
                uint8_t* addr = reinterpret_cast<uint8_t*>(vtable_storage.back().data());
                vtable_addr_by_name[d.name] = addr;
                if (!d.type_name.empty() && vtable_addr_by_name.find(d.type_name) ==
                                                vtable_addr_by_name.end()) {
                    vtable_addr_by_name[d.type_name] = addr;
                }
            }
            built_vdefs = static_cast<uint32_t>(module.vtables.defs.size());
            vtables_changed_ = true;
        }
    }

    void wire_vtable_globals() {
        // no interface in the module (a comptime module has none): nothing to do
        if (vtable_addr_by_name.empty() || callees.empty()) return;
        if (!vtables_changed_ && wired_symbols_ == callees.size()) return;
        for (uint32_t s = 0; s < global_addr.size(); ++s) {
            auto it = vtable_addr_by_name.find(sym_name[s]);
            if (it != vtable_addr_by_name.end()) global_addr[s] = it->second;
        }
        wired_symbols_ = callees.size();
        vtables_changed_ = false;
    }

    // ── const-expr evaluation (global initializers) ────────────────────────
    void eval_const_into(uint8_t* dst, TypeId t, ConstExprId id) {
        if (id == MIR_INVALID_ID || id >= module.const_exprs.size()) return;
        const MIRConstExpr e = module.const_exprs.get(id);
        const TypeInfo& info = ti(t);
        switch (e.kind) {
            case MIRConstKind::Zero:
                return;
            case MIRConstKind::Int:
            case MIRConstKind::Bool:
            case MIRConstKind::Float:
            case MIRConstKind::Double:
                store_scalar(dst, info, e.bits);
                return;
            case MIRConstKind::Null:
                store_scalar(dst, info, 0);
                return;
            case MIRConstKind::String: {
                if (info.kind == MIRTypeKind::Array) {
                    // a char[]/byte-array global initialized by a string literal:
                    // the bytes live in the global itself
                    if (info.size && e.data_count <= info.size) {
                        std::memcpy(dst, module.const_exprs.data.data_at(e.data_offset),
                                    e.data_count);
                    } else {
                        std::memcpy(dst, module.const_exprs.data.data_at(e.data_offset),
                                    e.data_count);
                    }
                    return;
                }
                char* buf = static_cast<char*>(std::malloc(e.data_count + 1));
                std::memcpy(buf, module.const_exprs.data.data_at(e.data_offset), e.data_count);
                buf[e.data_count] = '\0';
                store_scalar(dst, info, reinterpret_cast<uint64_t>(buf));
                return;
            }
            case MIRConstKind::Negate: {
                const MIRConstChild* c = module.const_exprs.child(id, 0);
                if (!c) return;
                uint8_t tmp[16] = {0};
                const TypeId ct = module.const_exprs.get(c->expr).type;
                eval_const_into(tmp, ct, c->expr);
                if (ti(ct).is_float) {
                    float f; std::memcpy(&f, tmp, 4); f = -f; std::memcpy(dst, &f, 4);
                } else if (ti(ct).is_double) {
                    double d; std::memcpy(&d, tmp, 8); d = -d; std::memcpy(dst, &d, 8);
                } else {
                    store_scalar(dst, info,
                                 static_cast<uint64_t>(-static_cast<int64_t>(
                                     load_scalar(tmp, ti(ct)))));
                }
                return;
            }
            case MIRConstKind::FunctionRef:
                store_scalar(dst, info, reinterpret_cast<uint64_t>(&callees[e.symbol]));
                return;
            case MIRConstKind::Aggregate:
            case MIRConstKind::Variant: {
                if (e.kind == MIRConstKind::Variant) write_variant_tag(dst, t, e.case_index);
                const MIRTypeRecord rec = module.types.get(t);
                for (uint32_t i = 0; i < e.child_count; ++i) {
                    const MIRConstChild* c = module.const_exprs.child(id, i);
                    if (!c) continue;
                    uint32_t off = 0;
                    if (c->name_length == 0) {
                        if (rec.kind == MIRTypeKind::Array) off = i * ti(rec.element).size;
                    } else {
                        FieldSlot fs;
                        if (resolve_field(t, module.const_exprs.child_name_view(*c), fs)) off = fs.offset;
                    }
                    const TypeId ct = module.const_exprs.get(c->expr).type;
                    eval_const_into(dst + off, ct, c->expr);
                }
                return;
            }
        }
    }

    void write_variant_tag(uint8_t* dst, TypeId t, uint32_t case_index) {
        const MIRTypeRecord rec = module.types.get(t);
        for (uint32_t i = 0; i < rec.data_count; ++i) {
            const MIRFieldInfo* f = module.types.field_of(t, i);
            if (!f) break;
            if (f->kind == static_cast<uint8_t>(MIRFieldKind::VariantTag)) {
                store_scalar(dst + f->offset, ti(f->type), case_index);
                return;
            }
        }
    }

    // ── per-function field-offset preparation ──────────────────────────────
    static const MIROperand* op_at(const MIRFunction& fn, const MIRInstruction& ins, uint32_t i) {
        if (i >= ins.operand_count) return nullptr;
        return &fn.operands[ins.operand_offset + i];
    }

    /**
     * The field/index slots of one function, resolved on first use. A comptime
     * driver lowers a whole callee graph into the module but runs only a few of
     * its functions, so resolving every operand of every function up front (a
     * 16-byte slot per operand) was pure waste — it is what made constructing an
     * interpreter dominate comptime-heavy lowering.
     */
    const std::vector<FieldSlot>& ensure_field_slots(size_t fn_index) {
        if (field_slots.size() <= fn_index) field_slots.resize(module.functions.size());
        FunctionSlots& s = field_slots[fn_index];
        const MIRFunction& fn = module.functions[fn_index];
        if (s.built && s.operand_count == fn.operands.size()) return s.slots;
        s.slots.clear();
        s.built = true;
        build_field_slots(fn_index, s);
        s.operand_count = fn.operands.size();
        return s.slots;
    }

    void build_field_slots(size_t fi, FunctionSlots& target) {
        const MIRFunction& fn = module.functions[fi];
        std::vector<FieldSlot>& fs = target.slots;
        fs.assign(fn.operands.size(), FieldSlot{});
        for (uint32_t ii = 0; ii < fn.instructions.size(); ++ii) {
            const MIRInstruction& ins = fn.instructions[ii];
            if (ins.operand_count < 2) continue;
            const MIROperand* o0 = op_at(fn, ins, 0);
            const MIROperand* o1 = op_at(fn, ins, 1);
            if (!o0 || !o1) continue;
            TypeId agg = MIR_INVALID_ID;
            switch (ins.opcode()) {
                case MIROpcode::Load:
                case MIROpcode::Store:
                case MIROpcode::FieldAddr:
                    if (o1->kind() != MIROperandKind::Constant) continue;
                    agg = base_aggregate(fn, *o0);
                    break;
                case MIROpcode::OffsetOf:
                    if (o1->kind() != MIROperandKind::Constant) continue;
                    agg = o0->type();
                    break;
                case MIROpcode::DynamicCall: {
                    const MIROperand* meth = op_at(fn, ins, 2);
                    if (!meth) continue;
                    const std::string vt_name = constant_text(o1->id);
                    const std::string m_name = constant_text(meth->id);
                    auto it = vtable_methods.find(vt_name);
                    if (it != vtable_methods.end()) {
                        for (uint32_t k = 0; k < it->second.size(); ++k) {
                            if (it->second[k] == m_name) {
                                fs[ins.operand_offset + 1].dyn_slot = k;
                                break;
                            }
                        }
                    }
                    continue;
                }
                default:
                    continue;
            }
            if (agg == MIR_INVALID_ID) continue;
            FieldSlot slot;
            if (resolve_field(agg, constant_text_view(o1->id), slot)) {
                fs[ins.operand_offset + 1] = slot;
            }
        }
    }

    // ── scalar memory ──────────────────────────────────────────────────────
    static uint64_t load_scalar(const uint8_t* p, const TypeInfo& t) {
        if (t.is_float) {
            uint32_t v; std::memcpy(&v, p, 4); return v;
        }
        if (t.is_double) {
            uint64_t v; std::memcpy(&v, p, 8); return v;
        }
        switch (t.size) {
            case 0: return 0;
            case 1: return *p;
            case 2: { uint16_t v; std::memcpy(&v, p, 2); return v; }
            case 4: { uint32_t v; std::memcpy(&v, p, 4); return v; }
            default: { uint64_t v; std::memcpy(&v, p, 8); return v; }
        }
    }

    static void store_scalar(uint8_t* p, const TypeInfo& t, uint64_t bits) {
        switch (t.is_float ? 4u : t.is_double ? 8u : t.size) {
            case 0: return;
            case 1: *p = static_cast<uint8_t>(bits); return;
            case 2: { uint16_t v = static_cast<uint16_t>(bits); std::memcpy(p, &v, 2); return; }
            case 4: { uint32_t v = static_cast<uint32_t>(bits); std::memcpy(p, &v, 4); return; }
            default: std::memcpy(p, &bits, 8); return;
        }
    }

    static uint64_t truncate(uint64_t v, const TypeInfo& t) {
        switch (t.size) {
            case 1: return static_cast<uint8_t>(v);
            case 2: return static_cast<uint16_t>(v);
            case 4: return static_cast<uint32_t>(v);
            default: return v;
        }
    }

    static int64_t sign_extend(uint64_t v, const TypeInfo& t) {
        if (!t.is_signed) return static_cast<int64_t>(v);
        switch (t.size) {
            case 1: return static_cast<int8_t>(v);
            case 2: return static_cast<int16_t>(v);
            case 4: return static_cast<int32_t>(v);
            default: return static_cast<int64_t>(v);
        }
    }

    // ── operand / place access ─────────────────────────────────────────────
    uint64_t operand_bits(Frame& f, const MIROperand& o) {
        switch (o.kind()) {
            case MIROperandKind::Value:
                return o.id < f.values.size() ? f.values[o.id] : 0;
            case MIROperandKind::Constant:
                if (o.id < const_str.size() && const_str[o.id])
                    return reinterpret_cast<uint64_t>(const_str[o.id]);
                return o.id < const_bits.size() ? const_bits[o.id] : 0;
            case MIROperandKind::Place:
                return reinterpret_cast<uint64_t>(f.places[o.id]);
            default:
                return 0;
        }
    }

    // address of a field/base operand (deref a pointer-typed place)
    uint8_t* base_addr(Frame& f, const MIROperand& base) {
        if (base.kind() == MIROperandKind::Place) {
            const MIRTypeRecord rec = module.types.get(f.fn->places[base.id].type);
            if (rec.kind == MIRTypeKind::Pointer || rec.kind == MIRTypeKind::Reference)
                return reinterpret_cast<uint8_t*>(
                    load_scalar(f.places[base.id], ti(f.fn->places[base.id].type)));
            return f.places[base.id];
        }
        return reinterpret_cast<uint8_t*>(operand_bits(f, base));
    }

    uint8_t* index_base(Frame& f, const MIROperand& base) {
        if (base.kind() == MIROperandKind::Place) {
            const MIRTypeRecord rec = module.types.get(f.fn->places[base.id].type);
            if (rec.kind == MIRTypeKind::Pointer || rec.kind == MIRTypeKind::Reference)
                return reinterpret_cast<uint8_t*>(
                    load_scalar(f.places[base.id], ti(f.fn->places[base.id].type)));
            return f.places[base.id];
        }
        return reinterpret_cast<uint8_t*>(operand_bits(f, base));
    }

    // The element type an index operation's base refers to (for stride and width).
    TypeId element_type_of(const MIRFunction& fn, const MIROperand& base) {
        const TypeId bt = (base.kind() == MIROperandKind::Place)
                              ? fn.places[base.id].type
                              : fn.values[base.id].type;
        if (bt >= types.size()) return MIR_INVALID_ID;
        const MIRTypeRecord r = module.types.get(bt);
        if (r.kind == MIRTypeKind::Array) return r.element;
        if (r.kind == MIRTypeKind::Pointer || r.kind == MIRTypeKind::Reference) {
            const TypeId pointee = r.element;
            if (pointee < types.size() &&
                module.types.get(pointee).kind == MIRTypeKind::Array) {
                return module.types.get(pointee).element;
            }
            return pointee;
        }
        return MIR_INVALID_ID;
    }

    // ── arithmetic ─────────────────────────────────────────────────────────
    static uint64_t do_unary(MIRUnaryOp op, uint64_t a, const TypeInfo& rt) {
        if (rt.is_float) {
            float f; uint32_t b = static_cast<uint32_t>(a); std::memcpy(&f, &b, 4);
            if (op == MIRUnaryOp::Neg) f = -f;
            else if (op == MIRUnaryOp::Not) f = (f == 0.0f) ? 1.0f : 0.0f;
            uint32_t o; std::memcpy(&o, &f, 4); return o;
        }
        if (rt.is_double) {
            double d; std::memcpy(&d, &a, 8);
            if (op == MIRUnaryOp::Neg) d = -d;
            else if (op == MIRUnaryOp::Not) d = (d == 0.0) ? 1.0 : 0.0;
            uint64_t o; std::memcpy(&o, &d, 8); return o;
        }
        switch (op) {
            case MIRUnaryOp::Neg: return truncate(static_cast<uint64_t>(-sign_extend(a, rt)), rt);
            case MIRUnaryOp::Not: return a == 0 ? 1 : 0;
            case MIRUnaryOp::BitNot: return truncate(~a, rt);
            case MIRUnaryOp::Plus: default: return truncate(a, rt);
        }
    }

    static float to_f32(uint64_t v, const TypeInfo& t) {
        if (t.is_float) { float f; uint32_t b = static_cast<uint32_t>(v); std::memcpy(&f, &b, 4); return f; }
        if (t.is_double) { double d; std::memcpy(&d, &v, 8); return static_cast<float>(d); }
        if (t.is_pointer) return 0.0f;
        if (t.is_signed) return static_cast<float>(sign_extend(v, t));
        return static_cast<float>(truncate(v, t));
    }

    static double to_f64(uint64_t v, const TypeInfo& t) {
        if (t.is_float) { float f; uint32_t b = static_cast<uint32_t>(v); std::memcpy(&f, &b, 4); return static_cast<double>(f); }
        if (t.is_double) { double d; std::memcpy(&d, &v, 8); return d; }
        if (t.is_pointer) return 0.0;
        if (t.is_signed) return static_cast<double>(sign_extend(v, t));
        return static_cast<double>(truncate(v, t));
    }

    static uint64_t width_mask(uint32_t size) {
        return size >= 8 ? ~0ULL : ((1ULL << (size * 8)) - 1);
    }

    // C integer promotion: a type narrower than int promotes to (signed) int.
    static TypeInfo promote(const TypeInfo& t) {
        TypeInfo c;
        if (t.is_float) { c.size = 4; c.is_float = true; return c; }
        if (t.is_double) { c.size = 8; c.is_double = true; return c; }
        if (t.is_pointer) { c.size = 8; c.is_signed = false; return c; }
        if (t.size < 4) { c.size = 4; c.is_signed = true; return c; }
        c.size = t.size ? t.size : 4;
        c.is_signed = t.is_signed;
        return c;
    }

    // C usual arithmetic conversions on the promoted operands.
    static TypeInfo usual(const TypeInfo& A, const TypeInfo& B) {
        const TypeInfo a = promote(A), b = promote(B);
        if (a.is_double || b.is_double) { TypeInfo c; c.size = 8; c.is_double = true; return c; }
        if (a.is_float || b.is_float) { TypeInfo c; c.size = 4; c.is_float = true; return c; }
        if (a.is_signed == b.is_signed) return a.size >= b.size ? a : b;
        const TypeInfo& u = a.is_signed ? b : a;
        const TypeInfo& s = a.is_signed ? a : b;
        if (u.size >= s.size) return u;
        return s; // the wider signed type represents every value of the unsigned one
    }

    uint64_t to_common(uint64_t v, const TypeInfo& src, const TypeInfo& C) const {
        uint64_t x;
        if (src.is_pointer) x = v;
        else if (src.is_signed) x = static_cast<uint64_t>(sign_extend(v, src));
        else x = truncate(v, src);
        return x & width_mask(C.size ? C.size : 8);
    }

    uint64_t elem_size(TypeId ptr_t) const {
        if (ptr_t >= types.size()) return 1;
        const MIRTypeRecord r = module.types.get(ptr_t);
        if ((r.kind == MIRTypeKind::Pointer || r.kind == MIRTypeKind::Reference) &&
            r.element < types.size()) {
            const uint32_t s = types[r.element].size;
            return s ? s : 1;
        }
        return 1;
    }

    uint64_t do_binary(MIRBinaryOp op, uint64_t a, uint64_t b, TypeId at, TypeId bt,
                       const TypeInfo& rt, const std::vector<TypeInfo>& types,
                       const MIRModule& module, bool a_addr = false, bool b_addr = false) {
        const TypeInfo& ot = (at < types.size()) ? types[at] : TypeInfo{};
        const TypeInfo& bt_i = (bt < types.size()) ? types[bt] : TypeInfo{};
        const bool a_ptr = ot.is_pointer || a_addr;
        const bool b_ptr = bt_i.is_pointer || b_addr;
        auto scale_of = [&](TypeId t, const TypeInfo& info, bool addr) -> uint64_t {
            if (info.is_pointer) return elem_size(t);
            if (addr) return info.size ? info.size : 1;
            return 1;
        };
        const bool a_fp = ot.is_float || ot.is_double;
        const bool b_fp = bt_i.is_float || bt_i.is_double;
        if (a_fp || b_fp) {
            // C usual arithmetic conversions: if either side is floating, both are.
            if (!(ot.is_double || bt_i.is_double)) {
                const float x = to_f32(a, ot), y = to_f32(b, bt_i);
                float r = 0;
                switch (op) {
                    case MIRBinaryOp::Add: r = x + y; break;
                    case MIRBinaryOp::Sub: r = x - y; break;
                    case MIRBinaryOp::Mul: r = x * y; break;
                    case MIRBinaryOp::Div: r = x / y; break;
                    case MIRBinaryOp::Eq: r = (x == y); break;
                    case MIRBinaryOp::Ne: r = (x != y); break;
                    case MIRBinaryOp::Lt: r = (x < y); break;
                    case MIRBinaryOp::Le: r = (x <= y); break;
                    case MIRBinaryOp::Gt: r = (x > y); break;
                    case MIRBinaryOp::Ge: r = (x >= y); break;
                    default: break;
                }
                if (rt.is_float) { uint32_t o; std::memcpy(&o, &r, 4); return o; }
                if (rt.is_double) { uint64_t o; std::memcpy(&o, &r, 8); return o; }
                return (r != 0.0f) ? 1 : 0;
            }
            const double x = to_f64(a, ot), y = to_f64(b, bt_i);
            double r = 0;
            switch (op) {
                case MIRBinaryOp::Add: r = x + y; break;
                case MIRBinaryOp::Sub: r = x - y; break;
                case MIRBinaryOp::Mul: r = x * y; break;
                case MIRBinaryOp::Div: r = x / y; break;
                case MIRBinaryOp::Eq: r = (x == y); break;
                case MIRBinaryOp::Ne: r = (x != y); break;
                case MIRBinaryOp::Lt: r = (x < y); break;
                case MIRBinaryOp::Le: r = (x <= y); break;
                case MIRBinaryOp::Gt: r = (x > y); break;
                case MIRBinaryOp::Ge: r = (x >= y); break;
                default: break;
            }
            if (rt.is_double) { uint64_t o; std::memcpy(&o, &r, 8); return o; }
            if (rt.is_float) { uint32_t o; std::memcpy(&o, &r, 4); return o; }
            return (r != 0.0) ? 1 : 0;
        }
        // C pointer arithmetic scales by the pointee size; do it before the
        // arithmetic conversions.
        if ((a_ptr || b_ptr) && (op == MIRBinaryOp::Add || op == MIRBinaryOp::Sub)) {
            if (a_ptr && b_ptr) {
                if (op == MIRBinaryOp::Sub) {
                    const uint64_t scale = scale_of(at, ot, a_addr);
                    return static_cast<uint64_t>(
                        (static_cast<int64_t>(a) - static_cast<int64_t>(b)) /
                        static_cast<int64_t>(scale));
                }
            } else if (a_ptr) {
                const uint64_t scale = scale_of(at, ot, a_addr);
                return (op == MIRBinaryOp::Add) ? a + b * scale : a - b * scale;
            } else if (op == MIRBinaryOp::Add) {
                return b + a * scale_of(bt, bt_i, b_addr);
            }
        }

        const TypeInfo C = usual(ot, bt_i);
        const uint64_t ca = to_common(a, ot, C);
        const uint64_t cb = to_common(b, bt_i, C);
        const bool cs = C.is_signed;

        if (op >= MIRBinaryOp::Lt && op <= MIRBinaryOp::Ne) {
            bool r = false;
            if (cs) {
                const int64_t x = sign_extend(ca, C), y = sign_extend(cb, C);
                switch (op) {
                    case MIRBinaryOp::Lt: r = x < y; break;
                    case MIRBinaryOp::Le: r = x <= y; break;
                    case MIRBinaryOp::Gt: r = x > y; break;
                    case MIRBinaryOp::Ge: r = x >= y; break;
                    case MIRBinaryOp::Eq: r = x == y; break;
                    case MIRBinaryOp::Ne: r = x != y; break;
                    default: break;
                }
            } else {
                const uint64_t x = ca, y = cb;
                switch (op) {
                    case MIRBinaryOp::Lt: r = x < y; break;
                    case MIRBinaryOp::Le: r = x <= y; break;
                    case MIRBinaryOp::Gt: r = x > y; break;
                    case MIRBinaryOp::Ge: r = x >= y; break;
                    case MIRBinaryOp::Eq: r = x == y; break;
                    case MIRBinaryOp::Ne: r = x != y; break;
                    default: break;
                }
            }
            return r ? 1 : 0;
        }
        if (cs) {
            const int64_t x = sign_extend(ca, C), y = sign_extend(cb, C);
            int64_t r = 0;
            switch (op) {
                case MIRBinaryOp::Add: r = x + y; break;
                case MIRBinaryOp::Sub: r = x - y; break;
                case MIRBinaryOp::Mul: r = x * y; break;
                case MIRBinaryOp::Div: r = y ? x / y : 0; break;
                case MIRBinaryOp::Rem: r = y ? x % y : 0; break;
                case MIRBinaryOp::Shl: r = x << (y & (C.size * 8 - 1)); break;
                case MIRBinaryOp::Shr: r = x >> (y & (C.size * 8 - 1)); break;
                case MIRBinaryOp::BitAnd: r = x & y; break;
                case MIRBinaryOp::BitOr: r = x | y; break;
                case MIRBinaryOp::BitXor: r = x ^ y; break;
                default: break;
            }
            return truncate(static_cast<uint64_t>(r), rt);
        }
        const uint64_t x = ca, y = cb;
        uint64_t r = 0;
        switch (op) {
            case MIRBinaryOp::Add: r = x + y; break;
            case MIRBinaryOp::Sub: r = x - y; break;
            case MIRBinaryOp::Mul: r = x * y; break;
            case MIRBinaryOp::Div: r = y ? x / y : 0; break;
            case MIRBinaryOp::Rem: r = y ? x % y : 0; break;
            case MIRBinaryOp::Shl: r = x << (y & (C.size * 8 - 1)); break;
            case MIRBinaryOp::Shr: r = x >> (y & (C.size * 8 - 1)); break;
            case MIRBinaryOp::BitAnd: r = x & y; break;
            case MIRBinaryOp::BitOr: r = x | y; break;
            case MIRBinaryOp::BitXor: r = x ^ y; break;
            default: break;
        }
        return truncate(r, rt);
    }

    static uint64_t do_cast(uint64_t v, const TypeInfo& s, const TypeInfo& d,
                            bool src_is_addr = false) {
        if (d.is_float) {
            float f;
            if (s.is_float) { uint32_t b = static_cast<uint32_t>(v); std::memcpy(&f, &b, 4); }
            else if (s.is_double) { double x; std::memcpy(&x, &v, 8); f = static_cast<float>(x); }
            else if (s.is_pointer) { f = 0; }
            else if (s.is_signed) { f = static_cast<float>(sign_extend(v, s)); }
            else { f = static_cast<float>(v); }
            uint32_t b; std::memcpy(&b, &f, 4); return b;
        }
        if (d.is_double) {
            double x;
            if (s.is_float) { float f; uint32_t b = static_cast<uint32_t>(v); std::memcpy(&f, &b, 4); x = f; }
            else if (s.is_double) { std::memcpy(&x, &v, 8); }
            else if (s.is_pointer) { x = 0; }
            else if (s.is_signed) { x = static_cast<double>(sign_extend(v, s)); }
            else { x = static_cast<double>(v); }
            uint64_t b; std::memcpy(&b, &x, 8); return b;
        }
        if (s.is_float) {
            float f; uint32_t b = static_cast<uint32_t>(v); std::memcpy(&f, &b, 4);
            if (d.is_pointer) return static_cast<uint64_t>(static_cast<int64_t>(f));
            return truncate(static_cast<uint64_t>(static_cast<int64_t>(f)), d);
        }
        if (s.is_double) {
            double x; std::memcpy(&x, &v, 8);
            if (d.is_pointer) return static_cast<uint64_t>(static_cast<int64_t>(x));
            return truncate(static_cast<uint64_t>(static_cast<int64_t>(x)), d);
        }
        if (d.is_pointer) {
            if (src_is_addr || s.is_pointer || s.is_aggregate) return v; // address passthrough
            if (s.is_signed) return static_cast<uint64_t>(sign_extend(v, s));
            return truncate(v, s);
        }
        if (d.is_aggregate || s.is_aggregate || s.is_pointer || src_is_addr) return v;
        // int -> int (or bool): sign- or zero-extend from the source width first,
        // then narrow to the destination.
        {
            const uint64_t x = s.is_signed ? static_cast<uint64_t>(sign_extend(v, s))
                                           : truncate(v, s);
            return truncate(x, d);
        }
    }

    // ── value read/write helpers ───────────────────────────────────────────
    static bool valid_ptr(const void* p) {
        const uintptr_t v = reinterpret_cast<uintptr_t>(p);
        if (v == 0) return true;
        if (v < 0x1000) return false;
        // A real heap/stack pointer has non-zero high 32 bits; a value with the
        // high half zero and a size above the low mmap range is a truncated
        // 64-bit pointer, which is never a valid target.
        if (v >= 0x100000 && (v >> 32) == 0) return false;
        return v < 0x0000800000000000ULL;
    }

    void read_value(Frame& f, ValueId rp, uint8_t* src, const TypeInfo& rti) {
        if (!valid_ptr(src)) {
            std::fprintf(stderr, "[MIR] read_value bad src=%p fn=%s\n", src,
                         symbol_name(f.fn->symbol).c_str());
            std::fflush(stderr);
            std::abort();
        }
        if (!src) { f.values[rp] = 0; return; }
        if (rti.is_aggregate) {
            const uint32_t sz = rti.size ? rti.size : 1;
            uint8_t* copy = f.arena.alloc(sz);
            if (rti.size) std::memcpy(copy, src, rti.size);
            f.values[rp] = reinterpret_cast<uint64_t>(copy);
        } else {
            f.values[rp] = load_scalar(src, rti);
        }
    }

    // Write `src` (a Value scalar/pointer, or a Place) into `dst` typed `dti`.
    void store_into(Frame& f, uint8_t* dst, const TypeInfo& dti, const MIROperand& src) {
        if (!valid_ptr(dst)) {
            std::fprintf(stderr, "[MIR] store_into bad dst=%p fn=%s\n", dst,
                         symbol_name(f.fn->symbol).c_str());
            std::fflush(stderr);
            std::abort();
        }
        if (src.kind() == MIROperandKind::Place) {
            uint8_t* s = f.places[src.id];
            const TypeInfo& sti = ti(f.fn->places[src.id].type);
            if (dti.is_aggregate) {
                // an array source copies sizeof(source) bytes (a partial literal
                // leaves the destination's tail as it was); a same-size aggregate
                // copies the whole value.
                uint32_t n = dti.size;
                if (sti.size && sti.size < n) n = sti.size;
                if (n && s) std::memcpy(dst, s, n);
            } else {
                store_scalar(dst, dti, load_scalar(s, sti));
            }
        } else {
            const uint64_t bits = operand_bits(f, src);
            if (dti.is_aggregate) {
                uint32_t n = dti.size;
                if (src.kind() == MIROperandKind::Value && src.id < f.values.size()) {
                    const TypeInfo& sti = ti(f.fn->values[src.id].type);
                    if (sti.size && sti.size < n) n = sti.size;
                }
                if (n) std::memcpy(dst, reinterpret_cast<uint8_t*>(bits), n);
            } else {
                store_scalar(dst, dti, bits);
            }
        }
    }

    // ── the interpreter loop ───────────────────────────────────────────────
    uint64_t call_function(size_t fn_index, const uint64_t* args, uint32_t nargs) {
        if (stack_exhausted) return 0;
        if (chem::comptime_stack_exhausted()) {
            // a comptime function which calls itself never returns; each nested
            // call adds a frame on the compiler's stack, so stop before it
            // overflows and let the top-level call report it
            stack_exhausted = true;
            return 0;
        }
        const MIRFunction& fn = module.functions[fn_index];
        if (depth == pool.size()) pool.push_back(std::make_unique<Frame>());
        Frame& f = *pool[depth++];
        f.arena.reset();
        f.prepare(&fn, types);
        const std::vector<FieldSlot>& fslots = ensure_field_slots(fn_index);
        const TypeId ret_tid =
            (fn.function_type != MIR_INVALID_ID && fn.function_type < module.types.size())
                ? module.types.get(fn.function_type).element
                : MIR_INVALID_ID;
        const TypeInfo ret_ti = ti(ret_tid);
        const bool trunc_ret = !ret_ti.is_aggregate;

        uint32_t ai = 0;
        for (uint32_t i = 0; i < fn.instructions.size(); ++i) {
            const MIRInstruction& ins = fn.instructions[i];
            if (ins.opcode() == MIROpcode::Param) {
                f.values[ins.result_or_place] = ai < nargs ? args[ai] : 0;
                ++ai;
            }
        }

        uint32_t ip = fn.blocks[fn.entry_block].inst_start;
        uint64_t result = 0;
        bool returned = false;
        while (!returned) {
            const MIRInstruction& ins = fn.instructions[ip];
            const uint32_t rp = ins.result_or_place;
            switch (ins.opcode()) {
                case MIROpcode::Nop:
                case MIROpcode::Alloca:
                case MIROpcode::Param:
                    break;
                case MIROpcode::ConstInt:
                case MIROpcode::ConstBool:
                case MIROpcode::ConstNull:
                case MIROpcode::ConstFloat:
                case MIROpcode::ConstDouble:
                case MIROpcode::ConstString: {
                    const MIROperand* c = op_at(fn, ins, 0);
                    f.values[rp] = c ? operand_bits(f, *c) : 0;
                    break;
                }
                case MIROpcode::GlobalAddr: {
                    const MIROperand* s = op_at(fn, ins, 0);
                    f.values[rp] = (s && s->id < global_addr.size())
                                       ? reinterpret_cast<uint64_t>(global_addr[s->id])
                                       : 0;
                    break;
                }
                case MIROpcode::FunctionAddr: {
                    const MIROperand* s = op_at(fn, ins, 0);
                    f.values[rp] = (s && s->id < callees.size())
                                       ? reinterpret_cast<uint64_t>(&callees[s->id])
                                       : 0;
                    break;
                }
                case MIROpcode::Load: {
                    const MIROperand* base = op_at(fn, ins, 0);
                    const MIROperand* o1 = op_at(fn, ins, 1);
                    const TypeInfo& rti = ti(fn.values[rp].type);
                    if (!base || !o1) break;
                    if (o1->kind() == MIROperandKind::Type) {
                        uint8_t* src = (base->kind() == MIROperandKind::Place)
                                           ? f.places[base->id]
                                           : reinterpret_cast<uint8_t*>(operand_bits(f, *base));
                        read_value(f, rp, src, rti);
                    } else if (o1->kind() == MIROperandKind::Constant) {
                        const FieldSlot& slot = fslots[ins.operand_offset + 1];
                        read_value(f, rp, base_addr(f, *base) + slot.offset, rti);
                    } else if (o1->kind() == MIROperandKind::Value) {
                        const int64_t idx = static_cast<int64_t>(operand_bits(f, *o1));
                        read_value(f, rp, index_base(f, *base) + idx * rti.size, rti);
                    }
                    break;
                }
                case MIROpcode::Store: {
                    const MIROperand* o0 = op_at(fn, ins, 0);
                    const MIROperand* o1 = op_at(fn, ins, 1);
                    if (!o0 || !o1) break;
                    if (o1->kind() == MIROperandKind::Constant) {
                        // field store: base, Constant(field), value
                        const MIROperand* val = op_at(fn, ins, 2);
                        if (!val) break;
                        const FieldSlot& slot = fslots[ins.operand_offset + 1];
                        const TypeInfo& vti = ti(slot.valid ? slot.type : MIR_INVALID_ID);
                        store_into(f, base_addr(f, *o0) + slot.offset, vti, *val);
                    } else if (o1->kind() == MIROperandKind::Value &&
                               ins.operand_count >= 3) {
                        // element store: base, index(Value), value. The stored
                        // width is the array element's, not the value's.
                        const MIROperand* val = op_at(fn, ins, 2);
                        if (!val) break;
                        TypeId et = element_type_of(fn, *o0);
                        if (et == MIR_INVALID_ID) {
                            et = (val->kind() == MIROperandKind::Place)
                                     ? f.fn->places[val->id].type
                                     : f.fn->values[val->id].type;
                        }
                        const TypeInfo& eti = ti(et);
                        const int64_t idx = static_cast<int64_t>(operand_bits(f, *o1));
                        store_into(f, index_base(f, *o0) + idx * eti.size, eti, *val);
                    } else {
                        // plain: dest, src (dest may be a place or a pointer value)
                        uint8_t* dst;
                        TypeId dt;
                        if (o0->kind() == MIROperandKind::Place) {
                            dst = f.places[o0->id];
                            dt = f.fn->places[o0->id].type;
                        } else {
                            dst = reinterpret_cast<uint8_t*>(operand_bits(f, *o0));
                            dt = pointee(f.fn->values[o0->id].type);
                            if (dt == MIR_INVALID_ID) {
                                dt = (o1->kind() == MIROperandKind::Place)
                                         ? f.fn->places[o1->id].type
                                         : f.fn->values[o1->id].type;
                            }
                        }
                        store_into(f, dst, ti(dt), *o1);
                    }
                    break;
                }
                case MIROpcode::AddressOf: {
                    const MIROperand* p = op_at(fn, ins, 0);
                    f.values[rp] = p ? reinterpret_cast<uint64_t>(f.places[p->id]) : 0;
                    break;
                }
                case MIROpcode::FieldAddr: {
                    const MIROperand* base = op_at(fn, ins, 0);
                    if (!base) break;
                    const FieldSlot& slot = fslots[ins.operand_offset + 1];
                    f.values[rp] = reinterpret_cast<uint64_t>(
                        base_addr(f, *base) + (slot.valid ? slot.offset : 0));
                    break;
                }
                case MIROpcode::IndexAddr: {
                    const MIROperand* base = op_at(fn, ins, 0);
                    const MIROperand* idx = op_at(fn, ins, 1);
                    if (!base || !idx) break;
                    const uint32_t stride = ti(fn.values[rp].type).size;
                    const int64_t i = static_cast<int64_t>(operand_bits(f, *idx));
                    f.values[rp] = reinterpret_cast<uint64_t>(index_base(f, *base) + i * stride);
                    break;
                }
                case MIROpcode::Unary: {
                    const MIROperand* a = op_at(fn, ins, 0);
                    const MIROperand* opc = op_at(fn, ins, 1);
                    if (!a || !opc) break;
                    const auto op = static_cast<MIRUnaryOp>(module.constants.get(opc->id).bits);
                    f.values[rp] = do_unary(op, operand_bits(f, *a), ti(fn.values[rp].type));
                    break;
                }
                case MIROpcode::Binary:
                case MIROpcode::Compare: {
                    const MIROperand* a = op_at(fn, ins, 0);
                    const MIROperand* b = op_at(fn, ins, 1);
                    const MIROperand* opc = op_at(fn, ins, 2);
                    if (!a || !b || !opc) break;
                    const auto op = static_cast<MIRBinaryOp>(module.constants.get(opc->id).bits);
                    const bool aa = a->kind() == MIROperandKind::Value &&
                                    a->id < fn.values.size() &&
                                    (fn.values[a->id].flags & VF_ADDRESSABLE) != 0;
                    const bool bb = b->kind() == MIROperandKind::Value &&
                                    b->id < fn.values.size() &&
                                    (fn.values[b->id].flags & VF_ADDRESSABLE) != 0;
                    f.values[rp] = do_binary(op, operand_bits(f, *a), operand_bits(f, *b),
                                             a->type(), b->type(), ti(fn.values[rp].type),
                                             types, module, aa, bb);
                    break;
                }
                case MIROpcode::Cast: {
                    const MIROperand* a = op_at(fn, ins, 0);
                    if (!a) break;
                    const bool src_addr =
                        a->kind() == MIROperandKind::Value && a->id < fn.values.size() &&
                        (fn.values[a->id].flags & VF_ADDRESSABLE) != 0;
                    f.values[rp] = do_cast(operand_bits(f, *a), ti(a->type()),
                                           ti(fn.values[rp].type), src_addr);
                    break;
                }
                case MIROpcode::SizeOf: {
                    const MIROperand* t = op_at(fn, ins, 0);
                    f.values[rp] = t ? ti(t->type()).size : 0;
                    break;
                }
                case MIROpcode::AlignOf: {
                    const MIROperand* t = op_at(fn, ins, 0);
                    f.values[rp] = t ? ti(t->type()).align : 1;
                    break;
                }
                case MIROpcode::OffsetOf: {
                    const FieldSlot& slot = fslots[ins.operand_offset + 1];
                    f.values[rp] = slot.valid ? slot.offset : 0;
                    break;
                }
                case MIROpcode::Call: {
                    const MIROperand* callee = op_at(fn, ins, 0);
                    if (!callee) break;
                    const MIROperand* maybe_place = op_at(fn, ins, 1);
                    const bool sret = maybe_place && maybe_place->kind() == MIROperandKind::Place;
                    const uint32_t arg_start = sret ? 2 : 1;
                    uint64_t stack_args[64];
                    uint32_t n = 0;
                    if (sret) stack_args[n++] = reinterpret_cast<uint64_t>(f.places[maybe_place->id]);
                    for (uint32_t a = arg_start; a < ins.operand_count && n < 64; ++a)
                        stack_args[n++] = operand_bits(f, *op_at(fn, ins, a));
                    const Callee& c = (callee->id < callees.size()) ? callees[callee->id] : Callee{};
                    uint64_t r = 0;
                    if (c.kind == Callee::Kind::Mir) r = call_function(c.fn_index, stack_args, n);
                    else if (c.kind == Callee::Kind::Host && c.host) r = c.host(this, stack_args, n);
                    if (!sret && rp < f.values.size()) f.values[rp] = r;
                    break;
                }
                case MIROpcode::CallIndirect: {
                    const MIROperand* fnop = op_at(fn, ins, 0);
                    if (!fnop) break;
                    const Callee* c = reinterpret_cast<const Callee*>(operand_bits(f, *fnop));
                    if (!c) break;
                    const MIROperand* maybe_place = op_at(fn, ins, 1);
                    const bool sret = maybe_place && maybe_place->kind() == MIROperandKind::Place;
                    const uint32_t arg_start = sret ? 2 : 1;
                    uint64_t stack_args[64];
                    uint32_t n = 0;
                    if (sret) stack_args[n++] = reinterpret_cast<uint64_t>(f.places[maybe_place->id]);
                    for (uint32_t a = arg_start; a < ins.operand_count && n < 64; ++a)
                        stack_args[n++] = operand_bits(f, *op_at(fn, ins, a));
                    uint64_t r = 0;
                    if (c->kind == Callee::Kind::Mir) r = call_function(c->fn_index, stack_args, n);
                    else if (c->kind == Callee::Kind::Host && c->host) r = c->host(this, stack_args, n);
                    if (!sret && rp < f.values.size()) f.values[rp] = r;
                    break;
                }
                case MIROpcode::Init: {
                    const MIROperand* dest = op_at(fn, ins, 0);
                    const MIROperand* ctor = op_at(fn, ins, 1);
                    if (!dest || !ctor) break;
                    uint64_t stack_args[64];
                    uint32_t n = 0;
                    stack_args[n++] = reinterpret_cast<uint64_t>(f.places[dest->id]);
                    for (uint32_t a = 2; a < ins.operand_count && n < 64; ++a)
                        stack_args[n++] = operand_bits(f, *op_at(fn, ins, a));
                    const Callee& c = (ctor->id < callees.size()) ? callees[ctor->id] : Callee{};
                    if (c.kind == Callee::Kind::Mir) call_function(c.fn_index, stack_args, n);
                    break;
                }
                case MIROpcode::CopyInit:
                case MIROpcode::MoveInit: {
                    const MIROperand* dest = op_at(fn, ins, 0);
                    const MIROperand* src = op_at(fn, ins, 1);
                    if (!dest || !src) break;
                    uint8_t* d = f.places[dest->id];
                    const TypeInfo& dt = ti(fn.places[dest->id].type);
                    uint8_t* s = (src->kind() == MIROperandKind::Place)
                                     ? f.places[src->id]
                                     : reinterpret_cast<uint8_t*>(operand_bits(f, *src));
                    if (s && d && dt.size) std::memcpy(d, s, dt.size);
                    break;
                }
                case MIROpcode::Assign: {
                    const MIROperand* dest = op_at(fn, ins, 0);
                    const MIROperand* src = op_at(fn, ins, 1);
                    if (!dest || !src) break;
                    uint8_t* d = f.places[dest->id];
                    const TypeInfo& dt = ti(fn.places[dest->id].type);
                    if (dt.is_aggregate) {
                        uint8_t* s = (src->kind() == MIROperandKind::Place)
                                         ? f.places[src->id]
                                         : reinterpret_cast<uint8_t*>(operand_bits(f, *src));
                        if (s && d && dt.size) std::memcpy(d, s, dt.size);
                    } else {
                        store_scalar(d, dt, operand_bits(f, *src));
                    }
                    break;
                }
                case MIROpcode::MemCpy: {
                    const MIROperand* dest = op_at(fn, ins, 0);
                    const MIROperand* src = op_at(fn, ins, 1);
                    const MIROperand* sz = op_at(fn, ins, 2);
                    if (!dest || !src) break;
                    uint8_t* d = f.places[dest->id];
                    uint8_t* s = (src->kind() == MIROperandKind::Place)
                                     ? f.places[src->id]
                                     : reinterpret_cast<uint8_t*>(operand_bits(f, *src));
                    const size_t n = sz ? static_cast<size_t>(operand_bits(f, *sz))
                                        : ti(fn.places[dest->id].type).size;
                    if (d && s && n) std::memmove(d, s, n);
                    break;
                }
                case MIROpcode::MemSet: {
                    const MIROperand* dest = op_at(fn, ins, 0);
                    const MIROperand* byte = op_at(fn, ins, 1);
                    const MIROperand* sz = op_at(fn, ins, 2);
                    if (!dest) break;
                    uint8_t* d = (dest->kind() == MIROperandKind::Place)
                                     ? f.places[dest->id]
                                     : reinterpret_cast<uint8_t*>(operand_bits(f, *dest));
                    const uint8_t b = byte ? static_cast<uint8_t>(operand_bits(f, *byte)) : 0;
                    size_t n;
                    if (sz) n = static_cast<size_t>(operand_bits(f, *sz));
                    else if (dest->kind() == MIROperandKind::Place) n = ti(fn.places[dest->id].type).size;
                    else break;
                    if (d && n) std::memset(d, b, n);
                    break;
                }
                case MIROpcode::SetDrop: {
                    const MIROperand* p = op_at(fn, ins, 0);
                    const MIROperand* c = op_at(fn, ins, 1);
                    if (!p || !c) break;
                    store_scalar(f.places[p->id], ti(fn.places[p->id].type), operand_bits(f, *c));
                    break;
                }
                case MIROpcode::MoveOut:
                    // dataflow marker only
                    break;
                case MIROpcode::Drop:
                case MIROpcode::Destroy: {
                    const MIROperand* p = op_at(fn, ins, 0);
                    const MIROperand* d = op_at(fn, ins, 1);
                    if (!p || !d) break;
                    if (ins.opcode() == MIROpcode::Drop && ins.operand_count >= 3) {
                        const MIROperand* flag = op_at(fn, ins, 2);
                        if (flag && operand_bits(f, *flag) == 0) break;
                    }
                    const TypeId ptype = f.fn->places[p->id].type;
                    const MIRTypeRecord pr = module.types.get(ptype);
                    if (pr.kind == MIRTypeKind::Array) {
                        const uint32_t cnt = pr.data_count;
                        const uint32_t stride = ti(pr.element).size;
                        for (uint32_t i = cnt; i-- > 0;) {
                            call_dtor(d->id, reinterpret_cast<uint64_t>(f.places[p->id] + i * stride));
                        }
                    } else {
                        uint64_t arg = (pr.kind == MIRTypeKind::Pointer || pr.kind == MIRTypeKind::Reference)
                                           ? load_scalar(f.places[p->id], ti(ptype))
                                           : reinterpret_cast<uint64_t>(f.places[p->id]);
                        call_dtor(d->id, arg);
                    }
                    break;
                }
                case MIROpcode::Br: {
                    const MIROperand* t = op_at(fn, ins, 0);
                    if (!t) { returned = true; break; }
                    ip = fn.blocks[t->id].inst_start;
                    continue;
                }
                case MIROpcode::CondBr: {
                    const MIROperand* c = op_at(fn, ins, 0);
                    const MIROperand* t = op_at(fn, ins, 1);
                    const MIROperand* e = op_at(fn, ins, 2);
                    if (!c || !t || !e) { returned = true; break; }
                    ip = fn.blocks[operand_bits(f, *c) != 0 ? t->id : e->id].inst_start;
                    continue;
                }
                case MIROpcode::Return: {
                    if (ins.operand_count > 0) {
                        result = operand_bits(f, *op_at(fn, ins, 0));
                        if (trunc_ret) result = truncate(result, ret_ti);
                    }
                    returned = true;
                    break;
                }
                case MIROpcode::Select: {
                    const MIROperand* c = op_at(fn, ins, 0);
                    const MIROperand* t = op_at(fn, ins, 1);
                    const MIROperand* e = op_at(fn, ins, 2);
                    if (!c || !t || !e) break;
                    f.values[rp] = operand_bits(f, *c) ? operand_bits(f, *t) : operand_bits(f, *e);
                    break;
                }
                case MIROpcode::Gep: {
                    const MIROperand* base = op_at(fn, ins, 0);
                    if (!base) break;
                    uint64_t addr = operand_bits(f, *base);
                    for (uint32_t k = 1; k + 1 < ins.operand_count; ++k)
                        addr += static_cast<int64_t>(operand_bits(f, *op_at(fn, ins, k)));
                    f.values[rp] = addr;
                    break;
                }
                case MIROpcode::DynamicCall: {
                    const MIROperand* fat = op_at(fn, ins, 0);
                    if (!fat) break;
                    uint8_t* fa = base_addr(f, *fat);
                    if (!valid_ptr(fa)) break;
                    uint64_t first = 0, second = 0;
                    std::memcpy(&first, fa, 8);
                    std::memcpy(&second, fa + 8, 8);
                    const uint32_t slot = fslots[ins.operand_offset + 1].dyn_slot;
                    if (slot == MIR_INVALID_ID || second == 0) break;
                    const Callee* c = reinterpret_cast<const Callee**>(second)[slot];
                    if (!c) break;
                    uint64_t stack_args[64];
                    uint32_t n = 0;
                    stack_args[n++] = first;
                    for (uint32_t a = 3; a < ins.operand_count && n < 64; ++a)
                        stack_args[n++] = operand_bits(f, *op_at(fn, ins, a));
                    uint64_t r = 0;
                    if (c->kind == Callee::Kind::Mir) r = call_function(c->fn_index, stack_args, n);
                    else if (c->kind == Callee::Kind::Host && c->host)
                        r = c->host(this, stack_args, n);
                    if (rp < f.values.size()) f.values[rp] = r;
                    break;
                }
                case MIROpcode::AtomicFence:
                    // Comptime is single-threaded; a fence has no observable
                    // effect. Atomics with observable loads/stores remain
                    // unsupported below and are hard diagnostics.
                    break;
                default:
                    unsupported_error = std::string("MIR interpreter: unsupported opcode '")
                        + opcode_name(ins.opcode()) + "'";
                    returned = true;
                    break;
            }
            if (returned) break;
            ++ip;
        }
        --depth;
        return result;
    }

    void call_dtor(SymbolId dtor, uint64_t self_ptr) {
        if (dtor >= callees.size()) return;
        const Callee& c = callees[dtor];
        if (c.kind == Callee::Kind::Mir) call_function(c.fn_index, &self_ptr, 1);
    }
};

// ── host functions ─────────────────────────────────────────────────────────
uint64_t MIRInterpreter::Impl::host_malloc(void* self, const uint64_t* a, uint32_t n) {
    const size_t sz = n ? static_cast<size_t>(a[0]) : 1;
    return reinterpret_cast<uint64_t>(std::malloc(sz ? sz : 1));
}
uint64_t MIRInterpreter::Impl::host_free(void*, const uint64_t*, uint32_t) { return 0; }
uint64_t MIRInterpreter::Impl::host_realloc(void*, const uint64_t* a, uint32_t n) {
    if (n < 2) return 0;
    return reinterpret_cast<uint64_t>(
        std::realloc(reinterpret_cast<void*>(a[0]), static_cast<size_t>(a[1])));
}
uint64_t MIRInterpreter::Impl::host_memcpy(void*, const uint64_t* a, uint32_t n) {
    if (n < 3) return (a && n) ? a[0] : 0;
    std::memcpy(reinterpret_cast<void*>(a[0]), reinterpret_cast<void*>(a[1]),
                static_cast<size_t>(a[2]));
    return a[0];
}
uint64_t MIRInterpreter::Impl::host_memmove(void*, const uint64_t* a, uint32_t n) {
    if (n < 3) return (a && n) ? a[0] : 0;
    std::memmove(reinterpret_cast<void*>(a[0]), reinterpret_cast<void*>(a[1]),
                 static_cast<size_t>(a[2]));
    return a[0];
}
uint64_t MIRInterpreter::Impl::host_memset(void*, const uint64_t* a, uint32_t n) {
    if (n < 3) return (a && n) ? a[0] : 0;
    std::memset(reinterpret_cast<void*>(a[0]), static_cast<int>(a[1]),
                static_cast<size_t>(a[2]));
    return a[0];
}
uint64_t MIRInterpreter::Impl::host_strlen(void*, const uint64_t* a, uint32_t n) {
    if (!n) return 0;
    const char* s = reinterpret_cast<const char*>(a[0]);
    return s ? std::strlen(s) : 0;
}
uint64_t MIRInterpreter::Impl::host_strcmp(void*, const uint64_t* a, uint32_t n) {
    if (n < 2) return 0;
    const char* x = reinterpret_cast<const char*>(a[0]);
    const char* y = reinterpret_cast<const char*>(a[1]);
    if (!x || !y) return x == y ? 0 : (x ? 1 : static_cast<uint64_t>(-1));
    return static_cast<uint64_t>(static_cast<int64_t>(std::strcmp(x, y)));
}
uint64_t MIRInterpreter::Impl::host_putchar(void*, const uint64_t* a, uint32_t n) {
    if (!n) return 0;
    std::putchar(static_cast<int>(a[0]));
    return static_cast<uint64_t>(static_cast<unsigned char>(a[0]));
}
uint64_t MIRInterpreter::Impl::host_abort(void*, const uint64_t*, uint32_t) {
    std::fputs("[MIR interpreter] abort()\n", stderr);
    std::abort();
    return 0;
}
uint64_t MIRInterpreter::Impl::host_exit(void*, const uint64_t* a, uint32_t n) {
    std::exit(n ? static_cast<int>(a[0]) : 0);
    return 0;
}

uint64_t MIRInterpreter::Impl::host_printf(void*, const uint64_t* a, uint32_t n) {
    if (n == 0) return 0;
    const char* fmt = reinterpret_cast<const char*>(a[0]);
    if (!fmt) return 0;
    uint32_t ai = 1;
    std::string out;
    for (const char* p = fmt; *p; ++p) {
        if (*p != '%') { out.push_back(*p); continue; }
        ++p;
        while (*p && std::strchr("-+ #0", *p)) ++p;
        while (*p >= '0' && *p <= '9') ++p;
        if (*p == '.') { ++p; while (*p >= '0' && *p <= '9') ++p; }
        while (*p == 'l' || *p == 'h' || *p == 'z' || *p == 'j' || *p == 't') ++p;
        const uint64_t arg = ai < n ? a[ai++] : 0;
        char buf[64];
        switch (*p) {
            case 'd': case 'i':
                std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(arg));
                out += buf; break;
            case 'u':
                std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(arg));
                out += buf; break;
            case 'x':
                std::snprintf(buf, sizeof(buf), "%llx", static_cast<unsigned long long>(arg));
                out += buf; break;
            case 'X':
                std::snprintf(buf, sizeof(buf), "%llX", static_cast<unsigned long long>(arg));
                out += buf; break;
            case 'c':
                out.push_back(static_cast<char>(arg)); break;
            case 's': {
                const char* s = reinterpret_cast<const char*>(arg);
                out += s ? s : "(null)";
                break;
            }
            case 'p':
                std::snprintf(buf, sizeof(buf), "0x%llx", static_cast<unsigned long long>(arg));
                out += buf; break;
            case '%':
                out.push_back('%'); --ai; break;
            case 'f': case 'g': {
                double d; std::memcpy(&d, &arg, 8);
                std::snprintf(buf, sizeof(buf), *p == 'f' ? "%f" : "%g", d);
                out += buf; break;
            }
            default:
                break;
        }
    }
    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fflush(stdout);
    return out.size();
}

// ── lifecycle ──────────────────────────────────────────────────────────────

MIRInterpreter::MIRInterpreter(const MIRModule& module) : impl_(new Impl(module)) {
    update();
}

MIRInterpreter::~MIRInterpreter() = default;

void MIRInterpreter::update() {
    impl_->build_types();
    impl_->build_constants();
    impl_->build_symbols_and_globals();
    impl_->build_vtables();
    impl_->wire_vtable_globals();
    // field slots stay lazy (built per function on its first call)
}

bool MIRInterpreter::has_function(const std::string& entry) const {
    return impl_->fn_by_name.find(entry) != impl_->fn_by_name.end();
}

bool MIRInterpreter::call(const std::string& name, const uint64_t* args, uint32_t nargs,
                          uint64_t& out_result, std::string& error) {
    update();
    auto it = impl_->fn_by_name.find(name);
    if (it == impl_->fn_by_name.end()) {
        error = "no function named '" + name + "' in the MIR module";
        return false;
    }
    impl_->stack_exhausted = false;
    impl_->unsupported_error.clear();
    out_result = impl_->call_function(it->second, args, nargs);
    if (impl_->stack_exhausted) {
        error = chem::comptime_stack_error_message();
        return false;
    }
    if (!impl_->unsupported_error.empty()) {
        error = impl_->unsupported_error;
        return false;
    }
    return true;
}

bool MIRInterpreter::call_sret(const std::string& name, const uint64_t* args, uint32_t nargs,
                               void* out_buf, std::string& error) {
    update();
    auto it = impl_->fn_by_name.find(name);
    if (it == impl_->fn_by_name.end()) {
        error = "no function named '" + name + "' in the MIR module";
        return false;
    }
    std::vector<uint64_t> cells;
    cells.reserve(static_cast<size_t>(nargs) + 1);
    cells.push_back(reinterpret_cast<uint64_t>(out_buf));
    for (uint32_t i = 0; i < nargs; ++i) cells.push_back(args[i]);
    impl_->stack_exhausted = false;
    impl_->unsupported_error.clear();
    impl_->call_function(it->second, cells.data(), nargs + 1);
    if (impl_->stack_exhausted) {
        error = chem::comptime_stack_error_message();
        return false;
    }
    if (!impl_->unsupported_error.empty()) {
        error = impl_->unsupported_error;
        return false;
    }
    return true;
}

bool MIRInterpreter::run(const std::string& entry, int argc, const char* const* argv,
                         long long& out_result, std::string& error) {
    update();
    auto it = impl_->fn_by_name.find(entry);
    if (it == impl_->fn_by_name.end()) {
        error = "no function named '" + entry + "' in the MIR module";
        return false;
    }
    const size_t fn_index = it->second;
    const MIRFunction& fn = impl_->module.functions[fn_index];

    uint64_t args[2] = {0, 0};
    uint32_t nargs = 0;
    if (fn.function_type != MIR_INVALID_ID) {
        const MIRTypeRecord ft = impl_->module.types.get(fn.function_type);
        if (ft.data_count > 0) {
            args[0] = static_cast<uint64_t>(argc);
            args[1] = reinterpret_cast<uint64_t>(argv);
            nargs = ft.data_count < 2 ? ft.data_count : 2;
        }
    }
    impl_->stack_exhausted = false;
    impl_->unsupported_error.clear();
    out_result = static_cast<long long>(impl_->call_function(fn_index, args, nargs));
    if (impl_->stack_exhausted) {
        error = chem::comptime_stack_error_message();
        return false;
    }
    if (!impl_->unsupported_error.empty()) {
        error = impl_->unsupported_error;
        return false;
    }
    return true;
}

bool MIRInterpreter::stack_exhausted() const {
    return impl_->stack_exhausted;
}

} // namespace mir
