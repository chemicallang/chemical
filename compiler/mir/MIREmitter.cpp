// Copyright (c) Chemical Language Foundation 2025.

#include "MIREmitter.h"

#include "MIRFacts.h"
#include "MIRMangleBuffer.h"
#include "MIRTraversal.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace mir {

namespace {

// Format a floating-point literal with the shortest precision that still
// round-trips exactly (a plain "%g" truncates to 6 significant digits, which
// silently corrupts literals such as 3.14159265).
std::string format_float_literal(double value, int min_prec, int max_prec,
                                 const char* suffix) {
    char buf[64];
    for (int prec = min_prec; prec <= max_prec; ++prec) {
        std::snprintf(buf, sizeof(buf), "%.*g", prec, value);
        if (std::strtod(buf, nullptr) == value) break;
    }
    std::string s(buf);
    if (s.find_first_of(".eE") == std::string::npos) s += ".0";
    s += suffix;
    return s;
}

std::string c_int_type(uint32_t size, bool is_signed) {
    switch (size) {
        case 1: return is_signed ? "int8_t" : "uint8_t";
        case 2: return is_signed ? "int16_t" : "uint16_t";
        case 8: return is_signed ? "int64_t" : "uint64_t";
        case 4: default: return is_signed ? "int32_t" : "uint32_t";
    }
}

const char* c_int_kind_spelling(uint8_t k) {
    switch (k) {
        case IK_I8: return "int8_t";
        case IK_I16: return "int16_t";
        case IK_I32: return "int32_t";
        case IK_I64: return "int64_t";
        case IK_I128: return "__int128";
        case IK_U8: return "uint8_t";
        case IK_U16: return "uint16_t";
        case IK_U32: return "uint32_t";
        case IK_U64: return "uint64_t";
        case IK_U128: return "unsigned __int128";
        case IK_CHAR: return "char";
        case IK_SHORT: return "short";
        case IK_INT: return "int";
        case IK_LONG: return "long";
        case IK_LONGLONG: return "long long";
        case IK_UCHAR: return "unsigned char";
        case IK_USHORT: return "unsigned short";
        case IK_UINT: return "unsigned int";
        case IK_ULONG: return "unsigned long";
        case IK_ULONGLONG: return "unsigned long long";
        default: return nullptr;
    }
}

inline void append_vname(ValueId id, std::string& out) {
    out += "__chx_v";
    append_uint(out, id);
}
std::string vname(ValueId id) {
    std::string s;
    append_vname(id, s);
    return s;
}

// `"__chx_a<i>"` in a stack buffer: the C name of the i-th parameter, without a
// `"__chx_a" + std::to_string(i)` temporary per parameter of every prototype,
// declaration and function header.
std::string_view param_arg_name(char (&buf)[24], uint32_t i) {
    char* p = buf;
    std::memcpy(p, "__chx_a", 7);
    p += 7;
    char digits[10];
    int n = 0;
    do {
        digits[n++] = static_cast<char>('0' + (i % 10));
        i /= 10;
    } while (i);
    while (n) *p++ = digits[--n];
    return std::string_view(buf, static_cast<size_t>(p - buf));
}

// The declarator of a value (`__chx_v<id>`) appended straight into `out`. The
// scalar/pointer shape — every instruction's temp in practice — writes the name
// itself, so no `vname` temporary is built per emitted instruction; only the
// function-pointer / array declarator shapes (which need the name in the middle
// of the spelling) fall back to one.
void append_vname_declarator(const MIRModule& module, TypeId type, ValueId id,
                             std::string& out) {
    bool simple = type == MIR_INVALID_ID || type >= module.types.size();
    if (!simple) {
        const MIRTypeRecord& r = module.types.get(type);
        simple = r.kind != MIRTypeKind::Function && r.kind != MIRTypeKind::Array;
        if (simple && r.kind == MIRTypeKind::Pointer && r.element != MIR_INVALID_ID &&
            r.element < module.types.size() &&
            module.types.get(r.element).kind == MIRTypeKind::Array) {
            simple = false;
        }
    }
    if (simple) {
        append_c_type_of(module, type, out);
        out += " __chx_v";
        append_uint(out, id);
        return;
    }
    append_c_declarator(module, type, vname(id), out);
}

// Optional per-function map of place id -> C argument index. A scalar parameter
// that the body never reassigns and never addresses *is* the C argument, so the
// emitter aliases it and the stack slot plus the prologue spill store disappear
// (evaluation §1.2, action C3). thread_local so parallel emission stays safe.
thread_local const std::vector<uint32_t>* g_param_alias = nullptr;

// The C expression naming a place. Frame-resident places live in the coroutine
// frame (`__chx__af-><field>`); an aliased parameter is its C argument;
// everything else is a stack slot.
void append_place_name(const MIRModule& module, const MIRFunction& fn, PlaceId id,
                       std::string& out) {
    if (g_param_alias && id < g_param_alias->size() &&
        (*g_param_alias)[id] != MIR_INVALID_ID) {
        out += "__chx_a";
        append_uint(out, (*g_param_alias)[id]);
        return;
    }
    if (id < fn.places.size()) {
        const MIRPlaceDef& pd = fn.places[id];
        if (pd.storage_class == static_cast<uint8_t>(MIRStorageClass::FramePtr)) {
            out += "__chx__af";
            return;
        }
        if (pd.storage_class == static_cast<uint8_t>(MIRStorageClass::FrameField) &&
            pd.frame_field < module.constants.size()) {
            const MIRConstant& c = module.constants.get(pd.frame_field);
            if (c.kind == MIRConstantKind::String) {
                out += "__chx__af->";
                out.append(module.constants.data.data_at(c.data_offset), c.data_count);
                return;
            }
        }
    }
    out += "__chx_p";
    append_uint(out, id);
}

std::string place_name(const MIRModule& module, const MIRFunction& fn, PlaceId id) {
    std::string s;
    append_place_name(module, fn, id, s);
    return s;
}

void append_escape_c_string(const char* data, uint32_t len, std::string& out) {
    out += "\"";
    for (uint32_t i = 0; i < len; ++i) {
        const char c = data[i];
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    out += "\"";
}

std::string escape_c_string(const char* data, uint32_t len) {
    std::string s;
    append_escape_c_string(data, len, s);
    return s;
}

const char* binop_c(MIRBinaryOp op) {
    switch (op) {
        case MIRBinaryOp::Add: return "+";
        case MIRBinaryOp::Sub: return "-";
        case MIRBinaryOp::Mul: return "*";
        case MIRBinaryOp::Div: return "/";
        case MIRBinaryOp::Rem: return "%";
        case MIRBinaryOp::Shl: return "<<";
        case MIRBinaryOp::Shr: return ">>";
        case MIRBinaryOp::BitAnd: return "&";
        case MIRBinaryOp::BitOr: return "|";
        case MIRBinaryOp::BitXor: return "^";
        case MIRBinaryOp::Lt: return "<";
        case MIRBinaryOp::Le: return "<=";
        case MIRBinaryOp::Gt: return ">";
        case MIRBinaryOp::Ge: return ">=";
        case MIRBinaryOp::Eq: return "==";
        case MIRBinaryOp::Ne: return "!=";
    }
    return "?";
}

const char* unop_c(MIRUnaryOp op) {
    switch (op) {
        case MIRUnaryOp::Neg: return "-";
        case MIRUnaryOp::Plus: return "+";
        case MIRUnaryOp::Not: return "!";
        case MIRUnaryOp::BitNot: return "~";
    }
    return "?";
}

bool symbol_name(const MIRModule& module, SymbolId sym, std::string& out) {
    if (sym == MIR_INVALID_ID || sym >= module.symbols.size()) return false;
    const std::string_view name = symbol_name_view(module, sym);
    if (!name.empty()) out.assign(name.data(), name.size());
    return true;
}

bool constant_string(const MIRModule& module, ConstantId cid, std::string& out) {
    if (cid >= module.constants.size()) return false;
    if (module.constants.get(cid).kind != MIRConstantKind::String) return false;
    const std::string_view s = constant_string_view(module, cid);
    out.assign(s.data(), s.size());
    return true;
}

std::string member_access(const MIRFunction& fn, const MIRModule& module, PlaceId base,
                          const std::string& fname) {
    const MIRTypeRecord& bt = module.types.get(fn.places[base].type);
    const bool arrow = bt.kind == MIRTypeKind::Pointer || bt.kind == MIRTypeKind::Reference;
    return place_name(module, fn, base) + (arrow ? "->" : ".") + fname;
}

// Append-style counterparts used by the emit loop (no temporary string).
bool append_symbol_name(const MIRModule& module, SymbolId sym, std::string& out) {
    if (sym == MIR_INVALID_ID || sym >= module.symbols.size()) return false;
    const std::string_view name = symbol_name_view(module, sym);
    if (!name.empty()) out.append(name.data(), name.size());
    return true;
}

bool append_constant_string(const MIRModule& module, ConstantId cid, std::string& out) {
    if (cid >= module.constants.size()) return false;
    if (module.constants.get(cid).kind != MIRConstantKind::String) return false;
    const std::string_view s = constant_string_view(module, cid);
    out.append(s.data(), s.size());
    return true;
}

void append_member_access(const MIRFunction& fn, const MIRModule& module, PlaceId base,
                          const std::string& fname, std::string& out) {
    const MIRTypeRecord& bt = module.types.get(fn.places[base].type);
    const bool arrow = bt.kind == MIRTypeKind::Pointer || bt.kind == MIRTypeKind::Reference;
    append_place_name(module, fn, base, out);
    out += arrow ? "->" : ".";
    out += fname;
}

} // namespace

// Append the C spelling of `type` to `out`. Every speller here has this append
// form; `c_type_of`/`c_declarator` are the string-returning wrappers for callers
// that need to store or compare a spelling.
void append_c_type_of(const MIRModule& module, TypeId type, std::string& out) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) { out += "void"; return; }
    const MIRTypeRecord& r = module.types.get(type);
    switch (r.kind) {
        case MIRTypeKind::Void: out += "void"; return;
        case MIRTypeKind::Bool: out += "_Bool"; return;
        case MIRTypeKind::Int:
            if (const char* s = c_int_kind_spelling(r.int_kind)) { out += s; return; }
            out += c_int_type(r.size, (r.flags & TF_SIGNED) != 0);
            return;
        case MIRTypeKind::Float:
            if (r.size == 4) out += "float";
            else if (r.size == 16) out += "long double";
            else out += "double";
            return;
        case MIRTypeKind::Pointer: {
            if (r.element != MIR_INVALID_ID && r.element < module.types.size() &&
                module.types.get(r.element).kind == MIRTypeKind::Array) {
                // pointer to array: `T (*)[N]...` (dimensions outer-to-inner)
                std::string dims;
                TypeId cur = r.element;
                while (cur != MIR_INVALID_ID && cur < module.types.size() &&
                       module.types.get(cur).kind == MIRTypeKind::Array) {
                    const MIRTypeRecord& rr = module.types.get(cur);
                    dims += "[";
                    append_uint(dims, rr.data_count);
                    dims += "]";
                    cur = rr.element;
                }
                if (!(r.flags & TF_MUTABLE)) out += "const ";
                append_c_type_of(module, cur, out);
                out += " (*)";
                out += dims;
                return;
            }
            if (!(r.flags & TF_MUTABLE)) out += "const ";
            append_c_type_of(module, r.element, out);
            out += "*";
            return;
        }
        case MIRTypeKind::Reference:
            append_c_type_of(module, r.element, out);
            out += (r.flags & TF_MUTABLE) ? "*" : "*const";
            return;
        case MIRTypeKind::Array:
            append_c_type_of(module, r.element, out);
            out += "[";
            append_uint(out, r.data_count);
            out += "]";
            return;
        case MIRTypeKind::Struct:
        case MIRTypeKind::Variant:
            if (r.name_length == 0) { out += "void*"; return; }
            if (!(r.flags & TF_TYPEDEF)) out += "struct ";
            out.append(module.types.name_data(r), r.name_length);
            return;
        case MIRTypeKind::Union:
            if (r.name_length == 0) { out += "void*"; return; }
            if (!(r.flags & TF_TYPEDEF)) out += "union ";
            out.append(module.types.name_data(r), r.name_length);
            return;
        default:
            out += "void*"; // aggregates are emitted by the aggregate milestone
            return;
    }
}

std::string c_type_of(const MIRModule& module, TypeId type) {
    std::string s;
    append_c_type_of(module, type, s);
    return s;
}

// True when `c_type_of` would spell `type` as `void` (the sret/void-result test
// the emit loop uses), without building the type string.
static bool c_type_spells_void(const MIRModule& module, TypeId type) {
    return type == MIR_INVALID_ID || type >= module.types.size() ||
           module.types.get(type).kind == MIRTypeKind::Void;
}

// The bare function-pointer spelling of a MIR function type, e.g.
// `void**(*)(const void**)`. Used for casts and parameter declarators.
//
// A function pointer whose return type is struct-like uses the same sret
// convention as function definitions and the legacy declarations: the pointer
// returns `void` and takes a leading `struct Ret*` out-parameter.
static bool mir_struct_like_return(const MIRModule& module, TypeId t) {
    if (t >= module.types.size()) return false;
    const MIRTypeRecord& r = module.types.get(t);
    return r.kind == MIRTypeKind::Struct || r.kind == MIRTypeKind::Union ||
           r.kind == MIRTypeKind::Variant;
}

static void append_fn_ptr_type(const MIRModule& module, const MIRTypeRecord& r, std::string& out) {
    const bool sret = mir_struct_like_return(module, r.element);
    if (sret) out += "void";
    else append_c_type_of(module, r.element, out);
    out += "(*)(";
    bool first = true;
    if (sret) {
        append_c_type_of(module, r.element, out);
        out += "*";
        first = false;
    }
    for (uint32_t i = 0; i < r.data_count; ++i) {
        if (!first) out += ", ";
        append_c_type_of(module, module.types.data_at(r.data_offset + i), out);
        first = false;
    }
    if (r.flags & TF_VARIADIC) {
        if (!first) out += ", ";
        out += "...";
        first = false;
    }
    if (first) out += "void";
    out += ")";
}

static std::string declarator(const MIRModule& module, TypeId type, std::string_view name);

static void append_c_declarator_at(const MIRModule& module, TypeId type, std::string_view name,
                                   std::string& out, uint32_t depth);

// The declarator for an entity named `name` whose type is `type`, appended to
// `out`. Recurses through function return types so a function returning a
// function pointer is spelled with nested declarators
// (`int(*(*name)(params))(retparams)`), matching the legacy
// `wrap_nested_func_return` spelling.
//
// A nested declarator needs its name as one contiguous span before the return
// type can be spelled in front of it, and the parameter list needs the same, so
// both live in `decl_scratch` — this runs for every parameter of every
// prototype and every function-pointer-typed declaration, and a throwaway
// `std::string` each time is pure waste. `depth` keeps a recursive call from
// clobbering a buffer its caller is still reading.
static void append_decl_type(const MIRModule& module, TypeId type, std::string_view name,
                             std::string& out, uint32_t depth) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) {
        out += "void ";
        out += name;
        return;
    }
    const MIRTypeRecord& r = module.types.get(type);
    if (r.kind != MIRTypeKind::Function) {
        append_c_type_of(module, type, out);
        out += ' ';
        out += name;
        return;
    }
    const bool sret = mir_struct_like_return(module, r.element);
    std::string& params = decl_scratch(depth);
    if (sret) {
        append_c_type_of(module, r.element, params);
        params += '*';
    }
    for (uint32_t i = 0; i < r.data_count; ++i) {
        if (!params.empty()) params += ", ";
        char nm[24];
        append_c_declarator_at(module, module.types.data_at(r.data_offset + i),
                               param_arg_name(nm, i), params, depth + 1);
    }
    if (r.flags & TF_VARIADIC) {
        if (!params.empty()) params += ", ";
        params += "...";
    }
    if (params.empty()) params += "void";
    std::string& inner = decl_scratch(depth + 1);
    inner += "(*";
    inner += name;
    inner += ")(";
    inner += params;
    inner += ')';
    if (sret) {
        out += "void ";
        out += inner;
        return;
    }
    append_decl_type(module, r.element, inner, out, depth + 2);
}

void append_c_declarator(const MIRModule& module, TypeId type, std::string_view name,
                         std::string& out) {
    append_c_declarator_at(module, type, name, out, 0);
}

void append_c_declarator_scratched(const MIRModule& module, TypeId type, std::string_view name,
                                   std::string& out) {
    append_c_declarator_at(module, type, name, out, 1);
}

static void append_c_declarator_at(const MIRModule& module, TypeId type, std::string_view name,
                                   std::string& out, uint32_t depth) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) {
        out += "void ";
        out += name;
        return;
    }
    const MIRTypeRecord& r = module.types.get(type);
    if (r.kind == MIRTypeKind::Function) {
        append_decl_type(module, type, name, out, depth);
        return;
    }
    if (r.kind == MIRTypeKind::Array) {
        // `[4][256]T` spells `T name[4][256]`: collect the innermost element and
        // whether an array in the chain holds a function type (only then does
        // the name need its bracket suffix before the declarator is spelled).
        // Two cheap type walks instead of a `std::string dims` per array-typed
        // declarator, which is what every array local, member and parameter is.
        TypeId cur = type;
        bool fn_elem = false;
        while (cur != MIR_INVALID_ID && cur < module.types.size() &&
               module.types.get(cur).kind == MIRTypeKind::Array) {
            const TypeId elem = module.types.get(cur).element;
            if (elem != MIR_INVALID_ID && elem < module.types.size() &&
                module.types.get(elem).kind == MIRTypeKind::Function) {
                fn_elem = true;
                cur = elem;
                break;
            }
            cur = elem;
        }
        if (fn_elem) {
            std::string& suffixed = decl_scratch(depth);
            suffixed += name;
            for (TypeId d = type; d != MIR_INVALID_ID && d < module.types.size() &&
                                   module.types.get(d).kind == MIRTypeKind::Array;
                 d = module.types.get(d).element) {
                suffixed += '[';
                append_uint(suffixed, module.types.get(d).data_count);
                suffixed += ']';
            }
            append_decl_type(module, cur, suffixed, out, depth + 1);
            return;
        }
        append_c_declarator_at(module, cur, name, out, depth);
        for (TypeId d = type; d != MIR_INVALID_ID && d < module.types.size() &&
                               module.types.get(d).kind == MIRTypeKind::Array;
             d = module.types.get(d).element) {
            out += '[';
            append_uint(out, module.types.get(d).data_count);
            out += ']';
        }
        return;
    }
    if (r.kind == MIRTypeKind::Pointer || r.kind == MIRTypeKind::Reference) {
        const MIRTypeRecord& pointee = module.types.get(r.element);
        if (pointee.kind == MIRTypeKind::Array) {
            // pointer to array: `int (*name)[N]`
            std::string& wrapped = decl_scratch(depth);
            wrapped += "(*";
            wrapped += name;
            wrapped += ')';
            append_c_declarator_at(module, pointee.element, wrapped, out, depth + 1);
            out += '[';
            append_uint(out, pointee.data_count);
            out += ']';
            return;
        }
    }
    append_c_type_of(module, type, out);
    out += " ";
    out += name;
}

static std::string fn_ptr_type(const MIRModule& module, const MIRTypeRecord& r) {
    std::string s;
    append_fn_ptr_type(module, r, s);
    return s;
}

static std::string declarator(const MIRModule& module, TypeId type, std::string_view name) {
    std::string s;
    append_c_declarator(module, type, name, s);
    return s;
}

std::string c_decl_type(const MIRModule& module, TypeId type, std::string_view name) {
    std::string out;
    append_decl_type(module, type, name, out, 0);
    return out;
}

void append_c_decl_type(const MIRModule& module, TypeId type, std::string_view name,
                        std::string& out) {
    append_decl_type(module, type, name, out, 0);
}

std::string c_declarator(const MIRModule& module, TypeId type, std::string_view name) {
    return declarator(module, type, name);
}

std::string c_function_proto(const MIRModule& module, TypeId function_type,
                             std::string_view name) {
    std::string out;
    append_c_function_proto(module, function_type, name, out);
    return out;
}

void append_c_function_proto(const MIRModule& module, TypeId function_type,
                             std::string_view name, std::string& out) {
    if (function_type == MIR_INVALID_ID || function_type >= module.types.size()) {
        out += "void ";
        out += name;
        out += "(void)";
        return;
    }
    const MIRTypeRecord& r = module.types.get(function_type);
    if (r.kind != MIRTypeKind::Function) {
        append_decl_type(module, function_type, name, out, 0);
        return;
    }
    const bool sret = mir_struct_like_return(module, r.element);
    // the parameter list, and then the `name(params)` span a nested declarator
    // needs, come from the reusable scratch instead of a throwaway string per
    // prototype item
    std::string& params = decl_scratch(0);
    if (sret) {
        append_c_type_of(module, r.element, params);
        params += '*';
    }
    for (uint32_t i = 0; i < r.data_count; ++i) {
        if (!params.empty()) params += ", ";
        char nm[24];
        append_c_declarator_at(module, module.types.data_at(r.data_offset + i),
                               param_arg_name(nm, i), params, 2);
    }
    if (r.flags & TF_VARIADIC) {
        if (!params.empty()) params += ", ";
        params += "...";
    }
    if (params.empty()) params += "void";
    if (module.types.get(r.element).kind == MIRTypeKind::Function) {
        std::string& nested = decl_scratch(1);
        nested += name;
        nested += '(';
        nested += params;
        nested += ')';
        append_decl_type(module, r.element, nested, out, 3);
        return;
    }
    if (sret) out += "void";
    else append_c_type_of(module, r.element, out);
    out += ' ';
    out += name;
    out += '(';
    out += params;
    out += ')';
}

namespace {

bool append_const_literal(const MIRModule& module, ConstantId cid, TypeId cast_type,
                          std::string& out) {
    if (cid >= module.constants.size()) return false;
    const MIRConstant& c = module.constants.get(cid);
    switch (c.kind) {
        case MIRConstantKind::Int:
            out += "(";
            append_c_type_of(module, cast_type, out);
            out += ")(";
            append_uint(out, c.bits);
            out += "ull)";
            return true;
        case MIRConstantKind::Bool:
            out += c.bits ? "true" : "false";
            return true;
        case MIRConstantKind::Null:
            out += "0";
            return true;
        case MIRConstantKind::Float: {
            float f;
            uint32_t b = static_cast<uint32_t>(c.bits);
            std::memcpy(&f, &b, sizeof(f));
            out += format_float_literal(static_cast<double>(f), 6, 9, "f");
            return true;
        }
        case MIRConstantKind::Double: {
            double d;
            std::memcpy(&d, &c.bits, sizeof(d));
            out += format_float_literal(d, 15, 17, "");
            return true;
        }
        case MIRConstantKind::String:
            append_escape_c_string(module.constants.data.data_at(c.data_offset), c.data_count, out);
            return true;
        case MIRConstantKind::Bytes:
            out += "0";
            return true;
    }
    return false;
}

bool const_literal(const MIRModule& module, ConstantId cid, TypeId cast_type, std::string& out) {
    out.clear();
    return append_const_literal(module, cid, cast_type, out);
}

// Optional per-function map of value id -> inlined C expression. When set,
// `operand_expr` returns the inlined expression (parenthesized) instead of the
// value's temporary name. This lets the emitter collapse single-use pure
// values (address chains, arithmetic) into their single use site, which keeps
// the generated C close to the legacy translator's output and avoids the
// register pressure / spilling that a temporary-per-instruction style causes
// in TinyCC. thread_local so parallel lowering stays safe.
thread_local const std::vector<std::string>* g_inline_expr = nullptr;

void append_operand_expr(const MIRFunction& fn, const MIRModule& module,
                         const MIROperand& op, std::string& out) {
    (void) fn; // reserved for future place-projection spelling
    switch (op.kind()) {
        case MIROperandKind::Value:
            if (g_inline_expr && op.id < g_inline_expr->size() &&
                !(*g_inline_expr)[op.id].empty()) {
                out += "(";
                out += (*g_inline_expr)[op.id];
                out += ")";
            } else {
                append_vname(op.id, out);
            }
            return;
        case MIROperandKind::Place:
            append_place_name(module, fn, op.id, out);
            return;
        case MIROperandKind::Symbol:
            append_symbol_name(module, op.id, out);
            return;
        case MIROperandKind::Constant:
            append_const_literal(module, op.id, op.type(), out);
            return;
        default:
            out += "?";
            return;
    }
}

std::string operand_expr(const MIRFunction& fn, const MIRModule& module,
                         const MIROperand& op) {
    std::string s;
    append_operand_expr(fn, module, op, s);
    return s;
}

} // namespace

bool emit_function_c(const MIRFunction& function, const MIRModule& module,
                     std::string& out, std::string& error) {
    if (function.function_type == MIR_INVALID_ID ||
        function.function_type >= module.types.size()) {
        error = "function has no valid function type";
        return false;
    }
    const MIRTypeRecord& ftr = module.types.get(function.function_type);

    // The generated C for one function runs to many KB (aggregate-heavy
    // functions far more), so reserve up front and stop `out` reallocating as
    // it grows. ~24 bytes per instruction is a deliberately rough estimate.
    out.reserve(out.size() + function.instructions.size() * 24 + 128);

    std::string fnname;
    if (!symbol_name(module, function.symbol, fnname) || fnname.empty()) {
        fnname = "__chx_fn" + std::to_string(function.symbol);
    }

    const bool is_static = function.symbol != MIR_INVALID_ID &&
                           function.symbol < module.symbols.size() &&
                           module.symbols.get(function.symbol).linkage == MIRLinkage::Internal;

    // reused across calls so the parameter-type list does not reallocate per function
    static thread_local std::vector<TypeId> ptypes;
    ptypes.clear();
    ptypes.reserve(ftr.data_count);
    for (uint32_t i = 0; i < ftr.data_count; ++i) {
        ptypes.push_back(module.types.data_at(ftr.data_offset + i));
    }

    if (is_static) out += "static ";
    if (module.types.get(ftr.element).kind == MIRTypeKind::Function) {
        // a function returning a function pointer needs a nested declarator:
        // `ret (*(*name)(params))(retparams)` — the one case that needs the
        // parameter list as a string before the name can be spelled
        std::string params;
        if (ptypes.empty()) {
            params = "void";
        } else {
            for (uint32_t i = 0; i < ptypes.size(); ++i) {
                if (i) params += ", ";
                char nm[24];
                params += declarator(module, ptypes[i], param_arg_name(nm, i));
            }
        }
        std::string head(fnname);
        head += '(';
        head += params;
        head += ')';
        out += declarator(module, ftr.element, head);
    } else {
        // every other function prints its header straight into `out`: no
        // parameter-list string, and no `declarator` temporary per parameter
        append_c_type_of(module, ftr.element, out);
        out += ' ';
        out += fnname;
        out += '(';
        if (ptypes.empty()) {
            out += "void";
        } else {
            for (uint32_t i = 0; i < ptypes.size(); ++i) {
                if (i) out += ", ";
                char nm[24];
                append_c_declarator(module, ptypes[i], param_arg_name(nm, i), out);
            }
        }
        out += ')';
    }
    out += " {\n";

    // Per-function facts (evaluation action B): block offsets and labels, place
    // write counts, parameter aliasing. Computed once in MIR core (`MIRFacts`)
    // so every backend reads the same answer instead of re-deriving it — this
    // file used to run the whole analysis itself.
    //
    // `label_at_inst` drops a block nothing jumps to: an unreferenced label is
    // dead text and an extra jump target for the C compiler, which the legacy
    // translator never emitted, and a branch to the block starting at the very
    // next instruction is a fall-through that prints no jump at all.
    // Reused across calls (one emitter runs per thread): the facts vectors keep
    // their capacity, so computing facts allocates nothing after warm-up.
    static thread_local MIRFunctionFacts facts;
    compute_function_facts(module, function, facts);
    const std::vector<BlockId>& label_at = facts.label_at_inst;
    const std::vector<uint8_t>& block_starts = facts.block_starts_inst;

    // Per-value compaction facts (single-use / pure classification), also MIR's
    // to publish so the inlining decision reads the same answer here and in any
    // other backend.
    static thread_local MIRValueFacts value_facts;
    compute_value_facts(module, function, value_facts);
    const std::vector<uint8_t>& single_use = value_facts.single_use;

    // Build the single-use inlining map: a pure value used exactly once is
    // collapsed into its use site (address chains, arithmetic, casts,
    // constants). This keeps the generated C close to the legacy translator's
    // output and avoids TinyCC spilling a stack temporary per MIR instruction.
    // Reused across calls so the inline-expression table does not reallocate per
    // function; `assign` keeps its capacity.
    static thread_local std::vector<std::string> inline_expr;
    inline_expr.assign(function.values.size(), std::string());
    struct InlineGuard {
        const std::vector<std::string>* prev;
        const std::vector<uint32_t>* prev_alias;
        ~InlineGuard() {
            g_inline_expr = prev;
            g_param_alias = prev_alias;
        }
    } inline_guard{g_inline_expr, g_param_alias};
    g_inline_expr = &inline_expr;

    auto expr_of_inst = [&](const MIRInstruction& inst) -> std::string {
        const ValueId v = inst.result_or_place;
        switch (inst.opcode()) {
            case MIROpcode::ConstInt:
            case MIROpcode::ConstBool:
            case MIROpcode::ConstFloat:
            case MIROpcode::ConstDouble:
            case MIROpcode::ConstString:
            case MIROpcode::ConstNull: {
                const MIROperand* c = operand_at(function, inst, 0);
                std::string lit;
                if (!c || !const_literal(module, c->id, function.values[v].type, lit)) return {};
                return lit;
            }
            case MIROpcode::FunctionAddr: {
                const MIROperand* s = operand_at(function, inst, 0);
                if (!s) return {};
                std::string name;
                symbol_name(module, s->id, name);
                return name;
            }
            case MIROpcode::GlobalAddr: {
                const MIROperand* s = operand_at(function, inst, 0);
                if (!s) return {};
                std::string name;
                symbol_name(module, s->id, name);
                bool decay = false;
                if (s->id < module.symbols.size()) {
                    const TypeId st = module.symbols.get(s->id).type;
                    if (st < module.types.size() &&
                        module.types.get(st).kind == MIRTypeKind::Array) {
                        decay = true;
                    }
                }
                return decay ? name : ("&" + name);
            }
            case MIROpcode::AddressOf: {
                const MIROperand* p = operand_at(function, inst, 0);
                if (!p || p->kind() != MIROperandKind::Place) return {};
                const TypeId pt = function.places[p->id].type;
                if (pt < module.types.size() &&
                    module.types.get(pt).kind == MIRTypeKind::Array) {
                    return place_name(module, function, p->id);
                }
                return "&" + place_name(module, function, p->id);
            }
            case MIROpcode::FieldAddr: {
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* fld = operand_at(function, inst, 1);
                if (!p || !fld) return {};
                std::string fname;
                if (!constant_string(module, fld->id, fname)) return {};
                const bool ptr = p->kind() != MIROperandKind::Place;
                const std::string base =
                    ptr ? operand_expr(function, module, *p)
                        : place_name(module, function, p->id);
                bool arrow = ptr;
                if (!ptr) {
                    const MIRTypeRecord& bt = module.types.get(function.places[p->id].type);
                    arrow = (bt.kind == MIRTypeKind::Pointer ||
                             bt.kind == MIRTypeKind::Reference);
                }
                return "&" + base + (arrow ? "->" : ".") + fname;
            }
            case MIROpcode::IndexAddr: {
                const MIROperand* b = operand_at(function, inst, 0);
                const MIROperand* idx = operand_at(function, inst, 1);
                if (!b || !idx) return {};
                std::string bexpr;
                bool base_is_array = false;
                if (b->kind() == MIROperandKind::Place) {
                    bexpr = place_name(module, function, b->id);
                } else {
                    bexpr = operand_expr(function, module, *b);
                    base_is_array = b->type() < module.types.size() &&
                                    module.types.get(b->type()).kind == MIRTypeKind::Array;
                }
                const std::string access =
                    (base_is_array ? ("(*" + bexpr + ")") : bexpr) + "[" +
                    operand_expr(function, module, *idx) + "]";
                return "&" + access;
            }
            case MIROpcode::Unary: {
                const MIROperand* a = operand_at(function, inst, 0);
                const MIROperand* opc = operand_at(function, inst, 1);
                if (!a || !opc || opc->id >= module.constants.size()) return {};
                // wrap in the value's type: C integer promotion would otherwise
                // change the result (`int8_t t = -x` != `-x` in an int context)
                return "(" + c_type_of(module, function.values[v].type) + ")(" +
                       std::string(unop_c(static_cast<MIRUnaryOp>(
                           module.constants.get(opc->id).bits))) +
                       operand_expr(function, module, *a) + ")";
            }
            case MIROpcode::Binary:
            case MIROpcode::Compare: {
                const MIROperand* a = operand_at(function, inst, 0);
                const MIROperand* b = operand_at(function, inst, 1);
                const MIROperand* opc = operand_at(function, inst, 2);
                if (!a || !b || !opc || opc->id >= module.constants.size()) return {};
                return "(" + c_type_of(module, function.values[v].type) + ")(" +
                       operand_expr(function, module, *a) + " " +
                       binop_c(static_cast<MIRBinaryOp>(
                           module.constants.get(opc->id).bits)) +
                       " " + operand_expr(function, module, *b) + ")";
            }
            case MIROpcode::Cast: {
                const MIROperand* a = operand_at(function, inst, 0);
                if (!a) return {};
                return "(" + c_type_of(module, function.values[v].type) + ")(" +
                       operand_expr(function, module, *a) + ")";
            }
            case MIROpcode::Borrow:
            case MIROpcode::BorrowMut: {
                // the address value already has the right pointer spelling
                const MIROperand* a = operand_at(function, inst, 0);
                if (!a) return {};
                return operand_expr(function, module, *a);
            }
            // Load and Call are never reached by the single-use pass above (a
            // Load is handled by its own forwarding rules and a Call is not
            // `inlineable_opcode`), but short-circuit reconstruction needs to
            // spell whatever a branch's RHS computes as one expression.
            case MIROpcode::Load: {
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* fld = operand_at(function, inst, 1);
                if (!p) return {};
                if (p->kind() == MIROperandKind::Place) {
                    if (fld && fld->kind() == MIROperandKind::Value) {
                        return place_name(module, function, p->id) + "[" +
                               operand_expr(function, module, *fld) + "]";
                    }
                    if (fld && fld->kind() == MIROperandKind::Constant) {
                        std::string fname;
                        if (!constant_string(module, fld->id, fname)) return {};
                        return member_access(function, module, p->id, fname);
                    }
                    return place_name(module, function, p->id);
                }
                if (p->kind() != MIROperandKind::Value) return {};
                const bool base_is_array =
                    p->type() < module.types.size() &&
                    module.types.get(p->type()).kind == MIRTypeKind::Array;
                const std::string base = operand_expr(function, module, *p);
                if (fld && fld->kind() == MIROperandKind::Value) {
                    return (base_is_array ? ("(*" + base + ")") : base) + "[" +
                           operand_expr(function, module, *fld) + "]";
                }
                if (fld && fld->kind() == MIROperandKind::Constant) {
                    std::string fname;
                    if (!constant_string(module, fld->id, fname)) return {};
                    return base + "->" + fname;
                }
                return "*" + base;
            }
            case MIROpcode::Call: {
                const MIROperand* callee = operand_at(function, inst, 0);
                if (!callee || callee->kind() != MIROperandKind::Symbol) return {};
                const MIROperand* maybe_place = operand_at(function, inst, 1);
                if (maybe_place && maybe_place->kind() == MIROperandKind::Place) {
                    return {}; // sret: a statement, not a call expression
                }
                std::string call;
                if (!symbol_name(module, callee->id, call)) return {};
                call += "(";
                for (uint32_t a = 1; a < inst.operand_count; ++a) {
                    if (a != 1) call += ", ";
                    append_operand_expr(function, module, *operand_at(function, inst, a), call);
                }
                call += ")";
                return call;
            }
            case MIROpcode::CallIndirect: {
                const MIROperand* fn = operand_at(function, inst, 0);
                if (!fn) return {};
                const MIROperand* maybe_place = operand_at(function, inst, 1);
                if (maybe_place && maybe_place->kind() == MIROperandKind::Place) {
                    return {}; // sret
                }
                std::string callee;
                append_operand_expr(function, module, *fn, callee);
                if (fn->type() < module.types.size()) {
                    const MIRTypeRecord& fr = module.types.get(fn->type());
                    if (fr.kind == MIRTypeKind::Function) {
                        std::string typed = "((";
                        append_fn_ptr_type(module, fr, typed);
                        typed += ")";
                        typed += callee;
                        typed += ")";
                        callee = std::move(typed);
                    }
                }
                std::string call = callee;
                call += "(";
                for (uint32_t a = 1; a < inst.operand_count; ++a) {
                    if (a != 1) call += ", ";
                    append_operand_expr(function, module, *operand_at(function, inst, a), call);
                }
                call += ")";
                return call;
            }
            default:
                return {};
        }
    };
    // Only type-preserving values may be inlined. Arithmetic must NOT be
    // (C integer promotion would change the result of overflow/negation tests:
    // `int8_t t = -x; use(t)` is not `use(-x)`).
    auto inlineable_opcode = [](MIROpcode op) {
        switch (op) {
            case MIROpcode::ConstInt:
            case MIROpcode::ConstBool:
            case MIROpcode::ConstFloat:
            case MIROpcode::ConstDouble:
            case MIROpcode::ConstString:
            case MIROpcode::ConstNull:
            case MIROpcode::AddressOf:
            case MIROpcode::FieldAddr:
            case MIROpcode::IndexAddr:
            case MIROpcode::GlobalAddr:
            case MIROpcode::FunctionAddr:
            case MIROpcode::Unary:
            case MIROpcode::Binary:
            case MIROpcode::Compare:
            case MIROpcode::Cast:
            case MIROpcode::Load:
                return true;
            default:
                return false;
        }
    };
    // Which places are written more than once (their initializing store)? A
    // parameter slot written only by its initial spill can be read by name
    // instead of through a temporary (`v = p; *v` -> `*p`).
    const std::vector<uint32_t>& place_writes = facts.place_writes;
    // Param instruction -> its C argument index (`__chx_aN`), independent of
    // whether the Param itself is emitted (single-use Params are inlined).
    const std::vector<uint32_t>& param_idx_at = facts.param_index_at_inst;
    // A scalar parameter passed by value and never reassigned or addressed
    // *is* the C argument: alias it, so neither its stack slot nor the
    // prologue spill store is emitted at all (evaluation action C3).
    g_param_alias = &facts.param_alias;

    // True when instruction `j` writes place `pid` (a write takes the place as
    // operand 0). Mirrors MIRFacts' `writes_place` opcode set.
    auto inst_writes_place = [&](uint32_t j, PlaceId pid) -> bool {
        const MIRInstruction& wi = function.instructions[j];
        switch (wi.opcode()) {
            case MIROpcode::Store:
            case MIROpcode::Assign:
            case MIROpcode::CopyInit:
            case MIROpcode::MoveInit:
            case MIROpcode::Init:
            case MIROpcode::MemCpy:
            case MIROpcode::MemSet:
                break;
            default:
                return false;
        }
        const MIROperand* wp = operand_at(function, wi, 0);
        return wp && wp->kind() == MIROperandKind::Place && wp->id == pid;
    };

    // Transitive "inline cone" bound. A pure value that inlines a loaded place
    // re-emits that place name at *every* one of its own use sites, and so on
    // recursively, so a forwarded load is only sound while every site that can
    // re-emit the name precedes the next write to the place. `max_use_of`
    // returns the greatest instruction index that can emit `v`'s place name,
    // following exactly the pure-op inlining the emitter performs below.
    std::vector<std::vector<uint32_t>> value_uses(function.values.size());
    for (uint32_t j = 0; j < function.instructions.size(); ++j) {
        const MIRInstruction& in = function.instructions[j];
        for (uint32_t k = 0; k < in.operand_count; ++k) {
            const MIROperand* op = operand_at(function, in, k);
            if (op && op->kind() == MIROperandKind::Value && op->id < value_uses.size()) {
                value_uses[op->id].push_back(j);
            }
        }
    }
    std::vector<uint32_t> cone_max(function.values.size(), 0);
    std::vector<uint8_t> cone_known(function.values.size(), 0);
    std::function<uint32_t(ValueId)> max_use_of = [&](ValueId v) -> uint32_t {
        if (v >= cone_max.size()) return 0;
        if (cone_known[v]) return cone_max[v];
        cone_known[v] = 1;
        uint32_t m = 0;
        for (uint32_t u : value_uses[v]) {
            if (u > m) m = u;
            const MIRInstruction& ui = function.instructions[u];
            const ValueId w = ui.result_or_place;
            if (w >= function.values.size() || function.values[w].def_inst != u) continue;
            switch (ui.opcode()) {
                case MIROpcode::Unary:
                case MIROpcode::Binary:
                case MIROpcode::Compare:
                case MIROpcode::Cast:
                case MIROpcode::AddressOf:
                case MIROpcode::FieldAddr:
                case MIROpcode::IndexAddr:
                case MIROpcode::GlobalAddr:
                case MIROpcode::FunctionAddr: {
                    const uint32_t cw = max_use_of(w);
                    if (cw > m) m = cw;
                    break;
                }
                default:
                    break;
            }
        }
        cone_max[v] = m;
        return m;
    };

    // The pass is re-run after short-circuit reconstruction (below): values
    // that were already inlined spell the merge value by its old name, so the
    // whole map must be rebuilt with the reconstructed merge expressions in
    // place. `frozen` values are the reconstructed merges themselves, which
    // must keep the expression the reconstruction chose.
    auto run_inline_pass = [&](const std::vector<uint8_t>& frozen) {
    for (size_t ii = 0; ii < function.instructions.size(); ++ii) {
        const MIRInstruction& inst = function.instructions[ii];
        const ValueId v = inst.result_or_place;
        if (v >= function.values.size()) continue;
        if (v < frozen.size() && frozen[v]) continue;
        if (inst.opcode() == MIROpcode::Load) {
            const MIROperand* p = operand_at(function, inst, 0);
            // a plain load's only other operand is the result Type; a
            // field/index load carries a Constant/Value and is never inlined.
            const MIROperand* f = operand_at(function, inst, 1);
            if (f && (f->kind() == MIROperandKind::Value ||
                      f->kind() == MIROperandKind::Constant)) {
                continue;
            }
            if (p && p->kind() == MIROperandKind::Place) {
                const PlaceId pid = p->id;
                if (function.places[pid].storage_class ==
                    static_cast<uint8_t>(MIRStorageClass::Parameter)) {
                    if (single_use[v] && place_writes[pid] <= 1) {
                        inline_expr[v] = place_name(module, function, pid);
                    }
                    continue;
                }
                // Forward a load from a non-parameter place across *all* of
                // its uses (not just a single one) when the place is a
                // non-addressed scalar and nothing writes it *after* the load.
                // The last-write guard is function-wide, so a use in any block
                // reads the same value the load did. The value then reads the
                // local by name instead of round-tripping through a stack
                // temporary (`T v = p; ...use v...` -> `...use p...`): the SSA
                // materialization the legacy translator never emitted
                // (evaluation action C4).
                if (facts.place_escaped[pid]) continue;
                const TypeId lt = function.values[v].type;
                if (lt >= module.types.size()) continue;
                switch (module.types.get(lt).kind) {
                    case MIRTypeKind::Void:
                    case MIRTypeKind::Array:
                    case MIRTypeKind::Struct:
                    case MIRTypeKind::Union:
                    case MIRTypeKind::Variant:
                    case MIRTypeKind::Function:
                    case MIRTypeKind::Opaque:
                        continue;
                    default:
                        break;
                }
                if (value_facts.first_use_inst.size() != function.values.size()) continue;
                // Forward only when the place is never written after the load:
                // a use site (or an expression containing the value, re-emitted
                // at a later use) must observe the same value the load did. Any
                // later write would make reading the place by name wrong.
                if (value_facts.first_use_inst[v] == MIR_INVALID_ID ||
                    value_facts.first_use_inst[v] <= ii) {
                    continue;
                }
                if (facts.place_last_write[pid] != MIR_INVALID_ID &&
                    facts.place_last_write[pid] > ii) {
                    // The place is assigned again later, so it cannot be read
                    // by name across arbitrary blocks. It *can* be read by name
                    // while every site that would re-emit the name — the load's
                    // uses and the (transitively inlined) pure values built from
                    // them — stays in the load's own basic block and precedes the
                    // next write: that block is straight-line, so the place still
                    // holds what the load read. This removes the `T v = p;`
                    // materialization the legacy translator never emitted for
                    // loop-carried and repeatedly-assigned places (`v = i; i++`).
                    const uint32_t cone = max_use_of(v);
                    if (cone <= ii) continue;
                    bool same_block = true;
                    for (uint32_t j = ii + 1; j <= cone && same_block; ++j) {
                        if (j < block_starts.size() && block_starts[j]) same_block = false;
                    }
                    if (!same_block) continue;
                    bool wrote = false;
                    for (uint32_t j = ii + 1; j <= cone && !wrote; ++j) {
                        if (inst_writes_place(j, pid)) wrote = true;
                    }
                    if (wrote) continue;
                }
                inline_expr[v] = place_name(module, function, pid);
                continue;
            }
            if (!single_use[v]) continue;
            if (!p) continue;
            // A dereference used only by the store *immediately* after it
            // (`T v = *src; dest = v;`) collapses into that store instead of
            // materializing the temp: the read moves one statement later, so
            // nothing can happen in between, and the destination is one of this
            // frame's own slots (a by-value parameter copy or a compiler
            // temporary), which the source cannot overlap. This removes the
            // second aggregate copy the legacy translator never emitted
            // (evaluation §1.2, action C2). The store must also be inside the
            // same basic block, so the read cannot become unconditional on a path
            // that never executed the load.
            if (ii + 1 >= function.instructions.size()) continue;
            if (block_starts[ii + 1]) continue;
            const MIRInstruction& nxt = function.instructions[ii + 1];
            if (nxt.opcode() != MIROpcode::Store || nxt.operand_count != 2) continue;
            const MIROperand* nd = operand_at(function, nxt, 0);
            const MIROperand* nv = operand_at(function, nxt, 1);
            if (!nd || !nv || nd->kind() != MIROperandKind::Place ||
                nv->kind() != MIROperandKind::Value || nv->id != v) {
                continue;
            }
            if (nd->id >= function.places.size() || v >= function.values.size()) continue;
            const MIRPlaceDef& npd = function.places[nd->id];
            if (npd.storage_class != static_cast<uint8_t>(MIRStorageClass::Parameter) &&
                npd.storage_class != static_cast<uint8_t>(MIRStorageClass::Temporary)) {
                continue;
            }
            const TypeId lt = function.values[v].type;
            if (lt != npd.type || lt >= module.types.size()) continue;
            switch (module.types.get(lt).kind) {
                case MIRTypeKind::Void:
                case MIRTypeKind::Array:
                case MIRTypeKind::Function:
                case MIRTypeKind::Opaque:
                    continue;
                default:
                    break;
            }
            inline_expr[v] = "*" + operand_expr(function, module, *p);
            continue;
        }
        // A `Param` value is the C argument: a single-use one is its argument
        // directly, so neither it nor its spill is materialized.
        if (inst.opcode() == MIROpcode::Param) {
            if (single_use[v]) {
                inline_expr[v] = "__chx_a" + std::to_string(param_idx_at[ii]);
            }
            continue;
        }
        if (!inlineable_opcode(inst.opcode())) continue;
        std::string e = expr_of_inst(inst);
        if (!e.empty()) inline_expr[v] = std::move(e);
    }
    };
    const std::vector<uint8_t> no_frozen;
    run_inline_pass(no_frozen);

    // ---- Short-circuit expression reconstruction (evaluation action C) ----
    // `a && b` / `a || b` lower to a CFG shape the legacy translator never
    // emitted: the LHS is stored to a `_Bool` temporary, the branch tests it,
    // the RHS is evaluated in its own block and stored to the same temporary,
    // and the merge loads it (`p = a; if (!p) goto end; p = b; end: ...p...`).
    // The generated C has no optimizer to recover the operator. When the RHS
    // region is a single straight-line block whose non-terminator instructions
    // are all spellable as C expressions, the shape *is* C's own short-circuit
    // operator, so fold it: keep the LHS store (the LHS must still evaluate
    // exactly where it did), drop the branch and the RHS block, and read the
    // merge as `<place> && <rhs>` / `<place> || <rhs>`.
    std::vector<uint8_t> logical_skip(function.instructions.size(), 0);
    std::vector<uint8_t> suppress_label(function.instructions.size(), 0);
    // Merge load instruction index -> the `a && b` / `a || b` expression that
    // replaces it when its value has more than one use (materialized once, not
    // duplicated at each use).
    std::vector<std::pair<uint32_t, std::string>> logical_decl;
    struct LogicalResult {
        uint32_t end_start;
        ValueId mrv;
        std::string expr;
        bool inline_at_use;
    };
    std::vector<LogicalResult> logicals;
    {
        const uint32_t n_inst = static_cast<uint32_t>(function.instructions.size());
        const uint32_t nv = static_cast<uint32_t>(function.values.size());
        const uint32_t nb = static_cast<uint32_t>(function.blocks.size());
        std::vector<BlockId> block_at(n_inst, MIR_INVALID_ID);
        for (BlockId b = 0; b < nb; ++b) {
            const uint32_t ss = facts.block_start[b];
            if (ss != MIR_INVALID_ID && ss < n_inst) block_at[ss] = b;
        }
        std::vector<uint32_t> jump_count(nb, 0);
        for (uint32_t j = 0; j < n_inst; ++j) {
            const MIRInstruction& in = function.instructions[j];
            if (in.opcode() == MIROpcode::Br) {
                const MIROperand* t = operand_at(function, in, 0);
                if (t) jump_count[facts.resolve_block(t->id)]++;
            } else if (in.opcode() == MIROpcode::CondBr) {
                const MIROperand* t = operand_at(function, in, 1);
                const MIROperand* f = operand_at(function, in, 2);
                if (t) jump_count[facts.resolve_block(t->id)]++;
                if (f) jump_count[facts.resolve_block(f->id)]++;
            }
        }
        auto is_expr_inst = [&](const MIRInstruction& in) -> bool {
            switch (in.opcode()) {
                case MIROpcode::ConstInt:
                case MIROpcode::ConstBool:
                case MIROpcode::ConstFloat:
                case MIROpcode::ConstDouble:
                case MIROpcode::ConstString:
                case MIROpcode::ConstNull:
                case MIROpcode::AddressOf:
                case MIROpcode::FieldAddr:
                case MIROpcode::IndexAddr:
                case MIROpcode::GlobalAddr:
                case MIROpcode::FunctionAddr:
                case MIROpcode::Unary:
                case MIROpcode::Binary:
                case MIROpcode::Compare:
                case MIROpcode::Cast:
                case MIROpcode::Load:
                    return true;
                case MIROpcode::Call:
                case MIROpcode::CallIndirect: {
                    const MIROperand* m = operand_at(function, in, 1);
                    if (m && m->kind() == MIROperandKind::Place) return false; // sret
                    const ValueId w = in.result_or_place;
                    return w < nv && !c_type_spells_void(module, function.values[w].type);
                }
                default:
                    return false;
            }
        };
        std::vector<std::pair<ValueId, std::string>> saved;
        for (uint32_t s = 0; s + 1 < n_inst; ++s) {
            if (logical_skip[s]) continue;
            const MIRInstruction& st = function.instructions[s];
            if (st.opcode() != MIROpcode::Store || st.operand_count != 2) continue;
            const MIROperand* sp = operand_at(function, st, 0);
            const MIROperand* sv = operand_at(function, st, 1);
            if (!sp || !sv || sp->kind() != MIROperandKind::Place ||
                sv->kind() != MIROperandKind::Value)
                continue;
            const PlaceId res = sp->id;
            if (res >= function.places.size() ||
                function.places[res].storage_class !=
                    static_cast<uint8_t>(MIRStorageClass::Temporary))
                continue;
            if (facts.place_escaped[res] || facts.place_writes[res] != 2) continue;
            const MIRInstruction& cb = function.instructions[s + 1];
            if (cb.opcode() != MIROpcode::CondBr) continue;
            const MIROperand* cond = operand_at(function, cb, 0);
            const MIROperand* t = operand_at(function, cb, 1);
            const MIROperand* f = operand_at(function, cb, 2);
            if (!cond || !t || !f) continue;
            if (cond->kind() != MIROperandKind::Value || cond->id != sv->id) continue;
            if (s + 2 >= n_inst || !block_starts[s + 2]) continue;
            const uint32_t rhs_start = s + 2;
            const BlockId tb = facts.resolve_block(t->id);
            const BlockId fb = facts.resolve_block(f->id);
            const char* why = nullptr;
            bool is_and = false;
            BlockId rhs_blk = 0, end_blk = 0;
            uint32_t end_start = 0;
            if (tb >= nb || fb >= nb) {
                why = "tb/fb oob";
            } else {
                const uint32_t tstart = facts.block_start[tb];
                const uint32_t fstart = facts.block_start[fb];
                if (tstart == rhs_start) {
                    is_and = true; rhs_blk = tb; end_blk = fb; end_start = fstart;
                } else if (fstart == rhs_start) {
                    is_and = false; rhs_blk = fb; end_blk = tb; end_start = tstart;
                } else {
                    why = "orientation";
                }
            }
            if (!why && (end_start < rhs_start + 2 || end_start > n_inst)) why = "end_start";
            if (!why && jump_count[rhs_blk] != 1) why = "jump_count";
            if (!why) {
                for (uint32_t j = rhs_start + 1; j < end_start; ++j) {
                    if (block_starts[j] && !logical_skip[j]) { why = "nested"; break; }
                }
            }
            std::string rhs_expr;
            bool ok = false;
            if (!why) {
                const MIRInstruction& last = function.instructions[end_start - 1];
                const MIRInstruction& pen = function.instructions[end_start - 2];
                const MIROperand* bt = last.opcode() == MIROpcode::Br
                                            ? operand_at(function, last, 0) : nullptr;
                const MIROperand* psp = pen.opcode() == MIROpcode::Store
                                             ? operand_at(function, pen, 0) : nullptr;
                const MIROperand* psv2 = pen.opcode() == MIROpcode::Store
                                             ? operand_at(function, pen, 1) : nullptr;
                const MIRInstruction& mload = function.instructions[end_start];
                const MIROperand* mp = mload.opcode() == MIROpcode::Load
                                           ? operand_at(function, mload, 0) : nullptr;
                const MIROperand* mf = mload.opcode() == MIROpcode::Load
                                           ? operand_at(function, mload, 1) : nullptr;
                if (last.opcode() != MIROpcode::Br) why = "last not br";
                else if (!bt || facts.resolve_block(bt->id) != end_blk) why = "br target";
                else if (pen.opcode() != MIROpcode::Store || pen.operand_count != 2) why = "pen not store";
                else if (!psp || psp->kind() != MIROperandKind::Place || psp->id != res) why = "pen place";
                else if (!psv2) why = "pen value";
                else if (mload.opcode() != MIROpcode::Load) why = "merge not load";
                else if (!mp || mp->kind() != MIROperandKind::Place || mp->id != res ||
                         (mf && (mf->kind() == MIROperandKind::Value ||
                                 mf->kind() == MIROperandKind::Constant)))
                    why = "merge place";
                else {
                    const ValueId mrv = mload.result_or_place;
                    if (mrv >= nv) {
                        why = "merge value";
                    } else {
                        ok = true;
                        saved.clear();
                        for (uint32_t j = rhs_start; j + 2 < end_start; ++j) {
                            if (logical_skip[j]) continue;
                            const MIRInstruction& in = function.instructions[j];
                            if (!is_expr_inst(in)) { why = "not expr"; break; }
                            const ValueId w = in.result_or_place;
                            if (w >= nv) { why = "w oob"; break; }
                            bool used_out = false;
                            for (uint32_t u : value_uses[w]) {
                                if (u < rhs_start || u >= end_start) { used_out = true; break; }
                            }
                            if (used_out) { why = "use outside"; break; }
                            // Recompute even when the generic pass already
                            // inlined this value: its stored expression may
                            // name an earlier value of this region that is
                            // about to be inlined as well (`v = cmp(call)` was
                            // spelled with `call` as a name before `call` was
                            // folded). Rebuilding sees the updated map.
                            std::string e = expr_of_inst(in);
                            if (e.empty()) { why = "expr empty"; break; }
                            saved.emplace_back(w, inline_expr[w]);
                            inline_expr[w] = std::move(e);
                        }
                    }
                }
            }
            if (ok && !why) {
                const MIROperand* bo = operand_at(function, function.instructions[end_start - 2], 1);
                rhs_expr = bo ? operand_expr(function, module, *bo) : std::string();
                if (rhs_expr.empty()) ok = false;
            }
            if (!ok || why) {
                for (auto& pr : saved) inline_expr[pr.first] = std::move(pr.second);
                continue;
            }
            {
                std::string lexpr = "(" + place_name(module, function, res) + ") ";
                lexpr += is_and ? "&&" : "||";
                lexpr += " (";
                lexpr += rhs_expr;
                lexpr += ")";
                const ValueId mrv = function.instructions[end_start].result_or_place;
                // Inline only when the value is read by the very next
                // instruction. Inlining at an arbitrary later use would move
                // the RHS *past* whatever sits between the merge point and
                // that use (e.g. a destructor: `return a && f(v)` would read
                // `v` after its delete). Otherwise materialize the expression
                // where the merge load was, which is the RHS block's own
                // position and preserves evaluation order exactly.
                // It must also be in the same block: a use that starts the
                // next block can be reached from another predecessor, where
                // the LHS place would not have been stored on this path.
                const bool inline_at_use =
                    value_uses[mrv].size() == 1 && value_uses[mrv][0] == end_start + 1 &&
                    !block_starts[end_start + 1];
                if (inline_at_use) {
                    // A single, adjacent use: fold the expression straight into
                    // it, so no temporary is materialized at all.
                    inline_expr[mrv] = lexpr;
                } else {
                    // Otherwise evaluate the RHS once, exactly where the merge
                    // load used to be, instead of recomputing it (and re-running
                    // any call in it) at every use.
                    inline_expr[mrv].clear();
                    logical_decl.emplace_back(end_start, lexpr);
                }
                logicals.push_back({end_start, mrv, std::move(lexpr), inline_at_use});
            }
            logical_skip[s + 1] = 1;
            for (uint32_t j = rhs_start; j < end_start; ++j) logical_skip[j] = 1;
        }
        if (!logicals.empty()) {
            // Rebuild every inline expression now that each merge value stands
            // for its short-circuit expression. Consumers inlined before the
            // reconstruction still name the old merge (`!p`, `p == x`, ...),
            // which would read only the LHS and silently drop the RHS.
            std::fill(inline_expr.begin(), inline_expr.end(), std::string());
            std::vector<uint8_t> frozen(function.values.size(), 0);
            for (const auto& L : logicals) {
                if (L.mrv >= frozen.size()) continue;
                frozen[L.mrv] = 1;
                if (L.inline_at_use) inline_expr[L.mrv] = L.expr;
            }
            run_inline_pass(frozen);
        }
        bool any = false;
        for (uint8_t b : logical_skip) {
            if (b) { any = true; break; }
        }
        if (any) {
            std::vector<uint8_t> live(nb, 0);
            for (uint32_t j = 0; j < n_inst; ++j) {
                if (logical_skip[j]) continue;
                const MIRInstruction& in = function.instructions[j];
                if (in.opcode() == MIROpcode::Br) {
                    const MIROperand* t = operand_at(function, in, 0);
                    if (t) live[facts.resolve_block(t->id)] = 1;
                } else if (in.opcode() == MIROpcode::CondBr) {
                    const MIROperand* t = operand_at(function, in, 1);
                    const MIROperand* f = operand_at(function, in, 2);
                    if (t) live[facts.resolve_block(t->id)] = 1;
                    if (f) live[facts.resolve_block(f->id)] = 1;
                }
            }
            for (uint32_t i = 0; i < n_inst; ++i) {
                if (!block_starts[i] || label_at[i] == MIR_INVALID_ID) continue;
                const BlockId b = block_at[i];
                if (b != MIR_INVALID_ID && !live[b]) suppress_label[i] = 1;
            }
        }
    }

    uint32_t param_index = 0;
    // Set when an `Alloca` folds its immediately-following plain `Store`
    // into the declaration, so the loop skips that store.
    uint32_t skip_inst = MIR_INVALID_ID;
    for (uint32_t idx = 0; idx < function.instructions.size(); ++idx) {
        if (label_at[idx] != MIR_INVALID_ID && !suppress_label[idx]) {
            out += "__chx_bb" + std::to_string(label_at[idx]) + ":;\n";
        }
        if (logical_skip[idx]) continue;
        if (idx == skip_inst) continue;
        // A trampoline block emits nothing (see MIRFacts): its single `Br` is a
        // fall-through and jumps to it were redirected to its target, so it has
        // no label and this instruction is dropped.
        if (idx < facts.inst_transparent_br.size() && facts.inst_transparent_br[idx]) {
            continue;
        }
        const MIRInstruction& inst = function.instructions[idx];
        // A single-use address/cast/load/param value was collapsed into its use.
        if ((inlineable_opcode(inst.opcode()) || inst.opcode() == MIROpcode::Param) &&
            inst.result_or_place < inline_expr.size() &&
            !inline_expr[inst.result_or_place].empty()) {
            continue;
        }
        // An aliased parameter place has no storage of its own: neither its
        // declaration nor its prologue spill store is emitted.
        if (inst.opcode() == MIROpcode::Alloca &&
            inst.result_or_place < facts.param_alias.size() &&
            facts.param_alias[inst.result_or_place] != MIR_INVALID_ID) {
            continue;
        }
        if (inst.opcode() == MIROpcode::Store) {
            const MIROperand* d = operand_at(function, inst, 0);
            if (d && d->kind() == MIROperandKind::Place &&
                d->id < facts.param_alias.size() &&
                facts.param_alias[d->id] != MIR_INVALID_ID) {
                continue;
            }
        }
        out += "    ";

        switch (inst.opcode()) {
            case MIROpcode::Alloca: {
                const PlaceId p = inst.result_or_place;
                // Fuse an immediately-following plain store into the
                // declaration: `T __chx_pN; __chx_pN = <rhs>;` becomes
                // `T __chx_pN = <rhs>;`. The store is adjacent and
                // unconditional, so this is the same program with one line
                // fewer. The lowering materializes a fresh local for nearly
                // every expression and short-circuit condition, so this is
                // one of the largest sources of flat-C lines (evaluation §C).
                bool fused = false;
                if (idx + 1 < function.instructions.size()) {
                    const MIRInstruction& nxt = function.instructions[idx + 1];
                    if (nxt.opcode() == MIROpcode::Store) {
                        const MIROperand* sp = operand_at(function, nxt, 0);
                        const MIROperand* sv = operand_at(function, nxt, 1);
                        const MIROperand* sv2 = operand_at(function, nxt, 2);
                        if (sp && sv && !sv2 && sp->kind() == MIROperandKind::Place &&
                            sp->id == p &&
                            (sv->kind() == MIROperandKind::Value ||
                             sv->kind() == MIROperandKind::Constant) &&
                            module.types.get(function.places[p].type).kind != MIRTypeKind::Array) {
                            append_c_declarator(module, function.places[p].type,
                                                place_name(module, function, p), out);
                            out += " = ";
                            append_operand_expr(function, module, *sv, out);
                            out += ";\n";
                            skip_inst = idx + 1;
                            fused = true;
                        }
                    }
                }
                if (!fused) {
                    append_c_declarator(module, function.places[p].type,
                                        place_name(module, function, p), out);
                    out += ";\n";
                }
                // An aggregate is *not* zeroed here unconditionally. The lowerer
                // emits an explicit `MemSet` (`zero_init`) at exactly the sites
                // that need well-defined tail bytes (a partial aggregate / array
                // initializer, a struct temp with unset members, a struct's
                // defaulted fields). Zeroing every aggregate alloca as well
                // duplicated that work (two byte loops per temp) and was the
                // single largest source of generated-C waste (evaluation §1.2).
                break;
            }
            case MIROpcode::Param: {
                const ValueId v = inst.result_or_place;
                const uint32_t pai = param_idx_at[idx] != MIR_INVALID_ID
                                         ? param_idx_at[idx] : param_index++;
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = __chx_a";
                append_uint(out, pai);
                out += ";\n";
                break;
            }
            case MIROpcode::ConstInt:
            case MIROpcode::ConstBool:
            case MIROpcode::ConstFloat:
            case MIROpcode::ConstDouble:
            case MIROpcode::ConstString:
            case MIROpcode::ConstNull: {
                const ValueId v = inst.result_or_place;
                const MIROperand* c = operand_at(function, inst, 0);
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = ";
                if (!c || !append_const_literal(module, c->id, function.values[v].type, out)) {
                    error = "invalid constant operand";
                    return false;
                }
                out += ";\n";
                break;
            }
            case MIROpcode::Load: {
                const ValueId v = inst.result_or_place;
                // A reconstructed short-circuit merge: the load is replaced by
                // the `a && b` / `a || b` expression its value stands for.
                const std::string* logical = nullptr;
                for (const auto& ld : logical_decl) {
                    if (ld.first == idx) { logical = &ld.second; break; }
                }
                if (logical) {
                    append_vname_declarator(module, function.values[v].type, v, out);
                    out += " = ";
                    out += *logical;
                    out += ";\n";
                    break;
                }
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* fld = operand_at(function, inst, 1);
                if (!p) { error = "load missing place"; return false; }
                if (p->kind() == MIROperandKind::Value) {
                    if (fld && fld->kind() == MIROperandKind::Value) {
                        const bool base_is_array =
                            p->type() < module.types.size() &&
                            module.types.get(p->type()).kind == MIRTypeKind::Array;
                        append_vname_declarator(module, function.values[v].type, v, out);
                        out += " = ";
                        if (base_is_array) out += "(*";
                        append_operand_expr(function, module, *p, out);
                        if (base_is_array) out += ")";
                        out += "[";
                        append_operand_expr(function, module, *fld, out);
                        out += "];\n";
                    } else if (fld && fld->kind() == MIROperandKind::Constant) {
                        append_vname_declarator(module, function.values[v].type, v, out);
                        out += " = ";
                        append_operand_expr(function, module, *p, out);
                        out += "->";
                        if (!append_constant_string(module, fld->id, out)) {
                            error = "field load has invalid field name";
                            return false;
                        }
                        out += ";\n";
                    } else {
                        append_vname_declarator(module, function.values[v].type, v, out);
                        out += " = *";
                        append_operand_expr(function, module, *p, out);
                        out += ";\n";
                    }
                } else if (fld && fld->kind() == MIROperandKind::Value) {
                    append_vname_declarator(module, function.values[v].type, v, out);
                    out += " = ";
                    append_place_name(module, function, p->id, out);
                    out += "[";
                    append_operand_expr(function, module, *fld, out);
                    out += "];\n";
                } else if (fld && fld->kind() == MIROperandKind::Constant) {
                    std::string fname;
                    if (!constant_string(module, fld->id, fname)) {
                        error = "field load has invalid field name";
                        return false;
                    }
                    append_vname_declarator(module, function.values[v].type, v, out);
                    out += " = ";
                    append_member_access(function, module, p->id, fname, out);
                    out += ";\n";
                } else {
                    const TypeId vt = function.values[v].type;
                    if (vt != MIR_INVALID_ID && vt < module.types.size() &&
                        module.types.get(vt).kind == MIRTypeKind::Array) {
                        // arrays are not assignable in C: declare then memcpy
                        append_vname_declarator(module, vt, v, out);
                        out += ";\n    memcpy(";
                        append_vname(v, out);
                        out += ", ";
                        append_place_name(module, function, p->id, out);
                        out += ", sizeof(";
                        append_vname(v, out);
                        out += "));\n";
                    } else {
                        append_vname_declarator(module, function.values[v].type, v, out);
                        out += " = ";
                        append_place_name(module, function, p->id, out);
                        out += ";\n";
                    }
                }
                break;
            }
            case MIROpcode::Store: {
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* a = operand_at(function, inst, 1);
                const MIROperand* b = operand_at(function, inst, 2);
                if (!p || !a) { error = "store missing operands"; return false; }
                if (p->kind() == MIROperandKind::Value) {
                    if (a->kind() == MIROperandKind::Value && b) {
                        const bool base_is_array =
                            p->type() < module.types.size() &&
                            module.types.get(p->type()).kind == MIRTypeKind::Array;
                        if (base_is_array) out += "(*";
                        append_operand_expr(function, module, *p, out);
                        if (base_is_array) out += ")";
                        out += "[";
                        append_operand_expr(function, module, *a, out);
                        out += "] = ";
                        append_operand_expr(function, module, *b, out);
                        out += ";\n";
                    } else if (a->kind() == MIROperandKind::Constant && b) {
                        std::string access;
                        append_operand_expr(function, module, *p, access);
                        access += "->";
                        if (!append_constant_string(module, a->id, access)) {
                            error = "field store has invalid field name";
                            return false;
                        }
                        const TypeId bt =
                            b->kind() == MIROperandKind::Value && b->id < function.values.size()
                                ? function.values[b->id].type : MIR_INVALID_ID;
                        if (bt != MIR_INVALID_ID && bt < module.types.size() &&
                            module.types.get(bt).kind == MIRTypeKind::Array) {
                            // arrays are not assignable: copy the source bytes
                            out += "memcpy(&";
                            out += access;
                            out += ", ";
                            append_operand_expr(function, module, *b, out);
                            out += ", sizeof(";
                            out += access;
                            out += "));\n";
                        } else {
                            out += access;
                            out += " = ";
                            append_operand_expr(function, module, *b, out);
                            out += ";\n";
                        }
                    } else {
                        out += "*";
                        append_operand_expr(function, module, *p, out);
                        out += " = ";
                        if (a->kind() == MIROperandKind::Place) {
                            const MIRTypeRecord& atr =
                                module.types.get(function.places[a->id].type);
                            if (atr.kind == MIRTypeKind::Pointer ||
                                atr.kind == MIRTypeKind::Reference) {
                                out += "*";
                            }
                            append_place_name(module, function, a->id, out);
                        } else {
                            append_operand_expr(function, module, *a, out);
                        }
                        out += ";\n";
                    }
                } else if (a->kind() == MIROperandKind::Value && b) {
                    if (b->kind() == MIROperandKind::Place) {
                        const MIRTypeRecord& btr = module.types.get(function.places[b->id].type);
                        if (btr.kind == MIRTypeKind::Array) {
                            // arrays are not assignable: copy the element bytes
                            out += "memcpy(&";
                            append_place_name(module, function, p->id, out);
                            out += "[";
                            append_operand_expr(function, module, *a, out);
                            out += "], ";
                            append_place_name(module, function, b->id, out);
                            out += ", sizeof(";
                            append_place_name(module, function, b->id, out);
                            out += "));\n";
                            break;
                        }
                    }
                    append_place_name(module, function, p->id, out);
                    out += "[";
                    append_operand_expr(function, module, *a, out);
                    out += "] = ";
                    append_operand_expr(function, module, *b, out);
                    out += ";\n";
                } else if (a->kind() == MIROperandKind::Constant && b) {
                    std::string fname;
                    if (!constant_string(module, a->id, fname)) {
                        error = "field store has invalid field name";
                        return false;
                    }
                    if (b->kind() == MIROperandKind::Value) {
                        const TypeId bt = function.values[b->id].type;
                        if (bt != MIR_INVALID_ID && bt < module.types.size() &&
                            module.types.get(bt).kind == MIRTypeKind::Array) {
                            // arrays are not assignable: copy the source bytes
                            std::string access;
                            append_member_access(function, module, p->id, fname, access);
                            out += "memcpy(&";
                            out += access;
                            out += ", ";
                            append_operand_expr(function, module, *b, out);
                            out += ", sizeof(";
                            out += access;
                            out += "));\n";
                            break;
                        }
                        append_member_access(function, module, p->id, fname, out);
                        out += " = ";
                        append_operand_expr(function, module, *b, out);
                        out += ";\n";
                        break;
                    }
                    if (b->kind() == MIROperandKind::Place) {
                        const MIRTypeRecord& btr = module.types.get(function.places[b->id].type);
                        if (btr.kind == MIRTypeKind::Array) {
                            // arrays are not assignable: copy the source bytes
                            std::string access;
                            append_member_access(function, module, p->id, fname, access);
                            out += "memcpy(&";
                            out += access;
                            out += ", &";
                            append_place_name(module, function, b->id, out);
                            out += ", sizeof(";
                            append_place_name(module, function, b->id, out);
                            out += "));\n";
                            break;
                        }
                        // aggregate parameters are pointer-typed places: deref
                        append_member_access(function, module, p->id, fname, out);
                        out += " = ";
                        if (btr.kind == MIRTypeKind::Pointer ||
                            btr.kind == MIRTypeKind::Reference) {
                            out += "*";
                        }
                        append_place_name(module, function, b->id, out);
                        out += ";\n";
                        break;
                    }
                    append_member_access(function, module, p->id, fname, out);
                    out += " = ";
                    append_operand_expr(function, module, *b, out);
                    out += ";\n";
                } else {
                    // arrays are not assignable in C: copy the source bytes
                    // (defensive: a bogus place id would index out of range)
                    if (p->id >= function.places.size()) {
                        error = "invalid place id during C emission";
                        return false;
                    }
                    const TypeId dst_t = function.places[p->id].type;
                    if (dst_t != MIR_INVALID_ID && dst_t < module.types.size() &&
                        module.types.get(dst_t).kind == MIRTypeKind::Array) {
                        out += "memcpy(&";
                        append_place_name(module, function, p->id, out);
                        out += ", ";
                        if (a->kind() == MIROperandKind::Place) {
                            const MIRTypeRecord& atr =
                                module.types.get(function.places[a->id].type);
                            if (atr.kind == MIRTypeKind::Pointer ||
                                atr.kind == MIRTypeKind::Reference) {
                                out += "*";
                                append_place_name(module, function, a->id, out);
                            } else {
                                out += "&";
                                append_place_name(module, function, a->id, out);
                            }
                        } else {
                            append_operand_expr(function, module, *a, out);
                        }
                        out += ", sizeof(";
                        append_place_name(module, function, p->id, out);
                        out += "));\n";
                    } else {
                        append_place_name(module, function, p->id, out);
                        out += " = ";
                        if (a->kind() == MIROperandKind::Place) {
                            const MIRTypeRecord& atr =
                                module.types.get(function.places[a->id].type);
                            if (atr.kind == MIRTypeKind::Pointer ||
                                atr.kind == MIRTypeKind::Reference) {
                                out += "*";
                            }
                            append_place_name(module, function, a->id, out);
                        } else {
                            append_operand_expr(function, module, *a, out);
                        }
                        out += ";\n";
                    }
                }
                break;
            }
            case MIROpcode::FieldAddr: {
                const ValueId v = inst.result_or_place;
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* fld = operand_at(function, inst, 1);
                if (!p || !fld) { error = "field_addr missing operands"; return false; }
                std::string fname;
                if (!constant_string(module, fld->id, fname)) {
                    error = "field_addr invalid field name";
                    return false;
                }
                const bool ptr = p->kind() != MIROperandKind::Place;
                // a place whose own type is a pointer/reference is dereferenced
                bool arrow = ptr;
                if (!ptr) {
                    const MIRTypeRecord& bt = module.types.get(function.places[p->id].type);
                    arrow = (bt.kind == MIRTypeKind::Pointer ||
                             bt.kind == MIRTypeKind::Reference);
                }
                std::string access;
                if (ptr) append_operand_expr(function, module, *p, access);
                else append_place_name(module, function, p->id, access);
                access += arrow ? "->" : ".";
                access += fname;
                const TypeId ft = function.values[v].type;
                if (ft < module.types.size() &&
                    module.types.get(ft).kind == MIRTypeKind::Array) {
                    // pointer-to-array: `T (*v)[d0][d1]...` with the dimensions
                    // in outer-to-inner order (delegated to declarator).
                    std::string dname = "(*";
                    append_vname(v, dname);
                    dname += ")";
                    append_c_declarator(module, ft, dname, out);
                    out += " = &";
                    out += access;
                    out += ";\n";
                } else {
                    // __typeof__ lets us take the address of a field whose type has
                    // no spellable C name (anonymous structs/unions)
                    out += "__typeof__(";
                    out += access;
                    out += ")* ";
                    append_vname(v, out);
                    out += " = &";
                    out += access;
                    out += ";\n";
                }
                break;
            }
            case MIROpcode::IndexAddr: {
                const ValueId v = inst.result_or_place;
                const MIROperand* b = operand_at(function, inst, 0);
                const MIROperand* idx = operand_at(function, inst, 1);
                if (!b || !idx) { error = "index_addr missing operands"; return false; }
                std::string access;
                if (b->kind() == MIROperandKind::Place) {
                    append_place_name(module, function, b->id, access);
                } else {
                    // an array-typed value is spelled as a pointer-to-array in C
                    // (`char (*v)[N]`); index it through a deref so the element
                    // stride is the element size, not the whole array size.
                    const bool base_is_array =
                        b->type() < module.types.size() &&
                        module.types.get(b->type()).kind == MIRTypeKind::Array;
                    if (base_is_array) access += "(*";
                    append_operand_expr(function, module, *b, access);
                    if (base_is_array) access += ")";
                }
                access += "[";
                append_operand_expr(function, module, *idx, access);
                access += "]";
                // __typeof__ handles element types with no spellable C name
                out += "__typeof__(";
                out += access;
                out += ")* ";
                append_vname(v, out);
                out += " = &";
                out += access;
                out += ";\n";
                break;
            }
            case MIROpcode::Borrow:
            case MIROpcode::BorrowMut: {
                const ValueId v = inst.result_or_place;
                const MIROperand* a = operand_at(function, inst, 0);
                if (!a) { error = "borrow missing address"; return false; }
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = ";
                out += operand_expr(function, module, *a);
                out += ";\n";
                break;
            }
            case MIROpcode::AddressOf: {
                const ValueId v = inst.result_or_place;
                const MIROperand* p = operand_at(function, inst, 0);
                if (!p) { error = "address_of missing place"; return false; }
                const TypeId pt = p->id < function.places.size()
                                      ? function.places[p->id].type
                                      : MIR_INVALID_ID;
                if (pt != MIR_INVALID_ID && pt < module.types.size() &&
                    module.types.get(pt).kind == MIRTypeKind::Array) {
                    // an array decays to a pointer to its first element
                    append_vname_declarator(module, function.values[v].type, v, out);
                    out += " = ";
                    append_place_name(module, function, p->id, out);
                    out += ";\n";
                } else {
                    append_vname_declarator(module, function.values[v].type, v, out);
                    out += " = &";
                    append_place_name(module, function, p->id, out);
                    out += ";\n";
                }
                break;
            }
            case MIROpcode::Unary: {
                const ValueId v = inst.result_or_place;
                const MIROperand* a = operand_at(function, inst, 0);
                const MIROperand* opc = operand_at(function, inst, 1);
                if (!a || !opc || opc->id >= module.constants.size()) {
                    error = "unary missing operands";
                    return false;
                }
                const auto op = static_cast<MIRUnaryOp>(module.constants.get(opc->id).bits);
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = (";
                out += unop_c(op);
                append_operand_expr(function, module, *a, out);
                out += ");\n";
                break;
            }
            case MIROpcode::Binary:
            case MIROpcode::Compare: {
                const ValueId v = inst.result_or_place;
                const MIROperand* a = operand_at(function, inst, 0);
                const MIROperand* b = operand_at(function, inst, 1);
                const MIROperand* opc = operand_at(function, inst, 2);
                if (!a || !b || !opc || opc->id >= module.constants.size()) {
                    error = "binary/compare missing operands";
                    return false;
                }
                const auto op = static_cast<MIRBinaryOp>(module.constants.get(opc->id).bits);
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = (";
                append_operand_expr(function, module, *a, out);
                out += " ";
                out += binop_c(op);
                out += " ";
                append_operand_expr(function, module, *b, out);
                out += ");\n";
                break;
            }
            case MIROpcode::Cast: {
                const ValueId v = inst.result_or_place;
                const MIROperand* a = operand_at(function, inst, 0);
                if (!a) { error = "cast missing operand"; return false; }
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = (";
                append_c_type_of(module, function.values[v].type, out);
                out += ")";
                append_operand_expr(function, module, *a, out);
                out += ";\n";
                break;
            }
            case MIROpcode::Call: {
                const MIROperand* callee = operand_at(function, inst, 0);
                if (!callee || callee->kind() != MIROperandKind::Symbol) {
                    error = "call missing callee symbol";
                    return false;
                }
                // sret if the operand after the symbol is a place
                const MIROperand* maybe_place = operand_at(function, inst, 1);
                const bool sret = maybe_place && maybe_place->kind() == MIROperandKind::Place;
                uint32_t arg_start = sret ? 2 : 1;

                std::string call;
                append_symbol_name(module, callee->id, call);
                call += "(";
                if (sret) {
                    call += "&";
                    append_place_name(module, function, maybe_place->id, call);
                }
                for (uint32_t a = arg_start; a < inst.operand_count; ++a) {
                    if (a != arg_start || sret) call += ", ";
                    append_operand_expr(function, module, *operand_at(function, inst, a), call);
                }
                call += ")";

                const bool is_void = sret ||
                    inst.result_or_place >= function.values.size() ||
                    c_type_spells_void(module, function.values[inst.result_or_place].type);
                if (sret) {
                    out += call;
                    out += ";\n";
                } else if (!is_void) {
                    append_vname_declarator(module, function.values[inst.result_or_place].type,
                                      inst.result_or_place, out);
                    out += " = ";
                    out += call;
                    out += ";\n";
                } else {
                    out += call;
                    out += ";\n";
                }
                break;
            }
            case MIROpcode::Return: {
                if (inst.operand_count == 0) {
                    out += "return;\n";
                } else {
                    const MIROperand* v = operand_at(function, inst, 0);
                    out += "return ";
                    append_operand_expr(function, module, *v, out);
                    out += ";\n";
                }
                break;
            }
            case MIROpcode::CopyInit:
            case MIROpcode::MoveInit: {
                const MIROperand* dest = operand_at(function, inst, 0);
                const MIROperand* src = operand_at(function, inst, 1);
                if (!dest || !src) { error = "copy/move_init missing operands"; return false; }
                std::string rhs;
                if (src->kind() == MIROperandKind::Place) {
                    append_place_name(module, function, src->id, rhs);
                } else {
                    append_operand_expr(function, module, *src, rhs);
                }
                bool deref_src = false;
                if (src->kind() == MIROperandKind::Place) {
                    const MIRTypeRecord& str = module.types.get(function.places[src->id].type);
                    if (str.kind == MIRTypeKind::Pointer ||
                        str.kind == MIRTypeKind::Reference) {
                        deref_src = true;
                    }
                }
                const TypeId dt = dest->id < function.places.size()
                                      ? function.places[dest->id].type
                                      : MIR_INVALID_ID;
                if (dt != MIR_INVALID_ID && dt < module.types.size() &&
                    module.types.get(dt).kind == MIRTypeKind::Array) {
                    // arrays are not assignable in C: copy the bytes instead
                    out += "memcpy(&";
                    append_place_name(module, function, dest->id, out);
                    out += ", ";
                    if (deref_src) out += "*";
                    else out += "&";
                    out += rhs;
                    out += ", sizeof(";
                    append_place_name(module, function, dest->id, out);
                    out += "));\n";
                } else {
                    append_place_name(module, function, dest->id, out);
                    out += " = ";
                    if (deref_src) out += "*";
                    out += rhs;
                    out += ";\n";
                }
                break;
            }
            case MIROpcode::MoveOut:
                // dataflow marker only: the value is consumed by the call
                break;
            case MIROpcode::StorageLive:
            case MIROpcode::StorageDead:
                // lexical storage markers: no runtime effect
                break;
            case MIROpcode::Drop: {
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* d = operand_at(function, inst, 1);
                const MIROperand* f = operand_at(function, inst, 2);
                if (!p || !d) { error = "drop missing operands"; return false; }
                const MIRTypeRecord& dpr = module.types.get(function.places[p->id].type);
                if (dpr.kind == MIRTypeKind::Array) {
                    // drop each element, last to first
                    out += "{ ";
                    if (f) {
                        out += "if (";
                        append_operand_expr(function, module, *f, out);
                        out += ") ";
                    }
                    out += "for (unsigned long __chx_di";
                    append_uint(out, idx);
                    out += " = ";
                    append_uint(out, dpr.data_count);
                    out += "; __chx_di";
                    append_uint(out, idx);
                    out += "-- > 0;) ";
                    append_symbol_name(module, d->id, out);
                    out += "(&";
                    append_place_name(module, function, p->id, out);
                    out += "[__chx_di";
                    append_uint(out, idx);
                    out += "]); }\n";
                    break;
                }
                if (f) {
                    out += "if (";
                    append_operand_expr(function, module, *f, out);
                    out += ") ";
                }
                append_symbol_name(module, d->id, out);
                out += "(";
                if (!(dpr.kind == MIRTypeKind::Pointer || dpr.kind == MIRTypeKind::Reference)) {
                    out += "&";
                }
                append_place_name(module, function, p->id, out);
                out += ");\n";
                break;
            }
            case MIROpcode::SetDrop: {
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* c = operand_at(function, inst, 1);
                if (!p || !c) { error = "set_drop missing operands"; return false; }
                append_place_name(module, function, p->id, out);
                out += " = ";
                append_operand_expr(function, module, *c, out);
                out += ";\n";
                break;
            }
            case MIROpcode::Destroy: {
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* d = operand_at(function, inst, 1);
                if (!p || !d || d->kind() != MIROperandKind::Symbol) {
                    error = "destroy missing operands";
                    return false;
                }
                const MIRTypeRecord& dpr = module.types.get(function.places[p->id].type);
                if (dpr.kind == MIRTypeKind::Array) {
                    // destroy each element, last to first (coroutine frame slots
                    // of an array type are not destroyed by a single call)
                    out += "{ for (unsigned long __chx_di";
                    append_uint(out, idx);
                    out += " = ";
                    append_uint(out, dpr.data_count);
                    out += "; __chx_di";
                    append_uint(out, idx);
                    out += "-- > 0;) ";
                    append_symbol_name(module, d->id, out);
                    out += "(&";
                    append_place_name(module, function, p->id, out);
                    out += "[__chx_di";
                    append_uint(out, idx);
                    out += "]); }\n";
                    break;
                }
                append_symbol_name(module, d->id, out);
                out += "(";
                if (!(dpr.kind == MIRTypeKind::Pointer || dpr.kind == MIRTypeKind::Reference)) {
                    out += "&";
                }
                append_place_name(module, function, p->id, out);
                out += ");\n";
                break;
            }
            case MIROpcode::MemCpy: {
                const MIROperand* dest = operand_at(function, inst, 0);
                const MIROperand* src = operand_at(function, inst, 1);
                if (!dest || !src) { error = "memcpy missing operands"; return false; }
                const MIROperand* sz = operand_at(function, inst, 2);
                out += "memcpy(&";
                append_place_name(module, function, dest->id, out);
                out += ", ";
                append_operand_expr(function, module, *src, out);
                out += ", ";
                if (sz) {
                    append_operand_expr(function, module, *sz, out);
                } else {
                    out += "sizeof(";
                    append_place_name(module, function, dest->id, out);
                    out += ")";
                }
                out += ");\n";
                break;
            }
            case MIROpcode::MemSet: {
                const MIROperand* dest = operand_at(function, inst, 0);
                if (!dest) { error = "memset missing destination"; return false; }
                const MIROperand* byte = operand_at(function, inst, 1);
                const MIROperand* size = operand_at(function, inst, 2);
                // `zero_init(place)` is the short shape the lowerer emits: the
                // whole place, zeroed. A scalar is a single store; everything
                // that lives in memory is a real `memset`, which the C compiler
                // lowers to width-sized stores (or a libcall) rather than the
                // per-byte loop this used to spell out. That loop was a measured
                // runtime cost: it sits *inside* `vector<T>::resize`'s growth
                // loop, once per element — 64 byte-stores per element for a
                // 64-byte T. `memcpy` is already emitted bare here, so `memset`
                // needs no new declaration or include.
                //
                // `memset_ptr` is the general shape: a byte and a size operand,
                // with the destination an address value (a field inside an
                // anonymous aggregate) rather than a place.
                if (dest->kind() == MIROperandKind::Place && !byte && !size) {
                    bool scalar = false;
                    if (dest->id < function.places.size()) {
                        const TypeId dt = function.places[dest->id].type;
                        if (dt != MIR_INVALID_ID && dt < module.types.size()) {
                            const MIRTypeKind k = module.types.get(dt).kind;
                            scalar = k == MIRTypeKind::Bool || k == MIRTypeKind::Int ||
                                     k == MIRTypeKind::Float || k == MIRTypeKind::Pointer ||
                                     k == MIRTypeKind::Reference;
                        }
                    }
                    if (scalar) {
                        append_place_name(module, function, dest->id, out);
                        out += " = 0;\n";
                    } else {
                        out += "memset(&";
                        append_place_name(module, function, dest->id, out);
                        out += ", 0, sizeof(";
                        append_place_name(module, function, dest->id, out);
                        out += "));\n";
                    }
                    break;
                }
                out += "memset(";
                if (dest->kind() == MIROperandKind::Place) {
                    out += "&";
                    append_place_name(module, function, dest->id, out);
                } else {
                    append_operand_expr(function, module, *dest, out);
                }
                out += ", ";
                if (byte) append_operand_expr(function, module, *byte, out);
                else out += "0";
                out += ", ";
                if (size) {
                    append_operand_expr(function, module, *size, out);
                } else if (dest->kind() == MIROperandKind::Place) {
                    out += "sizeof(";
                    append_place_name(module, function, dest->id, out);
                    out += ")";
                } else {
                    error = "memset with an address destination needs a size operand";
                    return false;
                }
                out += ");\n";
                break;
            }
            case MIROpcode::FunctionAddr: {
                const ValueId v = inst.result_or_place;
                const MIROperand* s = operand_at(function, inst, 0);
                if (!s) { error = "function_addr missing symbol"; return false; }
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = ";
                append_symbol_name(module, s->id, out);
                out += ";\n";
                break;
            }
            case MIROpcode::GlobalAddr: {
                const ValueId v = inst.result_or_place;
                const MIROperand* s = operand_at(function, inst, 0);
                if (!s) { error = "global_addr missing symbol"; return false; }
                // an array global decays to a pointer to its first element
                bool decay = false;
                if (s->id < module.symbols.size()) {
                    const TypeId st = module.symbols.get(s->id).type;
                    if (st < module.types.size() &&
                        module.types.get(st).kind == MIRTypeKind::Array) {
                        decay = true;
                    }
                }
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = ";
                if (!decay) out += "&";
                append_symbol_name(module, s->id, out);
                out += ";\n";
                break;
            }
            case MIROpcode::Init: {
                const MIROperand* dest = operand_at(function, inst, 0);
                const MIROperand* ctor = operand_at(function, inst, 1);
                if (!dest || !ctor || ctor->kind() != MIROperandKind::Symbol) {
                    error = "init missing destination/constructor";
                    return false;
                }
                append_symbol_name(module, ctor->id, out);
                out += "(&";
                append_place_name(module, function, dest->id, out);
                for (uint32_t a = 2; a < inst.operand_count; ++a) {
                    out += ", ";
                    append_operand_expr(function, module, *operand_at(function, inst, a), out);
                }
                out += ");\n";
                break;
            }
            case MIROpcode::CallIndirect: {
                const MIROperand* fn = operand_at(function, inst, 0);
                if (!fn) { error = "call_indirect missing callee"; return false; }
                std::string callee;
                append_operand_expr(function, module, *fn, callee);
                // the callee value is a function pointer; give it its precise
                // function-pointer type so the call type-checks
                if (fn->type() < module.types.size()) {
                    const MIRTypeRecord& fr = module.types.get(fn->type());
                    if (fr.kind == MIRTypeKind::Function) {
                        std::string typed = "((";
                        append_fn_ptr_type(module, fr, typed);
                        typed += ")";
                        typed += callee;
                        typed += ")";
                        callee = std::move(typed);
                    }
                }
                // sret if the operand after the callee is a place (aggregate return)
                const MIROperand* maybe_place = operand_at(function, inst, 1);
                const bool sret = maybe_place && maybe_place->kind() == MIROperandKind::Place;
                uint32_t arg_start = sret ? 2 : 1;
                std::string call = callee;
                call += "(";
                if (sret) {
                    call += "&";
                    append_place_name(module, function, maybe_place->id, call);
                }
                for (uint32_t a = arg_start; a < inst.operand_count; ++a) {
                    if (a != arg_start || sret) call += ", ";
                    append_operand_expr(function, module, *operand_at(function, inst, a), call);
                }
                call += ")";
                if (sret) {
                    out += call;
                    out += ";\n";
                    break;
                }
                const bool is_void = inst.result_or_place >= function.values.size() ||
                    c_type_spells_void(module, function.values[inst.result_or_place].type);
                if (is_void) {
                    out += call;
                    out += ";\n";
                } else {
                    append_vname_declarator(module, function.values[inst.result_or_place].type,
                                      inst.result_or_place, out);
                    out += " = ";
                    out += call;
                    out += ";\n";
                }
                break;
            }
            case MIROpcode::Br: {
                const MIROperand* target = operand_at(function, inst, 0);
                if (!target) { error = "br missing target"; return false; }
                // A branch to the next block needs no jump: control falls into it.
                if (!facts.is_fallthrough_inst(function, idx, target->id)) {
                    out += "goto __chx_bb";
                    append_uint(out, facts.resolve_block(target->id));
                    out += ";\n";
                }
                break;
            }
            case MIROpcode::CondBr: {
                const MIROperand* cond = operand_at(function, inst, 0);
                const MIROperand* t = operand_at(function, inst, 1);
                const MIROperand* f = operand_at(function, inst, 2);
                if (!cond || !t || !f) { error = "cond_br missing operands"; return false; }
                const BlockId tb = facts.resolve_block(t->id);
                const BlockId fb = facts.resolve_block(f->id);
                std::string cexpr;
                // Short-circuit lowering stores the condition to a place and
                // branches on the same value (`p = a; cond_br a`). The value is
                // still inlined at the branch, so the emitter would evaluate the
                // whole condition a second time (`p = a < b; if (!(a < b))`).
                // The store is adjacent and unconditional in the same block, so
                // the place provably holds exactly this value: read it instead.
                // This removes a duplicated comparison/shift chain per branch
                // (e.g. every `&&`/`||` operand), which the C backend cannot
                // recover because the generated C is compiled without an
                // optimizer.
                bool cond_from_place = false;
                std::string cond_place;
                if (cond->kind() == MIROperandKind::Value && idx > 0 &&
                    !block_starts[idx]) {
                    const MIRInstruction& prev = function.instructions[idx - 1];
                    if (prev.opcode() == MIROpcode::Store && prev.operand_count == 2) {
                        const MIROperand* sp = operand_at(function, prev, 0);
                        const MIROperand* sv = operand_at(function, prev, 1);
                        if (sp && sv && sp->kind() == MIROperandKind::Place &&
                            sv->kind() == MIROperandKind::Value && sv->id == cond->id) {
                            append_place_name(module, function, sp->id, cond_place);
                            cond_from_place = true;
                        }
                    }
                }
                if (cond_from_place) {
                    // Only when the recomputed expression is substantial: for a
                    // bare two-operand comparison the C backend already has the
                    // operands live and re-evaluating it is cheaper than
                    // reloading the `_Bool` place (measured on the suite, where
                    // applying it unconditionally grew the executable by 0.06%).
                    append_operand_expr(function, module, *cond, cexpr);
                    if (cexpr.size() >= 40) cexpr = cond_place;
                } else {
                    append_operand_expr(function, module, *cond, cexpr);
                }
                if (facts.is_fallthrough_inst(function, idx, f->id)) {
                    // the false arm falls through, so only the true jump is emitted
                    out += "if (";
                    out += cexpr;
                    out += ") goto __chx_bb";
                    append_uint(out, tb);
                    out += ";\n";
                } else if (facts.is_fallthrough_inst(function, idx, t->id)) {
                    // the true arm falls through: jump on the negated condition
                    out += "if (!(";
                    out += cexpr;
                    out += ")) goto __chx_bb";
                    append_uint(out, fb);
                    out += ";\n";
                } else {
                    out += "if (";
                    out += cexpr;
                    out += ") goto __chx_bb";
                    append_uint(out, tb);
                    out += "; else goto __chx_bb";
                    append_uint(out, fb);
                    out += ";\n";
                }
                break;
            }
            case MIROpcode::Unreachable: {
                out += "; /* unreachable */\n";
                break;
            }
            case MIROpcode::SizeOf: {
                const ValueId v = inst.result_or_place;
                const MIROperand* t = operand_at(function, inst, 0);
                if (!t) { error = "size_of missing type"; return false; }
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = sizeof(";
                append_c_type_of(module, t->type(), out);
                out += ");\n";
                break;
            }
            case MIROpcode::AlignOf: {
                const ValueId v = inst.result_or_place;
                const MIROperand* t = operand_at(function, inst, 0);
                if (!t) { error = "align_of missing type"; return false; }
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = _Alignof(";
                append_c_type_of(module, t->type(), out);
                out += ");\n";
                break;
            }
            case MIROpcode::OffsetOf: {
                const ValueId v = inst.result_or_place;
                const MIROperand* t = operand_at(function, inst, 0);
                const MIROperand* f = operand_at(function, inst, 1);
                if (!t || !f) { error = "offset_of missing operands"; return false; }
                std::string fname;
                if (!constant_string(module, f->id, fname)) {
                    error = "offset_of invalid field name";
                    return false;
                }
                append_vname_declarator(module, function.values[v].type, v, out);
                out += " = offsetof(";
                append_c_type_of(module, t->type(), out);
                out += ", ";
                out += fname;
                out += ");\n";
                break;
            }
            case MIROpcode::AsyncAwait: {
                const MIROperand* fr = operand_at(function, inst, 0);
                const MIROperand* child = operand_at(function, inst, 1);
                const MIROperand* result = operand_at(function, inst, 2);
                const MIROperand* poll_ty = operand_at(function, inst, 3);
                const MIROperand* state = operand_at(function, inst, 4);
                const MIROperand* ret = operand_at(function, inst, 5);
                const MIROperand* fn_poll_ty = operand_at(function, inst, 6);
                const MIROperand* spill_n = operand_at(function, inst, 7);
                if (!fr || !child || !result || !poll_ty || !state || !ret || !fn_poll_ty ||
                    !spill_n) {
                    error = "async_await missing operands";
                    return false;
                }
                std::string fexpr, cexpr, rexpr, pexpr, pty, fpty;
                append_place_name(module, function, fr->id, fexpr);
                append_place_name(module, function, child->id, cexpr);
                append_place_name(module, function, result->id, rexpr);
                append_place_name(module, function, ret->id, pexpr);
                append_c_type_of(module, poll_ty->type(), pty);
                append_c_type_of(module, fn_poll_ty->type(), fpty);
                const uint32_t resume_state =
                    state->id < module.constants.size()
                        ? static_cast<uint32_t>(module.constants.get(state->id).bits)
                        : 0;
                out += "{\n        ";
                out += pty;
                out += " __chx__p = (*({ ";
                out += pty;
                out += " __chx__t; ";
                out += cexpr;
                out += ".vtbl->poll(&__chx__t, ";
                out += cexpr;
                out += ".frame, ";
                out += fexpr;
                out += "->__cx); &__chx__t; }));\n";
                out += "        if(__chx__p.__chx__vt_621827 != 0) {\n";
                if (resume_state != 0) {
                    out += "            ";
                    out += fexpr;
                    out += "->__state = ";
                    append_uint(out, resume_state);
                    out += "u;\n";
                }
                const uint32_t n =
                    spill_n->id < module.constants.size()
                        ? static_cast<uint32_t>(module.constants.get(spill_n->id).bits)
                        : 0;
                for (uint32_t k = 0; k < n; ++k) {
                    const MIROperand* loc = operand_at(function, inst, 8 + k * 2);
                    const MIROperand* fld = operand_at(function, inst, 9 + k * 2);
                    if (!loc || !fld) { error = "async_await bad spill operands"; return false; }
                    std::string fname;
                    if (!constant_string(module, fld->id, fname)) {
                        error = "async_await bad spill field";
                        return false;
                    }
                    out += "            ";
                    out += fexpr;
                    out += "->";
                    out += fname;
                    out += " = ";
                    append_place_name(module, function, loc->id, out);
                    out += ";\n";
                }
                out += "            *";
                out += pexpr;
                out += " = (";
                out += fpty;
                out += "){ .__chx__vt_621827 = 1 };\n";
                out += "            return;\n        }\n";
                out += "        ";
                out += rexpr;
                out += " = __chx__p.Ready.value;\n";
                out += "        ";
                out += cexpr;
                out += ".vtbl->drop(";
                out += cexpr;
                out += ".frame);\n";
                out += "        ";
                out += cexpr;
                out += ".frame = (void*) 0;\n    }\n";
                break;
            }
            case MIROpcode::AsyncFinish: {
                const MIROperand* fr = operand_at(function, inst, 0);
                const MIROperand* ret = operand_at(function, inst, 1);
                const MIROperand* fn_poll_ty = operand_at(function, inst, 2);
                if (!fr || !ret || !fn_poll_ty) {
                    error = "async_finish missing operands";
                    return false;
                }
                append_place_name(module, function, fr->id, out);
                out += "->__state = 0xFFFFFFFFu;\n        *";
                append_place_name(module, function, ret->id, out);
                out += " = (";
                append_c_type_of(module, fn_poll_ty->type(), out);
                out += "){ .__chx__vt_621827 = 0, .Ready.value = ";
                append_place_name(module, function, fr->id, out);
                out += "->__result };\n        return;\n";
                break;
            }
            case MIROpcode::AsyncFrameAlloc: {
                const MIROperand* frame_ty = operand_at(function, inst, 0);
                if (!frame_ty) { error = "async_frame_alloc missing type"; return false; }
                std::string fname;
                append_c_type_of(module, frame_ty->type(), fname);
                append_vname_declarator(module, function.values[inst.result_or_place].type,
                                  inst.result_or_place, out);
                out += " = (";
                out += fname;
                out += "*) chemical_async_frame_alloc(sizeof(";
                out += fname;
                out += "), _Alignof(";
                out += fname;
                out += "));\n";
                break;
            }
            case MIROpcode::AsyncRampFinish: {
                const MIROperand* fr = operand_at(function, inst, 0);
                const MIROperand* sret = operand_at(function, inst, 1);
                const MIROperand* state = operand_at(function, inst, 2);
                const MIROperand* vtbl = operand_at(function, inst, 3);
                const MIROperand* handle_ty = operand_at(function, inst, 4);
                if (!fr || !sret || !state || !vtbl || !handle_ty) {
                    error = "async_ramp_finish missing operands";
                    return false;
                }
                std::string vname2;
                if (!constant_string(module, vtbl->id, vname2)) {
                    error = "async_ramp_finish bad vtbl name";
                    return false;
                }
                const uint32_t st =
                    state->id < module.constants.size()
                        ? static_cast<uint32_t>(module.constants.get(state->id).bits)
                        : 0;
                append_place_name(module, function, fr->id, out);
                out += "->__state = ";
                append_uint(out, st);
                out += "u;\n";
                out += "        *";
                append_place_name(module, function, sret->id, out);
                out += " = (";
                append_c_type_of(module, handle_ty->type(), out);
                out += "){ .frame = (void*) ";
                append_place_name(module, function, fr->id, out);
                out += ", .vtbl = &";
                out += vname2;
                out += " };\n        return;\n";
                break;
            }
            case MIROpcode::InlineAsm: {
                uint32_t i = 0;
                const MIROperand* tmpl = operand_at(function, inst, i++);
                if (!tmpl) { error = "inline_asm missing template"; return false; }
                std::string tmpl_s;
                if (!constant_string(module, tmpl->id, tmpl_s)) {
                    error = "inline_asm invalid template";
                    return false;
                }
                auto read_count = [&](uint32_t& n) -> bool {
                    const MIROperand* c = operand_at(function, inst, i++);
                    if (!c || c->kind() != MIROperandKind::Constant ||
                        c->id >= module.constants.size()) {
                        return false;
                    }
                    n = static_cast<uint32_t>(module.constants.get(c->id).bits);
                    return true;
                };
                auto read_string = [&](std::string& s) -> bool {
                    const MIROperand* c = operand_at(function, inst, i++);
                    if (!c || c->kind() != MIROperandKind::Constant) return false;
                    return constant_string(module, c->id, s);
                };
                auto write_operand = [&](std::string& s) -> bool {
                    const MIROperand* o = operand_at(function, inst, i++);
                    if (!o) return false;
                    if (o->kind() == MIROperandKind::Place) {
                        s = place_name(module, function, o->id);
                        return true;
                    }
                    if (o->kind() == MIROperandKind::Value) {
                        s = "*" + operand_expr(function, module, *o);
                        return true;
                    }
                    return false;
                };
                // the template keeps its source escapes (`\n`, `\t`), so write it raw
                out += "__asm__ __volatile__(\"";
                out += tmpl_s;
                out += "\"";
                uint32_t n_out = 0, n_in = 0, n_clob = 0;
                out += " : ";
                if (!read_count(n_out)) { error = "inline_asm bad output count"; return false; }
                for (uint32_t k = 0; k < n_out; ++k) {
                    if (k) out += ", ";
                    std::string con, expr;
                    if (!read_string(con) || !write_operand(expr)) {
                        error = "inline_asm bad output operand";
                        return false;
                    }
                    out += "\"" + con + "\"(" + expr + ")";
                }
                out += " : ";
                if (!read_count(n_in)) { error = "inline_asm bad input count"; return false; }
                for (uint32_t k = 0; k < n_in; ++k) {
                    if (k) out += ", ";
                    std::string con, expr;
                    if (!read_string(con) || !write_operand(expr)) {
                        error = "inline_asm bad input operand";
                        return false;
                    }
                    out += "\"" + con + "\"(" + expr + ")";
                }
                if (!read_count(n_clob)) { error = "inline_asm bad clobber count"; return false; }
                if (n_clob > 0) {
                    out += " : ";
                    for (uint32_t k = 0; k < n_clob; ++k) {
                        if (k) out += ", ";
                        std::string con;
                        if (!read_string(con)) {
                            error = "inline_asm bad clobber";
                            return false;
                        }
                        out += "\"" + con + "\"";
                    }
                }
                out += ");\n";
                break;
            }
            case MIROpcode::DynamicCall: {
                const MIROperand* fat = operand_at(function, inst, 0);
                const MIROperand* vt = operand_at(function, inst, 1);
                const MIROperand* meth = operand_at(function, inst, 2);
                if (!fat || !vt || !meth) { error = "dynamic_call missing operands"; return false; }
                std::string vt_name, m_name;
                if (!constant_string(module, vt->id, vt_name) ||
                    !constant_string(module, meth->id, m_name)) {
                    error = "dynamic_call invalid names";
                    return false;
                }
                std::string access;
                if (fat->kind() == MIROperandKind::Place) {
                    append_place_name(module, function, fat->id, access);
                } else {
                    append_operand_expr(function, module, *fat, access);
                }
                const bool arrow =
                    fat->type() < module.types.size() &&
                    (module.types.get(fat->type()).kind == MIRTypeKind::Pointer ||
                     module.types.get(fat->type()).kind == MIRTypeKind::Reference);
                access += arrow ? "->" : ".";
                std::string call = "((";
                call += vt_name;
                call += "*)";
                call += access;
                call += "second)->";
                call += m_name;
                call += "(";
                call += access;
                call += "first";
                for (uint32_t a = 3; a < inst.operand_count; ++a) {
                    call += ", ";
                    append_operand_expr(function, module, *operand_at(function, inst, a), call);
                }
                call += ")";
                const bool is_void =
                    inst.result_or_place >= function.values.size() ||
                    c_type_spells_void(module, function.values[inst.result_or_place].type);
                if (is_void) {
                    out += call;
                    out += ";\n";
                } else {
                    append_vname_declarator(module, function.values[inst.result_or_place].type,
                                      inst.result_or_place, out);
                    out += " = ";
                    out += call;
                    out += ";\n";
                }
                break;
            }
            default:
                error = std::string("C emitter: unsupported opcode '") +
                        opcode_name(inst.opcode()) + "' (straight-line subset only)";
                return false;
        }
    }

    // A bare `return;` as the function's final statement is redundant: a C
    // function falls off its closing brace with the same effect. The lowering
    // emits one after every `Return` (void and hidden-sret alike), and the
    // legacy backend never printed a bare `return;` at all, so stripping the
    // trailing one removes a `return;` from every function that ends by
    // falling through — pure text, no semantics (evaluation §C). A `return;`
    // that must leave *before* the end is still emitted above.
    static constexpr char kTrailingVoidReturn[] = "    return;\n";
    static constexpr size_t kTrailingVoidReturnLen = sizeof(kTrailingVoidReturn) - 1;
    if (out.size() >= kTrailingVoidReturnLen &&
        out.compare(out.size() - kTrailingVoidReturnLen, kTrailingVoidReturnLen,
                    kTrailingVoidReturn) == 0) {
        out.resize(out.size() - kTrailingVoidReturnLen);
    }

    out += "}\n";
    return true;
}

} // namespace mir
