// Copyright (c) Chemical Language Foundation 2025.
//
// MIR declaration printer — see MIRDeclPrinter.h.

#include "MIRDeclPrinter.h"

#include "MIREmitter.h"

#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace mir {

namespace {

const char* k_variant_tag_name = "__chx__vt_621827";

void append_indent(std::string& out, int indent) {
    out += '\n';
    for (int i = 0; i < indent; ++i) out += '\t';
}

// ── scalar formatting (mirrors preprocess/2c/BufferedWriter) ────────────────

/** Reproduces `BufferedWriter::append_float` exactly (to_chars general/9, `.0`, `f`). */
std::string format_float(float v) {
    char buf[64];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::general, 9);
    if (ec != std::errc()) return "NaN";
    std::string out(buf, ptr);
    if (std::isfinite(v) && v == std::trunc(v) && std::abs(v) < 1e9f) out += ".0";
    out += 'f';
    return out;
}

/** Reproduces `BufferedWriter::append_double` exactly (to_chars general/17, `.0`). */
std::string format_double(double v) {
    char buf[80];
    auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), v, std::chars_format::general, 17);
    if (ec != std::errc()) return "NaN";
    std::string out(buf, ptr);
    if (std::isfinite(v) && v == std::trunc(v) && std::abs(v) < 1e17) out += ".0";
    return out;
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
 * One member declarator at `indent`, including the trailing `;`. `indent` is
 * the level of the member itself: an anonymous member nests its inner fields at
 * `indent + 1` and closes at `indent`, matching the member walk this replaces.
 */
std::string member_declarator_at(const MIRModule& module, const MIRFieldInfo& field,
                                 int indent) {
    const std::string name = module.types.field_name_of(field);

    // an anonymous member keeps its inline `struct { ... } name;` spelling —
    // the synthesized name MIR gives that type is not what the source wrote
    if (field.flags & (MFF_ANONYMOUS_STRUCT | MFF_ANONYMOUS_UNION)) {
        const bool as_struct = (field.flags & MFF_ANONYMOUS_STRUCT) != 0;
        std::string out = as_struct ? "struct {" : "union {";
        field_range inner = fields_of(module, field.type);
        while (const MIRFieldInfo* f = inner.next()) {
            append_indent(out, indent + 1);
            out += member_declarator_at(module, *f, indent + 1);
        }
        append_indent(out, indent);
        out += "} " + name + ";";
        return out;
    }

    std::string out;
    if (field.align > 0) out += "__chx_align(" + std::to_string(field.align) + ") ";
    if (field.flags & MFF_VOLATILE) out += "volatile ";
    if (field.flags & MFF_FAT_POINTER) {
        // a capturing-function member is a pointer to the fat pointer, not the
        // fat pointer itself (the AST spelling the declarations must match)
        out += "__chemical_fat_pointer__* " + name + ";";
        return out;
    }
    out += c_declarator(module, field.type, name) + ";";
    return out;
}

} // namespace

std::string const_expr_c(const MIRModule& module, ConstExprId id) {
    if (id == MIR_INVALID_ID || id >= module.const_exprs.size()) return "0";
    const MIRConstExpr& e = module.const_exprs.get(id);
    switch (e.kind) {
        case MIRConstKind::Zero:
            return "0";
        case MIRConstKind::Int: {
            bool unsig = (e.flags & MCF_UNSIGNED) != 0;
            if (!unsig && e.type < module.types.size()) {
                const MIRTypeRecord& r = module.types.get(e.type);
                if (r.kind == MIRTypeKind::Int && !(r.flags & TF_SIGNED)) unsig = true;
            }
            return unsig ? std::to_string(static_cast<unsigned long long>(e.bits))
                         : std::to_string(static_cast<long long>(e.bits));
        }
        case MIRConstKind::Float: {
            const uint32_t b = static_cast<uint32_t>(e.bits);
            float f = 0.0f;
            std::memcpy(&f, &b, sizeof(f));
            return format_float(f);
        }
        case MIRConstKind::Double: {
            double d = 0.0;
            std::memcpy(&d, &e.bits, sizeof(d));
            return format_double(d);
        }
        case MIRConstKind::Bool:
            return e.bits ? "1" : "0";
        case MIRConstKind::Null:
            return "NULL";
        case MIRConstKind::String: {
            std::string out;
            append_escaped_string(out, module.const_exprs.data.data() + e.data_offset,
                                  e.data_count);
            return out;
        }
        case MIRConstKind::Negate: {
            const MIRConstChild* c = module.const_exprs.child(id, 0);
            return "-" + const_expr_c(module, c ? c->expr : MIR_INVALID_ID);
        }
        case MIRConstKind::FunctionRef: {
            if (e.symbol >= module.symbols.size()) return "0";
            return module.symbols.mangled_name(module.symbols.get(e.symbol));
        }
        case MIRConstKind::Aggregate: {
            const bool is_array = e.type < module.types.size() &&
                                  module.types.get(e.type).kind == MIRTypeKind::Array;
            std::string out;
            if (e.flags & MCF_CAST_PREFIX) out += "(" + c_type_of(module, e.type) + ") ";
            out += '{';
            bool first = true;
            for (uint32_t i = 0; i < e.child_count; ++i) {
                const MIRConstChild* c = module.const_exprs.child(id, i);
                if (!c) break;
                if (!first) out += ", ";
                if (!is_array) out += "." + module.const_exprs.child_name(*c) + " = ";
                out += const_expr_c(module, c->expr);
                first = false;
            }
            if (first) out += '0'; // an empty array literal is `{0}`
            out += '}';
            return out;
        }
        case MIRConstKind::Variant: {
            std::string out = "(" + c_type_of(module, e.type) + ") { ." +
                              k_variant_tag_name + " = " + std::to_string(e.case_index);
            for (uint32_t i = 0; i < e.child_count; ++i) {
                const MIRConstChild* c = module.const_exprs.child(id, i);
                if (!c) break;
                out += ", ." + module.const_exprs.child_name(*c) + " = " +
                       const_expr_c(module, c->expr);
            }
            out += " }";
            return out;
        }
    }
    return "0";
}

std::string member_declarator_c(const MIRModule& module, const MIRFieldInfo& field) {
    return member_declarator_at(module, field, 1);
}

std::string aggregate_body_c(const MIRModule& module, TypeId id) {
    if (id == MIR_INVALID_ID || id >= module.types.size()) return {};
    const MIRTypeRecord& rec = module.types.get(id);
    const std::string name = module.types.name_of(id);
    if (name.empty()) return {};

    switch (rec.kind) {
        case MIRTypeKind::Struct: {
            std::string out = "\n";
            if (rec.explicit_alignment > 0) {
                out += "__chx_align_struct(" + std::to_string(rec.explicit_alignment) + ") " + name;
            } else {
                out += "struct " + name;
            }
            out += " {";
            field_range fs = fields_of(module, id);
            while (const MIRFieldInfo* f = fs.next()) {
                append_indent(out, 1);
                out += member_declarator_c(module, *f);
            }
            out += "\n};";
            return out;
        }
        case MIRTypeKind::Union: {
            std::string out = "\nunion " + name + " {";
            field_range fs = fields_of(module, id);
            while (const MIRFieldInfo* f = fs.next()) {
                append_indent(out, 1);
                out += member_declarator_c(module, *f);
            }
            out += "\n};";
            return out;
        }
        case MIRTypeKind::Variant: {
            std::string out = "\nstruct " + name + " {";
            field_range fs = fields_of(module, id);
            const MIRFieldInfo* f = nullptr;
            while ((f = fs.next())) {
                if (!is_base(*f)) break;
                append_indent(out, 1);
                out += member_declarator_c(module, *f);
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
                    out += c_declarator(module, p->type, module.types.field_name_of(*p)) + ";";
                }
                out += "\n\t\t} " + module.types.field_name_of(*f) + ";";
            }
            out += "\n\t};\n};";
            return out;
        }
        default:
            return {};
    }
}

std::string inline_aggregate_c(const MIRModule& module, TypeId id) {
    if (id == MIR_INVALID_ID || id >= module.types.size()) return {};
    const MIRTypeRecord& rec = module.types.get(id);
    const std::string name = module.types.name_of(id);
    if (name.empty()) return {};
    if (rec.kind != MIRTypeKind::Struct && rec.kind != MIRTypeKind::Union) return {};

    std::string out = (rec.kind == MIRTypeKind::Struct ? "struct " : "union ") + name + " { ";
    field_range fs = fields_of(module, id);
    while (const MIRFieldInfo* f = fs.next()) {
        if (is_base(*f)) continue;
        out += member_declarator_c(module, *f);
        out += ' ';
    }
    out += "};";
    return out;
}

} // namespace mir
