// Copyright (c) Chemical Language Foundation 2025.

#include "MIRDeclEmitter.h"

#include "MIREmitter.h"
#include "MIRABI.h"
#include "MIRTypeBuilder.h"
#include "MIRVtable.h"
#include "MIRPrototype.h"
#include "MIRGlobalDecl.h"

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
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/EmbeddedNode.h"
#include "compiler/mangler/NameMangler.h"
#include "preprocess/2c/BufferedWriter.h"
#include "core/source/LocationManager.h"

#include <cstdio>

namespace mir {

// ---------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------

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

// The prototype ABI (sret, aggregates/arrays by pointer, the static-interface
// erased self, variadic tail) lives in MIRPrototype so the C prototype and the
// LLVM call/symbol cannot drift — see `mir::mir_function_signature`.

std::string MIRDeclEmitter::function_proto(FunctionDeclaration* decl) {
    MIRPrototype proto;
    build_function_prototype(mangler_, types_, module_, decl, proto);
    return c_function_proto(module_, proto.type, proto.name);
}

std::string MIRDeclEmitter::mangle_name(ASTNode* node) {
    if (!node) return std::string();
    BufferedWriter w(256);
    mangler_.mangle(w, node);
    return std::string(w.data(), w.getPosition());
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
    // The prototype (name, signature, linkage flags) is a MIR record built by the
    // shared `build_function_prototype` (action A3), the same one the LLVM path
    // uses via `mir_function_signature`.
    MIRPrototype proto;
    build_function_prototype(mangler_, types_, module_, decl, proto);
    w.append_char('\n');
    if (proto.flags & MPF_EXTERN) w.append("extern ", 7);
    if (proto.flags & MPF_STATIC) w.append("static ", 7);
    if (proto.flags & MPF_DLLIMPORT) w.append("__chem_dllimport ", 17);
    w << c_function_proto(module_, proto.type, proto.name);
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

// The C body of an aggregate comes from MIR (action A3): `aggregate_body_c`
// spells every member, the inherited bases, the variant tag and the inline case
// payloads from the type/field tables. The front end only decides *that* this
// declaration exists (and in what order), which is what the surrounding walk is
// for. Unreachable by construction: a declared aggregate always maps to a named
// MIR type; if it ever does not, the body is absent and the C compile fails
// loudly rather than the module silently losing a definition.
void MIRDeclEmitter::declare_aggregate_body(BufferedWriter& w, BaseType* type,
                                            const char* what) {
    if (!type) return;
    const TypeId id = types_.map(type);
    const std::string body = aggregate_body_c(module_, id);
    if (body.empty()) {
        const std::string msg = std::string("\n/* MIR declaration: unmapped or unnamed ") +
                                what + " - its C body could not be printed */";
        w.append(msg.data(), msg.size());
        return;
    }
    w << body;
}

void MIRDeclEmitter::declare_struct_def_only(BufferedWriter& w, StructDefinition* def) {
    early_declare_composed_variables(w, *def);
    declare_aggregate_body(w, def->known_type(), "struct");
}

void MIRDeclEmitter::declare_union_def_only(BufferedWriter& w, UnionDef* def) {
    early_declare_composed_variables(w, *def);
    declare_aggregate_body(w, def->known_type(), "union");
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
    declare_aggregate_body(w, def->known_type(), "variant");
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
    // the ordered slot list (inherited first, then own methods) and each slot's
    // self-erased type come from the shared MIR vtable layout (MIRVtable.h), so
    // the C and LLVM backends cannot drift
    std::vector<MIRVtableSlot> slots;
    mir_vtable_slots(types_, module_, interface, slots);
    w.append_char('\n');
    w.append("typedef struct ", 15);
    vtable_type_name(w, interface);
    w.append(" {", 2);
    interface->active_user = nullptr;
    for (const MIRVtableSlot& slot : slots) {
        w.append_char('\n');
        w << c_decl_type(module_, slot.type, slot.name);
        w.append_char(';');
    }
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
    std::vector<MIRVtableSlot> slots;
    std::vector<FunctionDeclaration*> funcs;
    mir_vtable_slots(types_, module_, interface, slots, &funcs);
    w.append_char('\n');
    w.append("const ", 6);
    vtable_type_name(w, interface);
    w.append_char(' ');
    vtable_name(w, interface, def);
    w.append(" = {", 4);
    for (size_t i = 0; i < funcs.size(); ++i) {
        FunctionDeclaration* func = funcs[i];
        w.append_char('\n');
        FunctionParam* self = func->get_self_param();
        std::string name = "__chx_unresolved";
        ASTNode* parent = func->parent();
        InterfaceDefinition* owner =
            (parent && parent->kind() == ASTNodeKind::InterfaceDecl)
                ? parent->as_interface_def_unsafe() : nullptr;
        if (self) {
            FunctionDeclaration* overrider = def->direct_child_function(func->name_view());
            if (overrider) {
                name = mangle_name(overrider);
            } else if (owner) {
                const auto prev = owner->active_user;
                owner->active_user = def;
                name = mangle_name(func);
                owner->active_user = prev;
            }
        } else if (owner) {
            const auto prev = owner->active_user;
            owner->active_user = def;
            name = mangle_name(func);
            owner->active_user = prev;
        }
        w.append_char('(');
        w << c_decl_type(module_, slots[i].type, "");
        w.append(") ", 2);
        w << name;
        w.append_char(',');
    }
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

namespace {
/** The C declarator of a global declaration (see MIRGlobalDecl.h). */
std::string global_decl_spelling(const MIRModule& module, const MIRGlobalDecl& d) {
    if (d.unsized_array) return c_declarator(module, d.elem, d.name + "[]");
    if (d.type == MIR_INVALID_ID) return "void " + d.name;
    return c_declarator(module, d.type, d.name);
}
} // namespace

void MIRDeclEmitter::declare_var(BufferedWriter& w, VarInitStatement* init) {
    if (!init->is_top_level()) return;
    if (init->is_comptime()) return;
    // name / declared type / unsized-array / flags come from the shared
    // `build_global_declaration` (action A3), the same record `implement_global`
    // prints its definition from.
    MIRGlobalDecl decl;
    build_global_declaration(mangler_, types_, module_, init, decl);
    BaseType* init_type = init->type ? init->type : (init->value ? init->value->getType() : nullptr);
    early_declare_type(w, init_type, nullptr);
    w.append_char('\n');
    if (decl.flags & MGD_THREAD_LOCAL) w.append("__chx_thread_local ", 19);
    if (decl.flags & MGD_STATIC) w.append("static ", 7);
    if (decl.flags & MGD_VOLATILE) w.append("volatile ", 9);
    const std::string spelling = global_decl_spelling(module_, decl);
    w.append(spelling.data(), spelling.size());
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

void MIRDeclEmitter::implement_global(BufferedWriter& w, ASTNode* node, ConstExprId init) {
    auto* init_stmt = node->as_var_init_unsafe();
    if (!init_stmt->is_top_level()) return;
    if (init_stmt->is_comptime()) return;
    // MIR_INVALID_ID means the global had no initializer (the declaration pass
    // already wrote its declaration).
    if (init == MIR_INVALID_ID) return;
    MIRGlobalDecl decl;
    build_global_declaration(mangler_, types_, module_, init_stmt, decl);
    w.append_char('\n');
    if (decl.flags & MGD_THREAD_LOCAL) w.append("__chx_thread_local ", 19);
    if (decl.flags & MGD_STATIC) w.append("static ", 7);
    if (decl.flags & MGD_VOLATILE) w.append("volatile ", 9);
    const std::string spelling = global_decl_spelling(module_, decl);
    w.append(spelling.data(), spelling.size());
    w.append(" = ", 3);
    const std::string expr = const_expr_c(module_, init);
    w.append(expr.data(), expr.size());
    w.append_char(';');
}

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
        case ASTNodeKind::VarInitStmt:
            // a global definition is written by `implement_global` (its
            // initializer is a MIR constant-expression tree, action A3), so the
            // driver routes VarInitStmt there instead of here.
            return;
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
