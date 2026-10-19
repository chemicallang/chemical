// Copyright (c) Chemical Language Foundation 2025.
//
// MIR declaration printer — see MIRDeclPrinter.h.
//
// Everything here prints into the caller's buffer. The public functions return
// a `std::string` because the declaration path stores/compares whole
// declarations, but the recursive parts (a member, a constant tree) append, so
// printing a struct with N fields or a table initializer with N entries no
// longer allocates a throwaway `std::string` per element.

#include "MIRDeclPrinter.h"

#include "MIREmitter.h"
#include "MIRMangleBuffer.h"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

namespace mir {

namespace {

const char* k_variant_tag_name = "__chx__vt_621827";

void append_indent(std::string& out, int indent) {
    out += '\n';
    for (int i = 0; i < indent; ++i) out += '\t';
}

// ── scalar formatting (mirrors preprocess/2c/BufferedWriter) ────────────────

void append_int64(std::string& out, int64_t v) {
    if (v < 0) {
        out += '-';
        // `-(v + 1) + 1` so INT64_MIN does not overflow
        append_uint(out, static_cast<uint64_t>(-(v + 1)) + 1);
        return;
    }
    append_uint(out, static_cast<uint64_t>(v));
}

/** Reproduces `BufferedWriter::append_float` exactly (to_chars general/9, `.0`, `f`). */
void append_float(std::string& out, float v) {
    char buf[64];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::general, 9);
    if (ec != std::errc()) { out += "NaN"; return; }
    out.append(buf, static_cast<size_t>(ptr - buf));
    if (std::isfinite(v) && v == std::trunc(v) && std::abs(v) < 1e9f) out += ".0";
    out += 'f';
}

/** Reproduces `BufferedWriter::append_double` exactly (to_chars general/17, `.0`). */
void append_double(std::string& out, double v) {
    char buf[80];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::general, 17);
    if (ec != std::errc()) { out += "NaN"; return; }
    out.append(buf, static_cast<size_t>(ptr - buf));
    if (std::isfinite(v) && v == std::trunc(v) && std::abs(v) < 1e17) out += ".0";
}

/** Mirrors `MIRDeclEmitter::write_escaped_string` (the C string-literal form). */
void append_escaped_string(std::string& out, const char* s, uint32_t n) {
    out += '"';
    for (uint32_t i = 0; i < n; ++i) {
        const char c = s[i];
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\%03o",
                                  static_cast<unsigned>(static_cast<unsigned char>(c)));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    out += '"';
}

bool is_base(const MIRFieldInfo& f) {
    return f.kind == static_cast<uint8_t>(MIRFieldKind::Base);
}

/** The fields of an aggregate, in declaration order. */
struct field_range {
    const MIRTypeTable* types = nullptr;
    TypeId aggregate = MIR_INVALID_ID;
    uint32_t count = 0;
    uint32_t cursor = 0;

    const MIRFieldInfo* next() {
        while (cursor < count) {
            const MIRFieldInfo* f = types->field_of(aggregate, cursor++);
            if (f) return f;
        }
        return nullptr;
    }
};

field_range fields_of(const MIRModule& module, TypeId id) {
    field_range r;
    if (id == MIR_INVALID_ID || id >= module.types.size()) return r;
    r.types = &module.types;
    r.aggregate = id;
    r.count = module.types.get(id).data_count;
    return r;
}

/**
 * One member declarator at `indent`, including the trailing `;`, appended to
 * `out`. `indent` is the level of the member itself: an anonymous member nests
 * its inner fields at `indent + 1` and closes at `indent`, matching the member
 * walk this replaces.
 */
void append_member_declarator(const MIRModule& module, const MIRFieldInfo& field, int indent,
                              std::string& out) {
    const std::string_view name = module.types.field_name_view(field);

    // an anonymous member keeps its inline `struct { ... } name;` spelling —
    // the synthesized name MIR gives that type is not what the source wrote
    if (field.flags & (MFF_ANONYMOUS_STRUCT | MFF_ANONYMOUS_UNION)) {
        const bool as_struct = (field.flags & MFF_ANONYMOUS_STRUCT) != 0;
        out += as_struct ? "struct {" : "union {";
        field_range inner = fields_of(module, field.type);
        while (const MIRFieldInfo* f = inner.next()) {
            append_indent(out, indent + 1);
            append_member_declarator(module, *f, indent + 1, out);
        }
        append_indent(out, indent);
        out += "} ";
        out += name;
        out += ";";
        return;
    }

    if (field.align > 0) {
        out += "__chx_align(";
        append_uint(out, field.align);
        out += ") ";
    }
    if (field.flags & MFF_VOLATILE) out += "volatile ";
    if (field.flags & MFF_FAT_POINTER) {
        // a capturing-function member is a pointer to the fat pointer, not the
        // fat pointer itself (the AST spelling the declarations must match)
        out += "__chemical_fat_pointer__* ";
        out += name;
        out += ";";
        return;
    }
    append_c_declarator(module, field.type, name, out);
    out += ";";
}

void append_const_expr_c(const MIRModule& module, ConstExprId id, std::string& out) {
    if (id == MIR_INVALID_ID || id >= module.const_exprs.size()) {
        out += '0';
        return;
    }
    const MIRConstExpr& e = module.const_exprs.get(id);
    switch (e.kind) {
        case MIRConstKind::Zero:
            out += '0';
            return;
        case MIRConstKind::Int: {
            bool unsig = (e.flags & MCF_UNSIGNED) != 0;
            if (!unsig && e.type < module.types.size()) {
                const MIRTypeRecord& r = module.types.get(e.type);
                if (r.kind == MIRTypeKind::Int && !(r.flags & TF_SIGNED)) unsig = true;
            }
            if (unsig) append_uint(out, static_cast<uint64_t>(e.bits));
            else append_int64(out, static_cast<int64_t>(e.bits));
            return;
        }
        case MIRConstKind::Float: {
            const uint32_t b = static_cast<uint32_t>(e.bits);
            float f = 0.0f;
            std::memcpy(&f, &b, sizeof(f));
            append_float(out, f);
            return;
        }
        case MIRConstKind::Double: {
            double d = 0.0;
            std::memcpy(&d, &e.bits, sizeof(d));
            append_double(out, d);
            return;
        }
        case MIRConstKind::Bool:
            out += e.bits ? '1' : '0';
            return;
        case MIRConstKind::Null:
            out += "NULL";
            return;
        case MIRConstKind::String:
            append_escaped_string(out, module.const_exprs.data.data_at(e.data_offset),
                                  e.data_count);
            return;
        case MIRConstKind::Negate: {
            const MIRConstChild* c = module.const_exprs.child(id, 0);
            out += '-';
            append_const_expr_c(module, c ? c->expr : MIR_INVALID_ID, out);
            return;
        }
        case MIRConstKind::FunctionRef: {
            if (e.symbol >= module.symbols.size()) { out += '0'; return; }
            out += module.symbols.mangled_view(module.symbols.get(e.symbol));
            return;
        }
        case MIRConstKind::Aggregate: {
            const bool is_array = e.type < module.types.size() &&
                                  module.types.get(e.type).kind == MIRTypeKind::Array;
            if (e.flags & MCF_CAST_PREFIX) {
                out += '(';
                append_c_type_of(module, e.type, out);
                out += ") ";
            }
            out += '{';
            bool first = true;
            for (uint32_t i = 0; i < e.child_count; ++i) {
                const MIRConstChild* c = module.const_exprs.child(id, i);
                if (!c) break;
                if (!first) out += ", ";
                if (!is_array) {
                    out += '.';
                    out += module.const_exprs.child_name_view(*c);
                    out += " = ";
                }
                append_const_expr_c(module, c->expr, out);
                first = false;
            }
            if (first) out += '0'; // an empty array literal is `{0}`
            out += '}';
            return;
        }
        case MIRConstKind::Variant: {
            out += '(';
            append_c_type_of(module, e.type, out);
            out += ") { .";
            out += k_variant_tag_name;
            out += " = ";
            append_uint(out, e.case_index);
            for (uint32_t i = 0; i < e.child_count; ++i) {
                const MIRConstChild* c = module.const_exprs.child(id, i);
                if (!c) break;
                out += ", .";
                out += module.const_exprs.child_name_view(*c);
                out += " = ";
                append_const_expr_c(module, c->expr, out);
            }
            out += " }";
            return;
        }
    }
    out += '0';
}

void append_global_decl_spelling(const MIRModule& module, const MIRGlobalDecl& d,
                                 std::string& out) {
    if (d.unsized_array) {
        // The brackets belong to the declarator, so the name (with `[]`
        // appended) is spelled once into `decl_scratch(0)` instead of a
        // `name + "[]"` temporary — entered at depth 1 so the nested
        // declarators of `d.elem` cannot clobber the scratch the name lives in.
        std::string& buffered = decl_scratch(0);
        buffered += d.name;
        buffered += "[]";
        append_c_declarator_scratched(module, d.elem, buffered, out);
        return;
    }
    if (d.type == MIR_INVALID_ID) {
        out += "void ";
        out += d.name;
        return;
    }
    append_c_declarator(module, d.type, d.name, out);
}

} // namespace

std::string const_expr_c(const MIRModule& module, ConstExprId id) {
    std::string out;
    append_const_expr_c(module, id, out);
    return out;
}

/** The C declarator of a global declaration (see MIRGlobalDecl.h). */
std::string global_decl_spelling(const MIRModule& module, const MIRGlobalDecl& d) {
    std::string out;
    append_global_decl_spelling(module, d, out);
    return out;
}

std::string vtable_def_c(const MIRModule& module, const MIRVtableRecord& rec) {
    // A container impl (kind 1) casts each slot to its self-erased function
    // type; a primitive impl (kind 2) leaves the callee implicit and writes `0`
    // for a slot with no overrider. Both are exactly what the declaration pass
    // wrote member by member.
    const bool cast = rec.kind == 1;
    std::string out;
    out += '\n';
    out += "const ";
    out += rec.type_name;
    out += ' ';
    out += rec.name;
    out += " = {";
    for (const MIRVtableSlot& slot : rec.slots) {
        out += '\n';
        if (cast) {
            out += '(';
            append_c_decl_type(module, slot.type, std::string_view(), out);
            out += ") ";
            if (!slot.target.empty()) out += slot.target;
            else out += "__chx_unresolved";
        } else {
            if (!slot.target.empty()) out += slot.target;
            else out += '0';
        }
        out += ',';
    }
    out += "\n};";
    return out;
}

std::string decl_list_c(const MIRModule& module, const MIRDeclList& list) {
    std::string out;
    out.reserve(list.items.size() * 48 + 64);
    for (const MIRDeclItem& it : list.items) {
        switch (it.kind) {
            case MIRDeclKind::ForwardStruct:
                out += "\nstruct ";
                out += it.name;
                out += ';';
                break;
            case MIRDeclKind::ForwardUnion:
                out += "\nunion ";
                out += it.name;
                out += ';';
                break;
            case MIRDeclKind::AggregateBody: {
                // the body is a whole C definition; print it into `out`
                const size_t before = out.size();
                append_aggregate_body_c(module, it.type, out);
                if (out.size() == before) {
                    out += "\n/* MIR declaration: unmapped or unnamed ";
                    out += it.what;
                    out += " - its C body could not be printed */";
                }
                break;
            }
            case MIRDeclKind::Typedef:
                out += "\ntypedef ";
                append_global_decl_spelling(
                    module, MIRGlobalDecl{it.name, it.type, it.elem, it.unsized_array}, out);
                out += ';';
                break;
            case MIRDeclKind::Prototype:
                out += '\n';
                if (it.proto.flags & MPF_EXTERN) out += "extern ";
                if (it.proto.flags & MPF_STATIC) out += "static ";
                if (it.proto.flags & MPF_DLLIMPORT) out += "__chem_dllimport ";
                append_c_function_proto(module, it.proto.type, it.proto.name, out);
                out += ';';
                break;
            case MIRDeclKind::Global:
                out += '\n';
                if (it.global.flags & MGD_THREAD_LOCAL) out += "__chx_thread_local ";
                if (it.global.flags & MGD_STATIC) out += "static ";
                if (it.global.flags & MGD_VOLATILE) out += "volatile ";
                append_global_decl_spelling(module, it.global, out);
                out += ';';
                break;
            case MIRDeclKind::VtableType:
                out += "\ntypedef struct ";
                out += it.name;
                out += " {";
                for (const MIRVtableSlot& slot : it.slots) {
                    out += '\n';
                    append_c_decl_type(module, slot.type, slot.name, out);
                    out += ';';
                }
                out += "\n} ";
                out += it.name;
                out += ';';
                break;
            case MIRDeclKind::VtableExtern:
                out += "\nextern const ";
                out += it.type_name;
                out += ' ';
                out += it.name;
                out += ';';
                break;
        }
    }
    return out;
}

std::string member_declarator_c(const MIRModule& module, const MIRFieldInfo& field) {
    std::string out;
    append_member_declarator(module, field, 1, out);
    return out;
}

void append_aggregate_body_c(const MIRModule& module, TypeId id, std::string& out) {
    if (id == MIR_INVALID_ID || id >= module.types.size()) return;
    const MIRTypeRecord& rec = module.types.get(id);
    const std::string_view name = module.types.name_view(id);
    if (name.empty()) return;

    switch (rec.kind) {
        case MIRTypeKind::Struct: {
            out += '\n';
            if (rec.explicit_alignment > 0) {
                out += "__chx_align_struct(";
                append_uint(out, rec.explicit_alignment);
                out += ") ";
            } else {
                out += "struct ";
            }
            out += name;
            out += " {";
            field_range fs = fields_of(module, id);
            while (const MIRFieldInfo* f = fs.next()) {
                append_indent(out, 1);
                append_member_declarator(module, *f, 1, out);
            }
            out += "\n};";
            return;
        }
        case MIRTypeKind::Union: {
            out += "\nunion ";
            out += name;
            out += " {";
            field_range fs = fields_of(module, id);
            while (const MIRFieldInfo* f = fs.next()) {
                append_indent(out, 1);
                append_member_declarator(module, *f, 1, out);
            }
            out += "\n};";
            return;
        }
        case MIRTypeKind::Variant: {
            out += "\nstruct ";
            out += name;
            out += " {";
            field_range fs = fields_of(module, id);
            const MIRFieldInfo* f = nullptr;
            while ((f = fs.next())) {
                if (!is_base(*f)) break;
                append_indent(out, 1);
                append_member_declarator(module, *f, 1, out);
            }
            out += "\n\tint ";
            out += k_variant_tag_name;
            out += ";";
            out += "\n\tunion {";
            // the first non-base field is the tag; the cases follow it
            for (; f; f = fs.next()) {
                if (f->kind != static_cast<uint8_t>(MIRFieldKind::VariantCase)) continue;
                out += "\n\t\tstruct {";
                field_range payload = fields_of(module, f->type);
                while (const MIRFieldInfo* p = payload.next()) {
                    // two tabs, not three: the walk this replaces wrote
                    // `append("\n\t\t\t", 3)`, i.e. the length argument cut the
                    // third tab off. Kept so the declaration text is identical.
                    append_indent(out, 2);
                    append_c_declarator(module, p->type, module.types.field_name_view(*p), out);
                    out += ";";
                }
                out += "\n\t\t} ";
                out += module.types.field_name_view(*f);
                out += ";";
            }
            out += "\n\t};\n};";
            return;
        }
        default:
            return;
    }
}

std::string aggregate_body_c(const MIRModule& module, TypeId id) {
    std::string out;
    append_aggregate_body_c(module, id, out);
    return out;
}

void append_inline_aggregate_c(const MIRModule& module, TypeId id, std::string& out) {
    if (id == MIR_INVALID_ID || id >= module.types.size()) return;
    const MIRTypeRecord& rec = module.types.get(id);
    const std::string_view name = module.types.name_view(id);
    if (name.empty()) return;
    if (rec.kind != MIRTypeKind::Struct && rec.kind != MIRTypeKind::Union) return;

    out += (rec.kind == MIRTypeKind::Struct ? "struct " : "union ");
    out += name;
    out += " { ";
    field_range fs = fields_of(module, id);
    while (const MIRFieldInfo* f = fs.next()) {
        if (is_base(*f)) continue;
        append_member_declarator(module, *f, 1, out);
        out += ' ';
    }
    out += "};";
}

std::string inline_aggregate_c(const MIRModule& module, TypeId id) {
    std::string out;
    append_inline_aggregate_c(module, id, out);
    return out;
}

} // namespace mir
