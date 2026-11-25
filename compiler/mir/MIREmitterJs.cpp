// Copyright (c) Chemical Language Foundation 2026.
//
// MIR-to-JavaScript emitter. See MIREmitterJs.h and
// lang/docs/js-backend-abi.md for the ABI this prints against.
//
// Shapes of the emitted code:
//
//   values  `v<id>`   an SSA value: an integer, a float, or an address
//   places  `p<id>`   a frame slot, allocated once at function entry
//
// Every instruction result is assigned to `v<id>` (`result_or_place` indexes the
// value table for anything except `alloc`), which is exactly what the MIR
// interpreter does — see MIRInterpreter's per-opcode `f.values[rp] = ...`.

#include "MIREmitterJs.h"
#include "MIRTargetSupport.h"
#include "MIREmitter.h" // append_uint

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mir {

std::string minify_js_source(const std::string& source) {
    std::string out;
    out.reserve(source.size());
    const size_t n = source.size();

    // Whitespace is decided when the *next* token is known: a run of spaces
    // becomes one space only where two word tokens (or `+ +` / `- -`) would
    // otherwise merge into a different token.
    bool pending_space = false;
    auto is_word = [](char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
               (ch >= '0' && ch <= '9') || ch == '_' || ch == '$';
    };
    auto flush_space = [&](char next) {
        if (!pending_space) return;
        pending_space = false;
        if (out.empty()) return;
        const char prev = out.back();
        if ((is_word(prev) && is_word(next)) ||
            ((prev == '+' || prev == '-') && prev == next)) {
            out += ' ';
        }
    };

    size_t i = 0;
    while (i < n) {
        const char c = source[i];
        // Comments are dropped entirely. `//# sourceMappingURL` is not special:
        // a minified module has no map (see MIREmitJsOptions::minify).
        if (c == '/' && i + 1 < n && source[i + 1] == '/') {
            while (i < n && source[i] != '\n') ++i;
            pending_space = true;
            continue;
        }
        if (c == '/' && i + 1 < n && source[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(source[i] == '*' && source[i + 1] == '/')) ++i;
            i = i + 2 < n ? i + 2 : n;
            pending_space = true;
            continue;
        }
        // Strings, template literals: byte for byte, whitespace and `//` included.
        if (c == '"' || c == '\'' || c == '`') {
            flush_space(c);
            out += c;
            ++i;
            while (i < n) {
                const char s = source[i];
                if (s == '\\' && i + 1 < n) {
                    out += s;
                    out += source[i + 1];
                    i += 2;
                    continue;
                }
                out += s;
                ++i;
                if (s == c) break;
            }
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            pending_space = true;
            ++i;
            continue;
        }
        flush_space(c);
        out += c;
        ++i;
    }
    return out;
}

namespace {

// ── type facts, computed once per TypeId ────────────────────────────────────

struct JsType {
    uint32_t size = 0;
    uint32_t align = 1;
    bool is_float = false;  // f32
    bool is_double = false; // f64
    bool is_ptr = false;
    bool is_agg = false;
    bool is_signed = false;
    bool valid = false;

    bool is_scalar() const { return valid && !is_agg; }
};

std::string float_literal(double v, bool single) {
    if (std::isnan(v)) return "NaN";
    if (std::isinf(v)) return v < 0 ? "-Infinity" : "Infinity";
    char buf[64];
    std::snprintf(buf, sizeof(buf), single ? "%.9g" : "%.17g", v);
    std::string s(buf);
    if (s.find_first_of(".eE") == std::string::npos) s += ".0";
    return s;
}

bool constant_text(const MIRModule& module, uint32_t id, std::string_view& out) {
    if (id >= module.constants.size()) return false;
    const MIRConstant& c = module.constants.get(id);
    if (c.kind != MIRConstantKind::String && c.kind != MIRConstantKind::Bytes) return false;
    out = std::string_view(module.constants.data.data_at(c.data_offset), c.data_count);
    return true;
}

// ── source maps ─────────────────────────────────────────────────────────────

/** One generated line that came from a Chemical instruction. */
struct JsMapping {
    uint32_t line = 0;     // 0-based line in the emitted module
    uint32_t file = 0;     // index into MIRModule::source_files
    uint32_t src_line = 0; // 0-based in the source file
    uint32_t src_col = 0;
};

/** Base64 VLQ, the encoding source-map "mappings" uses. */
void vlq_encode(std::string& out, int32_t value) {
    static const char* kB64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    uint32_t v = value < 0 ? ((static_cast<uint32_t>(-static_cast<int64_t>(value)) << 1) | 1u)
                           : (static_cast<uint32_t>(value) << 1);
    do {
        uint32_t digit = v & 0x1F;
        v >>= 5;
        if (v != 0) digit |= 0x20u;
        out += kB64[digit];
    } while (v != 0);
}

void json_escape(std::string& out, std::string_view text) {
    for (char c : text) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
}

/**
 * Build a version-3 source map for one emitted module.
 *
 * One segment per mapped line, at column 0, so the mapping is exact for
 * statement-level debugging (a browser breakpoint or stack frame lands on the
 * Chemical line that produced the JS line). `line_offset` is the number of
 * lines the module's preamble occupies.
 */
std::string build_source_map(const MIRModule& module, std::vector<JsMapping> maps,
                             uint32_t line_offset, const std::string& file_name) {
    std::stable_sort(maps.begin(), maps.end(),
                     [](const JsMapping& a, const JsMapping& b) { return a.line < b.line; });

    std::string mappings;
    // `current_line` is the group (generated line) currently open. It advances
    // only when the loop below emits a ';' — a segment written for line L must
    // not move the cursor, or the next mapping is appended to the same group.
    uint32_t current_line = 0;
    bool group_has_segment = false;
    int32_t prev_file = 0;
    int32_t prev_line = 0;
    int32_t prev_col = 0;
    for (const JsMapping& m : maps) {
        const uint32_t line = m.line + line_offset;
        if (line < current_line) continue; // defensive: keep the stream sorted
        while (current_line < line) {
            mappings += ';';
            ++current_line;
            group_has_segment = false;
        }
        if (group_has_segment) mappings += ',';
        vlq_encode(mappings, 0); // generated column
        vlq_encode(mappings, static_cast<int32_t>(m.file) - prev_file);
        vlq_encode(mappings, static_cast<int32_t>(m.src_line) - prev_line);
        vlq_encode(mappings, static_cast<int32_t>(m.src_col) - prev_col);
        prev_file = static_cast<int32_t>(m.file);
        prev_line = static_cast<int32_t>(m.src_line);
        prev_col = static_cast<int32_t>(m.src_col);
        group_has_segment = true;
    }

    std::string out;
    out += "{\"version\":3,\"file\":\"";
    json_escape(out, file_name);
    out += "\",\"sources\":[";
    for (size_t i = 0; i < module.source_files.size(); ++i) {
        if (i) out += ',';
        out += '"';
        json_escape(out, module.source_files[i]);
        out += '"';
    }
    out += "],\"names\":[],\"mappings\":\"";
    out += mappings;
    out += "\"}\n";
    return out;
}

// ── the emitter ─────────────────────────────────────────────────────────────

class ModuleEmitter {
public:
    ModuleEmitter(const MIRModule& module, const std::vector<const MIRFunction*>& functions,
                  std::string& error, const MIREmitJsOptions& options)
        : module_(module), functions_(functions), error_(error), options_(options) {
        body_.reserve(1 << 16);
        ptr_size_ = module_.layout.is64Bit ? 8u : 4u;
    }

    /**
     * Split emission: this emitter prints one Chemical module of the program.
     * `symbol_stems` maps every emitted symbol to the file stem that owns it, so a
     * reference to another module's function or global becomes an `import` here
     * instead of a "not emitted" diagnostic. `imports` collects what was imported,
     * per stem, for the preamble.
     */
    void set_split(const std::string& own_stem,
                   const std::unordered_map<SymbolId, std::string>* symbol_stems,
                   std::map<std::string, std::vector<std::string>>* imports) {
        own_stem_ = own_stem;
        symbol_stems_ = symbol_stems;
        imports_ = imports;
    }

    bool run(std::string& out, std::string* source_map);

private:
    // ── structure ───────────────────────────────────────────────────────────
    void collect();
    bool emit_all_functions();
    void emit_preamble(std::string& out);
    /**
     * Emit one table per interface vtable definition, as a block of function
     * addresses in the heap, and bind its MIR global symbol (`$g<sym>`) to that
     * block — `dyn I` stores the address in the fat pointer's `second` field.
     */
    bool emit_vtables();
    /** The symbol interned under `name` (mangled), or MIR_INVALID_ID. */
    SymbolId symbol_for_name(std::string_view name);
    /** Raw text of a string constant operand. */
    bool constant_text_of(ConstantId id, std::string& out) const;
    /** The slot index of `method` in interface `vt_name`'s vtable type. */
    bool vtable_slot_of(const std::string& vt_name, const std::string& method,
                        uint32_t& slot) const;

    // ── per-function emission ───────────────────────────────────────────────
    bool emit_function(const MIRFunction& fn, std::string& out);

    // ── types ───────────────────────────────────────────────────────────────
    JsType type_of(TypeId id) const;
    TypeId operand_type(const MIROperand& o) const;
    TypeId value_type(ValueId id) const;
    TypeId place_type(PlaceId id) const;
    TypeId pointee_of(TypeId id) const;
    TypeId element_of(TypeId id) const;

    // ── operand spelling ────────────────────────────────────────────────────
    bool operand_expr(const MIROperand& o, std::string& out);
    bool base_addr_expr(const MIROperand& o, std::string& out);
    /**
     * The storage of a place (or the address a pointer value holds) without
     * dereferencing a pointer-typed place: this is what `store`/`memcpy`
     * destinations mean. `base_addr_expr` is the projection form, where a
     * pointer-typed place must be loaded first.
     */
    bool storage_addr_expr(const MIROperand& o, std::string& out);
    bool scalar_read(const MIROperand& o, const JsType& t, std::string& out);
    bool constant_expr(ConstantId id, std::string& out) const;
    bool literal_of(const MIRConstant& c, std::string& out) const;

    bool resolve_field(TypeId agg, std::string_view dotted, TypeId& field_type,
                       uint32_t& offset) const;

    // ── symbols ─────────────────────────────────────────────────────────────
    bool callee_expr(SymbolId sym, std::string& out);
    bool extern_expr(std::string_view name, std::string& out);
    /** The stem of another module that owns `sym` (false when this module owns it). */
    bool foreign_stem(SymbolId sym, std::string& stem) const {
        if (symbol_stems_ == nullptr || own_stem_.empty()) return false;
        auto it = symbol_stems_->find(sym);
        if (it == symbol_stems_->end() || it->second == own_stem_) return false;
        stem = it->second;
        return true;
    }
    /** Record `js_name` as imported from `stem` (deduplicated, ordered). */
    void note_import(const std::string& stem, const std::string& js_name);
    /** Whether this emitter defines `sym` (always true outside split mode). */
    bool owns_symbol(SymbolId sym) const {
        if (symbol_stems_ == nullptr || own_stem_.empty()) return true;
        auto it = symbol_stems_->find(sym);
        return it == symbol_stems_->end() || it->second == own_stem_;
    }
    bool function_address_expr(SymbolId sym, std::string& out);

    bool emit_instruction(const MIRInstruction& ins, std::string& out);
    bool emit_terminator(const MIRInstruction& ins, std::string& out);
    bool emit_const_into(ConstExprId id, uint32_t base_offset, const std::string& dest,
                         std::string& out);

    bool fail(const std::string& message) {
        if (errors_.size() < kMaxErrors) errors_.push_back(message);
        error_ = errors_.front();
        return false;
    }
    /** Fail with the current function and the current instruction's position. */
    bool fail_at(const std::string& message) {
        std::string where = "js backend: " + message;
        if (fn_ != nullptr) {
            where += " (in '";
            where += fn_name_;
            where += "'";
            if (!current_loc_.empty()) {
                where += " at ";
                where += current_loc_;
            }
            where += ")";
        }
        return fail(where);
    }
    /** The instruction's source position, as indices (for the source map). */
    bool source_ref_of(const MIRFunction& fn, const MIRInstruction& ins, uint32_t& file,
                       uint32_t& line, uint32_t& col) const {
        if (ins.source_index == MIR_NO_SOURCE || ins.source_index >= fn.sources.size()) return false;
        const MIRSourceLoc& loc = fn.sources[ins.source_index];
        if (loc.file >= module_.source_files.size()) return false;
        file = loc.file;
        line = loc.line;
        col = loc.column;
        return true;
    }

    /** The instruction's `file:line:column`, or empty when it has no source. */
    std::string location_of(const MIRFunction& fn, const MIRInstruction& ins) const {
        if (ins.source_index == MIR_NO_SOURCE || ins.source_index >= fn.sources.size()) return {};
        const MIRSourceLoc& loc = fn.sources[ins.source_index];
        std::string text;
        // copied under the intern lock: parallel lowering may still add a path
        if (module_.source_file_at(loc.file, text)) text += ':';
        // the lowerer records 0-based positions; a diagnostic is 1-based
        text += std::to_string(loc.line + 1);
        text += ':';
        text += std::to_string(loc.column + 1);
        return text;
    }

    static void line(std::string& out, const std::string& text) {
        out += text;
        out += '\n';
    }
    /**
     * `a`, `b`, … `Z`, `aa`, … — letters only, so a local can never collide with a
     * parameter (`a0`, `a1`) or with anything the module owns (`$f…`, `$g…`, `$k…`,
     * `$pc`, `$m`, `$p`).
     */
    static std::string short_name(uint32_t index) {
        std::string out;
        uint32_t n = index + 1;
        while (n > 0) {
            const uint32_t rem = (n - 1) % 52;
            out += rem < 26 ? static_cast<char>('a' + rem) : static_cast<char>('A' + (rem - 26));
            n = (n - 1) / 52;
        }
        return out;
    }
    /**
     * A local's emitted name. In a release build MIR ids are noise the program never
     * reads (`v1234`), so values and places are renamed per function by first use.
     * Exported names are *not* mangled: another module imports `$f…` by that name,
     * and `$exports` is the bundle's symbol table.
     */
    /** A generated name this short can spell a JS keyword (`if`, `do`, `in`). */
    static bool js_reserved(const std::string& name) {
        static const char* kReserved[] = {
            "do", "if", "in", "for", "new", "var", "let", "try", "int", "byte", "case",
            "else", "enum", "goto", "long", "null", "this", "true", "void", "with", "char",
            "class", "const", "false", "final", "float", "short", "super", "throw", "while",
            "break", "catch", "yield", "await", "async", "debugger", "default", "delete",
            "double", "export", "extends", "import", "native", "public", "return", "static",
            "switch", "typeof", "boolean", "package", "private", "continue", "function",
            "abstract", "arguments", "interface", "protected", "implements", "instanceof",
        };
        for (const char* word : kReserved) {
            if (name == word) return true;
        }
        return false;
    }
    std::string next_name() {
        while (js_reserved(short_name(next_short_))) ++next_short_;
        return short_name(next_short_++);
    }
    std::string vname(ValueId id) {
        if (!options_.mangle_names) return "v" + std::to_string(id);
        if (id >= short_values_.size()) short_values_.resize(id + 1);
        std::string& slot = short_values_[id];
        if (slot.empty()) slot = next_name();
        return slot;
    }
    std::string pname(PlaceId id) {
        if (!options_.mangle_names) return "p" + std::to_string(id);
        if (id >= short_places_.size()) short_places_.resize(id + 1);
        std::string& slot = short_places_[id];
        if (slot.empty()) slot = next_name();
        return slot;
    }
    /** Per-function short names (`mangle_names` only). */
    std::vector<std::string> short_values_;
    std::vector<std::string> short_places_;
    uint32_t next_short_ = 0;

    const MIRModule& module_;
    const std::vector<const MIRFunction*>& functions_;
    std::string& error_;
    /**
     * Every diagnostic collected during the run. Emission keeps going after a
     * function fails so one build reports every unsupported construct, not the
     * first one — an agent fixing this needs the whole list.
     */
    std::vector<std::string> errors_;
    static constexpr size_t kMaxErrors = 24;
    const MIREmitJsOptions& options_;

    std::string body_;    // emitted functions
    std::string globals_; // module-load global storage + initializers
    std::string constants_;
    /** Lines already written into `body_`, so a mapping can be made absolute. */
    uint32_t body_lines_ = 0;
    std::vector<JsMapping> mappings_;

    uint32_t ptr_size_ = 8;

    /** Split mode: this file's stem, and who owns each symbol (empty = whole program). */
    std::string own_stem_;
    const std::unordered_map<SymbolId, std::string>* symbol_stems_ = nullptr;
    std::map<std::string, std::vector<std::string>>* imports_ = nullptr;

    /** Global symbols whose address the emitted code takes (`dyn I` takes a vtable's). */
    std::unordered_set<SymbolId> addressed_symbols_;

    /**
     * The function being emitted runs its body in an `async` arrow, so its
     * storage must survive suspension. `rt.frame_alloc` hands out memory from the
     * calling frame, which the (synchronous) caller releases on return — long
     * before a suspended arrow resumes — so an async body allocates on the heap.
     */
    bool async_body_ = false;
    /** This function's body is emitted inside an `async` arrow (it awaits). */
    bool async_arrow_ = false;
    /** This is a ramp whose body lives in the poll, so it hands out that promise. */
    bool async_defers_ = false;
    std::string alloc_call(uint32_t size, uint32_t align) const {
        if (async_body_) return "rt.malloc(" + std::to_string(size ? size : 1) + ")";
        return "rt.frame_alloc(" + std::to_string(size ? size : 1) + ", " +
               std::to_string(align ? align : 1) + ")";
    }

    /**
     * Coroutine-frame slots. In MIR a `FrameField` place *is* a field of the
     * frame: the lowering writes it through an explicit `frame + offset` field
     * store and reads it back as a place. A pointer-based backend aliases the two;
     * JS cannot, so each such place is declared without storage and bound to the
     * address of the field store/load that defines it. There is one frame per
     * async function, so the field name identifies the slot uniquely.
     */
    std::vector<std::pair<PlaceId, std::string>> frame_slots_;
    /** Append `pN = addr` for every frame slot named `designator`. */
    std::string bind_frame_slots(std::string_view designator, const std::string& addr) {
        std::string out;
        for (const auto& slot : frame_slots_) {
            if (slot.second.size() != designator.size()) continue;
            if (slot.second.compare(0, slot.second.size(), designator) != 0) continue;
            out += pname(slot.first) + " = " + addr + "; ";
        }
        return out;
    }
    /**
     * Bind every frame slot to `frame + offset`. Used where the frame becomes
     * known: the ramp's allocation, or the poll's frame parameter landing in its
     * place. Slots the function never touches are bound harmlessly.
     */
    std::string bind_all_frame_slots(const std::string& frame) {
        std::string out;
        for (const auto& slot : frame_slots_) {
            TypeId ft = MIR_INVALID_ID;
            uint32_t off = 0;
            if (!resolve_field(frame_type_, slot.second, ft, off)) continue;
            out += pname(slot.first) + " = " + frame + " + " + std::to_string(off) + "; ";
        }
        return out;
    }
    /** The place holding the coroutine frame pointer, and the frame's type. */
    PlaceId frame_place_ = MIR_INVALID_ID;
    TypeId frame_type_ = MIR_INVALID_ID;

    /** Mangled name -> symbol id, built once (the symbol table is append-only). */
    std::unordered_map<std::string, SymbolId> symbol_by_name_;
    bool symbol_index_built_ = false;

    std::unordered_map<SymbolId, std::string> defined_js_name_;
    std::unordered_map<std::string, SymbolId> defined_by_name_;
    /** Every lowered function in the program, by mangled name (not just the
     *  reachable set) — used only to keep diagnostics truthful. */
    std::unordered_map<std::string, const MIRFunction*> all_by_name_;
    std::unordered_set<SymbolId> emitted_;
    std::vector<SymbolId> fa_order_;
    std::unordered_map<SymbolId, std::string> fa_expr_;

    const MIRFunction* fn_ = nullptr;
    std::string fn_name_;
    /** `file:line:column` of the instruction being emitted, for diagnostics. */
    std::string current_loc_;
    /** Set by `callee_expr` when the callee is a `__chc_*` trap helper. */
    std::string trap_name_;
};

// ── module assembly ─────────────────────────────────────────────────────────

void ModuleEmitter::collect() {
    for (const MIRFunction& fn : module_.functions) {
        if (fn.symbol == MIR_INVALID_ID || fn.symbol >= module_.symbols.size()) continue;
        if (fn.instructions.size() == 0) continue;
        all_by_name_.emplace(module_.symbols.mangled_name(module_.symbols.get(fn.symbol)), &fn);
    }
    for (const MIRFunction* fnp : functions_) {
        if (fnp == nullptr) continue;
        const MIRFunction& fn = *fnp;
        if (fn.symbol == MIR_INVALID_ID || fn.symbol >= module_.symbols.size()) continue;
        const MIRSymbolRecord& rec = module_.symbols.get(fn.symbol);
        defined_js_name_[fn.symbol] = js_function_name(fn.symbol);
        const std::string name = module_.symbols.mangled_name(rec);
        if (!name.empty()) defined_by_name_.emplace(name, fn.symbol);
    }
}

bool ModuleEmitter::emit_all_functions() {
    for (const MIRFunction* fnp : functions_) {
        if (fnp == nullptr) continue;
        const MIRFunction& fn = *fnp;
        if (fn.symbol == MIR_INVALID_ID) continue;
        if (!emitted_.insert(fn.symbol).second) continue; // duplicate symbol id
        if (fn.instructions.size() == 0) continue;
        // A failed function contributes no body, but the rest still emit so one
        // build reports every unsupported construct.
        emit_function(fn, body_);
    }
    return true;
}

SymbolId ModuleEmitter::symbol_for_name(std::string_view name) {
    if (!symbol_index_built_) {
        symbol_index_built_ = true;
        for (SymbolId s = 0; s < module_.symbols.size(); ++s) {
            symbol_by_name_.emplace(module_.symbols.mangled_name(module_.symbols.get(s)), s);
        }
    }
    auto it = symbol_by_name_.find(std::string(name));
    return it == symbol_by_name_.end() ? static_cast<SymbolId>(MIR_INVALID_ID) : it->second;
}

bool ModuleEmitter::constant_text_of(ConstantId id, std::string& out) const {
    if (id >= module_.constants.size()) return false;
    const MIRConstant& c = module_.constants.get(id);
    if (c.kind != MIRConstantKind::String) return false;
    out.assign(module_.constants.data.data_at(c.data_offset), c.data_count);
    return true;
}

bool ModuleEmitter::vtable_slot_of(const std::string& vt_name, const std::string& method,
                                   uint32_t& slot) const {
    for (const MIRVtableRecord& t : module_.vtables.types) {
        if (t.name != vt_name && t.type_name != vt_name) continue;
        for (uint32_t k = 0; k < t.slots.size(); ++k) {
            if (t.slots[k].name == method) {
                slot = k;
                return true;
            }
        }
    }
    return false;
}

bool ModuleEmitter::emit_vtables() {
    if (module_.vtables.defs.empty()) return true;
    const std::string ptr = std::to_string(ptr_size_);
    for (const MIRVtableRecord& def : module_.vtables.defs) {
        const SymbolId sym = symbol_for_name(def.name);
        if (sym == MIR_INVALID_ID) continue;
        // Only a table whose address the program actually takes exists in the
        // bundle: an impl that is never used through `dyn` would otherwise pull
        // its methods in (and, for a library, its whole hierarchy).
        if (addressed_symbols_.count(sym) == 0) continue;
        // Split mode: exactly one module allocates a table's storage (the module
        // that owns the implementation); the rest import `$g<sym>`.
        if (!owns_symbol(sym)) continue;
        const uint32_t slot_count = def.slots.empty() ? 1u : static_cast<uint32_t>(def.slots.size());
        const std::string decl = "$g" + std::to_string(sym);
        globals_ += "// vtable " + def.name + "\n";
        globals_ += "export const " + decl + " = rt.alloc(" +
                    std::to_string(ptr_size_ * slot_count) + ", " + ptr + ");\n";
        for (uint32_t k = 0; k < slot_count; ++k) {
            // A slot with no target stays a null function address: the dispatcher
            // reports it by name instead of calling whatever is at address 0.
            std::string addr = "0";
            if (k < def.slots.size() && !def.slots[k].target.empty()) {
                const SymbolId callee = symbol_for_name(def.slots[k].target);
                if (callee == MIR_INVALID_ID) {
                    return fail_at("vtable '" + def.name + "' slot '" + def.slots[k].name +
                                   "' names a callee ('" + def.slots[k].target +
                                   "') that has no symbol in this program");
                }
                // A function address is the symbol id (see `function_address_expr`).
                std::string callee_js;
                if (!function_address_expr(callee, callee_js)) return false;
                addr = std::to_string(static_cast<unsigned long long>(callee));
            }
            globals_ += "rt.st(" + decl + " + " + std::to_string(k * ptr_size_) + ", " + ptr +
                        ", 0, " + addr + ");\n";
        }
    }
    return true;
}

void ModuleEmitter::note_import(const std::string& stem, const std::string& js_name) {
    if (imports_ == nullptr) return;
    auto& names = (*imports_)[stem];
    for (const std::string& existing : names) {
        if (existing == js_name) return;
    }
    names.push_back(js_name);
}

void ModuleEmitter::emit_preamble(std::string& out) {
    out += "// Generated by the Chemical compiler (JS backend). Do not edit.\n";
    if (!options_.module_name.empty()) {
        out += "// chemical module: ";
        out += options_.module_name;
        out += "\n";
    }
    out += "import { rt } from \"";
    out += options_.runtime_specifier;
    out += "\";\n";
    // Imports from the other modules of this bundle. `run()` emits the bodies
    // first, so by the time the preamble is written every cross-module reference
    // is known; ES module bindings are hoisted, so order does not matter anyway.
    if (imports_ != nullptr) {
        for (const auto& entry : *imports_) {
            out += "import { ";
            for (size_t i = 0; i < entry.second.size(); ++i) {
                if (i) out += ", ";
                out += entry.second[i];
            }
            out += " } from \"./";
            out += entry.first;
            out += ".mjs\";\n";
        }
    }
    out += "\n";

    if (!constants_.empty()) {
        out += "// --- string constants ---───────────────────────────────────────────────\n";
        out += constants_;
        out += "\n";
    }
    if (!globals_.empty()) {
        out += "// --- globals ---────────────────────────────────────────────────────────\n";
        out += globals_;
        out += "\n";
    }
    out += "// --- function addresses (function_addr / indirect calls) ---\n";
    out += "// A function address is a MIR symbol id; this table resolves it.\n";
    out += "const $fns = [];\n";
    for (SymbolId sym : fa_order_) {
        out += "$fns[";
        append_uint(out, sym);
        out += "] = ";
        out += fa_expr_[sym];
        out += ";\n";
    }
    // Host callbacks (`dom` event listeners, `fetch` completion) call back into
    // Chemical code by function address, so the runtime needs this table.
    out += "rt.register_fns($fns);\n";
    out += "\n";
}

bool ModuleEmitter::run(std::string& out, std::string* source_map) {
    collect();

    // String constants: a `const.string` operates on bytes in the heap, so every
    // interned string constant gets a module-level address.
    for (ConstantId id = 0; id < module_.constants.size(); ++id) {
        const MIRConstant& c = module_.constants.get(id);
        if (c.kind != MIRConstantKind::String) continue;
        std::string bytes = "rt.cstr(new Uint8Array([";
        for (uint32_t i = 0; i < c.data_count; ++i) {
            if (i) bytes += ",";
            bytes += std::to_string(static_cast<unsigned>(
                static_cast<unsigned char>(module_.constants.data.data_at(c.data_offset)[i])));
        }
        bytes += "]));\n";
        constants_ += "const $k" + std::to_string(id) + " = " + bytes;
    }

    // Globals: storage plus the MIR initializer tree. In split mode only the
    // owning module allocates the storage (a second `$g` in another file would be
    // a *different* global with zeroed contents); the others import it.
    for (const MIRGlobal& global : module_.globals) {
        if (global.symbol == MIR_INVALID_ID || global.symbol >= module_.symbols.size()) continue;
        if (!owns_symbol(global.symbol)) continue;
        const MIRSymbolRecord& rec = module_.symbols.get(global.symbol);
        if (rec.type == MIR_INVALID_ID || rec.type >= module_.types.size()) continue;
        const JsType t = type_of(rec.type);
        const uint32_t size = t.size ? t.size : 1;
        const uint32_t align = t.align ? t.align : 1;
        const std::string decl = "$g" + std::to_string(global.symbol);
        // Exported so another module of the bundle can address it.
        globals_ += "export const " + decl + " = rt.alloc(" + std::to_string(size) + ", " +
                    std::to_string(align) + ");\n";
        globals_ += "rt.fill(" + decl + ", 0, " + std::to_string(size) + ");\n";
        if (global.init != MIR_INVALID_ID) {
            if (!emit_const_into(global.init, 0, decl, globals_)) return false;
        }
    }

    // Bodies first: a vtable is only emitted when the emitted code takes its
    // address (`dyn I`), and only then does the emitter know which slots are
    // needed. The preamble (which carries `globals_`) is assembled afterwards.
    if (!emit_all_functions()) return false;

    if (!emit_vtables()) return false;

    if (!errors_.empty()) {
        std::string all;
        for (size_t i = 0; i < errors_.size(); ++i) {
            if (i) all += '\n';
            all += errors_[i];
        }
        if (errors_.size() >= kMaxErrors) {
            all += "\n... (further diagnostics suppressed)";
        }
        error_ = std::move(all);
        return false;
    }

    // The preamble precedes the function bodies, so a mapping recorded relative
    // to `body_` needs its line count added.
    std::string preamble;
    emit_preamble(preamble);
    uint32_t preamble_lines = 0;
    for (char c : preamble) {
        if (c == '\n') ++preamble_lines;
    }
    out += preamble;
    out += body_;

    // The exported symbol table: mangled name -> function. This is how a driver
    // finds `main` without knowing the generated JS identifiers.
    out += "\n// --- symbol table ---─────────────────────────────────────────────────────\n";
    out += "export const $exports = {\n";
    std::unordered_set<SymbolId> listed;
    for (const MIRFunction* fnp : functions_) {
        if (fnp == nullptr) continue;
        const MIRFunction& fn = *fnp;
        if (fn.symbol == MIR_INVALID_ID || fn.symbol >= module_.symbols.size()) continue;
        if (!listed.insert(fn.symbol).second) continue;
        if (emitted_.count(fn.symbol) == 0) continue; // no body
        const std::string name = module_.symbols.mangled_name(module_.symbols.get(fn.symbol));
        if (name.empty()) continue;
        out += "  \"";
        out += name;
        out += "\": ";
        out += js_function_name(fn.symbol);
        out += ",\n";
    }
    out += "};\n";

    if (source_map != nullptr && !options_.minify) {
        const std::string file_name = js_module_file_stem(options_.module_name) + ".mjs";
        *source_map = build_source_map(module_, mappings_, preamble_lines, file_name);
        out += "//# sourceMappingURL=";
        out += file_name;
        out += ".map\n";
    }
    // Formatting last, so every section above is minified by the same rules.
    if (options_.minify) out = minify_js_source(out);
    return true;
}

// ── types ───────────────────────────────────────────────────────────────────

JsType ModuleEmitter::type_of(TypeId id) const {
    JsType t;
    if (id == MIR_INVALID_ID || id >= module_.types.size()) return t;
    const MIRTypeRecord& r = module_.types.get(id);
    t.valid = true;
    t.size = r.size;
    t.align = r.alignment ? r.alignment : 1;
    switch (r.kind) {
        case MIRTypeKind::Void:
            t.size = 0;
            break;
        case MIRTypeKind::Bool:
            t.size = r.size ? r.size : 1;
            break;
        case MIRTypeKind::Int:
            t.is_signed = (r.flags & TF_SIGNED) != 0;
            break;
        case MIRTypeKind::Float:
            if (r.size == 4) t.is_float = true;
            else t.is_double = true;
            break;
        case MIRTypeKind::Pointer:
        case MIRTypeKind::Reference:
        case MIRTypeKind::Function:
        case MIRTypeKind::Opaque:
            t.is_ptr = true;
            t.size = r.size ? r.size : ptr_size_;
            t.align = r.alignment ? r.alignment : ptr_size_;
            break;
        case MIRTypeKind::Array:
        case MIRTypeKind::Struct:
        case MIRTypeKind::Union:
        case MIRTypeKind::Variant:
            t.is_agg = true;
            break;
    }
    if (!t.is_agg && t.size == 0 && r.kind != MIRTypeKind::Void) t.size = 1;
    return t;
}

TypeId ModuleEmitter::value_type(ValueId id) const {
    if (fn_ == nullptr || id >= fn_->values.size()) return MIR_INVALID_ID;
    return fn_->values[id].type;
}

TypeId ModuleEmitter::place_type(PlaceId id) const {
    if (fn_ == nullptr || id >= fn_->places.size()) return MIR_INVALID_ID;
    return fn_->places[id].type;
}

TypeId ModuleEmitter::operand_type(const MIROperand& o) const {
    switch (o.kind()) {
        case MIROperandKind::Value:
            return value_type(o.id);
        case MIROperandKind::Place:
            return place_type(o.id);
        case MIROperandKind::Constant:
            if (o.id < module_.constants.size()) return module_.constants.get(o.id).type;
            return MIR_INVALID_ID;
        case MIROperandKind::Symbol:
            if (o.id < module_.symbols.size()) return module_.symbols.get(o.id).type;
            return MIR_INVALID_ID;
        case MIROperandKind::Type:
            return o.type();
        default:
            return MIR_INVALID_ID;
    }
}

TypeId ModuleEmitter::pointee_of(TypeId id) const {
    if (id == MIR_INVALID_ID || id >= module_.types.size()) return MIR_INVALID_ID;
    const MIRTypeRecord& r = module_.types.get(id);
    if (r.kind == MIRTypeKind::Pointer || r.kind == MIRTypeKind::Reference) return r.element;
    return MIR_INVALID_ID;
}

TypeId ModuleEmitter::element_of(TypeId id) const {
    if (id == MIR_INVALID_ID || id >= module_.types.size()) return MIR_INVALID_ID;
    const MIRTypeRecord& r = module_.types.get(id);
    if (r.kind == MIRTypeKind::Array) return r.element;
    const TypeId p = pointee_of(id);
    if (p != MIR_INVALID_ID && p < module_.types.size() &&
        module_.types.get(p).kind == MIRTypeKind::Array) {
        return module_.types.get(p).element;
    }
    return p;
}

// ── constants ───────────────────────────────────────────────────────────────

bool ModuleEmitter::constant_expr(ConstantId id, std::string& out) const {
    if (id >= module_.constants.size()) return false;
    const MIRConstant& c = module_.constants.get(id);
    if (c.kind == MIRConstantKind::String) {
        out += "$k" + std::to_string(id);
        return true;
    }
    return literal_of(c, out);
}

bool ModuleEmitter::literal_of(const MIRConstant& c, std::string& out) const {
    switch (c.kind) {
        case MIRConstantKind::Int: {
            const JsType t = type_of(c.type);
            const uint64_t bits = c.bits;
            if (t.size == 1) {
                out += std::to_string(t.is_signed ? static_cast<int64_t>(static_cast<int8_t>(bits))
                                                  : static_cast<int64_t>(static_cast<uint8_t>(bits)));
            } else if (t.size == 2) {
                out += std::to_string(t.is_signed ? static_cast<int64_t>(static_cast<int16_t>(bits))
                                                  : static_cast<int64_t>(static_cast<uint16_t>(bits)));
            } else if (t.size == 4) {
                out += std::to_string(t.is_signed ? static_cast<int64_t>(static_cast<int32_t>(bits))
                                                  : static_cast<int64_t>(static_cast<uint32_t>(bits)));
            } else if (t.is_signed) {
                out += std::to_string(static_cast<int64_t>(bits));
            } else {
                out += std::to_string(bits);
            }
            return true;
        }
        case MIRConstantKind::Float: {
            float f = 0;
            const uint32_t b = static_cast<uint32_t>(c.bits);
            std::memcpy(&f, &b, sizeof(f));
            out += float_literal(static_cast<double>(f), true);
            return true;
        }
        case MIRConstantKind::Double: {
            double d = 0;
            std::memcpy(&d, &c.bits, sizeof(d));
            out += float_literal(d, false);
            return true;
        }
        case MIRConstantKind::Bool:
            out += c.bits ? "1" : "0";
            return true;
        case MIRConstantKind::Null:
            out += "0";
            return true;
        default:
            return false;
    }
}

// ── operand spelling ────────────────────────────────────────────────────────

bool ModuleEmitter::scalar_read(const MIROperand& o, const JsType& t, std::string& out) {
    if (!t.valid || t.size == 0) {
        out += "0";
        return true;
    }
    const std::string size = std::to_string(t.size);
    switch (o.kind()) {
        case MIROperandKind::Place: {
            if (t.is_float || t.is_double) {
                out += "rt.ldF(" + pname(o.id) + ", " + size + ")";
            } else {
                out += "rt.ld(" + pname(o.id) + ", " + size + ", ";
                out += t.is_signed ? "1)" : "0)";
            }
            return true;
        }
        case MIROperandKind::Value:
            out += vname(o.id);
            return true;
        case MIROperandKind::Constant:
            return constant_expr(o.id, out);
        case MIROperandKind::Symbol:
            return function_address_expr(o.id, out);
        default:
            return false;
    }
}

bool ModuleEmitter::operand_expr(const MIROperand& o, std::string& out) {
    if (o.kind() == MIROperandKind::Place) {
        const JsType t = type_of(place_type(o.id));
        if (t.is_agg) {
            // aggregate values are addresses in this ABI
            out += pname(o.id);
            return true;
        }
        return scalar_read(o, t, out);
    }
    return scalar_read(o, type_of(operand_type(o)), out);
}

bool ModuleEmitter::base_addr_expr(const MIROperand& o, std::string& out) {
    switch (o.kind()) {
        case MIROperandKind::Place: {
            const JsType t = type_of(place_type(o.id));
            if (t.is_ptr) {
                // a pointer variable: the base is the pointer it holds
                out += "rt.ld(" + pname(o.id) + ", " + std::to_string(t.size) + ", 0)";
            } else {
                out += pname(o.id);
            }
            return true;
        }
        case MIROperandKind::Value:
            out += vname(o.id);
            return true;
        case MIROperandKind::Constant:
            return constant_expr(o.id, out);
        default:
            return false;
    }
}

bool ModuleEmitter::storage_addr_expr(const MIROperand& o, std::string& out) {
    switch (o.kind()) {
        case MIROperandKind::Place:
            out += pname(o.id);
            return true;
        case MIROperandKind::Value:
            out += vname(o.id);
            return true;
        case MIROperandKind::Constant:
            return constant_expr(o.id, out);
        default:
            return false;
    }
}

// ── field designators ───────────────────────────────────────────────────────
bool ModuleEmitter::resolve_field(TypeId agg, std::string_view dotted, TypeId& field_type,
                                  uint32_t& offset) const {
    TypeId cur = agg;
    uint32_t off = 0;
    size_t start = 0;
    for (;;) {
        const size_t dot = dotted.find('.', start);
        const std::string_view seg =
            dot == std::string_view::npos ? dotted.substr(start) : dotted.substr(start, dot - start);
        if (cur == MIR_INVALID_ID || cur >= module_.types.size()) return false;
        const MIRTypeRecord& rec = module_.types.get(cur);
        bool found = false;
        for (uint32_t i = 0; i < rec.data_count; ++i) {
            const MIRFieldInfo* f = module_.types.field_of(cur, i);
            if (f == nullptr) break;
            if (module_.types.field_name_view(*f) == seg) {
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
    field_type = cur;
    offset = off;
    return true;
}

// ── symbol resolution ───────────────────────────────────────────────────────

bool ModuleEmitter::extern_expr(std::string_view name, std::string& out) {
    struct Entry {
        const char* name;
        const char* expr;
    };
    static const Entry kExterns[] = {
        {"printf", "rt.printf"},    {"malloc", "rt.malloc"},   {"free", "rt.free_"},
        {"realloc", "rt.realloc_"}, {"memcpy", "rt.memcpy"},   {"memmove", "rt.memmove"},
        {"memset", "rt.memset"},    {"strlen", "rt.strlen"},   {"strcmp", "rt.strcmp"},
        {"strncmp", "rt.strncmp"},  {"strcpy", "rt.strcpy"},   {"strncpy", "rt.strncpy"},
        {"strcat", "rt.strcat"},    {"strncat", "rt.strncat"}, {"strchr", "rt.strchr"},
        {"strrchr", "rt.strrchr"},  {"strstr", "rt.strstr"},   {"strdup", "rt.strdup"},
        {"memcmp", "rt.memcmp"},    {"memchr", "rt.memchr"},
        {"isspace", "rt.isspace"},  {"isdigit", "rt.isdigit"}, {"isalpha", "rt.isalpha"},
        {"isalnum", "rt.isalnum"},  {"isxdigit", "rt.isxdigit"},
        {"isupper", "rt.isupper"},  {"islower", "rt.islower"},
        {"isprint", "rt.isprint"},  {"isgraph", "rt.isgraph"},
        {"iscntrl", "rt.iscntrl"},  {"ispunct", "rt.ispunct"},
        {"toupper", "rt.toupper"},  {"tolower", "rt.tolower"},
        {"abs", "rt.abs"},          {"labs", "rt.labs"},       {"llabs", "rt.llabs"},
        {"strtol", "rt.strtol"},    {"strtoll", "rt.strtoll"},
        {"strtoul", "rt.strtoul"},  {"strtoull", "rt.strtoull"},
        {"atoi", "rt.atoi"},        {"strtod", "rt.strtod"},   {"atof", "rt.atof"},
        {"abort", "rt.abort"},      {"exit", "rt.exit"},       {"putchar", "rt.putchar"},
        // The `dom` host (proposal 0008, `lang/libs/dom`). Each entry is one
        // `@extern @no_mangle` declaration in the `dom` module's `raw` namespace;
        // the JS-side implementation lives in `scripts/js_backend/runtime.mjs`.
        {"chx_dom_document", "rt.dom.document"},
        {"chx_dom_body", "rt.dom.body"},
        {"chx_dom_get_element_by_id", "rt.dom.get_element_by_id"},
        {"chx_dom_query_selector", "rt.dom.query_selector"},
        {"chx_dom_create_element", "rt.dom.create_element"},
        {"chx_dom_set_text", "rt.dom.set_text"},
        {"chx_dom_text", "rt.dom.text"},
        {"chx_dom_set_html", "rt.dom.set_html"},
        {"chx_dom_set_attribute", "rt.dom.set_attribute"},
        {"chx_dom_get_attribute", "rt.dom.get_attribute"},
        {"chx_dom_has_attribute", "rt.dom.has_attribute"},
        {"chx_dom_remove_attribute", "rt.dom.remove_attribute"},
        {"chx_dom_set_value", "rt.dom.set_value"},
        {"chx_dom_value", "rt.dom.value"},
        {"chx_dom_append_child", "rt.dom.append_child"},
        {"chx_dom_remove", "rt.dom.remove"},
        {"chx_dom_focus", "rt.dom.focus"},
        {"chx_dom_has_class", "rt.dom.has_class"},
        {"chx_dom_add_class", "rt.dom.add_class"},
        {"chx_dom_remove_class", "rt.dom.remove_class"},
        {"chx_dom_toggle_class", "rt.dom.toggle_class"},
        {"chx_dom_parent", "rt.dom.parent"},
        {"chx_dom_child_count", "rt.dom.child_count"},
        {"chx_dom_child_at", "rt.dom.child_at"},
        {"chx_dom_matches", "rt.dom.matches"},
        {"chx_dom_closest", "rt.dom.closest"},
        {"chx_dom_get_context_2d", "rt.dom.get_context_2d"},
        {"chx_dom_canvas_fill_rect", "rt.dom.canvas_fill_rect"},
        {"chx_dom_canvas_clear_rect", "rt.dom.canvas_clear_rect"},
        {"chx_dom_canvas_begin_path", "rt.dom.canvas_begin_path"},
        {"chx_dom_canvas_move_to", "rt.dom.canvas_move_to"},
        {"chx_dom_canvas_line_to", "rt.dom.canvas_line_to"},
        {"chx_dom_canvas_arc", "rt.dom.canvas_arc"},
        {"chx_dom_canvas_close_path", "rt.dom.canvas_close_path"},
        {"chx_dom_canvas_fill", "rt.dom.canvas_fill"},
        {"chx_dom_canvas_stroke", "rt.dom.canvas_stroke"},
        {"chx_dom_canvas_save", "rt.dom.canvas_save"},
        {"chx_dom_canvas_restore", "rt.dom.canvas_restore"},
        {"chx_dom_canvas_set_fill_style", "rt.dom.canvas_set_fill_style"},
        {"chx_dom_canvas_set_stroke_style", "rt.dom.canvas_set_stroke_style"},
        {"chx_dom_canvas_set_line_width", "rt.dom.canvas_set_line_width"},
        {"chx_dom_canvas_set_font", "rt.dom.canvas_set_font"},
        {"chx_dom_canvas_fill_text", "rt.dom.canvas_fill_text"},
        {"chx_dom_canvas_translate", "rt.dom.canvas_translate"},
        {"chx_dom_canvas_rotate", "rt.dom.canvas_rotate"},
        {"chx_dom_canvas_scale", "rt.dom.canvas_scale"},
        {"chx_dom_canvas_set_transform", "rt.dom.canvas_set_transform"},
        {"chx_dom_request_animation_frame", "rt.dom.request_animation_frame"},
        {"chx_dom_cancel_animation_frame", "rt.dom.cancel_animation_frame"},
        {"chx_dom_event_client_x", "rt.dom.event_client_x"},
        {"chx_dom_event_client_y", "rt.dom.event_client_y"},
        {"chx_dom_event_button", "rt.dom.event_button"},
        {"chx_dom_event_pointer_id", "rt.dom.event_pointer_id"},
        {"chx_dom_set_pointer_capture", "rt.dom.set_pointer_capture"},
        {"chx_dom_release_pointer_capture", "rt.dom.release_pointer_capture"},
        {"chx_dom_on", "rt.dom.on"},
        {"chx_dom_prevent_default", "rt.dom.prevent_default"},
        {"chx_dom_stop_propagation", "rt.dom.stop_propagation"},
        {"chx_dom_target", "rt.dom.target"},
        {"chx_dom_key", "rt.dom.key"},
        {"chx_dom_fetch_text", "rt.dom.fetch_text"},
        // Returns a `FutureHandle<int>` through the sret slot; the host writes a
        // promise token into it (see runtime.mjs `dom.fetch_status`).
        {"chx_dom_fetch_status", "rt.dom.fetch_status"},
        {"chx_dom_storage_get", "rt.dom.storage_get"},
        {"chx_dom_storage_set", "rt.dom.storage_set"},
        {"chx_dom_storage_remove", "rt.dom.storage_remove"},
        {"chx_dom_url_origin", "rt.dom.url_origin"},
        {"chx_dom_url_pathname", "rt.dom.url_pathname"},
        {"chx_dom_url_search_param", "rt.dom.url_search_param"},
    };
    for (const Entry& e : kExterns) {
        if (name == e.name) {
            out += e.expr;
            return true;
        }
    }
    // The C backend's checked-operation helpers (`__chc_div_zero_fail`, …) are
    // traps here: see `trap` in scripts/js_backend/runtime.mjs. The helper's name
    // travels with the call so a trap reports which check fired.
    if (name.size() >= 6 && name.compare(0, 6, "__chc_") == 0) {
        out += "rt.trap";
        trap_name_ = std::string(name);
        return true;
    }
    return false;
}

bool ModuleEmitter::callee_expr(SymbolId sym, std::string& out) {
    trap_name_.clear();
    if (sym == MIR_INVALID_ID || sym >= module_.symbols.size()) {
        return fail_at("call with an unresolved callee symbol");
    }
    auto it = defined_js_name_.find(sym);
    if (it != defined_js_name_.end()) {
        out += it->second;
        return true;
    }
    // A split bundle calls across module files: the function is exported by the
    // module that defines it, so the call is a binding imported here.
    std::string foreign;
    if (foreign_stem(sym, foreign)) {
        const std::string name = js_function_name(sym);
        note_import(foreign, name);
        out += name;
        return true;
    }
    const std::string name = module_.symbols.mangled_name(module_.symbols.get(sym));
    auto by_name = defined_by_name_.find(name);
    if (by_name != defined_by_name_.end()) {
        out += js_function_name(by_name->second);
        return true;
    }
    if (extern_expr(name, out)) return true;
    // Distinguish the three ways a callee can be unsatisfiable. The earlier
    // message ("defined in another Chemical module") was a placeholder for a
    // limitation the single-program bundle does not have, and it sent readers
    // looking in the wrong place.
    if (all_by_name_.find(name) != all_by_name_.end()) {
        return fail_at("'" + name + "' is lowered in this program but was not emitted "
                       "(internal: the reachable set missed it — please report)");
    }
    const MIRSymbolRecord& rec = module_.symbols.get(sym);
    if (rec.kind == MIRSymbolKind::External || !rec.has_body) {
        return fail_at("native symbol '" + name + "' has no JavaScript equivalent (declared " +
                       "@extern); a client or shared module must not depend on it");
    }
    return fail_at("'" + name + "' has no definition in this program (declared but never lowered)");
}

bool ModuleEmitter::function_address_expr(SymbolId sym, std::string& out) {
    // A function address is the MIR symbol id: a number that fits in a pointer
    // slot, so it survives being stored in a closure's function-pointer field and
    // read back. `$fns` maps it to the emitted function for indirect calls.
    if (fa_expr_.find(sym) == fa_expr_.end()) {
        std::string callee;
        if (!callee_expr(sym, callee)) return false;
        fa_expr_[sym] = callee;
        fa_order_.push_back(sym);
    }
    append_uint(out, sym);
    return true;
}

// ── constant initializers (globals) ─────────────────────────────────────────

bool ModuleEmitter::emit_const_into(ConstExprId id, uint32_t base_offset, const std::string& dest,
                                    std::string& out) {
    if (id == MIR_INVALID_ID || id >= module_.const_exprs.size()) return true;
    const MIRConstExpr& e = module_.const_exprs.get(id);
    const JsType t = type_of(e.type);
    const std::string addr = dest + " + " + std::to_string(base_offset);
    const std::string size = std::to_string(t.size ? t.size : 1);

    switch (e.kind) {
        case MIRConstKind::Zero:
            out += "rt.fill(" + addr + ", 0, " + size + ");\n";
            return true;
        case MIRConstKind::Int:
        case MIRConstKind::Bool:
        case MIRConstKind::Null: {
            if (e.kind == MIRConstKind::Null) {
                out += "rt.st(" + addr + ", " + std::to_string(t.size ? t.size : ptr_size_) +
                       ", 0, 0);\n";
                return true;
            }
            MIRConstant c;
            c.kind = MIRConstantKind::Int;
            c.type = e.type;
            c.bits = e.bits;
            std::string lit;
            if (!literal_of(c, lit)) return false;
            out += "rt.st(" + addr + ", " + size + ", " + (t.is_signed ? "1" : "0") + ", " + lit +
                   ");\n";
            return true;
        }
        case MIRConstKind::Float:
        case MIRConstKind::Double: {
            MIRConstant c;
            c.kind = e.kind == MIRConstKind::Float ? MIRConstantKind::Float : MIRConstantKind::Double;
            c.type = e.type;
            c.bits = e.bits;
            std::string lit;
            if (!literal_of(c, lit)) return false;
            out += "rt.stF(" + addr + ", " + size + ", " + lit + ");\n";
            return true;
        }
        case MIRConstKind::String: {
            const std::string_view bytes(module_.const_exprs.data.data_at(e.data_offset),
                                         e.data_count);
            std::string bytes_expr = "new Uint8Array([";
            for (uint32_t i = 0; i < e.data_count; ++i) {
                if (i) bytes_expr += ",";
                bytes_expr +=
                    std::to_string(static_cast<unsigned>(static_cast<unsigned char>(bytes[i])));
            }
            bytes_expr += "])";
            if (e.type < module_.types.size() &&
                module_.types.get(e.type).kind == MIRTypeKind::Array) {
                // the bytes live in the global itself
                out += "rt.HEAP.set(" + bytes_expr + ", " + addr + ");\n";
            } else {
                out += "rt.st(" + addr + ", " + std::to_string(t.size ? t.size : ptr_size_) +
                       ", 0, rt.cstr(" + bytes_expr + "));\n";
            }
            return true;
        }
        case MIRConstKind::Negate: {
            const MIRConstChild* child = module_.const_exprs.child(id, 0);
            if (child == nullptr) return true;
            const MIRConstExpr& inner = module_.const_exprs.get(child->expr);
            MIRConstant c;
            c.kind = MIRConstantKind::Int;
            c.type = inner.type;
            c.bits = inner.bits;
            std::string lit;
            if (!literal_of(c, lit)) return false;
            out += "rt.st(" + addr + ", " + size + ", " + (t.is_signed ? "1" : "0") + ", -(" + lit +
                   "));\n";
            return true;
        }
        case MIRConstKind::Aggregate:
        case MIRConstKind::Variant: {
            if (e.kind == MIRConstKind::Variant && e.type < module_.types.size()) {
                for (uint32_t i = 0; i < module_.types.get(e.type).data_count; ++i) {
                    const MIRFieldInfo* f = module_.types.field_of(e.type, i);
                    if (f == nullptr) break;
                    if (f->kind != static_cast<uint8_t>(MIRFieldKind::VariantTag)) continue;
                    const JsType ft = type_of(f->type);
                    out += "rt.st(" + addr + ", " + std::to_string(ft.size) + ", 0, " +
                           std::to_string(e.case_index) + ");\n";
                    break;
                }
            }
            for (uint32_t i = 0; i < e.child_count; ++i) {
                const MIRConstChild* child = module_.const_exprs.child(id, i);
                if (child == nullptr) continue;
                uint32_t off = 0;
                if (child->name_length == 0) {
                    if (e.type >= module_.types.size()) continue;
                    const MIRTypeRecord& rec = module_.types.get(e.type);
                    off = i * type_of(rec.element).size;
                } else {
                    TypeId ftype = MIR_INVALID_ID;
                    if (!resolve_field(e.type, module_.const_exprs.child_name_view(*child), ftype,
                                       off)) {
                        return fail("js backend: global initializer names an unknown field '" +
                                    std::string(module_.const_exprs.child_name_view(*child)) + "'");
                    }
                }
                if (!emit_const_into(child->expr, base_offset + off, dest, out)) return false;
            }
            return true;
        }
        case MIRConstKind::FunctionRef: {
            std::string fn_expr;
            if (!function_address_expr(e.symbol, fn_expr)) return false;
            out += "rt.st(" + addr + ", " + std::to_string(t.size ? t.size : ptr_size_) +
                   ", 0, " + fn_expr + ");\n";
            return true;
        }
    }
    return fail("js backend: unsupported global initializer");
}

// ── functions ───────────────────────────────────────────────────────────────

bool ModuleEmitter::emit_function(const MIRFunction& fn, std::string& out) {
    fn_ = &fn;
    // Short names are per function: a fresh alphabet for each, so no two
    // functions share a name and nothing depends on emission order.
    short_values_.assign(fn.values.size(), std::string());
    short_places_.assign(fn.places.size(), std::string());
    next_short_ = 0;
    // Async storage is heap-allocated: a suspended body outlives the frame of the
    // (synchronous) caller that started it.
    async_body_ = fn.async_body || fn.async_poll;
    // A ramp with no await site runs the whole body inline (the lowering's eager
    // case), so the ramp *is* the async function. With awaits the body lives in
    // the poll, and the ramp only sets the frame up and starts that promise.
    async_defers_ = fn.async_body && fn.async_deferred;
    async_arrow_ = fn.async_poll || (fn.async_body && !fn.async_deferred);
    fn_name_ = fn.symbol < module_.symbols.size()
                   ? module_.symbols.mangled_name(module_.symbols.get(fn.symbol))
                   : std::string("<anon>");

    // A line-counting buffer, because every mapped statement must know the line
    // it lands on in the emitted module (a source map is line-based).
    std::string body;
    uint32_t body_line = 0;
    body.reserve(4096);
    auto add = [&](const std::string& text) {
        body += text;
        for (char c : text) {
            if (c == '\n') ++body_line;
        }
    };
    // Record the mapping for the line `add` is about to produce.
    auto map_here = [&](const MIRInstruction& ins) {
        uint32_t file = 0, line = 0, col = 0;
        if (!source_ref_of(fn, ins, file, line, col)) return;
        mappings_.push_back(JsMapping{body_lines_ + body_line, file, line, col});
    };

    // `Param` instructions bind in instruction order - the same pre-pass the
    // interpreter performs, so an sret pointer (the first Param) lands in the
    // value `sret_pointer` names.
    std::vector<const MIRInstruction*> params;
    for (uint32_t i = 0; i < fn.instructions.size(); ++i) {
        if (fn.instructions[i].opcode() == MIROpcode::Param) params.push_back(&fn.instructions[i]);
    }

    add("/** " + fn_name_ + " */\n");
    std::string header = "export function " + js_function_name(fn.symbol) + "(";
    for (size_t i = 0; i < params.size(); ++i) {
        if (i) header += ", ";
        header += "a" + std::to_string(i) + " = 0";
    }
    header += ") {\n";
    add(header);
    add("  const $m = rt.frame_mark();\n");

    if (!fn.values.empty()) {
        // No initializers: a definition dominates every use in well-formed MIR,
        // and an initializer would be a dead store on every path.
        std::string lets = "  let ";
        for (uint32_t i = 0; i < fn.values.size(); ++i) {
            if (i) lets += (i % 16 == 0) ? ",\n      " : ", ";
            lets += vname(i);
        }
        lets += ";\n";
        add(lets);
    }
    frame_slots_.clear();
    frame_place_ = MIR_INVALID_ID;
    frame_type_ = MIR_INVALID_ID;
    for (uint32_t i = 0; i < fn.places.size() && i < 256; ++i) {
        const TypeId pt = pointee_of(place_type(i));
        if (pt == MIR_INVALID_ID) continue;
        TypeId ft = MIR_INVALID_ID;
        uint32_t off = 0;
        if (!resolve_field(pt, "__result", ft, off)) continue;
        frame_place_ = i;
        frame_type_ = pt;
        break;
    }
    for (uint32_t i = 0; i < fn.places.size(); ++i) {
        const MIRPlaceDef& pd = fn.places[i];
        if (pd.storage_class == static_cast<uint8_t>(MIRStorageClass::FrameField)) {
            // Bound to the frame field by the first store/load of that field.
            std::string_view field;
            if (pd.frame_field != MIR_INVALID_ID &&
                constant_text(module_, pd.frame_field, field)) {
                frame_slots_.emplace_back(i, std::string(field));
                add("  let " + pname(i) + " = 0;\n");
                continue;
            }
        }
        const JsType t = type_of(place_type(i));
        const uint32_t size = t.size ? t.size : 1;
        const uint32_t align = t.align ? t.align : 1;
        add("  const " + pname(i) + " = " + alloc_call(size, align) + ";\n");
    }
    for (size_t i = 0; i < params.size(); ++i) {
        add("  " + vname(params[i]->result_or_place) + " = a" + std::to_string(i) + ";\n");
    }

    // An async function's ramp body becomes the body of an `async` arrow whose
    // promise is stored behind a token in the returned `FutureHandle<T>` slot.
    // `await` then suspends that arrow instead of the coroutine's poll loop, and
    // the state machine below still works because `$pc` is a closure variable.
    if (async_arrow_) {
        if (fn.async_body && fn.sret_pointer == MIR_INVALID_ID) {
            return fail_at("async function without an sret pointer");
        }
        add("  const $p = (async () => {\n");
    }

    const BlockId entry = (fn.entry_block < fn.blocks.size()) ? fn.entry_block : 0;
    std::string pc = "  let $pc = ";
    if (fn.blocks.empty()) {
        pc += "0";
    } else {
        append_uint(pc, fn.blocks[entry].id);
    }
    pc += ";\n";
    add(pc);
    add("  for (;;) {\n");
    add("    switch ($pc) {\n");

    for (uint32_t bi = 0; bi < fn.blocks.size(); ++bi) {
        const MIRBlock& block = fn.blocks[bi];
        add("      case " + std::to_string(block.id) + ": {\n");
        for (uint32_t k = 0; k < block.inst_count; ++k) {
            const uint32_t index = block.inst_start + k;
            if (index >= fn.instructions.size()) break;
            const MIRInstruction& ins = fn.instructions[index];
            if (ins.opcode() == MIROpcode::Param) continue; // bound in the prologue
            if (options_.instruction_comments) {
                add("        // " + std::string(opcode_name(ins.opcode())) + "\n");
            }
            std::string stmt;
            current_loc_ = location_of(fn, ins);
            if (!emit_instruction(ins, stmt)) return false;
            if (!stmt.empty()) {
                map_here(ins);
                add("        " + stmt + "\n");
            }
        }
        const uint32_t term_index = block.inst_start + block.inst_count;
        if (term_index < fn.instructions.size()) {
            const MIRInstruction& term = fn.instructions[term_index];
            std::string stmt;
            current_loc_ = location_of(fn, term);
            if (!emit_terminator(term, stmt)) return false;
            map_here(term);
            add("        " + stmt + "\n");
        }
        add("      }\n");
    }
    add("      default: throw new Error(\"js backend: bad block \" + $pc);\n");
    add("    }\n");
    add("  }\n");
    if (async_arrow_) {
        // The arrow returns only through `AsyncRampFinish` / `AsyncFinish`;
        // falling out of the dispatch loop would mean a return path the lowering
        // did not emit.
        add("  throw new Error(\"js backend: async body fell through: " + fn_name_ + "\");\n");
        add("  })();\n");
        if (fn.async_body) {
            add("  rt.promise_put(" + vname(fn.sret_pointer) + ", $p);\n");
        } else {
            add("  return $p;\n");
        }
    }
    add("}\n\n");

    body_lines_ += body_line;
    out += body;
    fn_ = nullptr;
    async_body_ = false;
    async_arrow_ = false;
    async_defers_ = false;
    return true;
}

// ── instructions ────────────────────────────────────────────────────────────

bool ModuleEmitter::emit_instruction(const MIRInstruction& ins, std::string& out) {
    auto op = [&](uint32_t i) -> const MIROperand* {
        if (i >= ins.operand_count) return nullptr;
        return &fn_->operands[ins.operand_offset + i];
    };
    const uint32_t rp = ins.result_or_place;
    auto result = [&]() -> std::string { return vname(rp); };
    // `agg_type_of_base`: the aggregate a field designator is resolved against.
    auto agg_of = [&](const MIROperand& base) -> TypeId {
        const TypeId bt = operand_type(base);
        const TypeId p = pointee_of(bt);
        return p != MIR_INVALID_ID ? p : bt;
    };

    switch (ins.opcode()) {
        case MIROpcode::Nop:
        case MIROpcode::Alloca: // storage was allocated in the prologue
        case MIROpcode::StorageLive:
        case MIROpcode::StorageDead:
        case MIROpcode::MoveOut:
        case MIROpcode::Param:
            return true;

        case MIROpcode::ConstInt:
        case MIROpcode::ConstFloat:
        case MIROpcode::ConstDouble:
        case MIROpcode::ConstBool:
        case MIROpcode::ConstNull: {
            const MIROperand* c = op(0);
            if (c == nullptr) return fail_at("constant without an operand");
            std::string lit;
            if (!constant_expr(c->id, lit)) return fail_at("unsupported constant kind");
            out = result() + " = " + lit + ";";
            return true;
        }
        case MIROpcode::ConstString: {
            const MIROperand* c = op(0);
            if (c == nullptr) return fail_at("const.string without a constant");
            out = result() + " = $k" + std::to_string(c->id) + ";";
            return true;
        }
        case MIROpcode::GlobalAddr: {
            const MIROperand* s = op(0);
            if (s == nullptr) return fail_at("global_addr without a symbol");
            const std::string decl = "$g" + std::to_string(s->id);
            addressed_symbols_.insert(s->id);
            std::string foreign;
            if (foreign_stem(s->id, foreign)) note_import(foreign, decl);
            out = result() + " = " + decl + ";";
            return true;
        }
        case MIROpcode::FunctionAddr: {
            const MIROperand* s = op(0);
            if (s == nullptr) return fail_at("function_addr without a symbol");
            std::string addr;
            if (!function_address_expr(s->id, addr)) return false;
            out = result() + " = " + addr + ";";
            return true;
        }

        case MIROpcode::Load: {
            const MIROperand* base = op(0);
            const MIROperand* km = op(1);
            if (base == nullptr || km == nullptr) return fail_at("load without operands");
            TypeId accessed = value_type(rp);
            std::string addr;
            std::string alias;
            if (km->kind() == MIROperandKind::Constant) {
                std::string_view designator;
                if (!constant_text(module_, km->id, designator)) {
                    return fail_at("field load without a field name");
                }
                TypeId ftype = MIR_INVALID_ID;
                uint32_t off = 0;
                if (!resolve_field(agg_of(*base), designator, ftype, off)) {
                    return fail_at("unknown field '" + std::string(designator) + "'");
                }
                accessed = ftype;
                if (!base_addr_expr(*base, addr)) return fail_at("bad load base");
                addr = addr + " + " + std::to_string(off);
                alias = bind_frame_slots(designator, addr);
            } else if (km->kind() == MIROperandKind::Value) {
                const TypeId et = element_of(operand_type(*base));
                if (et == MIR_INVALID_ID) return fail_at("indexed load without an element type");
                accessed = et;
                if (!base_addr_expr(*base, addr)) return fail_at("bad load base");
                addr = addr + " + " + vname(km->id) + " * " + std::to_string(type_of(et).size);
            } else {
                // Plain `load p, ty`: the place's own storage, no dereference
                // (only the field/index forms above project through a pointer).
                if (!storage_addr_expr(*base, addr)) return fail_at("bad load base");
            }
            const JsType t = type_of(accessed);
            if (t.is_agg) {
                // An aggregate load produces a *copy*: MIR values are single
                // words, so the address of fresh storage is the value.
                out = result() + " = " + alloc_call(t.size, t.align) + "; rt.memcpy(" + result() + ", " + addr + ", " +
                      std::to_string(t.size) + ");";
                out = alias + out;
                return true;
            }
            if (t.is_float || t.is_double) {
                out = result() + " = rt.ldF(" + addr + ", " + std::to_string(t.size) + ");";
                out = alias + out;
                return true;
            }
            out = result() + " = rt.ld(" + addr + ", " + std::to_string(t.size) + ", " +
                  (t.is_signed ? "1" : "0") + ");";
            out = alias + out;
            return true;
        }
        case MIROpcode::Store: {
            const MIROperand* dst = op(0);
            const MIROperand* km = op(1);
            if (dst == nullptr || km == nullptr) return fail_at("store without operands");
            std::string addr;
            std::string alias;
            JsType t;
            const MIROperand* src = nullptr;
            if (km->kind() == MIROperandKind::Constant) {
                src = op(2);
                if (src == nullptr) return fail_at("field store without a value");
                std::string_view designator;
                if (!constant_text(module_, km->id, designator)) {
                    return fail_at("field store without a field name");
                }
                TypeId ftype = MIR_INVALID_ID;
                uint32_t off = 0;
                if (!resolve_field(agg_of(*dst), designator, ftype, off)) {
                    return fail_at("unknown field '" + std::string(designator) + "'");
                }
                t = type_of(ftype);
                if (!base_addr_expr(*dst, addr)) return fail_at("bad store base");
                addr = addr + " + " + std::to_string(off);
                alias = bind_frame_slots(designator, addr);
            } else if (km->kind() == MIROperandKind::Value && ins.operand_count >= 3) {
                src = op(2);
                if (src == nullptr) return fail_at("element store without a value");
                const TypeId et = element_of(operand_type(*dst));
                if (et == MIR_INVALID_ID) return fail_at("indexed store without an element type");
                t = type_of(et);
                if (!base_addr_expr(*dst, addr)) return fail_at("bad store base");
                addr = addr + " + " + vname(km->id) + " * " + std::to_string(t.size);
            } else {
                src = km;
                TypeId dt = MIR_INVALID_ID;
                if (dst->kind() == MIROperandKind::Place) {
                    dt = place_type(dst->id);
                } else {
                    dt = pointee_of(operand_type(*dst));
                    if (dt == MIR_INVALID_ID) dt = operand_type(*src);
                }
                t = type_of(dt);
                if (!storage_addr_expr(*dst, addr)) return fail_at("bad store destination");
            }
            if (t.is_agg) {
                std::string src_addr;
                if (src->kind() == MIROperandKind::Place) {
                    src_addr = pname(src->id);
                } else if (!operand_expr(*src, src_addr)) {
                    return fail_at("bad aggregate store source");
                }
                uint32_t n = t.size;
                const JsType st = type_of(operand_type(*src));
                if (st.size != 0 && st.size < n) n = st.size;
                out = "rt.memcpy(" + addr + ", " + src_addr + ", " + std::to_string(n) + ");";
                out = alias + out;
                return true;
            }
            std::string value;
            if (!operand_expr(*src, value)) return fail_at("bad store source");
            if (t.is_float || t.is_double) {
                out = "rt.stF(" + addr + ", " + std::to_string(t.size) + ", " + value + ");";
            } else {
                out = "rt.st(" + addr + ", " + std::to_string(t.size) + ", " +
                      (t.is_signed ? "1" : "0") + ", " + value + ");";
            }
            if (dst->kind() == MIROperandKind::Place && dst->id == frame_place_ &&
                frame_type_ != MIR_INVALID_ID) {
                // This store is where the poll learns the frame's address.
                out += " " + bind_all_frame_slots("rt.ld(" + pname(dst->id) + ", " +
                                                  std::to_string(ptr_size_) + ", 0)");
            }
            out = alias + out;
            return true;
        }
        case MIROpcode::AddressOf: {
            const MIROperand* p = op(0);
            if (p == nullptr || p->kind() != MIROperandKind::Place) {
                return fail_at("address_of without a place");
            }
            out = result() + " = " + pname(p->id) + ";";
            return true;
        }
        case MIROpcode::Gep: {
            const MIROperand* base = op(0);
            if (base == nullptr) return fail_at("gep without a base");
            std::string addr;
            // `gep` adds byte offsets to the operand's own address (a place's
            // storage or a pointer value); it does not project through a pointer.
            if (!storage_addr_expr(*base, addr)) return fail_at("bad gep base");
            for (uint32_t i = 1; i + 1 < ins.operand_count; ++i) {
                const MIROperand* k = op(i);
                if (k == nullptr) break;
                std::string off;
                if (!operand_expr(*k, off)) return fail_at("bad gep index");
                addr = addr + " + " + off;
            }
            out = result() + " = " + addr + ";";
            return true;
        }
        case MIROpcode::FieldAddr: {
            const MIROperand* base = op(0);
            const MIROperand* fld = op(1);
            if (base == nullptr || fld == nullptr) return fail_at("field_addr without operands");
            std::string_view designator;
            if (!constant_text(module_, fld->id, designator)) {
                return fail_at("field_addr without a field name");
            }
            TypeId ftype = MIR_INVALID_ID;
            uint32_t off = 0;
            if (!resolve_field(agg_of(*base), designator, ftype, off)) {
                return fail_at("unknown field '" + std::string(designator) + "'");
            }
            std::string addr;
            if (!base_addr_expr(*base, addr)) return fail_at("bad field_addr base");
            out = result() + " = " + addr + " + " + std::to_string(off) + ";";
            return true;
        }
        case MIROpcode::IndexAddr: {
            const MIROperand* base = op(0);
            const MIROperand* idx = op(1);
            if (base == nullptr || idx == nullptr) return fail_at("index_addr without operands");
            const JsType et = type_of(value_type(rp));
            std::string addr, index;
            if (!base_addr_expr(*base, addr)) return fail_at("bad index_addr base");
            if (!operand_expr(*idx, index)) return fail_at("bad index_addr index");
            out = result() + " = " + addr + " + " + index + " * " + std::to_string(et.size) + ";";
            return true;
        }
        case MIROpcode::Init: {
            const MIROperand* dest = op(0);
            const MIROperand* ctor = op(1);
            if (dest == nullptr || ctor == nullptr) return fail_at("init without operands");
            if (dest->kind() != MIROperandKind::Place) return fail_at("init without a destination");
            std::string callee;
            if (!callee_expr(ctor->id, callee)) return false;
            std::string call = callee + "(" + pname(dest->id);
            for (uint32_t i = 2; i < ins.operand_count; ++i) {
                std::string arg;
                if (!operand_expr(*op(i), arg)) return fail_at("bad init argument");
                call += ", " + arg;
            }
            out = call + ");";
            return true;
        }
        case MIROpcode::CopyInit:
        case MIROpcode::MoveInit:
        case MIROpcode::Assign: {
            const MIROperand* dest = op(0);
            const MIROperand* src = op(1);
            if (dest == nullptr || src == nullptr) return fail_at("copy without operands");
            if (dest->kind() != MIROperandKind::Place) return fail_at("copy without a destination");
            const JsType t = type_of(place_type(dest->id));
            if (t.is_agg) {
                std::string src_addr;
                if (src->kind() == MIROperandKind::Place) {
                    src_addr = pname(src->id);
                } else if (!operand_expr(*src, src_addr)) {
                    return fail_at("bad aggregate copy source");
                }
                out = "rt.memcpy(" + pname(dest->id) + ", " + src_addr + ", " +
                      std::to_string(t.size) + ");";
                return true;
            }
            std::string value;
            if (!operand_expr(*src, value)) return fail_at("bad copy source");
            if (t.is_float || t.is_double) {
                out = "rt.stF(" + pname(dest->id) + ", " + std::to_string(t.size) + ", " + value +
                      ");";
            } else {
                out = "rt.st(" + pname(dest->id) + ", " + std::to_string(t.size) + ", " +
                      (t.is_signed ? "1" : "0") + ", " + value + ");";
            }
            return true;
        }
        case MIROpcode::Copy:
        case MIROpcode::Move: {
            const MIROperand* src = op(0);
            if (src == nullptr) return fail_at("copy without a source");
            if (src->kind() == MIROperandKind::Place) {
                const JsType st = type_of(place_type(src->id));
                if (st.is_agg) {
                    out = result() + " = " + alloc_call(st.size, st.align) + "; rt.memcpy(" +
                          result() + ", " + pname(src->id) + ", " +
                          std::to_string(st.size) + ");";
                    return true;
                }
                std::string value;
                if (!scalar_read(*src, st, value)) return fail_at("bad copy source");
                out = result() + " = " + value + ";";
                return true;
            }
            std::string value;
            if (!operand_expr(*src, value)) return fail_at("bad copy source");
            out = result() + " = " + value + ";";
            return true;
        }
        case MIROpcode::Drop:
        case MIROpcode::Destroy: {
            const MIROperand* place = op(0);
            const MIROperand* dtor = op(1);
            if (place == nullptr || dtor == nullptr) return fail_at("drop without operands");
            if (place->kind() != MIROperandKind::Place) return fail_at("drop without a place");
            std::string callee;
            if (!callee_expr(dtor->id, callee)) return false;
            if (ins.opcode() == MIROpcode::Drop && ins.operand_count >= 3) {
                std::string flag;
                if (!operand_expr(*op(2), flag)) return fail_at("bad drop flag");
                out = "if (" + flag + ") { " + callee + "(" + pname(place->id) + "); }";
                return true;
            }
            const TypeId pt = place_type(place->id);
            if (pt != MIR_INVALID_ID && pt < module_.types.size() &&
                module_.types.get(pt).kind == MIRTypeKind::Array) {
                const MIRTypeRecord& pr = module_.types.get(pt);
                const JsType et = type_of(pr.element);
                out = "for (let $i = " + std::to_string(pr.data_count) + "; $i-- > 0;) { " + callee +
                      "(" + pname(place->id) + " + $i * " + std::to_string(et.size) + "); }";
                return true;
            }
            const JsType t = type_of(pt);
            if (t.is_ptr) {
                out = callee + "(rt.ld(" + pname(place->id) + ", " + std::to_string(t.size) +
                      ", 0));";
            } else {
                out = callee + "(" + pname(place->id) + ");";
            }
            return true;
        }
        case MIROpcode::SetDrop: {
            const MIROperand* place = op(0);
            const MIROperand* value = op(1);
            if (place == nullptr || value == nullptr) return fail_at("set_drop without operands");
            if (place->kind() != MIROperandKind::Place) return fail_at("set_drop without a place");
            const JsType t = type_of(place_type(place->id));
            std::string v;
            if (!operand_expr(*value, v)) return fail_at("bad drop flag");
            out = "rt.st(" + pname(place->id) + ", " + std::to_string(t.size) + ", 0, " + v + ");";
            return true;
        }
        case MIROpcode::MemCpy: {
            const MIROperand* dst = op(0);
            const MIROperand* src = op(1);
            if (dst == nullptr || src == nullptr) return fail_at("memcpy without operands");
            std::string d, s, n;
            if (!storage_addr_expr(*dst, d)) return fail_at("bad memcpy destination");
            if (!storage_addr_expr(*src, s)) return fail_at("bad memcpy source");
            if (ins.operand_count >= 3) {
                if (!operand_expr(*op(2), n)) return fail_at("bad memcpy size");
            } else if (dst->kind() == MIROperandKind::Place) {
                n = std::to_string(type_of(place_type(dst->id)).size);
            } else {
                return fail_at("memcpy without a size");
            }
            out = "rt.memcpy(" + d + ", " + s + ", " + n + ");";
            return true;
        }
        case MIROpcode::MemSet: {
            const MIROperand* dst = op(0);
            if (dst == nullptr) return fail_at("memset without a destination");
            std::string d, b, n;
            if (!storage_addr_expr(*dst, d)) return fail_at("bad memset destination");
            if (ins.operand_count >= 2) {
                if (!operand_expr(*op(1), b)) return fail_at("bad memset byte");
            } else {
                b = "0";
            }
            if (ins.operand_count >= 3) {
                if (!operand_expr(*op(2), n)) return fail_at("bad memset size");
            } else if (dst->kind() == MIROperandKind::Place) {
                n = std::to_string(type_of(place_type(dst->id)).size);
            } else {
                return fail_at("memset without a size");
            }
            out = "rt.fill(" + d + ", " + b + ", " + n + ");";
            return true;
        }
        case MIROpcode::Unary: {
            const MIROperand* a = op(0);
            const MIROperand* opc = op(1);
            if (a == nullptr || opc == nullptr) return fail_at("unary without operands");
            const JsType at = type_of(operand_type(*a));
            std::string av;
            if (!operand_expr(*a, av)) return fail_at("bad unary operand");
            out = result() + " = rt.unop(" + std::to_string(module_.constants.get(opc->id).bits) +
                  ", " + av + ", " + std::to_string(at.size * 8) + ", " +
                  (at.is_signed ? "1" : "0") + ", " + (at.is_float ? "1" : "0") + ", " +
                  (at.is_double ? "1" : "0") + ");";
            return true;
        }
        case MIROpcode::Binary: {
            const MIROperand* a = op(0);
            const MIROperand* b = op(1);
            const MIROperand* opc = op(2);
            if (a == nullptr || b == nullptr || opc == nullptr) {
                return fail_at("binary without operands");
            }
            const uint64_t op_value = module_.constants.get(opc->id).bits;
            const JsType at = type_of(operand_type(*a));
            const JsType bt = type_of(operand_type(*b));
            const JsType rt = type_of(value_type(rp));
            std::string av, bv;
            if (!operand_expr(*a, av)) return fail_at("bad binary operand");
            if (!operand_expr(*b, bv)) return fail_at("bad binary operand");

            if ((op_value == 0 || op_value == 1) && (at.is_ptr || bt.is_ptr)) {
                // C pointer arithmetic scales by the pointee size.
                const TypeId pt_type = at.is_ptr ? operand_type(*a) : operand_type(*b);
                uint32_t scale = type_of(pointee_of(pt_type)).size;
                if (scale == 0) scale = 1;
                if (at.is_ptr && bt.is_ptr) {
                    out = result() + " = rt.pdiff(" + av + ", " + bv + ", " +
                          std::to_string(scale) + ");";
                    return true;
                }
                if (at.is_ptr) {
                    out = result() + " = " + (op_value == 0 ? "rt.padd(" : "rt.psub(") + av + ", " +
                          bv + ", " + std::to_string(scale) + ");";
                    return true;
                }
                out = result() + " = rt.padd(" + bv + ", " + av + ", " + std::to_string(scale) +
                      ");";
                return true;
            }
            if (at.is_float || at.is_double || bt.is_float || bt.is_double) {
                const bool dbl = at.is_double || bt.is_double;
                out = result() + " = rt.fbinop(" + std::to_string(op_value) + ", " + av + ", " +
                      bv + ", " + (dbl ? "1" : "0") + ");";
                return true;
            }
            out = result() + " = rt.binop(" + std::to_string(op_value) + ", " + av + ", " +
                  std::to_string(at.size * 8) + ", " + (at.is_signed ? "1" : "0") + ", " + bv +
                  ", " + std::to_string(bt.size * 8) + ", " + (bt.is_signed ? "1" : "0") + ", " +
                  std::to_string(rt.size * 8) + ", " + (rt.is_signed ? "1" : "0") + ");";
            return true;
        }
        case MIROpcode::Compare: {
            const MIROperand* a = op(0);
            const MIROperand* b = op(1);
            const MIROperand* opc = op(2);
            if (a == nullptr || b == nullptr || opc == nullptr) {
                return fail_at("compare without operands");
            }
            const uint64_t op_value = module_.constants.get(opc->id).bits;
            const JsType at = type_of(operand_type(*a));
            const JsType bt = type_of(operand_type(*b));
            std::string av, bv;
            if (!operand_expr(*a, av)) return fail_at("bad compare operand");
            if (!operand_expr(*b, bv)) return fail_at("bad compare operand");
            if (at.is_float || at.is_double || bt.is_float || bt.is_double) {
                const bool dbl = at.is_double || bt.is_double;
                out = result() + " = rt.fcmp(" + std::to_string(op_value) + ", " + av + ", " + bv +
                      ", " + (dbl ? "1" : "0") + ");";
                return true;
            }
            out = result() + " = rt.cmp(" + std::to_string(op_value) + ", " + av + ", " +
                  std::to_string(at.size * 8) + ", " + (at.is_signed ? "1" : "0") + ", " + bv +
                  ", " + std::to_string(bt.size * 8) + ", " + (bt.is_signed ? "1" : "0") + ");";
            return true;
        }
        case MIROpcode::Cast: {
            const MIROperand* a = op(0);
            if (a == nullptr) return fail_at("cast without an operand");
            const JsType at = type_of(operand_type(*a));
            const JsType rt = type_of(value_type(rp));
            std::string av;
            if (!operand_expr(*a, av)) return fail_at("bad cast operand");
            if (at.is_agg || rt.is_agg) {
                out = result() + " = " + av + ";";
                return true;
            }
            out = result() + " = rt.cast(" + av + ", " + std::to_string(at.size * 8) + ", " +
                  (at.is_signed ? "1" : "0") + ", " + ((at.is_float || at.is_double) ? "1" : "0") +
                  ", " + std::to_string(rt.size * 8) + ", " + (rt.is_signed ? "1" : "0") + ", " +
                  ((rt.is_float || rt.is_double) ? "1" : "0") + ");";
            return true;
        }
        case MIROpcode::Select: {
            const MIROperand* cond = op(0);
            const MIROperand* t = op(1);
            const MIROperand* f = op(2);
            if (cond == nullptr || t == nullptr || f == nullptr) {
                return fail_at("select without operands");
            }
            std::string c, tv, fv;
            if (!operand_expr(*cond, c)) return fail_at("bad select condition");
            if (!operand_expr(*t, tv)) return fail_at("bad select operand");
            if (!operand_expr(*f, fv)) return fail_at("bad select operand");
            out = result() + " = (" + c + ") ? " + tv + " : " + fv + ";";
            return true;
        }
        case MIROpcode::SizeOf:
        case MIROpcode::AlignOf: {
            const MIROperand* t = op(0);
            if (t == nullptr) return fail_at("sizeof without a type");
            const JsType info = type_of(t->type());
            out = result() + " = " +
                  std::to_string(ins.opcode() == MIROpcode::SizeOf ? info.size : info.align) + ";";
            return true;
        }
        case MIROpcode::OffsetOf: {
            const MIROperand* t = op(0);
            const MIROperand* fld = op(1);
            if (t == nullptr || fld == nullptr) return fail_at("offsetof without operands");
            std::string_view designator;
            if (!constant_text(module_, fld->id, designator)) {
                return fail_at("offsetof without a field name");
            }
            TypeId ftype = MIR_INVALID_ID;
            uint32_t off = 0;
            if (!resolve_field(t->type(), designator, ftype, off)) {
                return fail_at("unknown field '" + std::string(designator) + "'");
            }
            out = result() + " = " + std::to_string(off) + ";";
            return true;
        }
        case MIROpcode::Call: {
            const MIROperand* callee = op(0);
            if (callee == nullptr) return fail_at("call without a callee");
            const MIROperand* maybe_sret = op(1);
            const bool sret = maybe_sret != nullptr && maybe_sret->kind() == MIROperandKind::Place;
            std::string name;
            if (!callee_expr(callee->id, name)) return false;
            const std::string trap = trap_name_;
            std::string call = name + "(";
            bool first = true;
            if (!trap.empty()) {
                call += "\"" + trap + "\"";
                first = false;
            }
            if (sret) {
                if (!first) call += ", ";
                call += pname(maybe_sret->id);
                first = false;
            }
            for (uint32_t i = sret ? 2 : 1; i < ins.operand_count; ++i) {
                if (!first) call += ", ";
                std::string arg;
                if (!operand_expr(*op(i), arg)) return fail_at("bad call argument");
                call += arg;
                first = false;
            }
            call += ");";
            out = sret ? call : result() + " = " + call;
            return true;
        }
        case MIROpcode::DynamicCall: {
            // `dyn I` is a fat pointer `{ first: self, second: vtable }`, exactly as
            // the C backend spells it (`((vt_t*)fat->second)->m(fat->first, ...)`).
            // The vtable is a block of function addresses emitted by
            // `emit_vtables`, so the dispatch is: load `second`, load the slot's
            // address from it, and call through the runtime's table.
            const MIROperand* fat = op(0);
            const MIROperand* vt = op(1);
            const MIROperand* meth = op(2);
            if (fat == nullptr || vt == nullptr || meth == nullptr) {
                return fail_at("dynamic_call missing operands");
            }
            std::string vt_name, method;
            if (!constant_text_of(vt->id, vt_name) || !constant_text_of(meth->id, method)) {
                return fail_at("dynamic_call has non-string vtable/method operands");
            }
            uint32_t slot = 0;
            if (!vtable_slot_of(vt_name, method, slot)) {
                return fail_at("'" + method + "' is not a slot of interface vtable '" + vt_name +
                               "' (the dispatch cannot be resolved from MIR)");
            }
            // A fat pointer reached through a pointer/&dyn parameter is the value
            // itself (an address); a by-value aggregate place is its storage.
            std::string fat_addr;
            const bool fat_is_ptr =
                fat->type() < module_.types.size() &&
                (module_.types.get(fat->type()).kind == MIRTypeKind::Pointer ||
                 module_.types.get(fat->type()).kind == MIRTypeKind::Reference);
            if (fat_is_ptr) {
                if (!operand_expr(*fat, fat_addr)) return fail_at("bad dynamic_call receiver");
            } else {
                if (!base_addr_expr(*fat, fat_addr)) return fail_at("bad dynamic_call receiver");
            }
            const std::string ptr = std::to_string(ptr_size_);
            const std::string self = "rt.ld(" + fat_addr + ", " + ptr + ", 0)";
            const std::string table = "rt.ld((" + fat_addr + " + " + ptr + "), " + ptr + ", 0)";
            const std::string addr = "rt.ld((" + table + " + " + std::to_string(slot * ptr_size_) +
                                     "), " + ptr + ", 0)";
            std::string call = "rt.indirect(rt.fns, " + addr + ")(" + self;
            // Operands 3.. are the arguments; `self` is implicit (operand 0).
            for (uint32_t i = 3; i < ins.operand_count; ++i) {
                std::string arg;
                if (!operand_expr(*op(i), arg)) return fail_at("bad dynamic_call argument");
                call += ", " + arg;
            }
            call += ");";
            out = result() + " = " + call;
            return true;
        }

        case MIROpcode::CallIndirect: {
            const MIROperand* callee = op(0);
            if (callee == nullptr) return fail_at("call_indirect without a callee");
            const MIROperand* maybe_sret = op(1);
            const bool sret = maybe_sret != nullptr && maybe_sret->kind() == MIROperandKind::Place;
            std::string name;
            if (!operand_expr(*callee, name)) return fail_at("bad indirect callee");
            // The callee operand is a function address (a symbol id) read out of
            // memory; `rt.indirect` resolves it and reports an unresolved address
            // instead of "undefined is not a function".
            // The merged table, not this module's: a bundle is one module per
            // Chemical module, and the address was very likely taken elsewhere.
            std::string call = "rt.indirect(rt.fns, " + name + ")(";
            bool first = true;
            if (sret) {
                call += pname(maybe_sret->id);
                first = false;
            }
            for (uint32_t i = sret ? 2 : 1; i < ins.operand_count; ++i) {
                if (!first) call += ", ";
                std::string arg;
                if (!operand_expr(*op(i), arg)) return fail_at("bad call argument");
                call += arg;
                first = false;
            }
            call += ");";
            out = sret ? call : result() + " = " + call;
            return true;
        }
        case MIROpcode::Borrow:
        case MIROpcode::BorrowMut: {
            const MIROperand* a = op(0);
            if (a == nullptr) return fail_at("borrow without an operand");
            std::string av;
            if (!operand_expr(*a, av)) return fail_at("bad borrow operand");
            out = result() + " = " + av + ";";
            return true;
        }

        // ── async: promises, not a poll protocol ────────────────────────────
        // The lowering is a coroutine because the native backends need one. In a
        // JS build each async body is a single `async` arrow and each `await`
        // suspends it on a promise, so the frame only has to hold the awaited
        // child's token between statements — the state the coroutine would keep
        // in `__state`/spills lives in the arrow's closure instead.
        case MIROpcode::AsyncFrameAlloc: {
            const MIROperand* frame_type = op(0);
            if (frame_type == nullptr || frame_type->kind() != MIROperandKind::Type) {
                return fail_at("async frame allocation without a frame type");
            }
            const JsType t = type_of(frame_type->id);
            // The frame's address is what every frame slot aliases from now on.
            out = result() + " = " + alloc_call(t.size, t.align) + "; " +
                  bind_all_frame_slots(result());
            return true;
        }
        case MIROpcode::AsyncAwait: {
            // operands: Place(frame), Place(child), Place(result), Type(poll),
            //           Constant(state), Place(ret), Type(fn_poll),
            //           Constant(spill_count), [Place, Constant(field)]*
            const MIROperand* child = op(1);
            const MIROperand* result_place = op(2);
            if (child == nullptr || result_place == nullptr ||
                result_place->kind() != MIROperandKind::Place) {
                return fail_at("async await without a result place");
            }
            if (child->kind() != MIROperandKind::Place) {
                return fail_at("async await of a non-place child");
            }
            const JsType rt_t = type_of(place_type(result_place->id));
            if (rt_t.is_agg) {
                return fail_at(
                    "the JS backend's promise lowering does not support an aggregate await result yet");
            }
            // The child handle is a fat pointer; the promise token lives in its
            // first slot (see `rt.promise_put`).
            const std::string token = "rt.ld(" + pname(child->id) + ", " +
                                      std::to_string(ptr_size_) + ", 0)";
            out = "rt.st(" + pname(result_place->id) + ", " + std::to_string(rt_t.size) + ", " +
                  (rt_t.is_signed ? "1" : "0") + ", await rt.promise_get(" + token + "));";
            return true;
        }
        default:
            // One answer for "is this expressible for JS?" lives in
            // MIRTargetSupport, so `--check`, the emitter and (later) the LSP agree
            // instead of each carrying its own list of gaps.
            if (const char* reason = mir_unsupported_reason(MIRTargetKind::JS, ins.opcode())) {
                return fail_at(reason);
            }
            return fail_at(std::string("opcode '") + opcode_name(ins.opcode()) +
                           "' is not supported by the JS backend");
    }
}

bool ModuleEmitter::emit_terminator(const MIRInstruction& ins, std::string& out) {
    auto op = [&](uint32_t i) -> const MIROperand* {
        if (i >= ins.operand_count) return nullptr;
        return &fn_->operands[ins.operand_offset + i];
    };
    switch (ins.opcode()) {
        case MIROpcode::Br: {
            const MIROperand* target = op(0);
            if (target == nullptr || target->kind() != MIROperandKind::Block) {
                return fail_at("br without a target");
            }
            out = "$pc = " + std::to_string(target->id) + "; continue;";
            return true;
        }
        case MIROpcode::CondBr: {
            const MIROperand* cond = op(0);
            const MIROperand* t = op(1);
            const MIROperand* e = op(2);
            if (cond == nullptr || t == nullptr || e == nullptr) {
                return fail_at("cond_br without operands");
            }
            std::string cv;
            if (!operand_expr(*cond, cv)) return fail_at("bad cond_br condition");
            out = "$pc = (" + cv + ") ? " + std::to_string(t->id) + " : " +
                  std::to_string(e->id) + "; continue;";
            return true;
        }
        case MIROpcode::Switch: {
            const MIROperand* scrut = op(0);
            const MIROperand* def = op(1);
            if (scrut == nullptr || def == nullptr) return fail_at("switch without operands");
            std::string sv;
            if (!operand_expr(*scrut, sv)) return fail_at("bad switch scrutinee");
            std::string stmt = "switch (" + sv + ") { ";
            for (uint32_t i = 2; i + 1 < ins.operand_count; i += 2) {
                const MIROperand* cv = op(i);
                const MIROperand* bv = op(i + 1);
                if (cv == nullptr || bv == nullptr) break;
                std::string case_value;
                if (!operand_expr(*cv, case_value)) return fail_at("bad switch case");
                stmt += "case " + case_value + ": $pc = " + std::to_string(bv->id) + "; break; ";
            }
            stmt += "default: $pc = " + std::to_string(def->id) + "; } continue;";
            out = stmt;
            return true;
        }
        case MIROpcode::Return: {
            const MIROperand* value = op(0);
            const TypeId ret_type =
                (fn_->function_type != MIR_INVALID_ID && fn_->function_type < module_.types.size())
                    ? module_.types.get(fn_->function_type).element
                    : MIR_INVALID_ID;
            const JsType ret = type_of(ret_type);
            if (value == nullptr) {
                out = "rt.frame_release($m); return 0;";
                return true;
            }
            if (ret.is_agg) {
                if (fn_->sret_pointer == MIR_INVALID_ID) {
                    return fail_at("aggregate return without an sret pointer");
                }
                std::string src;
                if (value->kind() == MIROperandKind::Place) {
                    src = pname(value->id);
                } else if (!operand_expr(*value, src)) {
                    return fail_at("bad return value");
                }
                out = "rt.memcpy(" + vname(fn_->sret_pointer) + ", " + src + ", " +
                      std::to_string(ret.size) + "); rt.frame_release($m); return 0;";
                return true;
            }
            std::string v;
            if (!operand_expr(*value, v)) return fail_at("bad return value");
            out = "rt.frame_release($m); return " + v + ";";
            return true;
        }
        case MIROpcode::AsyncRampFinish: {
            // operands: Place(frame), Place(sret), Constant(state),
            //           Constant(vtbl_name), Type(handle)
            //
            // The coroutine protocol builds a `{frame, vtbl}` handle here and lets
            // the caller poll it. A promise backend has two shapes instead:
            //  - the body ran inline (eager ramp): the arrow *is* the future, so
            //    resolve it with the value the body stored in `__result`;
            //  - the body lives in the poll: start that promise and hand it over.
            const MIROperand* frame = op(0);
            if (frame == nullptr) return fail_at("async ramp finish without a frame");
            const MIROperand* sret = op(1);
            const MIROperand* vtbl_name = op(3);
            if (async_defers_) {
                if (sret == nullptr || sret->kind() != MIROperandKind::Place) {
                    return fail_at("async ramp finish without an sret place");
                }
                std::string_view table;
                if (vtbl_name == nullptr || !constant_text(module_, vtbl_name->id, table) ||
                    table.size() < 6) {
                    return fail_at("async ramp finish without a vtable name");
                }
                // `<mangled>__vtbl` names the table; the poll beside it is
                // `<mangled>__poll`, and that body is this future.
                std::string poll_name(table.substr(0, table.size() - 6));
                poll_name += "__poll";
                const SymbolId poll_sym = symbol_for_name(poll_name);
                if (poll_sym == MIR_INVALID_ID) return fail_at("async ramp has no poll body");
                std::string frame_addr;
                if (!base_addr_expr(*frame, frame_addr)) return fail_at("bad async frame");
                // The sret operand is the place holding the hidden destination
                // pointer, not the pointer itself.
                const std::string handle_addr =
                    "rt.ld(" + pname(sret->id) + ", " + std::to_string(ptr_size_) + ", 0)";
                // The ramp hands the future over and is done: without the return
                // the case would fall through into the dispatch loop's default.
                out = "rt.promise_put(" + handle_addr + ", " + js_function_name(poll_sym) +
                      "(0, " + frame_addr + ", 0)); rt.frame_release($m); return;";
                return true;
            }
            TypeId result_type = MIR_INVALID_ID;
            uint32_t offset = 0;
            const TypeId frame_ptr_type = operand_type(*frame);
            const TypeId frame_type = pointee_of(frame_ptr_type) != MIR_INVALID_ID
                                          ? pointee_of(frame_ptr_type)
                                          : frame_ptr_type;
            if (!resolve_field(frame_type, "__result", result_type, offset)) {
                return fail_at("async frame has no __result field");
            }
            const JsType t = type_of(result_type);
            if (t.is_agg) {
                return fail_at(
                    "the JS backend's promise lowering does not support an aggregate async result yet");
            }
            std::string addr;
            if (!base_addr_expr(*frame, addr)) return fail_at("bad async frame");
            out = "return rt.ld(" + addr + " + " + std::to_string(offset) + ", " +
                  std::to_string(t.size) + ", " + (t.is_signed ? "1" : "0") + ");";
            return true;
        }
        case MIROpcode::AsyncFinish: {
            // operands: Place(frame), Place(ret), Type(poll_base)
            //
            // The poll reports `Ready(frame->__result)` by storing it into `ret`.
            // As an `async` arrow it simply resolves with that value — the caller
            // is awaiting, not polling.
            const MIROperand* frame = op(0);
            if (frame == nullptr) return fail_at("async finish without a frame");
            TypeId result_type = MIR_INVALID_ID;
            uint32_t offset = 0;
            const TypeId frame_ptr_type = operand_type(*frame);
            const TypeId frame_type = pointee_of(frame_ptr_type) != MIR_INVALID_ID
                                          ? pointee_of(frame_ptr_type)
                                          : frame_ptr_type;
            if (!resolve_field(frame_type, "__result", result_type, offset)) {
                return fail_at("async frame has no __result field");
            }
            const JsType t = type_of(result_type);
            if (t.is_agg) {
                return fail_at(
                    "the JS backend's promise lowering does not support an aggregate async result yet");
            }
            std::string addr;
            if (!base_addr_expr(*frame, addr)) return fail_at("bad async frame");
            out = "return rt.ld(" + addr + " + " + std::to_string(offset) + ", " +
                  std::to_string(t.size) + ", " + (t.is_signed ? "1" : "0") + ");";
            return true;
        }
        case MIROpcode::Unreachable:
            out = "throw new Error(\"js backend: unreachable\");";
            return true;
        default:
            return fail_at(std::string("unsupported terminator '") + opcode_name(ins.opcode()) +
                           "'");
    }
}

} // namespace

// ── public API ──────────────────────────────────────────────────────────────

std::string js_function_name(SymbolId symbol) {
    return "$f" + std::to_string(symbol);
}

std::string js_module_file_stem(const std::string& module_name) {
    std::string out;
    out.reserve(module_name.size());
    for (char c : module_name) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '_' || c == '-';
        if (ok) {
            out += c;
        } else if (out.empty() || out.back() != '_') {
            out += '_';
        }
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    if (out.empty()) out = "module";
    return out;
}

bool emit_module_js(const MIRModule& module, std::string& out, std::string& error,
                    const MIREmitJsOptions& options, std::string* source_map) {
    std::vector<const MIRFunction*> functions;
    functions.reserve(module.functions.size());
    for (const MIRFunction& fn : module.functions) functions.push_back(&fn);
    return emit_program_js(module, functions, out, error, options, source_map);
}

bool emit_program_js(const MIRModule& module, const std::vector<const MIRFunction*>& functions,
                     std::string& out, std::string& error, const MIREmitJsOptions& options,
                     std::string* source_map) {
    ModuleEmitter emitter(module, functions, error, options);
    return emitter.run(out, source_map);
}

bool emit_modules_js(const MIRModule& module, const std::vector<const MIRFunction*>& functions,
                     std::vector<JsEmittedModule>& files, std::string& error,
                     const MIREmitJsOptions& options) {
    // Group the reachable functions by the Chemical module that defined them.
    // A symbol with no recorded owner goes to the job's own module, so a program
    // lowered outside a job (a unit test) still emits one file.
    const std::string fallback_label =
        options.module_name.empty() ? std::string("program") : options.module_name;
    const std::string fallback_stem = js_module_file_stem(fallback_label);

    std::vector<std::string> order;
    std::map<std::string, std::vector<const MIRFunction*>> groups;
    std::map<std::string, std::string> label_of_stem;
    std::unordered_map<SymbolId, std::string> symbol_stems;

    auto stem_for = [&](const std::string& label) -> std::string {
        std::string stem = js_module_file_stem(label);
        if (stem.empty()) stem = fallback_stem;
        // Two Chemical module names can sanitize to one stem (`a::b` / `a__b`);
        // give the second a suffix rather than writing two modules to one file.
        std::string candidate = stem;
        for (unsigned n = 2;; ++n) {
            auto it = label_of_stem.find(candidate);
            if (it == label_of_stem.end() || it->second == label) break;
            candidate = stem + "_" + std::to_string(n);
        }
        if (label_of_stem.find(candidate) == label_of_stem.end()) {
            label_of_stem[candidate] = label;
            order.push_back(candidate);
        }
        return candidate;
    };

    for (const MIRFunction* fnp : functions) {
        if (fnp == nullptr) continue;
        const MIRFunction& fn = *fnp;
        const std::string owner(module.module_name_of(module.module_of_symbol(fn.symbol)));
        const std::string stem = stem_for(owner.empty() ? fallback_label : owner);
        symbol_stems[fn.symbol] = stem;
        groups[stem].push_back(fnp);
    }

    // Globals: only the owning module emits storage, so every emitted symbol has
    // to be attributable before any file is printed.
    for (const MIRGlobal& global : module.globals) {
        if (global.symbol == MIR_INVALID_ID) continue;
        if (symbol_stems.count(global.symbol)) continue;
        const std::string owner(module.module_name_of(module.module_of_symbol(global.symbol)));
        symbol_stems[global.symbol] = stem_for(owner.empty() ? fallback_label : owner);
    }

    // Vtables: a table belongs to the module that owns its implementation (the
    // first slot's callee), so exactly one file allocates it and the callers
    // import `$g<sym>`. Without this a table would be emitted by every module.
    if (!module.vtables.defs.empty()) {
        std::unordered_map<std::string, SymbolId> by_name;
        for (SymbolId s = 0; s < module.symbols.size(); ++s) {
            by_name.emplace(module.symbols.mangled_name(module.symbols.get(s)), s);
        }
        for (const MIRVtableRecord& def : module.vtables.defs) {
            auto self = by_name.find(def.name);
            if (self == by_name.end()) continue;
            if (symbol_stems.count(self->second)) continue;
            std::string owner;
            for (const MIRVtableSlot& slot : def.slots) {
                if (slot.target.empty()) continue;
                auto callee = by_name.find(slot.target);
                if (callee == by_name.end()) continue;
                auto stem = symbol_stems.find(callee->second);
                if (stem != symbol_stems.end()) {
                    owner = stem->second;
                    break;
                }
            }
            const std::string stem = owner.empty() ? fallback_stem : owner;
            symbol_stems[self->second] = stem;
            groups[stem]; // a vtable-only module still gets a file
            if (label_of_stem.find(stem) == label_of_stem.end()) {
                label_of_stem[stem] = stem;
                order.push_back(stem);
            }
        }
    }

    for (const std::string& stem : order) {
        auto group = groups.find(stem);
        if (group == groups.end()) {
            // A module that only defines globals still gets a file: its storage
            // has to live somewhere.
            static const std::vector<const MIRFunction*> empty;
            group = groups.emplace(stem, empty).first;
        }
        MIREmitJsOptions file_options = options;
        file_options.module_name = label_of_stem[stem];
        std::map<std::string, std::vector<std::string>> imports;
        ModuleEmitter emitter(module, group->second, error, file_options);
        emitter.set_split(stem, &symbol_stems, &imports);
        std::string source;
        std::string map;
        if (!emitter.run(source, &map)) {
            error = "js backend: module '" + file_options.module_name + "': " + error;
            return false;
        }
        JsEmittedModule out;
        out.stem = stem;
        out.module_name = file_options.module_name;
        out.source = std::move(source);
        out.source_map = std::move(map);
        files.push_back(std::move(out));
    }
    if (files.empty()) {
        error = "js backend: no function to emit";
        return false;
    }
    return true;
}

// ── reachability ────────────────────────────────────────────────────────────

namespace {

void collect_global_function_refs(const MIRModule& module, ConstExprId id,
                                  std::vector<std::string>& work) {
    if (id == MIR_INVALID_ID || id >= module.const_exprs.size()) return;
    const MIRConstExpr& e = module.const_exprs.get(id);
    if (e.kind == MIRConstKind::FunctionRef) {
        if (e.symbol != MIR_INVALID_ID && e.symbol < module.symbols.size()) {
            work.push_back(module.symbols.mangled_name(module.symbols.get(e.symbol)));
        }
        return;
    }
    for (uint32_t i = 0; i < e.child_count; ++i) {
        const MIRConstChild* child = module.const_exprs.child(id, i);
        if (child != nullptr) collect_global_function_refs(module, child->expr, work);
    }
}

} // namespace

std::vector<const MIRFunction*> reachable_functions(const MIRModule& module,
                                                    std::string_view entry) {
    std::unordered_map<std::string, const MIRFunction*> by_name;
    for (const MIRFunction& fn : module.functions) {
        if (fn.symbol == MIR_INVALID_ID || fn.symbol >= module.symbols.size()) continue;
        if (fn.instructions.size() == 0) continue;
        by_name.emplace(module.symbols.mangled_name(module.symbols.get(fn.symbol)), &fn);
    }

    std::vector<const MIRFunction*> out;
    std::unordered_set<std::string> visited;
    std::vector<std::string> work;
    work.emplace_back(entry);

    // A global's initializer can hold a function address, which makes that
    // function reachable even though no instruction names it.
    for (const MIRGlobal& global : module.globals) {
        collect_global_function_refs(module, global.init, work);
    }

    // An interface method is only ever named by a vtable slot: the call site says
    // `dynamic_call`, not the callee. So a `GlobalAddr` of a vtable global (the
    // `dyn I` conversion taking `&vtable`) makes that table's slots reachable.
    // Only referenced tables count: seeding from *every* definition would drag a
    // whole library's interface hierarchy into a program that uses none of it.
    std::unordered_map<std::string, std::vector<std::string>> vtable_targets;
    if (!module.vtables.defs.empty()) {
        for (const MIRVtableRecord& def : module.vtables.defs) {
            std::vector<std::string> targets;
            for (const MIRVtableSlot& slot : def.slots) {
                if (!slot.target.empty()) targets.push_back(slot.target);
            }
            if (!targets.empty()) vtable_targets.emplace(def.name, std::move(targets));
        }
    }

    while (!work.empty()) {
        std::string name = std::move(work.back());
        work.pop_back();
        if (name.empty() || !visited.insert(name).second) continue;
        auto it = by_name.find(name);
        if (it == by_name.end()) continue;
        const MIRFunction* fn = it->second;
        out.push_back(fn);

        for (const MIRInstruction& ins : fn->instructions) {
            switch (ins.opcode()) {
                case MIROpcode::Call:
                case MIROpcode::CallIndirect:
                case MIROpcode::Init:
                case MIROpcode::Drop:
                case MIROpcode::Destroy:
                case MIROpcode::GlobalAddr:
                case MIROpcode::FunctionAddr: {
                    // Any symbol operand is a reference: `Call` puts the callee
                    // first, but `Init`/`Drop`/`Destroy` put the destination place
                    // first and the ctor/dtor symbol second.
                    for (uint32_t i = 0; i < ins.operand_count; ++i) {
                        const MIROperand& op = fn->operands[ins.operand_offset + i];
                        if (op.kind() != MIROperandKind::Symbol) continue;
                        if (op.id >= module.symbols.size()) continue;
                        const std::string name = module.symbols.mangled_name(module.symbols.get(op.id));
                        work.push_back(name);
                        // Taking a vtable's address makes its slot methods
                        // reachable — they have no call site naming them.
                        auto vt = vtable_targets.find(name);
                        if (vt != vtable_targets.end()) {
                            for (const std::string& target : vt->second) work.push_back(target);
                        }
                    }
                    break;
                }
                case MIROpcode::AsyncRampFinish: {
                    // An async function's body can live in its poll, which no
                    // instruction calls by name. The ramp's vtable constant names
                    // the pair (`<mangled>__vtbl` / `<mangled>__poll`), so that is
                    // what makes the poll reachable.
                    const MIROperand* vt = nullptr;
                    if (ins.operand_count > 3) vt = &fn->operands[ins.operand_offset + 3];
                    std::string_view table;
                    if (vt != nullptr && vt->kind() == MIROperandKind::Constant &&
                        constant_text(module, vt->id, table) && table.size() > 6) {
                        std::string poll_name(table.substr(0, table.size() - 6));
                        poll_name += "__poll";
                        work.push_back(poll_name);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
    return out;
}

} // namespace mir
