// Copyright (c) Chemical Language Foundation 2025.

#include "MIREmitter.h"

#include "MIRFacts.h"
#include "MIRMangleBuffer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
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
    const MIRSymbolRecord& r = module.symbols.get(sym);
    out.assign(module.symbols.name_data(r), r.name_length);
    return true;
}

bool constant_string(const MIRModule& module, ConstantId cid, std::string& out) {
    if (cid >= module.constants.size()) return false;
    const MIRConstant& c = module.constants.get(cid);
    if (c.kind != MIRConstantKind::String) return false;
    out.assign(module.constants.data.data_at(c.data_offset), c.data_count);
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
    const MIRSymbolRecord& r = module.symbols.get(sym);
    out.append(module.symbols.name_data(r), r.name_length);
    return true;
}

bool append_constant_string(const MIRModule& module, ConstantId cid, std::string& out) {
    if (cid >= module.constants.size()) return false;
    const MIRConstant& c = module.constants.get(cid);
    if (c.kind != MIRConstantKind::String) return false;
    out.append(module.constants.data.data_at(c.data_offset), c.data_count);
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

const MIROperand* operand_at(const MIRFunction& fn, const MIRInstruction& inst, uint32_t i) {
    if (i >= inst.operand_count) return nullptr;
    return &fn.operands[inst.operand_offset + i];
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

    for (size_t ii = 0; ii < function.instructions.size(); ++ii) {
        const MIRInstruction& inst = function.instructions[ii];
        const ValueId v = inst.result_or_place;
        if (v >= function.values.size()) continue;
        if (function.values[v].use_count != 1) continue;
        if (inst.opcode() == MIROpcode::Param) {
            inline_expr[v] = "__chx_a" + std::to_string(param_idx_at[ii]);
            continue;
        }
        if (inst.opcode() == MIROpcode::Load) {
            const MIROperand* p = operand_at(function, inst, 0);
            if (!p) continue;
            // a plain load's only other operand is the result Type; a field/index
            // load carries a Constant/Value and must not be inlined here.
            const MIROperand* f = operand_at(function, inst, 1);
            if (f && (f->kind() == MIROperandKind::Value ||
                      f->kind() == MIROperandKind::Constant)) {
                continue;
            }
            if (p->kind() == MIROperandKind::Place) {
                if (function.places[p->id].storage_class !=
                    static_cast<uint8_t>(MIRStorageClass::Parameter)) {
                    continue;
                }
                if (place_writes[p->id] > 1) continue; // reassigned: keep the load
                inline_expr[v] = place_name(module, function, p->id);
                continue;
            }
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
        if (!inlineable_opcode(inst.opcode())) continue;
        std::string e = expr_of_inst(inst);
        if (!e.empty()) inline_expr[v] = std::move(e);
    }

    uint32_t param_index = 0;
    for (uint32_t idx = 0; idx < function.instructions.size(); ++idx) {
        if (label_at[idx] != MIR_INVALID_ID) {
            out += "__chx_bb" + std::to_string(label_at[idx]) + ":;\n";
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
                append_c_declarator(module, function.places[p].type, place_name(module, function, p), out);
                out += ";\n";
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
                    append_uint(out, target->id);
                    out += ";\n";
                }
                break;
            }
            case MIROpcode::CondBr: {
                const MIROperand* cond = operand_at(function, inst, 0);
                const MIROperand* t = operand_at(function, inst, 1);
                const MIROperand* f = operand_at(function, inst, 2);
                if (!cond || !t || !f) { error = "cond_br missing operands"; return false; }
                std::string cexpr;
                append_operand_expr(function, module, *cond, cexpr);
                if (facts.is_fallthrough_inst(function, idx, f->id)) {
                    // the false arm falls through, so only the true jump is emitted
                    out += "if (";
                    out += cexpr;
                    out += ") goto __chx_bb";
                    append_uint(out, t->id);
                    out += ";\n";
                } else if (facts.is_fallthrough_inst(function, idx, t->id)) {
                    // the true arm falls through: jump on the negated condition
                    out += "if (!(";
                    out += cexpr;
                    out += ")) goto __chx_bb";
                    append_uint(out, f->id);
                    out += ";\n";
                } else {
                    out += "if (";
                    out += cexpr;
                    out += ") goto __chx_bb";
                    append_uint(out, t->id);
                    out += "; else goto __chx_bb";
                    append_uint(out, f->id);
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

    out += "}\n";
    return true;
}

} // namespace mir
