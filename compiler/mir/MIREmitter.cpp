// Copyright (c) Chemical Language Foundation 2025.

#include "MIREmitter.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace mir {

namespace {

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
std::string pname(PlaceId id) { return "__chx_p" + std::to_string(id); }

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
    out.assign(module.constants.data.data() + c.data_offset, c.data_count);
    return true;
}

std::string member_access(const MIRFunction& fn, const MIRModule& module, PlaceId base,
                          const std::string& fname) {
    const MIRTypeRecord& bt = module.types.get(fn.places[base].type);
    const bool arrow = bt.kind == MIRTypeKind::Pointer || bt.kind == MIRTypeKind::Reference;
    return pname(base) + (arrow ? "->" : ".") + fname;
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
static std::string fn_ptr_type(const MIRModule& module, const MIRTypeRecord& r) {
    std::string s = c_type_of(module, r.element) + "(*)(";
    if (r.data_count == 0) {
        s += "void";
    } else {
        for (uint32_t i = 0; i < r.data_count; ++i) {
            if (i) s += ", ";
            s += c_type_of(module, module.types.data[r.data_offset + i]);
        }
    }
    s += ")";
    return s;
}

// A C declarator for `type name`, handling function-pointer types correctly.
static std::string declarator(const MIRModule& module, TypeId type, const std::string& name) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return "void " + name;
    const MIRTypeRecord& r = module.types.get(type);
    if (r.kind == MIRTypeKind::Function) {
        std::string s = c_type_of(module, r.element) + "(*" + name + ")(";
        if (r.data_count == 0) {
            s += "void";
        } else {
            for (uint32_t i = 0; i < r.data_count; ++i) {
                if (i) s += ", ";
                s += c_type_of(module, module.types.data[r.data_offset + i]);
            }
        }
        s += ")";
        return s;
    }
    if (r.kind == MIRTypeKind::Array) {
        return declarator(module, r.element, name) + "[" + std::to_string(r.data_count) + "]";
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
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%g", static_cast<double>(f));
            std::string s(buf);
            if (s.find_first_of(".eE") == std::string::npos) s += ".0";
            out = s + "f";
            return true;
        }
        case MIRConstantKind::Double: {
            double d;
            std::memcpy(&d, &c.bits, sizeof(d));
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%g", d);
            out = buf;
            if (out.find_first_of(".eE") == std::string::npos) out += ".0";
            return true;
        }
        case MIRConstantKind::String:
            out = escape_c_string(module.constants.data.data() + c.data_offset, c.data_count);
            return true;
        case MIRConstantKind::Bytes:
            out = "0";
            return true;
    }
    return false;
}

std::string operand_expr(const MIRFunction& fn, const MIRModule& module,
                         const MIROperand& op) {
    (void) fn; // reserved for future place-projection spelling
    switch (op.kind()) {
        case MIROperandKind::Value:
            return vname(op.id);
        case MIROperandKind::Place:
            return pname(op.id);
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
    out += c_type_of(module, ftr.element) + " " + fnname + "(";
    if (ptypes.empty()) {
        out += "void";
    } else {
        for (uint32_t i = 0; i < ptypes.size(); ++i) {
            if (i) out += ", ";
            out += declarator(module, ptypes[i], "__chx_a" + std::to_string(i));
        }
    }
    out += ") {\n";

    // Block labels: blocks are emitted in creation order, so their inst_start
    // offsets are monotonic. A label is emitted at each block start.
    std::vector<BlockId> label_at(function.instructions.size(), MIR_INVALID_ID);
    for (uint32_t b = 0; b < function.blocks.size(); ++b) {
        const MIRBlock& blk = function.blocks[b];
        if (blk.inst_start != MIR_INVALID_ID && blk.inst_start < label_at.size()) {
            label_at[blk.inst_start] = blk.id;
        }
    }

    uint32_t param_index = 0;
    for (uint32_t idx = 0; idx < function.instructions.size(); ++idx) {
        if (label_at[idx] != MIR_INVALID_ID) {
            out += "__chx_bb" + std::to_string(label_at[idx]) + ":;\n";
        }
        const MIRInstruction& inst = function.instructions[idx];
        out += "    ";

        switch (inst.opcode()) {
            case MIROpcode::Alloca: {
                const PlaceId p = inst.result_or_place;
                out += declarator(module, function.places[p].type, pname(p)) + ";\n";
                break;
            }
            case MIROpcode::Param: {
                const ValueId v = inst.result_or_place;
                out += declarator(module, function.values[v].type, vname(v)) +
                       " = __chx_a" + std::to_string(param_index++) + ";\n";
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
                           " = " + pname(p->id) + "[" +
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
                    out += declarator(module, function.values[v].type, vname(v)) +
                           " = " + pname(p->id) + ";\n";
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
                        out += operand_expr(function, module, *p) + "->" + fname + " = " +
                               operand_expr(function, module, *b) + ";\n";
                    } else {
                        std::string rhs;
                        if (a->kind() == MIROperandKind::Place) {
                            const MIRTypeRecord& atr =
                                module.types.get(function.places[a->id].type);
                            rhs = (atr.kind == MIRTypeKind::Pointer ||
                                   atr.kind == MIRTypeKind::Reference)
                                      ? ("*" + pname(a->id))
                                      : pname(a->id);
                        } else {
                            rhs = operand_expr(function, module, *a);
                        }
                        out += "*" + operand_expr(function, module, *p) + " = " + rhs + ";\n";
                    }
                } else if (a->kind() == MIROperandKind::Value && b) {
                    out += pname(p->id) + "[" + operand_expr(function, module, *a) +
                           "] = " + operand_expr(function, module, *b) + ";\n";
                } else if (a->kind() == MIROperandKind::Constant && b) {
                    std::string fname;
                    if (!constant_string(module, a->id, fname)) {
                        error = "field store has invalid field name";
                        return false;
                    }
                    std::string rhs;
                    if (b->kind() == MIROperandKind::Place) {
                        const MIRTypeRecord& btr = module.types.get(function.places[b->id].type);
                        const std::string access = member_access(function, module, p->id, fname);
                        if (btr.kind == MIRTypeKind::Array) {
                            // arrays are not assignable: copy the source bytes
                            out += "memcpy(&" + access + ", &" + pname(b->id) + ", sizeof(" +
                                   pname(b->id) + "));\n";
                            break;
                        }
                        // aggregate parameters are pointer-typed places: deref
                        rhs = (btr.kind == MIRTypeKind::Pointer ||
                               btr.kind == MIRTypeKind::Reference)
                                  ? ("*" + pname(b->id))
                                  : pname(b->id);
                    } else {
                        rhs = operand_expr(function, module, *b);
                    }
                    out += member_access(function, module, p->id, fname) + " = " + rhs + ";\n";
                } else {
                    out += pname(p->id) + " = " + operand_expr(function, module, *a) + ";\n";
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
                    ptr ? operand_expr(function, module, *p) : pname(p->id);
                const TypeId ft = function.values[v].type;
                std::string decl;
                if (ft < module.types.size() &&
                    module.types.get(ft).kind == MIRTypeKind::Array) {
                    const MIRTypeRecord& ar = module.types.get(ft);
                    decl = declarator(module, ar.element, "(*" + vname(v) + ")") + "[" +
                           std::to_string(ar.data_count) + "]";
                } else {
                    decl = c_type_of(module, ft) + "* " + vname(v);
                }
                out += decl + " = &" + base + (ptr ? "->" : ".") + fname + ";\n";
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
                           " = " + pname(p->id) + ";\n";
                } else {
                    out += declarator(module, function.values[v].type, vname(v)) +
                           " = &" + pname(p->id) + ";\n";
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
                if (sret) call += "&" + pname(maybe_place->id);
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
                    (src->kind() == MIROperandKind::Place ? pname(src->id) : vname(src->id));
                std::string src_expr = rhs;
                if (src->kind() == MIROperandKind::Place) {
                    const MIRTypeRecord& str = module.types.get(function.places[src->id].type);
                    if (str.kind == MIRTypeKind::Pointer || str.kind == MIRTypeKind::Reference) {
                        src_expr = "*" + pname(src->id);
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
                    out += "memcpy(&" + pname(dest->id) + ", " + src_addr + ", sizeof(" +
                           pname(dest->id) + "));\n";
                } else {
                    out += pname(dest->id) + " = " + src_expr + ";\n";
                }
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
                out += dname + "(&" + pname(p->id) + ");\n";
                break;
            }
            case MIROpcode::MemCpy: {
                const MIROperand* dest = operand_at(function, inst, 0);
                const MIROperand* src = operand_at(function, inst, 1);
                if (!dest || !src) { error = "memcpy missing operands"; return false; }
                out += "memcpy(&" + pname(dest->id) + ", " +
                       operand_expr(function, module, *src) + ", sizeof(" + pname(dest->id) +
                       "));\n";
                break;
            }
            case MIROpcode::MemSet: {
                const MIROperand* dest = operand_at(function, inst, 0);
                if (!dest) { error = "memset missing destination"; return false; }
                const std::string pn = pname(dest->id);
                const std::string iv = "__chx_zi" + std::to_string(idx);
                out += "for (unsigned long " + iv + " = 0; " + iv + " < sizeof(" + pn +
                       "); ++" + iv + ") ((unsigned char*)&" + pn + ")[" + iv + "] = 0;\n";
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
                out += declarator(module, function.values[v].type, vname(v)) +
                       " = &" + name + ";\n";
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
                out += cname + "(&" + pname(dest->id);
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
                std::string call = callee + "(";
                for (uint32_t a = 1; a < inst.operand_count; ++a) {
                    if (a > 1) call += ", ";
                    call += operand_expr(function, module, *operand_at(function, inst, a));
                }
                call += ")";
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
                out += "goto __chx_bb" + std::to_string(target->id) + ";\n";
                break;
            }
            case MIROpcode::CondBr: {
                const MIROperand* cond = operand_at(function, inst, 0);
                const MIROperand* t = operand_at(function, inst, 1);
                const MIROperand* f = operand_at(function, inst, 2);
                if (!cond || !t || !f) { error = "cond_br missing operands"; return false; }
                out += "if (" + operand_expr(function, module, *cond) + ") goto __chx_bb" +
                       std::to_string(t->id) + "; else goto __chx_bb" + std::to_string(f->id) + ";\n";
                break;
            }
            case MIROpcode::Unreachable: {
                out += "; /* unreachable */\n";
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
