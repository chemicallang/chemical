// Copyright (c) Chemical Language Foundation 2025.

#include "MIRDeclEmitter.h"

#include "MIREmitter.h"
#include "MIRTypeBuilder.h"

#include "ast/base/BaseType.h"
#include "ast/base/Value.h"
#include "ast/base/ASTAllocator.h"
#include "ast/base/ASTNode.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/PointerType.h"
#include "ast/types/DynamicType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/types/GenericType.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/If.h"
#include "ast/structures/Scope.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/Negative.h"
#include "ast/values/ComptimeValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/AccessChain.h"
#include "ast/values/CastedValue.h"
#include "compiler/mangler/NameMangler.h"
#include "preprocess/2c/BufferedWriter.h"
#include "core/source/LocationManager.h"

#include <cstdio>

namespace mir {

// ---------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------

static bool needs_sret(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    const MIRTypeKind k = module.types.get(type).kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union ||
           k == MIRTypeKind::Variant || k == MIRTypeKind::Array;
}

static bool needs_aggregate(const MIRModule& module, TypeId type) {
    if (type == MIR_INVALID_ID || type >= module.types.size()) return false;
    const MIRTypeRecord& rec = module.types.get(type);
    if (rec.flags & TF_TYPEDEF) return false;
    const MIRTypeKind k = rec.kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union ||
           k == MIRTypeKind::Variant || k == MIRTypeKind::Array;
}

// ---------------------------------------------------------------------------
// type / value spelling
// ---------------------------------------------------------------------------

std::string MIRDeclEmitter::type_spelling(BaseType* type) {
    if (!type) return "void";
    return c_type_of(module_, types_.map(type));
}

std::string MIRDeclEmitter::decl_spelling(BaseType* type, const std::string& name) {
    if (!type) return "void " + name;
    if (type->kind() == BaseTypeKind::Array) {
        auto* at = type->as_array_type();
        if (at && !at->has_array_size()) {
            // an unsized array (`char[]`) is declared `char name[]` (MIR maps it
            // to a pointer for decay, which is wrong for a declaration)
            return c_declarator(module_, types_.map(at->known_child_type()), name + "[]");
        }
    }
    return c_declarator(module_, types_.map(type), name);
}

TypeId MIRDeclEmitter::function_type_of(FunctionDeclaration* decl) {
    const TypeId ret = decl->returnType ? types_.map(decl->returnType) : types_.void_type();
    const bool sret = needs_sret(module_, ret);
    std::vector<TypeId> ptypes;
    if (sret) ptypes.push_back(types_.pointer_type(ret, true));

    FunctionParam* self_param = decl->has_self_param() ? decl->get_self_param() : nullptr;
    bool self_in_params = false;
    for (FunctionParam* p : decl->params) {
        if (p == self_param) { self_in_params = true; break; }
    }
    if (self_in_params) self_param = nullptr;

    auto map_param = [&](FunctionParam* p) {
        TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
        if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types_.pointer_type(module_.types.get(pt).element, true);
        } else if (needs_aggregate(module_, pt)) {
            pt = types_.pointer_type(pt, true);
        }
        return pt;
    };
    if (self_param) ptypes.push_back(map_param(self_param));
    for (FunctionParam* p : decl->params) ptypes.push_back(map_param(p));
    if (decl->isVariadic() && !decl->params.empty()) ptypes.pop_back();

    return types_.function_signature(sret ? types_.void_type() : ret, ptypes, decl->isVariadic());
}

std::string MIRDeclEmitter::function_proto(FunctionDeclaration* decl) {
    std::string name = mangle_name(decl);
    if (decl->is_std_call()) name = "__chem_stdcall " + name;
    const TypeId ftype = function_type_of(decl);
    return c_function_proto(module_, ftype, name);
}

std::string MIRDeclEmitter::mangle_name(ASTNode* node) {
    if (!node) return std::string();
    BufferedWriter w(256);
    mangler_.mangle(w, node);
    return std::string(w.data(), w.getPosition());
}

void MIRDeclEmitter::write_escaped_string(BufferedWriter& w, const std::string& s) {
    w.append_char('"');
    for (char c : s) {
        switch (c) {
            case '\\': w.append("\\\\", 2); break;
            case '"':  w.append("\\\"", 2); break;
            case '\n': w.append("\\n", 2); break;
            case '\r': w.append("\\r", 2); break;
            case '\t': w.append("\\t", 2); break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\%03o",
                                  static_cast<unsigned>(static_cast<unsigned char>(c)));
                    w.append(buf, std::strlen(buf));
                } else {
                    w.append_char(c);
                }
        }
    }
    w.append_char('"');
}

std::string MIRDeclEmitter::const_value_expr(Value* value) {
    if (!value) return "0";
    switch (value->val_kind()) {
        case ValueKind::IntN: {
            auto* v = value->as_int_num_value_unsafe();
            if (v->is_unsigned()) return std::to_string(v->get_num_value());
            return std::to_string(static_cast<long long>(v->get_num_value()));
        }
        case ValueKind::Float: {
            BufferedWriter w(64);
            w << value->as_float_unsafe()->value;
            return std::string(w.data(), w.getPosition());
        }
        case ValueKind::Double: {
            BufferedWriter w(64);
            w << value->as_double_unsafe()->value;
            return std::string(w.data(), w.getPosition());
        }
        case ValueKind::Bool:
            return value->as_bool_unsafe()->value ? "1" : "0";
        case ValueKind::String: {
            auto* sv = value->as_string_unsafe();
            BufferedWriter w(sv->value.size() + 8);
            write_escaped_string(w, std::string(sv->value.data(), sv->value.size()));
            return std::string(w.data(), w.getPosition());
        }
        case ValueKind::NullValue:
            return "NULL";
        case ValueKind::NegativeValue:
            return "-" + const_value_expr(value->as_negative_value_unsafe()->getValue());
        case ValueKind::ComptimeValue: {
            auto* cv = static_cast<ComptimeValue*>(value);
            Value* inner = cv->getValue();
            return inner ? const_value_expr(inner) : "0";
        }
        case ValueKind::ArrayValue: {
            auto* arr = value->as_array_value_unsafe();
            std::string s = "{";
            bool first = true;
            for (Value* v : arr->values) {
                if (!first) s += ", ";
                s += const_value_expr(v);
                first = false;
            }
            if (first) s += "0";
            s += "}";
            return s;
        }
        case ValueKind::StructValue: {
            auto* sv = value->as_struct_value_unsafe();
            std::string s = "{";
            bool first = true;
            for (auto& entry : sv->values) {
                if (!first) s += ", ";
                s += ".";
                s += std::string(entry.second.name.data(), entry.second.name.size());
                s += " = ";
                s += const_value_expr(entry.second.value);
                first = false;
            }
            s += "}";
            return s;
        }
        case ValueKind::FunctionCall: {
            // variant construction: `Variant.Member(args)`
            auto* call = value->as_func_call_unsafe();
            ASTNode* vleaf = nullptr;
            if (call->parent_val) {
                if (call->parent_val->val_kind() == ValueKind::AccessChain) {
                    auto* ch = call->parent_val->as_access_chain_unsafe();
                    if (!ch->values.empty()) vleaf = ch->values.back()->linked_node();
                } else {
                    vleaf = call->parent_val->linked_node();
                }
            }
            if (vleaf && vleaf->kind() == ASTNodeKind::VariantMember) {
                auto* vm = vleaf->as_variant_member_unsafe();
                VariantDefinition* vd = vm->parent();
                const long idx = vd ? vd->direct_child_index(vm->name) : 0;
                std::string s = "(" + type_spelling(call->getType()) + ") { .__chx__vt_621827 = " +
                                std::to_string(idx < 0 ? 0 : idx);
                size_t ai = 0;
                for (auto& entry : vm->values) {
                    VariantMemberParam* param = entry.second;
                    Value* arg = nullptr;
                    if (ai < call->values.size()) {
                        arg = call->values[ai];
                        ai++;
                    } else if (param) {
                        arg = param->def_value;
                    }
                    if (!arg) continue;
                    const std::string mname(vm->name.data(), vm->name.size());
                    const std::string pname(entry.first.data(), entry.first.size());
                    s += ", ." + mname + "." + pname + " = " + const_value_expr(arg);
                }
                s += " }";
                return s;
            }
            return "0";
        }
        case ValueKind::AccessChain: {
            // a bare variant member (`Variant.Member`, no args) or a chain whose
            // leaf is the constructor call
            auto* ch = value->as_access_chain_unsafe();
            if (ch->values.empty()) return "0";
            Value* last = ch->values.back();
            if (last->val_kind() == ValueKind::FunctionCall) return const_value_expr(last);
            ASTNode* linked = last->linked_node();
            if (linked) {
                if (linked->kind() == ASTNodeKind::VariantMember) {
                    auto* vm = linked->as_variant_member_unsafe();
                    VariantDefinition* vd = vm->parent();
                    const long idx = vd ? vd->direct_child_index(vm->name) : 0;
                    return "(" + type_spelling(value->getType()) +
                           ") { .__chx__vt_621827 = " + std::to_string(idx < 0 ? 0 : idx) + " }";
                }
                if (linked->kind() == ASTNodeKind::FunctionDecl) {
                    // a global function pointer/reference (`fn<T>`)
                    return mangle_name(linked);
                }
                if (linked->kind() == ASTNodeKind::VarInitStmt) {
                    auto* vi = linked->as_var_init_unsafe();
                    if (vi->value) return const_value_expr(vi->value);
                }
            }
            return "0";
        }
        case ValueKind::CastedValue:
            return const_value_expr(value->as_casted_value_unsafe()->value);
        default:
            return "0";
    }
}

// ---------------------------------------------------------------------------
// preamble
// ---------------------------------------------------------------------------

void MIRDeclEmitter::emit_preamble(BufferedWriter& w) {
    w << "#ifndef __cplusplus\n"
         "    #ifndef bool\n"
         "        #define bool _Bool\n"
         "    #endif\n"
         "    #ifndef true\n"
         "        #define true 1\n"
         "    #endif\n"
         "    #ifndef false\n"
         "        #define false 0\n"
         "    #endif\n"
         "#endif\n"
         "#ifndef __bool_true_false_are_defined\n"
         "    #define __bool_true_false_are_defined 1\n"
         "#endif\n"
         "#ifndef NULL\n"
         "    #ifdef __cplusplus\n"
         "        #define NULL 0\n"
         "    #else\n"
         "        #define NULL ((void*)0)\n"
         "    #endif\n"
         "#endif\n"
         "#ifndef offsetof\n"
         "    #define offsetof(type, member) ((uint64_t)&(((type*)0)->member))\n"
         "#endif\n";

    const auto& target = comptime_scope_.target_data;
    const char* int64_type = (target.is64Bit && !target.win64) ? "long" : "long long";
    const char* uint64_type = (target.is64Bit && !target.win64) ? "unsigned long" : "unsigned long long";
    w << "typedef signed char int8_t;\n"
         "typedef unsigned char uint8_t;\n"
         "typedef short int16_t;\n"
         "typedef unsigned short uint16_t;\n"
         "typedef int int32_t;\n"
         "typedef unsigned int uint32_t;\n"
         "typedef ";
    w.append(int64_type, std::strlen(int64_type));
    w << " int64_t;\n"
         "typedef ";
    w.append(uint64_type, std::strlen(uint64_type));
    w << " uint64_t;\n";

    // fat pointer
    w << "typedef struct {\n\tvoid* first;\n\tvoid* second;\n} __chemical_fat_pointer__;\n";

    if (target.win64) {
        w << "extern void* malloc(unsigned long long size);\n";
    } else {
        w << "extern void* malloc(unsigned long size);\n";
    }
    w << "extern void free(void* block);\n"
         "#if defined(_MSC_VER)\n"
         "    #define __chem_stdcall __stdcall\n"
         "    #define __chem_dllimport __declspec(\"dllimport\")\n"
         "#else\n"
         "    #define __chem_stdcall __attribute__((stdcall))\n"
         "    #define __chem_dllimport __attribute__((dllimport))\n"
         "#endif\n";
    w << "#ifndef __chx_align\n"
         "#if defined(_MSC_VER)\n"
         "  #define __chx_align(N) __declspec(align(N))\n"
         "  #define __chx_align_struct(N) __declspec(align(N)) struct\n"
         "#else\n"
         "  #define __chx_align(N) __attribute__((aligned(N)))\n"
         "  #define __chx_align_struct(N) struct __attribute__((aligned(N)))\n"
         "#endif\n"
         "#endif\n";
    w << "#ifndef __chx_thread_local\n"
         "#if defined(__TINYC__)\n"
         "  #define __chx_thread_local\n"
         "#elif defined(_MSC_VER)\n"
         "  #define __chx_thread_local __declspec(\"thread\")\n"
         "#elif defined(__cplusplus) && (__cplusplus >= 201103L)\n"
         "  #define __chx_thread_local thread_local\n"
         "#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) && !defined(__STDC_NO_THREADS__)\n"
         "  #define __chx_thread_local _Thread_local\n"
         "#elif defined(__GNUC__) || defined(__clang__)\n"
         "  #define __chx_thread_local __thread\n"
         "#else\n"
         "  #define __chx_thread_local _Thread_local\n"
         "#endif\n"
         "#endif\n";
    w << "#if defined(__clang__)\n"
         "#pragma clang diagnostic ignored \"-Wincompatible-pointer-types\"\n"
         "#elif defined(__GNUC__)\n"
         "#pragma GCC diagnostic ignored \"-Wincompatible-pointer-types\"\n"
         "#elif defined(_MSC_VER)\n"
         "#pragma warning(disable: 4047 4024)\n"
         "#endif\n";
}

// ---------------------------------------------------------------------------
// forward declarations / type aliases
// ---------------------------------------------------------------------------

void MIRDeclEmitter::fwd_declare(BufferedWriter& w, ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::NamespaceDecl:
            for (ASTNode* child : node->as_namespace_unsafe()->nodes) fwd_declare(w, child);
            break;
        case ASTNodeKind::Scope:
            for (ASTNode* child : node->as_scope_unsafe()->nodes) fwd_declare(w, child);
            break;
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl:
            w.append("\nstruct ", 8);
            mangler_.mangle(w, node);
            w.append_char(';');
            break;
        case ASTNodeKind::UnionDecl:
            w.append("\nunion ", 7);
            mangler_.mangle(w, node);
            w.append_char(';');
            break;
        default:
            break;
    }
}

void MIRDeclEmitter::declare_typedef(BufferedWriter& w, TypealiasStatement* stmt) {
    if (!stmt || stmt->has_declared) return;
    stmt->has_declared = true;
    // declaring the underlying type first pulls in composed/capturing-function
    // instance structs the typedef's type depends on
    early_declare_type(w, stmt->actual_type, nullptr, true, true);
    w.append_char('\n');
    w.append("typedef ", 8);
    w << decl_spelling(stmt->actual_type, mangle_name(stmt));
    w.append_char(';');
}

void MIRDeclEmitter::declare_type_alias(BufferedWriter& w, ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::TypealiasStmt:
            declare_typedef(w, node->as_typealias_unsafe());
            break;
        case ASTNodeKind::NamespaceDecl:
            for (ASTNode* child : node->as_namespace_unsafe()->nodes) declare_type_alias(w, child);
            break;
        case ASTNodeKind::Scope:
            for (ASTNode* child : node->as_scope_unsafe()->nodes) declare_type_alias(w, child);
            break;
        case ASTNodeKind::IfStmt: {
            auto* stmt = node->as_if_stmt_unsafe();
            if (stmt->computed_scope.has_value()) {
                Scope* scope = stmt->computed_scope.value();
                if (scope) for (ASTNode* child : scope->nodes) declare_type_alias(w, child);
            }
            break;
        }
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// declare-pass: functions / members
// ---------------------------------------------------------------------------

static bool is_linkage_public(AccessSpecifier spec) {
    return spec == AccessSpecifier::Public;
}

void MIRDeclEmitter::declare_contained_func(BufferedWriter& w, FunctionDeclaration* decl,
                                             bool terminate) {
    if (decl->is_comptime()) return;
    w.append_char('\n');
    const bool is_static = decl->body.has_value() && !decl->is_linkage_public();
    std::string name = mangle_name(decl);
    if (decl->is_std_call()) name = "__chem_stdcall " + name;
    const TypeId ftype = function_type_of(decl);
    if (decl->is_extern()) w.append("extern ", 7);
    if (is_static) w.append("static ", 7);
    if (decl->is_dll_import()) w.append("__chem_dllimport ", 17);
    w << c_function_proto(module_, ftype, name);
    if (terminate) w.append_char(';');
}

// ---------------------------------------------------------------------------
// early declaration (composed variable ordering)
// ---------------------------------------------------------------------------

void MIRDeclEmitter::early_declare_type(BufferedWriter& w, BaseType* type, ASTNode* container,
                                        bool do_ref, bool do_ptr) {
    if (!type) return;
    switch (type->kind()) {
        case BaseTypeKind::Reference:
            if (do_ref) early_declare_type(w, type->as_reference_type_unsafe()->type, container, do_ref, do_ptr);
            return;
        case BaseTypeKind::Pointer:
            if (do_ptr) early_declare_type(w, type->as_pointer_type_unsafe()->type, container, do_ref, do_ptr);
            return;
        case BaseTypeKind::Linked: {
            ASTNode* linked = type->as_linked_type_unsafe()->linked;
            if (linked != container) early_declare_node(w, linked);
            return;
        }
        case BaseTypeKind::Dynamic:
            early_declare_type(w, type->as_dynamic_type_unsafe()->referenced, container);
            return;
        case BaseTypeKind::CapturingFunction:
            early_declare_type(w, type->as_capturing_func_type_unsafe()->instance_type, container, do_ref, do_ptr);
            return;
        case BaseTypeKind::Array:
            early_declare_type(w, type->as_array_type_unsafe()->elem_type, container);
            return;
        case BaseTypeKind::Generic: {
            auto* gt = type->as_generic_type_unsafe();
            ASTNode* linked = gt->referenced ? gt->referenced->linked : nullptr;
            if (linked && linked != container) early_declare_node(w, linked);
            for (BaseType* ty : gt->types) early_declare_type(w, ty, container);
            return;
        }
        default:
            return;
    }
}

void MIRDeclEmitter::early_declare_composed_variables(BufferedWriter& w, VariablesContainer& container) {
    for (BaseDefMember* variable : container.variables()) {
        BaseType* t = variable->known_type();
        if (!t) continue;
        // A generic alias member (`std::function<T>`) links to the alias
        // declaration; declaring it pulls in the alias typedef which in turn
        // early-declares the capturing-function instance struct.
        ASTNode* node = t->get_direct_linked_node();
        if (node) early_declare_node(w, node);
    }
    for (auto& inh : container.inherited) {
        ASTNode* node = inh.type ? inh.type->get_direct_linked_node() : nullptr;
        if (node) early_declare_node(w, node);
    }
}

void MIRDeclEmitter::early_declare_node(BufferedWriter& w, ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::TypealiasStmt: {
            declare_typedef(w, node->as_typealias_unsafe());
            return;
        }
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl:
        case ASTNodeKind::UnionDecl:
        case ASTNodeKind::UnnamedStruct:
        case ASTNodeKind::UnnamedUnion: {
            if (!early_declare_in_progress_.insert(node).second) return;
            switch (node->kind()) {
                case ASTNodeKind::StructDecl: {
                    auto* def = node->as_struct_def_unsafe();
                    if (!def->has_declared) {
                        def->has_declared = true;
                        declare_struct_def_only(w, def);
                    }
                    break;
                }
                case ASTNodeKind::VariantDecl: {
                    auto* def = node->as_variant_def_unsafe();
                    if (!def->has_declared) {
                        def->has_declared = true;
                        declare_variant_def_only(w, def);
                    }
                    break;
                }
                case ASTNodeKind::UnionDecl: {
                    auto* def = node->as_union_def_unsafe();
                    if (!def->has_declared) {
                        def->has_declared = true;
                        declare_union_def_only(w, def);
                    }
                    break;
                }
                default:
                    break;
            }
            early_declare_in_progress_.erase(node);
            return;
        }
        default:
            return;
    }
}

// ---------------------------------------------------------------------------
// struct / union / variant definitions
// ---------------------------------------------------------------------------

static void write_indent(BufferedWriter& w, int indent) {
    w.append_char('\n');
    for (int i = 0; i < indent; ++i) w.append_char('\t');
}

void MIRDeclEmitter::emit_member(BufferedWriter& w, BaseDefMember* member, int indent) {
    if (!member) return;
    switch (member->kind()) {
        case ASTNodeKind::UnnamedStruct: {
            w.append("struct {", 8);
            for (BaseDefMember* f : member->as_unnamed_struct_unsafe()->variables()) {
                write_indent(w, indent + 1);
                emit_member(w, f, indent + 1);
            }
            write_indent(w, indent);
            w.append("} ", 2);
            w << member->name;
            w.append_char(';');
            return;
        }
        case ASTNodeKind::UnnamedUnion: {
            w.append("union {", 7);
            for (BaseDefMember* f : member->as_unnamed_union_unsafe()->variables()) {
                write_indent(w, indent + 1);
                emit_member(w, f, indent + 1);
            }
            write_indent(w, indent);
            w.append("} ", 2);
            w << member->name;
            w.append_char(';');
            return;
        }
        case ASTNodeKind::StructMember: {
            auto* sm = member->as_struct_member_unsafe();
            if (sm->get_required_alignment() > 0) {
                w.append("__chx_align(", 12);
                w << sm->get_required_alignment();
                w.append(") ", 2);
            }
            if (sm->is_volatile()) w.append("volatile ", 9);
            if (sm->type && sm->type->kind() == BaseTypeKind::CapturingFunction) {
                w.append("__chemical_fat_pointer__* ", 25);
                w << sm->name;
            } else {
                w << decl_spelling(sm->type, std::string(sm->name.data(), sm->name.size()));
            }
            w.append_char(';');
            return;
        }
        default:
            w << decl_spelling(member->known_type(), std::string(member->name.data(), member->name.size()));
            w.append_char(';');
            return;
    }
}

void MIRDeclEmitter::declare_struct_def_only(BufferedWriter& w, StructDefinition* def) {
    early_declare_composed_variables(w, *def);
    w.append_char('\n');
    if (def->get_required_alignment() > 0) {
        w << "__chx_align_struct(" << def->get_required_alignment() << ") ";
        mangler_.mangle(w, def);
    } else {
        w.append("struct ", 7);
        mangler_.mangle(w, def);
    }
    w.append(" {", 2);
    for (auto& inherits : def->inherited) {
        StructDefinition* sd = inherits.type ? inherits.type->get_direct_linked_struct() : nullptr;
        if (sd) {
            w.append("\n\tstruct ", 9);
            mangler_.mangle(w, sd);
            w.append_char(' ');
            w << sd->name_view();
            w.append_char(';');
        }
    }
    for (BaseDefMember* var : def->variables()) {
        write_indent(w, 1);
        emit_member(w, var, 1);
    }
    w.append("\n};", 3);
}

void MIRDeclEmitter::declare_union_def_only(BufferedWriter& w, UnionDef* def) {
    early_declare_composed_variables(w, *def);
    w.append_char('\n');
    w.append("union ", 6);
    mangler_.mangle(w, def);
    w.append(" {", 2);
    for (auto& inherits : def->inherited) {
        StructDefinition* sd = inherits.type ? inherits.type->get_direct_linked_struct() : nullptr;
        if (sd) {
            w.append("\n\tstruct ", 9);
            mangler_.mangle(w, sd);
            w.append_char(' ');
            w << sd->name_view();
            w.append_char(';');
        }
    }
    for (BaseDefMember* var : def->variables()) {
        write_indent(w, 1);
        emit_member(w, var, 1);
    }
    w.append("\n};", 3);
}

void MIRDeclEmitter::declare_variant_def_only(BufferedWriter& w, VariantDefinition* def) {
    // declare inherited + sub variables first
    for (auto& inh : def->inherited) {
        ASTNode* n = inh.type ? inh.type->get_direct_linked_node() : nullptr;
        if (n) early_declare_node(w, n);
    }
    for (BaseDefMember* var : def->variables()) {
        auto* member = var->as_variant_member_unsafe();
        for (auto& val : member->values) {
            early_declare_type(w, val.second->type, def);
        }
    }
    w.append_char('\n');
    w.append("struct ", 7);
    mangler_.mangle(w, def);
    w.append(" {", 2);
    for (auto& inherits : def->inherited) {
        StructDefinition* sd = inherits.type ? inherits.type->get_direct_linked_struct() : nullptr;
        if (sd) {
            w.append("\n\tstruct ", 9);
            mangler_.mangle(w, sd);
            w.append_char(' ');
            w << sd->name_view();
            w.append_char(';');
        }
    }
    w.append("\n\tint __chx__vt_621827;", 23);
    w.append("\n\tunion {", 9);
    for (BaseDefMember* var : def->variables()) {
        auto* member = var->as_variant_member_unsafe();
        w.append("\n\t\tstruct {", 11);
        for (auto& val : member->values) {
            w.append("\n\t\t\t", 3);
            w << decl_spelling(val.second->type, std::string(val.second->name.data(), val.second->name.size()));
            w.append_char(';');
        }
        w.append("\n\t\t} ", 5);
        w << member->name;
        w.append_char(';');
    }
    w.append("\n\t};", 4);
    w.append("\n};", 3);
}

void MIRDeclEmitter::declare_struct_iterations(BufferedWriter& w, StructDefinition* def) {
    if (!def->has_declared) {
        def->has_declared = true;
        declare_struct_def_only(w, def);
    }
    for (FunctionDeclaration* func : def->non_gen_range()) {
        declare_contained_func(w, func, true);
    }
    for (ASTNode* child : def->evaluated_nodes()) {
        if (child->kind() == ASTNodeKind::ImplDecl) {
            declare_impl(w, child->as_impl_def_unsafe());
        }
    }
}

void MIRDeclEmitter::declare_union_iterations(BufferedWriter& w, UnionDef* def) {
    if (!def->has_declared) {
        def->has_declared = true;
        declare_union_def_only(w, def);
    }
    for (FunctionDeclaration* func : def->non_gen_range()) {
        declare_contained_func(w, func, true);
    }
}

void MIRDeclEmitter::declare_variant_iterations(BufferedWriter& w, VariantDefinition* def) {
    if (!def->has_declared) {
        def->has_declared = true;
        declare_variant_def_only(w, def);
    }
    for (FunctionDeclaration* func : def->non_gen_range()) {
        declare_contained_func(w, func, true);
    }
}

// ---------------------------------------------------------------------------
// vtables
// ---------------------------------------------------------------------------

void MIRDeclEmitter::vtable_type_name(BufferedWriter& w, InterfaceDefinition* interface) {
    w.append("__chx_", 6);
    mangler_.mangle(w, interface);
    w.append("_vt_t", 5);
}

void MIRDeclEmitter::vtable_name(BufferedWriter& w, InterfaceDefinition* interface,
                                 ExtendableMembersContainerNode* def) {
    mangler_.mangle_vtable_name(w, interface, def);
}

void MIRDeclEmitter::create_vtable_type(BufferedWriter& w, InterfaceDefinition* interface) {
    w.append_char('\n');
    w.append("typedef struct ", 15);
    vtable_type_name(w, interface);
    w.append(" {", 2);
    interface->active_user = nullptr;
    // recursive over inherited interfaces then own functions
    std::function<void(InterfaceDefinition*)> rec = [&](InterfaceDefinition* iface) {
        for (auto& inh : iface->inherited) {
            InterfaceDefinition* inherited = inh.type ? inh.type->get_direct_linked_interface() : nullptr;
            if (inherited) rec(inherited);
        }
        for (FunctionDeclaration* func : iface->instantiated_functions()) {
            w.append_char('\n');
            FunctionParam* self = func->get_self_param();
            TypeId ret = func->returnType ? types_.map(func->returnType) : types_.void_type();
            const bool sret = needs_sret(module_, ret);
            std::vector<TypeId> ptypes;
            // vtable slots are called as `fn(void* self, ...)`; the interface's
            // `self` is an opaque pointer regardless of the concrete impl.
            if (self) ptypes.push_back(types_.pointer_type(types_.void_type(), true));
            if (sret) ptypes.push_back(types_.pointer_type(ret, true));
            bool skipped_self = false;
            for (FunctionParam* p : func->params) {
                if (p == self && !skipped_self) { skipped_self = true; continue; }
                TypeId pt = p && p->type ? types_.map(p->type) : types_.opaque_type();
                if (pt != MIR_INVALID_ID && module_.types.get(pt).kind == MIRTypeKind::Array) {
                    pt = types_.pointer_type(module_.types.get(pt).element, true);
                } else if (needs_aggregate(module_, pt)) {
                    pt = types_.pointer_type(pt, true);
                }
                ptypes.push_back(pt);
            }
            TypeId stype = types_.function_signature(sret ? types_.void_type() : ret, ptypes,
                                                     func->isVariadic());
            const std::string slot = std::string(func->name_view().data(), func->name_view().size());
            w << c_decl_type(module_, stype, slot);
            w.append_char(';');
        }
    };
    rec(interface);
    w.append("\n}", 2);
    w.append_char(' ');
    vtable_type_name(w, interface);
    w.append_char(';');
}

void MIRDeclEmitter::declare_vtable_extern(BufferedWriter& w, InterfaceDefinition* interface,
                                           ExtendableMembersContainerNode* def) {
    w.append_char('\n');
    w.append("extern const ", 13);
    vtable_type_name(w, interface);
    w.append_char(' ');
    vtable_name(w, interface, def);
    w.append_char(';');
}

void MIRDeclEmitter::create_vtable(BufferedWriter& w, InterfaceDefinition* interface,
                                   ExtendableMembersContainerNode* def) {
    w.append_char('\n');
    w.append("const ", 6);
    vtable_type_name(w, interface);
    w.append_char(' ');
    vtable_name(w, interface, def);
    w.append(" = {", 4);
    std::function<void(InterfaceDefinition*)> rec = [&](InterfaceDefinition* iface) {
        for (auto& inh : iface->inherited) {
            InterfaceDefinition* inherited = inh.type ? inh.type->get_direct_linked_interface() : nullptr;
            if (inherited) rec(inherited);
        }
        for (FunctionDeclaration* func : iface->instantiated_functions()) {
            w.append_char('\n');
            FunctionParam* self = func->get_self_param();
            std::string name = "__chx_unresolved";
            if (self) {
                FunctionDeclaration* overrider = def->direct_child_function(func->name_view());
                if (overrider) {
                    name = mangle_name(overrider);
                } else {
                    const auto prev = iface->active_user;
                    iface->active_user = def;
                    name = mangle_name(func);
                    iface->active_user = prev;
                }
            } else {
                const auto prev = iface->active_user;
                iface->active_user = def;
                name = mangle_name(func);
                iface->active_user = prev;
            }
            w << name;
            w.append_char(',');
        }
    };
    rec(interface);
    w.append("\n};", 3);
}

void MIRDeclEmitter::declare_vtable_primitive_extern(BufferedWriter& w, InterfaceDefinition* interface,
                                                     BaseType* impl) {
    w.append_char('\n');
    w.append("extern const ", 13);
    vtable_type_name(w, interface);
    w.append_char(' ');
    mangler_.mangle_vtable_name(w, interface, impl);
    w.append_char(';');
}

void MIRDeclEmitter::create_vtable_primitive(BufferedWriter& w, InterfaceDefinition* interface,
                                             ImplDefinition* def) {
    w.append_char('\n');
    w.append("const ", 6);
    vtable_type_name(w, interface);
    w.append_char(' ');
    mangler_.mangle_vtable_name(w, interface, def->struct_type);
    w.append(" = {", 4);
    for (FunctionDeclaration* func : interface->instantiated_functions()) {
        if (!func->has_self_param()) continue;
        FunctionDeclaration* overrider = def->direct_child_function(func->name_view());
        if (overrider) {
            w.append_char('\n');
            w << mangle_name(overrider);
            w.append_char(',');
        }
    }
    w.append("\n};", 3);
}

// ---------------------------------------------------------------------------
// impl / interface declarations
// ---------------------------------------------------------------------------

void MIRDeclEmitter::declare_impl(BufferedWriter& w, ImplDefinition* def) {
    if (!def->struct_type) return;
    InterfaceDefinition* interface_def = def->interface_type
        ? def->interface_type->get_direct_linked_interface() : nullptr;
    if (!interface_def) return;
    if (interface_def->is_static()) return;
    ExtendableMembersContainerNode* container = def->struct_type->get_members_container()
        ? def->struct_type->get_members_container()->as_extendable_members_container_unsafe() : nullptr;
    if (container) {
        interface_def->active_user = container;
        for (FunctionDeclaration* func : def->instantiated_functions()) {
            declare_contained_func(w, func, true);
        }
        interface_def->active_user = nullptr;
        if (interface_def->generates_vtable()) {
            if (!interface_def->has_declared) {
                create_vtable_type(w, interface_def);
                interface_def->has_declared = true;
            }
            declare_vtable_extern(w, interface_def, container);
        }
    } else {
        for (FunctionDeclaration* func : def->instantiated_functions()) {
            declare_contained_func(w, func, true);
        }
        if (interface_def->generates_vtable()) {
            if (!interface_def->has_declared) {
                create_vtable_type(w, interface_def);
                interface_def->has_declared = true;
            }
            declare_vtable_primitive_extern(w, interface_def, def->struct_type);
        }
    }
}

void MIRDeclEmitter::declare_interface(BufferedWriter& w, InterfaceDefinition* def) {
    if (def->is_static()) {
        for (FunctionDeclaration* func : def->non_gen_range()) {
            declare_contained_func(w, func, true);
        }
        if (!def->is_extern() && static_recorder_) static_recorder_(def);
    } else {
        def->active_user = nullptr;
        if (!def->has_declared && def->generates_vtable()) {
            create_vtable_type(w, def);
        }
        def->has_declared = true;
    }
}

// ---------------------------------------------------------------------------
// globals
// ---------------------------------------------------------------------------

void MIRDeclEmitter::declare_var(BufferedWriter& w, VarInitStatement* init) {
    if (!init->is_top_level()) return;
    if (init->is_comptime()) return;
    BaseType* init_type = init->type ? init->type : (init->value ? init->value->getType() : nullptr);
    early_declare_type(w, init_type, nullptr);
    w.append_char('\n');
    const bool is_static = !init->is_linkage_public();
    if (init->is_thread_local()) w.append("__chx_thread_local ", 19);
    if (is_static) w.append("static ", 7);
    if (init->is_volatile()) w.append("volatile ", 9);
    w << decl_spelling(init_type, mangle_name(init));
    w.append_char(';');
}

// ---------------------------------------------------------------------------
// top-level declare
// ---------------------------------------------------------------------------

void MIRDeclEmitter::declare(BufferedWriter& w, ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::NamespaceDecl: {
            auto* ns = node->as_namespace_unsafe();
            if (ns->is_comptime()) return;
            for (ASTNode* child : ns->nodes) declare(w, child);
            return;
        }
        case ASTNodeKind::Scope: {
            for (ASTNode* child : node->as_scope_unsafe()->nodes) declare(w, child);
            return;
        }
        case ASTNodeKind::FunctionDecl:
            declare_contained_func(w, node->as_function_unsafe(), true);
            return;
        case ASTNodeKind::StructDecl:
            declare_struct_iterations(w, node->as_struct_def_unsafe());
            return;
        case ASTNodeKind::UnionDecl:
            declare_union_iterations(w, node->as_union_def_unsafe());
            return;
        case ASTNodeKind::VariantDecl:
            declare_variant_iterations(w, node->as_variant_def_unsafe());
            return;
        case ASTNodeKind::InterfaceDecl:
            declare_interface(w, node->as_interface_def_unsafe());
            return;
        case ASTNodeKind::ImplDecl:
            declare_impl(w, node->as_impl_def_unsafe());
            return;
        case ASTNodeKind::VarInitStmt:
            declare_var(w, node->as_var_init_unsafe());
            return;
        case ASTNodeKind::TypealiasStmt: {
            declare_typedef(w, node->as_typealias_unsafe());
            return;
        }
        case ASTNodeKind::IfStmt: {
            auto* stmt = node->as_if_stmt_unsafe();
            if (!stmt->is_top_level()) return;
            if (stmt->computed_scope.has_value()) {
                Scope* scope = stmt->computed_scope.value();
                if (scope) for (ASTNode* child : scope->nodes) declare(w, child);
            }
            return;
        }
        case ASTNodeKind::EmbeddedNode: {
            auto* en = node->as_embedded_node_unsafe();
            ASTNode* repl = embedded_decl_repl_ ? embedded_decl_repl_(en) : nullptr;
            if (repl) {
                en->replacement = repl;
                declare(w, repl);
            }
            return;
        }
        case ASTNodeKind::EnumDecl:
            return;
        default:
            return;
    }
}

// ---------------------------------------------------------------------------
// implement-pass: globals + vtables
// ---------------------------------------------------------------------------

void MIRDeclEmitter::implement(BufferedWriter& w, ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::NamespaceDecl: {
            auto* ns = node->as_namespace_unsafe();
            if (ns->is_comptime()) return;
            for (ASTNode* child : ns->nodes) implement(w, child);
            return;
        }
        case ASTNodeKind::Scope: {
            for (ASTNode* child : node->as_scope_unsafe()->nodes) implement(w, child);
            return;
        }
        case ASTNodeKind::VarInitStmt: {
            auto* init = node->as_var_init_unsafe();
            if (!init->is_top_level()) return;
            if (init->is_comptime()) return;
            if (!init->value) return;
            BaseType* init_type = init->type ? init->type : init->value->getType();
            w.append_char('\n');
            const bool is_static = !init->is_linkage_public();
            if (init->is_thread_local()) w.append("__chx_thread_local ", 19);
            if (is_static) w.append("static ", 7);
            if (init->is_volatile()) w.append("volatile ", 9);
            w << decl_spelling(init_type, mangle_name(init));
            w.append(" = ", 3);
            w << const_value_expr(init->value);
            w.append_char(';');
            return;
        }
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl:
        case ASTNodeKind::UnionDecl: {
            auto* def = node->as_extendable_member_container();
            if (!def) return;
            for (ASTNode* child : def->evaluated_nodes()) {
                if (child->kind() == ASTNodeKind::ImplDecl) {
                    implement(w, child);
                }
            }
            return;
        }
        case ASTNodeKind::ImplDecl: {
            auto* def = node->as_impl_def_unsafe();
            if (!def->struct_type) return;
            InterfaceDefinition* interface_def = def->interface_type
                ? def->interface_type->get_direct_linked_interface() : nullptr;
            if (!interface_def || interface_def->is_static()) return;
            ExtendableMembersContainerNode* container = def->struct_type->get_members_container()
                ? def->struct_type->get_members_container()->as_extendable_members_container_unsafe() : nullptr;
            if (container) {
                if (interface_def->generates_vtable()) create_vtable(w, interface_def, container);
            } else {
                if (interface_def->generates_vtable()) create_vtable_primitive(w, interface_def, def);
            }
            return;
        }
        case ASTNodeKind::IfStmt: {
            auto* stmt = node->as_if_stmt_unsafe();
            if (!stmt->is_top_level()) return;
            if (stmt->computed_scope.has_value()) {
                Scope* scope = stmt->computed_scope.value();
                if (scope) for (ASTNode* child : scope->nodes) implement(w, child);
            }
            return;
        }
        default:
            return;
    }
}

void MIRDeclEmitter::end_translate(BufferedWriter& w) {
    (void)w;
}

} // namespace mir
