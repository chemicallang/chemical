// Copyright (c) Chemical Language Foundation 2025.

#include "MIREmitter.h"

#include "MIRFacts.h"

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

std::string vname(ValueId id) { return "__chx_v" + std::to_string(id); }

// Optional per-function map of place id -> C argument index. A scalar parameter
// that the body never reassigns and never addresses *is* the C argument, so the
// emitter aliases it and the stack slot plus the prologue spill store disappear
// (evaluation §1.2, action C3). thread_local so parallel emission stays safe.
thread_local const std::vector<uint32_t>* g_param_alias = nullptr;

// The C expression naming a place. Frame-resident places live in the coroutine
// frame (`__chx__af-><field>`); an aliased parameter is its C argument;
// everything else is a stack slot.
std::string place_name(const MIRModule& module, const MIRFunction& fn, PlaceId id) {
    if (g_param_alias && id < g_param_alias->size() &&
        (*g_param_alias)[id] != MIR_INVALID_ID) {
        return "__chx_a" + std::to_string((*g_param_alias)[id]);
    }
    if (id < fn.places.size()) {
        const MIRPlaceDef& pd = fn.places[id];
        if (pd.storage_class == static_cast<uint8_t>(MIRStorageClass::FramePtr)) {
            return "__chx__af";
        }
        if (pd.storage_class == static_cast<uint8_t>(MIRStorageClass::FrameField) &&
            pd.frame_field < module.constants.size()) {
            const MIRConstant& c = module.constants.get(pd.frame_field);
            if (c.kind == MIRConstantKind::String) {
                return "__chx__af->" +
                       std::string(module.constants.data.data_at(c.data_offset), c.data_count);
            }
        }
    }
    return "__chx_p" + std::to_string(id);
}

std::string escape_c_string(const char* data, uint32_t len) {
    std::string s = "\"";
    for (uint32_t i = 0; i < len; ++i) {
        const char c = data[i];
        switch (c) {
            case '\\': s += "\\\\"; break;
            case '"': s += "\\\""; break;
            case '\n': s += "\\n"; break;
            case '\r': s += "\\r"; break;
            case '\t': s += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned char>(c));
                    s += buf;
                } else {
                    s += c;
                }
        }
    }
    s += "\"";
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

} // namespace

std::string c_type_of(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return "void";
    const MIRTypeRecord& r = module.types.get(type);
    switch (r.kind) {
        case MIRTypeKind::Void: return "void";
        case MIRTypeKind::Bool: return "_Bool";
        case MIRTypeKind::Int: {
            if (const char* s = c_int_kind_spelling(r.int_kind)) return s;
            return c_int_type(r.size, (r.flags & TF_SIGNED) != 0);
        }
        case MIRTypeKind::Float:
            if (r.size == 4) return "float";
            if (r.size == 16) return "long double";
            return "double";
        case MIRTypeKind::Pointer: {
            if (r.element != MIR_INVALID_ID && r.element < module.types.size() &&
                module.types.get(r.element).kind == MIRTypeKind::Array) {
                // pointer to array: `T (*)[N]...` (dimensions outer-to-inner)
                std::string dims;
                TypeId cur = r.element;
                while (cur != MIR_INVALID_ID && cur < module.types.size() &&
                       module.types.get(cur).kind == MIRTypeKind::Array) {
                    const MIRTypeRecord& rr = module.types.get(cur);
                    dims += "[" + std::to_string(rr.data_count) + "]";
                    cur = rr.element;
                }
                std::string base = c_type_of(module, cur);
                if (!(r.flags & TF_MUTABLE)) base = "const " + base;
                return base + " (*)" + dims;
            }
            const std::string pointee = c_type_of(module, r.element);
            if (!(r.flags & TF_MUTABLE)) return "const " + pointee + "*";
            return pointee + "*";
        }
        case MIRTypeKind::Reference: {
            const std::string pointee = c_type_of(module, r.element);
            if (r.flags & TF_MUTABLE) return pointee + "*";
            return pointee + "*const";
        }
        case MIRTypeKind::Array:
            return c_type_of(module, r.element) + "[" + std::to_string(r.data_count) + "]";
        case MIRTypeKind::Struct:
        case MIRTypeKind::Variant: {
            const std::string n = module.types.name_of(type);
            if (n.empty()) return "void*";
            if (r.flags & TF_TYPEDEF) return n;
            return "struct " + n;
        }
        case MIRTypeKind::Union: {
            const std::string n = module.types.name_of(type);
            if (n.empty()) return "void*";
            if (r.flags & TF_TYPEDEF) return n;
            return "union " + n;
        }
        default:
            return "void*"; // aggregates are emitted by the aggregate milestone
    }
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

static std::string fn_ptr_type(const MIRModule& module, const MIRTypeRecord& r) {
    const bool sret = mir_struct_like_return(module, r.element);
    std::string s = (sret ? std::string("void") : c_type_of(module, r.element)) + "(*)(";
    bool first = true;
    if (sret) {
        s += c_type_of(module, r.element) + "*";
        first = false;
    }
    for (uint32_t i = 0; i < r.data_count; ++i) {
        if (!first) s += ", ";
        s += c_type_of(module, module.types.data[r.data_offset + i]);
        first = false;
    }
    if (r.flags & TF_VARIADIC) {
        if (!first) s += ", ";
        s += "...";
        first = false;
    }
    if (first) s += "void";
    s += ")";
    return s;
}

// A C declarator for `type name`, handling function-pointer types correctly.
static std::string declarator(const MIRModule& module, TypeId type, const std::string& name);

// The declarator for an entity named `name` whose type is `type`. Recurses
// through function return types so a function returning a function pointer is
// spelled with nested declarators (`int(*(*name)(params))(retparams)`), matching
// the legacy `wrap_nested_func_return` spelling.
static std::string decl_type(const MIRModule& module, TypeId type, const std::string& name) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return "void " + name;
    const MIRTypeRecord& r = module.types.get(type);
    if (r.kind != MIRTypeKind::Function) {
        return c_type_of(module, type) + " " + name;
    }
    const bool sret = mir_struct_like_return(module, r.element);
    std::string params;
    if (sret) params += c_type_of(module, r.element) + "*";
    for (uint32_t i = 0; i < r.data_count; ++i) {
        if (!params.empty()) params += ", ";
        params += declarator(module, module.types.data[r.data_offset + i],
                             "__chx_a" + std::to_string(i));
    }
    if (r.flags & TF_VARIADIC) {
        if (!params.empty()) params += ", ";
        params += "...";
    }
    if (params.empty()) params = "void";
    const std::string inner = "(*" + name + ")(" + params + ")";
    if (sret) return "void " + inner;
    return decl_type(module, r.element, inner);
}

static std::string declarator(const MIRModule& module, TypeId type, const std::string& name) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return "void " + name;
    const MIRTypeRecord& r = module.types.get(type);
    if (r.kind == MIRTypeKind::Function) {
        return decl_type(module, type, name);
    }
    if (r.kind == MIRTypeKind::Array) {
        // collect dimensions outer-to-inner so `[4][256]T` spells `T name[4][256]`
        std::string dims;
        TypeId cur = type;
        while (cur != MIR_INVALID_ID && cur < module.types.size() &&
               module.types.get(cur).kind == MIRTypeKind::Array) {
            const MIRTypeRecord& rr = module.types.get(cur);
            dims += "[" + std::to_string(rr.data_count) + "]";
            const MIRTypeRecord& elem = module.types.get(rr.element);
            if (elem.kind == MIRTypeKind::Function) {
                return decl_type(module, rr.element, name + dims);
            }
            cur = rr.element;
        }
        return declarator(module, cur, name) + dims;
    }
    if (r.kind == MIRTypeKind::Pointer || r.kind == MIRTypeKind::Reference) {
        const MIRTypeRecord& pointee = module.types.get(r.element);
        if (pointee.kind == MIRTypeKind::Array) {
            // pointer to array: `int (*name)[N]`
            return declarator(module, pointee.element, "(*" + name + ")") + "[" +
                   std::to_string(pointee.data_count) + "]";
        }
    }
    return c_type_of(module, type) + " " + name;
}

std::string c_decl_type(const MIRModule& module, TypeId type, const std::string& name) {
    return decl_type(module, type, name);
}

std::string c_declarator(const MIRModule& module, TypeId type, const std::string& name) {
    return declarator(module, type, name);
}

std::string c_function_proto(const MIRModule& module, TypeId function_type,
                             const std::string& name) {
    if (function_type == MIR_INVALID_ID || function_type >= module.types.size()) {
        return "void " + name + "(void)";
    }
    const MIRTypeRecord& r = module.types.get(function_type);
    if (r.kind != MIRTypeKind::Function) {
        return decl_type(module, function_type, name);
    }
    const bool sret = mir_struct_like_return(module, r.element);
    std::string params;
    if (sret) params += c_type_of(module, r.element) + "*";
    for (uint32_t i = 0; i < r.data_count; ++i) {
        if (!params.empty()) params += ", ";
        params += declarator(module, module.types.data[r.data_offset + i],
                             "__chx_a" + std::to_string(i));
    }
    if (r.flags & TF_VARIADIC) {
        if (!params.empty()) params += ", ";
        params += "...";
    }
    if (params.empty()) params = "void";
    if (module.types.get(r.element).kind == MIRTypeKind::Function) {
        return decl_type(module, r.element, name + "(" + params + ")");
    }
    const std::string ret = sret ? std::string("void") : c_type_of(module, r.element);
    return ret + " " + name + "(" + params + ")";
}

namespace {

bool const_literal(const MIRModule& module, ConstantId cid, TypeId cast_type, std::string& out) {
    if (cid >= module.constants.size()) return false;
    const MIRConstant& c = module.constants.get(cid);
    switch (c.kind) {
        case MIRConstantKind::Int: {
            out = "(" + c_type_of(module, cast_type) + ")(" + std::to_string(c.bits) + "ull)";
            return true;
        }
        case MIRConstantKind::Bool:
            out = c.bits ? "true" : "false";
            return true;
        case MIRConstantKind::Null:
            out = "0";
            return true;
        case MIRConstantKind::Float: {
            float f;
            uint32_t b = static_cast<uint32_t>(c.bits);
            std::memcpy(&f, &b, sizeof(f));
            out = format_float_literal(static_cast<double>(f), 6, 9, "f");
            return true;
        }
        case MIRConstantKind::Double: {
            double d;
            std::memcpy(&d, &c.bits, sizeof(d));
            out = format_float_literal(d, 15, 17, "");
            return true;
        }
        case MIRConstantKind::String:
            out = escape_c_string(module.constants.data.data_at(c.data_offset), c.data_count);
            return true;
        case MIRConstantKind::Bytes:
            out = "0";
            return true;
    }
    return false;
}

// Optional per-function map of value id -> inlined C expression. When set,
// `operand_expr` returns the inlined expression (parenthesized) instead of the
// value's temporary name. This lets the emitter collapse single-use pure
// values (address chains, arithmetic) into their single use site, which keeps
// the generated C close to the legacy translator's output and avoids the
// register pressure / spilling that a temporary-per-instruction style causes
// in TinyCC. thread_local so parallel lowering stays safe.
thread_local const std::vector<std::string>* g_inline_expr = nullptr;

std::string operand_expr(const MIRFunction& fn, const MIRModule& module,
                         const MIROperand& op) {
    (void) fn; // reserved for future place-projection spelling
    switch (op.kind()) {
        case MIROperandKind::Value:
            if (g_inline_expr && op.id < g_inline_expr->size() &&
                !(*g_inline_expr)[op.id].empty()) {
                return "(" + (*g_inline_expr)[op.id] + ")";
            }
            return vname(op.id);
        case MIROperandKind::Place:
            return place_name(module, fn, op.id);
        case MIROperandKind::Symbol: {
            std::string s;
            symbol_name(module, op.id, s);
            return s;
        }
        case MIROperandKind::Constant: {
            std::string s;
            const_literal(module, op.id, op.type(), s);
            return s;
        }
        default:
            return "?";
    }
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

    std::vector<TypeId> ptypes;
    ptypes.reserve(ftr.data_count);
    for (uint32_t i = 0; i < ftr.data_count; ++i) {
        ptypes.push_back(module.types.data[ftr.data_offset + i]);
    }

    if (is_static) out += "static ";
    std::string params;
    if (ptypes.empty()) {
        params = "void";
    } else {
        for (uint32_t i = 0; i < ptypes.size(); ++i) {
            if (i) params += ", ";
            params += declarator(module, ptypes[i], "__chx_a" + std::to_string(i));
        }
    }
    if (module.types.get(ftr.element).kind == MIRTypeKind::Function) {
        // a function returning a function pointer needs a nested declarator:
        // `ret (*(*name)(params))(retparams)`
        out += declarator(module, ftr.element, fnname + "(" + params + ")");
    } else {
        out += c_type_of(module, ftr.element) + " " + fnname + "(" + params + ")";
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
    MIRFunctionFacts facts;
    compute_function_facts(module, function, facts);
    const std::vector<BlockId>& label_at = facts.label_at_inst;
    const std::vector<uint8_t>& block_starts = facts.block_starts_inst;

    // Build the single-use inlining map: a pure value used exactly once is
    // collapsed into its use site (address chains, arithmetic, casts,
    // constants). This keeps the generated C close to the legacy translator's
    // output and avoids TinyCC spilling a stack temporary per MIR instruction.
    std::vector<std::string> inline_expr(function.values.size());
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
                out += declarator(module, function.places[p].type, place_name(module, function, p)) + ";\n";
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
                out += declarator(module, function.values[v].type, vname(v)) +
                       " = __chx_a" + std::to_string(pai) + ";\n";
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
                std::string lit;
                if (!c || !const_literal(module, c->id, function.values[v].type, lit)) {
                    error = "invalid constant operand";
                    return false;
                }
                out += declarator(module, function.values[v].type, vname(v)) +
                       " = " + lit + ";\n";
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
                        const std::string bexpr = operand_expr(function, module, *p);
                        out += declarator(module, function.values[v].type, vname(v)) +
                               " = " + (base_is_array ? ("(*" + bexpr + ")") : bexpr) + "[" +
                               operand_expr(function, module, *fld) + "];\n";
                    } else if (fld && fld->kind() == MIROperandKind::Constant) {
                        std::string fname;
                        if (!constant_string(module, fld->id, fname)) {
                            error = "field load has invalid field name";
                            return false;
                        }
                        out += declarator(module, function.values[v].type, vname(v)) +
                               " = " + operand_expr(function, module, *p) + "->" + fname + ";\n";
                    } else {
                        out += declarator(module, function.values[v].type, vname(v)) +
                               " = *" + operand_expr(function, module, *p) + ";\n";
                    }
                } else if (fld && fld->kind() == MIROperandKind::Value) {
                    out += declarator(module, function.values[v].type, vname(v)) +
                           " = " + place_name(module, function, p->id) + "[" +
                           operand_expr(function, module, *fld) + "];\n";
                } else if (fld && fld->kind() == MIROperandKind::Constant) {
                    std::string fname;
                    if (!constant_string(module, fld->id, fname)) {
                        error = "field load has invalid field name";
                        return false;
                    }
                    out += declarator(module, function.values[v].type, vname(v)) +
                           " = " + member_access(function, module, p->id, fname) + ";\n";
                } else {
                    const TypeId vt = function.values[v].type;
                    if (vt != MIR_INVALID_ID && vt < module.types.size() &&
                        module.types.get(vt).kind == MIRTypeKind::Array) {
                        // arrays are not assignable in C: declare then memcpy
                        const std::string dst = vname(v);
                        out += declarator(module, vt, dst) + ";\n    ";
                        out += "memcpy(" + dst + ", " + place_name(module, function, p->id) +
                               ", sizeof(" + dst + "));\n";
                    } else {
                        out += declarator(module, function.values[v].type, vname(v)) +
                               " = " + place_name(module, function, p->id) + ";\n";
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
                        const std::string bexpr = operand_expr(function, module, *p);
                        out += (base_is_array ? ("(*" + bexpr + ")") : bexpr) + "[" +
                               operand_expr(function, module, *a) + "] = " +
                               operand_expr(function, module, *b) + ";\n";
                    } else if (a->kind() == MIROperandKind::Constant && b) {
                        std::string fname;
                        if (!constant_string(module, a->id, fname)) {
                            error = "field store has invalid field name";
                            return false;
                        }
                        const std::string access =
                            operand_expr(function, module, *p) + "->" + fname;
                        const TypeId bt =
                            b->kind() == MIROperandKind::Value && b->id < function.values.size()
                                ? function.values[b->id].type : MIR_INVALID_ID;
                        if (bt != MIR_INVALID_ID && bt < module.types.size() &&
                            module.types.get(bt).kind == MIRTypeKind::Array) {
                            // arrays are not assignable: copy the source bytes
                            out += "memcpy(&" + access + ", " + operand_expr(function, module, *b) +
                                   ", sizeof(" + access + "));\n";
                        } else {
                            out += access + " = " + operand_expr(function, module, *b) + ";\n";
                        }
                    } else {
                        std::string rhs;
                        if (a->kind() == MIROperandKind::Place) {
                            const MIRTypeRecord& atr =
                                module.types.get(function.places[a->id].type);
                            rhs = (atr.kind == MIRTypeKind::Pointer ||
                                   atr.kind == MIRTypeKind::Reference)
                                      ? ("*" + place_name(module, function, a->id))
                                      : place_name(module, function, a->id);
                        } else {
                            rhs = operand_expr(function, module, *a);
                        }
                        out += "*" + operand_expr(function, module, *p) + " = " + rhs + ";\n";
                    }
                } else if (a->kind() == MIROperandKind::Value && b) {
                    if (b->kind() == MIROperandKind::Place) {
                        const MIRTypeRecord& btr = module.types.get(function.places[b->id].type);
                        if (btr.kind == MIRTypeKind::Array) {
                            // arrays are not assignable: copy the element bytes
                            const std::string base = place_name(module, function, p->id);
                            const std::string elem =
                                base + "[" + operand_expr(function, module, *a) + "]";
                            out += "memcpy(&" + elem + ", " + place_name(module, function, b->id) +
                                   ", sizeof(" + place_name(module, function, b->id) + "));\n";
                            break;
                        }
                    }
                    out += place_name(module, function, p->id) + "[" + operand_expr(function, module, *a) +
                           "] = " + operand_expr(function, module, *b) + ";\n";
                } else if (a->kind() == MIROperandKind::Constant && b) {
                    std::string fname;
                    if (!constant_string(module, a->id, fname)) {
                        error = "field store has invalid field name";
                        return false;
                    }
                    std::string rhs;
                    if (b->kind() == MIROperandKind::Value) {
                        const TypeId bt = function.values[b->id].type;
                        if (bt != MIR_INVALID_ID && bt < module.types.size() &&
                            module.types.get(bt).kind == MIRTypeKind::Array) {
                            // arrays are not assignable: copy the source bytes
                            const std::string access = member_access(function, module, p->id, fname);
                            out += "memcpy(&" + access + ", " + operand_expr(function, module, *b) +
                                   ", sizeof(" + access + "));\n";
                            break;
                        }
                        rhs = operand_expr(function, module, *b);
                    } else if (b->kind() == MIROperandKind::Place) {
                        const MIRTypeRecord& btr = module.types.get(function.places[b->id].type);
                        const std::string access = member_access(function, module, p->id, fname);
                        if (btr.kind == MIRTypeKind::Array) {
                            // arrays are not assignable: copy the source bytes
                            out += "memcpy(&" + access + ", &" + place_name(module, function, b->id) + ", sizeof(" +
                                   place_name(module, function, b->id) + "));\n";
                            break;
                        }
                        // aggregate parameters are pointer-typed places: deref
                        rhs = (btr.kind == MIRTypeKind::Pointer ||
                               btr.kind == MIRTypeKind::Reference)
                                  ? ("*" + place_name(module, function, b->id))
                                  : place_name(module, function, b->id);
                    } else {
                        rhs = operand_expr(function, module, *b);
                    }
                    out += member_access(function, module, p->id, fname) + " = " + rhs + ";\n";
                } else {
                    std::string rhs;
                    if (a->kind() == MIROperandKind::Place) {
                        const MIRTypeRecord& atr =
                            module.types.get(function.places[a->id].type);
                        rhs = (atr.kind == MIRTypeKind::Pointer ||
                               atr.kind == MIRTypeKind::Reference)
                                  ? ("*" + place_name(module, function, a->id))
                                  : place_name(module, function, a->id);
                    } else {
                        rhs = operand_expr(function, module, *a);
                    }
                    // arrays are not assignable in C: copy the source bytes
                    const TypeId dst_t = function.places[p->id].type;
                    if (dst_t != MIR_INVALID_ID && dst_t < module.types.size() &&
                        module.types.get(dst_t).kind == MIRTypeKind::Array) {
                        std::string src_addr;
                        if (a->kind() == MIROperandKind::Place) {
                            const MIRTypeRecord& atr =
                                module.types.get(function.places[a->id].type);
                            src_addr = (atr.kind == MIRTypeKind::Pointer ||
                                        atr.kind == MIRTypeKind::Reference)
                                           ? ("*" + place_name(module, function, a->id))
                                           : ("&" + place_name(module, function, a->id));
                        } else {
                            src_addr = operand_expr(function, module, *a);
                        }
                        out += "memcpy(&" + place_name(module, function, p->id) + ", " +
                               src_addr + ", sizeof(" + place_name(module, function, p->id) +
                               "));\n";
                    } else {
                        out += place_name(module, function, p->id) + " = " + rhs + ";\n";
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
                const std::string base =
                    ptr ? operand_expr(function, module, *p) : place_name(module, function, p->id);
                // a place whose own type is a pointer/reference is dereferenced
                bool arrow = ptr;
                if (!ptr) {
                    const MIRTypeRecord& bt = module.types.get(function.places[p->id].type);
                    arrow = (bt.kind == MIRTypeKind::Pointer ||
                             bt.kind == MIRTypeKind::Reference);
                }
                const std::string access = base + (arrow ? "->" : ".") + fname;
                const TypeId ft = function.values[v].type;
                if (ft < module.types.size() &&
                    module.types.get(ft).kind == MIRTypeKind::Array) {
                    // pointer-to-array: `T (*v)[d0][d1]...` with the dimensions
                    // in outer-to-inner order (delegated to declarator).
                    out += declarator(module, ft, "(*" + vname(v) + ")") +
                           " = &" + access + ";\n";
                } else {
                    // __typeof__ lets us take the address of a field whose type has
                    // no spellable C name (anonymous structs/unions)
                    out += "__typeof__(" + access + ")* " + vname(v) + " = &" + access + ";\n";
                }
                break;
            }
            case MIROpcode::IndexAddr: {
                const ValueId v = inst.result_or_place;
                const MIROperand* b = operand_at(function, inst, 0);
                const MIROperand* idx = operand_at(function, inst, 1);
                if (!b || !idx) { error = "index_addr missing operands"; return false; }
                std::string bexpr;
                bool base_is_array = false;
                if (b->kind() == MIROperandKind::Place) {
                    bexpr = place_name(module, function, b->id);
                } else {
                    bexpr = operand_expr(function, module, *b);
                    // an array-typed value is spelled as a pointer-to-array in C
                    // (`char (*v)[N]`); index it through a deref so the element
                    // stride is the element size, not the whole array size.
                    base_is_array = b->type() < module.types.size() &&
                                    module.types.get(b->type()).kind == MIRTypeKind::Array;
                }
                const std::string access =
                    (base_is_array ? ("(*" + bexpr + ")") : bexpr) + "[" +
                    operand_expr(function, module, *idx) + "]";
                // __typeof__ handles element types with no spellable C name
                out += "__typeof__(" + access + ")* " + vname(v) + " = &" + access + ";\n";
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
                    out += declarator(module, function.values[v].type, vname(v)) +
                           " = " + place_name(module, function, p->id) + ";\n";
                } else {
                    out += declarator(module, function.values[v].type, vname(v)) +
                           " = &" + place_name(module, function, p->id) + ";\n";
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
                out += declarator(module, function.values[v].type, vname(v)) +
                       " = (" + unop_c(op) + operand_expr(function, module, *a) + ");\n";
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
                out += declarator(module, function.values[v].type, vname(v)) + " = (" +
                       operand_expr(function, module, *a) + " " + binop_c(op) + " " +
                       operand_expr(function, module, *b) + ");\n";
                break;
            }
            case MIROpcode::Cast: {
                const ValueId v = inst.result_or_place;
                const MIROperand* a = operand_at(function, inst, 0);
                if (!a) { error = "cast missing operand"; return false; }
                out += declarator(module, function.values[v].type, vname(v)) +
                       " = (" + c_type_of(module, function.values[v].type) + ")" +
                       operand_expr(function, module, *a) + ";\n";
                break;
            }
            case MIROpcode::Call: {
                const MIROperand* callee = operand_at(function, inst, 0);
                if (!callee || callee->kind() != MIROperandKind::Symbol) {
                    error = "call missing callee symbol";
                    return false;
                }
                std::string cname;
                symbol_name(module, callee->id, cname);
                // sret if the operand after the symbol is a place
                const MIROperand* maybe_place = operand_at(function, inst, 1);
                const bool sret = maybe_place && maybe_place->kind() == MIROperandKind::Place;
                uint32_t arg_start = sret ? 2 : 1;

                std::string call = cname + "(";
                if (sret) call += "&" + place_name(module, function, maybe_place->id);
                for (uint32_t a = arg_start; a < inst.operand_count; ++a) {
                    if (a != arg_start || sret) call += ", ";
                    call += operand_expr(function, module, *operand_at(function, inst, a));
                }
                call += ")";

                const bool is_void = sret ||
                    inst.result_or_place >= function.values.size() ||
                    c_type_of(module, function.values[inst.result_or_place].type) == "void";
                if (sret) {
                    out += call + ";\n";
                } else if (!is_void) {
                    out += declarator(module, function.values[inst.result_or_place].type,
                                      vname(inst.result_or_place)) + " = " + call + ";\n";
                } else {
                    out += call + ";\n";
                }
                break;
            }
            case MIROpcode::Return: {
                if (inst.operand_count == 0) {
                    out += "return;\n";
                } else {
                    const MIROperand* v = operand_at(function, inst, 0);
                    out += "return " + operand_expr(function, module, *v) + ";\n";
                }
                break;
            }
            case MIROpcode::CopyInit:
            case MIROpcode::MoveInit: {
                const MIROperand* dest = operand_at(function, inst, 0);
                const MIROperand* src = operand_at(function, inst, 1);
                if (!dest || !src) { error = "copy/move_init missing operands"; return false; }
                const std::string rhs =
                    (src->kind() == MIROperandKind::Place
                         ? place_name(module, function, src->id)
                         : operand_expr(function, module, *src));
                std::string src_expr = rhs;
                if (src->kind() == MIROperandKind::Place) {
                    const MIRTypeRecord& str = module.types.get(function.places[src->id].type);
                    if (str.kind == MIRTypeKind::Pointer || str.kind == MIRTypeKind::Reference) {
                        src_expr = "*" + place_name(module, function, src->id);
                    }
                }
                const TypeId dt = dest->id < function.places.size()
                                      ? function.places[dest->id].type
                                      : MIR_INVALID_ID;
                if (dt != MIR_INVALID_ID && dt < module.types.size() &&
                    module.types.get(dt).kind == MIRTypeKind::Array) {
                    // arrays are not assignable in C: copy the bytes instead
                    const std::string src_addr =
                        (src->kind() == MIROperandKind::Place && src_expr != rhs)
                            ? src_expr
                            : ("&" + rhs);
                    out += "memcpy(&" + place_name(module, function, dest->id) + ", " + src_addr + ", sizeof(" +
                           place_name(module, function, dest->id) + "));\n";
                } else {
                    out += place_name(module, function, dest->id) + " = " + src_expr + ";\n";
                }
                break;
            }
            case MIROpcode::Drop: {
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* d = operand_at(function, inst, 1);
                const MIROperand* f = operand_at(function, inst, 2);
                if (!p || !d) { error = "drop missing operands"; return false; }
                std::string dname;
                symbol_name(module, d->id, dname);
                const MIRTypeRecord& dpr = module.types.get(function.places[p->id].type);
                if (dpr.kind == MIRTypeKind::Array) {
                    // drop each element, last to first
                    const std::string pn = place_name(module, function, p->id);
                    const std::string iv = "__chx_di" + std::to_string(idx);
                    out += "{ ";
                    if (f) out += "if (" + operand_expr(function, module, *f) + ") ";
                    out += "for (unsigned long " + iv + " = " + std::to_string(dpr.data_count) +
                           "; " + iv + "-- > 0;) " + dname + "(&" + pn + "[" + iv + "]); }\n";
                    break;
                }
                const std::string darg =
                    (dpr.kind == MIRTypeKind::Pointer || dpr.kind == MIRTypeKind::Reference)
                        ? place_name(module, function, p->id)
                        : ("&" + place_name(module, function, p->id));
                if (f) {
                    out += "if (" + operand_expr(function, module, *f) + ") " + dname + "(" +
                           darg + ");\n";
                } else {
                    out += dname + "(" + darg + ");\n";
                }
                break;
            }
            case MIROpcode::SetDrop: {
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* c = operand_at(function, inst, 1);
                if (!p || !c) { error = "set_drop missing operands"; return false; }
                out += place_name(module, function, p->id) + " = " + operand_expr(function, module, *c) + ";\n";
                break;
            }
            case MIROpcode::Destroy: {
                const MIROperand* p = operand_at(function, inst, 0);
                const MIROperand* d = operand_at(function, inst, 1);
                if (!p || !d || d->kind() != MIROperandKind::Symbol) {
                    error = "destroy missing operands";
                    return false;
                }
                std::string dname;
                symbol_name(module, d->id, dname);
                const MIRTypeRecord& dpr = module.types.get(function.places[p->id].type);
                if (dpr.kind == MIRTypeKind::Array) {
                    // destroy each element, last to first (coroutine frame slots
                    // of an array type are not destroyed by a single call)
                    const std::string pn = place_name(module, function, p->id);
                    const std::string iv = "__chx_di" + std::to_string(idx);
                    out += "{ for (unsigned long " + iv + " = " +
                           std::to_string(dpr.data_count) + "; " + iv + "-- > 0;) " +
                           dname + "(&" + pn + "[" + iv + "]); }\n";
                    break;
                }
                const std::string darg =
                    (dpr.kind == MIRTypeKind::Pointer || dpr.kind == MIRTypeKind::Reference)
                        ? place_name(module, function, p->id)
                        : ("&" + place_name(module, function, p->id));
                out += dname + "(" + darg + ");\n";
                break;
            }
            case MIROpcode::MemCpy: {
                const MIROperand* dest = operand_at(function, inst, 0);
                const MIROperand* src = operand_at(function, inst, 1);
                if (!dest || !src) { error = "memcpy missing operands"; return false; }
                const MIROperand* sz = operand_at(function, inst, 2);
                const std::string size_expr =
                    sz ? operand_expr(function, module, *sz)
                       : ("sizeof(" + place_name(module, function, dest->id) + ")");
                out += "memcpy(&" + place_name(module, function, dest->id) + ", " +
                       operand_expr(function, module, *src) + ", " + size_expr + ");\n";
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
                    const std::string pn = place_name(module, function, dest->id);
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
                        out += pn + " = 0;\n";
                    } else {
                        out += "memset(&" + pn + ", 0, sizeof(" + pn + "));\n";
                    }
                    break;
                }
                const std::string dexpr =
                    (dest->kind() == MIROperandKind::Place)
                        ? ("&" + place_name(module, function, dest->id))
                        : operand_expr(function, module, *dest);
                const std::string bexpr = byte ? operand_expr(function, module, *byte) : "0";
                std::string sexpr;
                if (size) {
                    sexpr = operand_expr(function, module, *size);
                } else if (dest->kind() == MIROperandKind::Place) {
                    sexpr = "sizeof(" + place_name(module, function, dest->id) + ")";
                } else {
                    error = "memset with an address destination needs a size operand";
                    return false;
                }
                out += "memset(" + dexpr + ", " + bexpr + ", " + sexpr + ");\n";
                break;
            }
            case MIROpcode::FunctionAddr: {
                const ValueId v = inst.result_or_place;
                const MIROperand* s = operand_at(function, inst, 0);
                if (!s) { error = "function_addr missing symbol"; return false; }
                std::string name;
                symbol_name(module, s->id, name);
                out += declarator(module, function.values[v].type, vname(v)) + " = " + name +
                       ";\n";
                break;
            }
            case MIROpcode::GlobalAddr: {
                const ValueId v = inst.result_or_place;
                const MIROperand* s = operand_at(function, inst, 0);
                if (!s) { error = "global_addr missing symbol"; return false; }
                std::string name;
                symbol_name(module, s->id, name);
                // an array global decays to a pointer to its first element
                bool decay = false;
                if (s->id < module.symbols.size()) {
                    const TypeId st = module.symbols.get(s->id).type;
                    if (st < module.types.size() &&
                        module.types.get(st).kind == MIRTypeKind::Array) {
                        decay = true;
                    }
                }
                out += declarator(module, function.values[v].type, vname(v)) +
                       " = " + (decay ? name : ("&" + name)) + ";\n";
                break;
            }
            case MIROpcode::Init: {
                const MIROperand* dest = operand_at(function, inst, 0);
                const MIROperand* ctor = operand_at(function, inst, 1);
                if (!dest || !ctor || ctor->kind() != MIROperandKind::Symbol) {
                    error = "init missing destination/constructor";
                    return false;
                }
                std::string cname;
                symbol_name(module, ctor->id, cname);
                out += cname + "(&" + place_name(module, function, dest->id);
                for (uint32_t a = 2; a < inst.operand_count; ++a) {
                    out += ", " + operand_expr(function, module, *operand_at(function, inst, a));
                }
                out += ");\n";
                break;
            }
            case MIROpcode::CallIndirect: {
                const MIROperand* fn = operand_at(function, inst, 0);
                if (!fn) { error = "call_indirect missing callee"; return false; }
                std::string callee = operand_expr(function, module, *fn);
                // the callee value is a function pointer; give it its precise
                // function-pointer type so the call type-checks
                if (fn->type() < module.types.size()) {
                    const MIRTypeRecord& fr = module.types.get(fn->type());
                    if (fr.kind == MIRTypeKind::Function) {
                        callee = "((" + fn_ptr_type(module, fr) + ")" + callee + ")";
                    }
                }
                // sret if the operand after the callee is a place (aggregate return)
                const MIROperand* maybe_place = operand_at(function, inst, 1);
                const bool sret = maybe_place && maybe_place->kind() == MIROperandKind::Place;
                uint32_t arg_start = sret ? 2 : 1;
                std::string call = callee + "(";
                if (sret) call += "&" + place_name(module, function, maybe_place->id);
                for (uint32_t a = arg_start; a < inst.operand_count; ++a) {
                    if (a != arg_start || sret) call += ", ";
                    call += operand_expr(function, module, *operand_at(function, inst, a));
                }
                call += ")";
                if (sret) {
                    out += call + ";\n";
                    break;
                }
                const bool is_void = inst.result_or_place >= function.values.size() ||
                    c_type_of(module, function.values[inst.result_or_place].type) == "void";
                if (is_void) {
                    out += call + ";\n";
                } else {
                    out += declarator(module, function.values[inst.result_or_place].type,
                                      vname(inst.result_or_place)) + " = " + call + ";\n";
                }
                break;
            }
            case MIROpcode::Br: {
                const MIROperand* target = operand_at(function, inst, 0);
                if (!target) { error = "br missing target"; return false; }
                // A branch to the next block needs no jump: control falls into it.
                if (!facts.is_fallthrough_inst(function, idx, target->id)) {
                    out += "goto __chx_bb" + std::to_string(target->id) + ";\n";
                }
                break;
            }
            case MIROpcode::CondBr: {
                const MIROperand* cond = operand_at(function, inst, 0);
                const MIROperand* t = operand_at(function, inst, 1);
                const MIROperand* f = operand_at(function, inst, 2);
                if (!cond || !t || !f) { error = "cond_br missing operands"; return false; }
                const std::string cexpr = operand_expr(function, module, *cond);
                if (facts.is_fallthrough_inst(function, idx, f->id)) {
                    // the false arm falls through, so only the true jump is emitted
                    out += "if (" + cexpr + ") goto __chx_bb" + std::to_string(t->id) + ";\n";
                } else if (facts.is_fallthrough_inst(function, idx, t->id)) {
                    // the true arm falls through: jump on the negated condition
                    out += "if (!(" + cexpr + ")) goto __chx_bb" + std::to_string(f->id) +
                           ";\n";
                } else {
                    out += "if (" + cexpr + ") goto __chx_bb" + std::to_string(t->id) +
                           "; else goto __chx_bb" + std::to_string(f->id) + ";\n";
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
                out += declarator(module, function.values[v].type, vname(v)) + " = sizeof(" +
                       c_type_of(module, t->type()) + ");\n";
                break;
            }
            case MIROpcode::AlignOf: {
                const ValueId v = inst.result_or_place;
                const MIROperand* t = operand_at(function, inst, 0);
                if (!t) { error = "align_of missing type"; return false; }
                out += declarator(module, function.values[v].type, vname(v)) + " = _Alignof(" +
                       c_type_of(module, t->type()) + ");\n";
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
                out += declarator(module, function.values[v].type, vname(v)) + " = offsetof(" +
                       c_type_of(module, t->type()) + ", " + fname + ");\n";
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
                const std::string fexpr = place_name(module, function, fr->id);
                const std::string cexpr = place_name(module, function, child->id);
                const std::string rexpr = place_name(module, function, result->id);
                const std::string pexpr = place_name(module, function, ret->id);
                const std::string pty = c_type_of(module, poll_ty->type());
                const std::string fpty = c_type_of(module, fn_poll_ty->type());
                const uint32_t resume_state =
                    state->id < module.constants.size()
                        ? static_cast<uint32_t>(module.constants.get(state->id).bits)
                        : 0;
                out += "{\n        " + pty + " __chx__p = (*({ " + pty + " __chx__t; " +
                       cexpr + ".vtbl->poll(&__chx__t, " + cexpr + ".frame, " + fexpr +
                       "->__cx); &__chx__t; }));\n";
                out += "        if(__chx__p.__chx__vt_621827 != 0) {\n";
                if (resume_state != 0) {
                    out += "            " + fexpr + "->__state = " + std::to_string(resume_state) +
                           "u;\n";
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
                    out += "            " + fexpr + "->" + fname + " = " +
                           place_name(module, function, loc->id) + ";\n";
                }
                out += "            *" + pexpr + " = (" + fpty +
                       "){ .__chx__vt_621827 = 1 };\n";
                out += "            return;\n        }\n";
                out += "        " + rexpr + " = __chx__p.Ready.value;\n";
                out += "        " + cexpr + ".vtbl->drop(" + cexpr + ".frame);\n";
                out += "        " + cexpr + ".frame = (void*) 0;\n    }\n";
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
                const std::string fexpr = place_name(module, function, fr->id);
                const std::string pexpr = place_name(module, function, ret->id);
                const std::string fpty = c_type_of(module, fn_poll_ty->type());
                out += fexpr + "->__state = 0xFFFFFFFFu;\n        *" + pexpr + " = (" + fpty +
                       "){ .__chx__vt_621827 = 0, .Ready.value = " + fexpr +
                       "->__result };\n        return;\n";
                break;
            }
            case MIROpcode::AsyncFrameAlloc: {
                const MIROperand* frame_ty = operand_at(function, inst, 0);
                if (!frame_ty) { error = "async_frame_alloc missing type"; return false; }
                const std::string fname = c_type_of(module, frame_ty->type());
                out += declarator(module, function.values[inst.result_or_place].type,
                                  vname(inst.result_or_place)) + " = (" + fname +
                       "*) chemical_async_frame_alloc(sizeof(" + fname + "), _Alignof(" + fname +
                       "));\n";
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
                const std::string fexpr = place_name(module, function, fr->id);
                const std::string sexpr = place_name(module, function, sret->id);
                std::string vname2;
                if (!constant_string(module, vtbl->id, vname2)) {
                    error = "async_ramp_finish bad vtbl name";
                    return false;
                }
                const uint32_t st =
                    state->id < module.constants.size()
                        ? static_cast<uint32_t>(module.constants.get(state->id).bits)
                        : 0;
                out += fexpr + "->__state = " + std::to_string(st) + "u;\n";
                out += "        *" + sexpr + " = (" + c_type_of(module, handle_ty->type()) +
                       "){ .frame = (void*) " + fexpr + ", .vtbl = &" + vname2 +
                       " };\n        return;\n";
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
                const std::string base = (fat->kind() == MIROperandKind::Place)
                                             ? place_name(module, function, fat->id)
                                             : operand_expr(function, module, *fat);
                const bool arrow =
                    fat->type() < module.types.size() &&
                    (module.types.get(fat->type()).kind == MIRTypeKind::Pointer ||
                     module.types.get(fat->type()).kind == MIRTypeKind::Reference);
                const std::string access = base + (arrow ? "->" : ".");
                std::string call = "((" + vt_name + "*)" + access + "second)->" + m_name + "(" +
                                   access + "first";
                for (uint32_t a = 3; a < inst.operand_count; ++a) {
                    call += ", " + operand_expr(function, module, *operand_at(function, inst, a));
                }
                call += ")";
                const bool is_void =
                    inst.result_or_place >= function.values.size() ||
                    c_type_of(module, function.values[inst.result_or_place].type) == "void";
                if (is_void) {
                    out += call + ";\n";
                } else {
                    out += declarator(module, function.values[inst.result_or_place].type,
                                      vname(inst.result_or_place)) + " = " + call + ";\n";
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
