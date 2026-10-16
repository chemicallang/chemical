// Copyright (c) Chemical Language Foundation 2025.

#include "MIRDeclEmitter.h"

#include "MIRMangleBuffer.h"

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
    BufferedWriter& w = mangle_writer();
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

void MIRDeclEmitter::fwd_declare(ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::NamespaceDecl:
            for (ASTNode* child : node->as_namespace_unsafe()->nodes) fwd_declare(child);
            break;
        case ASTNodeKind::Scope:
            for (ASTNode* child : node->as_scope_unsafe()->nodes) fwd_declare(child);
            break;
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl: {
            MIRDeclItem it;
            it.kind = MIRDeclKind::ForwardStruct;
            it.name = mangle_name(node);
            decl_list_.items.push_back(std::move(it));
            break;
        }
        case ASTNodeKind::UnionDecl: {
            MIRDeclItem it;
            it.kind = MIRDeclKind::ForwardUnion;
            it.name = mangle_name(node);
            decl_list_.items.push_back(std::move(it));
            break;
        }
        default:
            break;
    }
}

void MIRDeclEmitter::declare_typedef(TypealiasStatement* stmt) {
    if (!stmt || stmt->has_declared) return;
    stmt->has_declared = true;
    // declaring the underlying type first pulls in composed/capturing-function
    // instance structs the typedef's type depends on
    early_declare_type(stmt->actual_type, nullptr, true, true);
    MIRDeclItem it;
    it.kind = MIRDeclKind::Typedef;
    it.name = mangle_name(stmt);
    BaseType* t = stmt->actual_type;
    if (t) {
        if (t->kind() == BaseTypeKind::Array && !t->as_array_type()->has_array_size()) {
            it.unsized_array = true;
            it.elem = types_.map(t->as_array_type()->known_child_type());
        } else {
            it.type = types_.map(t);
        }
    }
    decl_list_.items.push_back(std::move(it));
}

void MIRDeclEmitter::declare_type_alias(ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::TypealiasStmt:
            declare_typedef(node->as_typealias_unsafe());
            break;
        case ASTNodeKind::NamespaceDecl:
            for (ASTNode* child : node->as_namespace_unsafe()->nodes) declare_type_alias(child);
            break;
        case ASTNodeKind::Scope:
            for (ASTNode* child : node->as_scope_unsafe()->nodes) declare_type_alias(child);
            break;
        case ASTNodeKind::IfStmt: {
            auto* stmt = node->as_if_stmt_unsafe();
            if (stmt->computed_scope.has_value()) {
                Scope* scope = stmt->computed_scope.value();
                if (scope) for (ASTNode* child : scope->nodes) declare_type_alias(child);
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

void MIRDeclEmitter::declare_contained_func(FunctionDeclaration* decl, bool terminate) {
    if (decl->is_comptime()) return;
    // The prototype (name, signature, linkage flags) is a MIR record built by the
    // shared `build_function_prototype` (action A3), the same one the LLVM path
    // uses via `mir_function_signature`.
    MIRDeclItem it;
    it.kind = MIRDeclKind::Prototype;
    build_function_prototype(mangler_, types_, module_, decl, it.proto);
    decl_list_.items.push_back(std::move(it));
    (void)terminate; // always terminated in the declare pass
}

// ---------------------------------------------------------------------------
// early declaration (composed variable ordering)
// ---------------------------------------------------------------------------

void MIRDeclEmitter::early_declare_type(BaseType* type, ASTNode* container,
                                        bool do_ref, bool do_ptr) {
    if (!type) return;
    switch (type->kind()) {
        case BaseTypeKind::Reference:
            if (do_ref) early_declare_type(type->as_reference_type_unsafe()->type, container, do_ref, do_ptr);
            return;
        case BaseTypeKind::Pointer:
            if (do_ptr) early_declare_type(type->as_pointer_type_unsafe()->type, container, do_ref, do_ptr);
            return;
        case BaseTypeKind::Linked: {
            ASTNode* linked = type->as_linked_type_unsafe()->linked;
            if (linked != container) early_declare_node(linked);
            return;
        }
        case BaseTypeKind::Dynamic:
            early_declare_type(type->as_dynamic_type_unsafe()->referenced, container);
            return;
        case BaseTypeKind::CapturingFunction:
            early_declare_type(type->as_capturing_func_type_unsafe()->instance_type, container, do_ref, do_ptr);
            return;
        case BaseTypeKind::Array:
            early_declare_type(type->as_array_type_unsafe()->elem_type, container);
            return;
        case BaseTypeKind::Generic: {
            auto* gt = type->as_generic_type_unsafe();
            ASTNode* linked = gt->referenced ? gt->referenced->linked : nullptr;
            if (linked && linked != container) early_declare_node(linked);
            for (BaseType* ty : gt->types) early_declare_type(ty, container);
            return;
        }
        default:
            return;
    }
}

void MIRDeclEmitter::early_declare_composed_variables(VariablesContainer& container) {
    for (BaseDefMember* variable : container.variables()) {
        BaseType* t = variable->known_type();
        if (!t) continue;
        // A generic alias member (`std::function<T>`) links to the alias
        // declaration; declaring it pulls in the alias typedef which in turn
        // early-declares the capturing-function instance struct.
        ASTNode* node = t->get_direct_linked_node();
        if (node) early_declare_node(node);
    }
    for (auto& inh : container.inherited) {
        ASTNode* node = inh.type ? inh.type->get_direct_linked_node() : nullptr;
        if (node) early_declare_node(node);
    }
}

void MIRDeclEmitter::early_declare_node(ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::TypealiasStmt: {
            declare_typedef(node->as_typealias_unsafe());
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
                        declare_struct_def_only(def);
                    }
                    break;
                }
                case ASTNodeKind::VariantDecl: {
                    auto* def = node->as_variant_def_unsafe();
                    if (!def->has_declared) {
                        def->has_declared = true;
                        declare_variant_def_only(def);
                    }
                    break;
                }
                case ASTNodeKind::UnionDecl: {
                    auto* def = node->as_union_def_unsafe();
                    if (!def->has_declared) {
                        def->has_declared = true;
                        declare_union_def_only(def);
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
void MIRDeclEmitter::declare_aggregate_body(BaseType* type, const char* what) {
    if (!type) return;
    MIRDeclItem it;
    it.kind = MIRDeclKind::AggregateBody;
    it.type = types_.map(type);
    it.what = what;
    decl_list_.items.push_back(std::move(it));
}

void MIRDeclEmitter::declare_struct_def_only(StructDefinition* def) {
    early_declare_composed_variables(*def);
    declare_aggregate_body(def->known_type(), "struct");
}

void MIRDeclEmitter::declare_union_def_only(UnionDef* def) {
    early_declare_composed_variables(*def);
    declare_aggregate_body(def->known_type(), "union");
}

void MIRDeclEmitter::declare_variant_def_only(VariantDefinition* def) {
    // declare inherited + sub variables first
    for (auto& inh : def->inherited) {
        ASTNode* n = inh.type ? inh.type->get_direct_linked_node() : nullptr;
        if (n) early_declare_node(n);
    }
    for (BaseDefMember* var : def->variables()) {
        auto* member = var->as_variant_member_unsafe();
        for (auto& val : member->values) {
            early_declare_type(val.second->type, def);
        }
    }
    declare_aggregate_body(def->known_type(), "variant");
}

void MIRDeclEmitter::declare_struct_iterations(StructDefinition* def) {
    if (!def->has_declared) {
        def->has_declared = true;
        declare_struct_def_only(def);
    }
    for (FunctionDeclaration* func : def->non_gen_range()) {
        declare_contained_func(func, true);
    }
    for (ASTNode* child : def->evaluated_nodes()) {
        if (child->kind() == ASTNodeKind::ImplDecl) {
            declare_impl(child->as_impl_def_unsafe());
        }
    }
}

void MIRDeclEmitter::declare_union_iterations(UnionDef* def) {
    if (!def->has_declared) {
        def->has_declared = true;
        declare_union_def_only(def);
    }
    for (FunctionDeclaration* func : def->non_gen_range()) {
        declare_contained_func(func, true);
    }
}

void MIRDeclEmitter::declare_variant_iterations(VariantDefinition* def) {
    if (!def->has_declared) {
        def->has_declared = true;
        declare_variant_def_only(def);
    }
    for (FunctionDeclaration* func : def->non_gen_range()) {
        declare_contained_func(func, true);
    }
}

// ---------------------------------------------------------------------------
// vtables
// ---------------------------------------------------------------------------

std::string MIRDeclEmitter::vtable_type_name(InterfaceDefinition* interface) {
    return "__chx_" + mangle_name(interface) + "_vt_t";
}

std::string MIRDeclEmitter::vtable_name(InterfaceDefinition* interface,
                                        ExtendableMembersContainerNode* def) {
    BufferedWriter w(256);
    mangler_.mangle_vtable_name(w, interface, def);
    return std::string(w.data(), w.getPosition());
}

void MIRDeclEmitter::create_vtable_type(InterfaceDefinition* interface) {
    // the ordered slot list (inherited first, then own methods) and each slot's
    // self-erased type come from the shared MIR vtable layout (MIRVtable.h), so
    // the C and LLVM backends cannot drift
    MIRDeclItem it;
    it.kind = MIRDeclKind::VtableType;
    it.name = vtable_type_name(interface);
    interface->active_user = nullptr;
    mir_vtable_slots(types_, module_, interface, it.slots);
    decl_list_.items.push_back(std::move(it));
}

void MIRDeclEmitter::declare_vtable_extern(InterfaceDefinition* interface,
                                           ExtendableMembersContainerNode* def) {
    MIRDeclItem it;
    it.kind = MIRDeclKind::VtableExtern;
    it.type_name = vtable_type_name(interface);
    it.name = vtable_name(interface, def);
    decl_list_.items.push_back(std::move(it));
}

void MIRDeclEmitter::create_vtable(BufferedWriter& w, InterfaceDefinition* interface,
                                   ExtendableMembersContainerNode* def) {
    std::vector<MIRVtableSlot> slots;
    std::vector<FunctionDeclaration*> funcs;
    mir_vtable_slots(types_, module_, interface, slots, &funcs);
    w.append_char('\n');
    w.append("const ", 6);
    const std::string tn = vtable_type_name(interface);
    w.append(tn.data(), tn.size());
    w.append_char(' ');
    const std::string vn = vtable_name(interface, def);
    w.append(vn.data(), vn.size());
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

void MIRDeclEmitter::declare_vtable_primitive_extern(InterfaceDefinition* interface,
                                                     BaseType* impl) {
    MIRDeclItem it;
    it.kind = MIRDeclKind::VtableExtern;
    it.type_name = vtable_type_name(interface);
    BufferedWriter w(256);
    mangler_.mangle_vtable_name(w, interface, impl);
    it.name.assign(w.data(), w.getPosition());
    decl_list_.items.push_back(std::move(it));
}

void MIRDeclEmitter::create_vtable_primitive(BufferedWriter& w, InterfaceDefinition* interface,
                                             ImplDefinition* def) {
    w.append_char('\n');
    w.append("const ", 6);
    const std::string tn = vtable_type_name(interface);
    w.append(tn.data(), tn.size());
    w.append_char(' ');
    {
        BufferedWriter nw(256);
        mangler_.mangle_vtable_name(nw, interface, def->struct_type);
        w.append(nw.data(), nw.getPosition());
    }
    w.append(" = {", 4);
    // The ordered slot list must match the vtable *type* (`create_vtable_type`):
    // inherited interface slots first, then the interface's own methods — every
    // one of them — so a primitive impl of an interface that inherits (or
    // declares non-self methods) stays aligned with the type. A non-self slot
    // and a self slot with no overrider are null entries, exactly as
    // `build_module_vtables` records the primitive vtable for LLVM; a dropped
    // entry would shift every later slot.
    std::vector<MIRVtableSlot> slots;
    std::vector<FunctionDeclaration*> funcs;
    mir_vtable_slots(types_, module_, interface, slots, &funcs);
    for (FunctionDeclaration* func : funcs) {
        w.append_char('\n');
        std::string name;
        if (func && func->has_self_param()) {
            FunctionDeclaration* overrider = def->direct_child_function(func->name_view());
            if (overrider) name = mangle_name(overrider);
        }
        if (name.empty()) {
            w.append_char('0');
        } else {
            w.append(name.data(), name.size());
        }
        w.append_char(',');
    }
    w.append("\n};", 3);
}

// ---------------------------------------------------------------------------
// impl / interface declarations
// ---------------------------------------------------------------------------

void MIRDeclEmitter::declare_impl(ImplDefinition* def) {
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
            declare_contained_func(func, true);
        }
        interface_def->active_user = nullptr;
        if (interface_def->generates_vtable()) {
            if (!interface_def->has_declared) {
                create_vtable_type(interface_def);
                interface_def->has_declared = true;
            }
            declare_vtable_extern(interface_def, container);
        }
    } else {
        for (FunctionDeclaration* func : def->instantiated_functions()) {
            declare_contained_func(func, true);
        }
        if (interface_def->generates_vtable()) {
            if (!interface_def->has_declared) {
                create_vtable_type(interface_def);
                interface_def->has_declared = true;
            }
            declare_vtable_primitive_extern(interface_def, def->struct_type);
        }
    }
}

void MIRDeclEmitter::declare_interface(InterfaceDefinition* def) {
    if (def->is_static()) {
        for (FunctionDeclaration* func : def->non_gen_range()) {
            declare_contained_func(func, true);
        }
        if (!def->is_extern() && static_recorder_) static_recorder_(def);
    } else {
        def->active_user = nullptr;
        if (!def->has_declared && def->generates_vtable()) {
            create_vtable_type(def);
        }
        def->has_declared = true;
    }
}

// ---------------------------------------------------------------------------
// globals
// ---------------------------------------------------------------------------

void MIRDeclEmitter::declare_var(VarInitStatement* init) {
    if (!init->is_top_level()) return;
    if (init->is_comptime()) return;
    // name / declared type / unsized-array / flags come from the shared
    // `build_global_declaration` (action A3), the same record `implement_global`
    // prints its definition from.
    MIRDeclItem it;
    it.kind = MIRDeclKind::Global;
    build_global_declaration(mangler_, types_, module_, init, it.global);
    BaseType* init_type = init->type ? init->type : (init->value ? init->value->getType() : nullptr);
    early_declare_type(init_type, nullptr);
    decl_list_.items.push_back(std::move(it));
}

// ---------------------------------------------------------------------------
// top-level declare
// ---------------------------------------------------------------------------

void MIRDeclEmitter::declare(ASTNode* node) {
    switch (node->kind()) {
        case ASTNodeKind::NamespaceDecl: {
            auto* ns = node->as_namespace_unsafe();
            if (ns->is_comptime()) return;
            for (ASTNode* child : ns->nodes) declare(child);
            return;
        }
        case ASTNodeKind::Scope: {
            for (ASTNode* child : node->as_scope_unsafe()->nodes) declare(child);
            return;
        }
        case ASTNodeKind::FunctionDecl:
            declare_contained_func(node->as_function_unsafe(), true);
            return;
        case ASTNodeKind::StructDecl:
            declare_struct_iterations(node->as_struct_def_unsafe());
            return;
        case ASTNodeKind::UnionDecl:
            declare_union_iterations(node->as_union_def_unsafe());
            return;
        case ASTNodeKind::VariantDecl:
            declare_variant_iterations(node->as_variant_def_unsafe());
            return;
        case ASTNodeKind::InterfaceDecl:
            declare_interface(node->as_interface_def_unsafe());
            return;
        case ASTNodeKind::ImplDecl:
            declare_impl(node->as_impl_def_unsafe());
            return;
        case ASTNodeKind::VarInitStmt:
            declare_var(node->as_var_init_unsafe());
            return;
        case ASTNodeKind::TypealiasStmt: {
            declare_typedef(node->as_typealias_unsafe());
            return;
        }
        case ASTNodeKind::IfStmt: {
            auto* stmt = node->as_if_stmt_unsafe();
            if (!stmt->is_top_level()) return;
            if (stmt->computed_scope.has_value()) {
                Scope* scope = stmt->computed_scope.value();
                if (scope) for (ASTNode* child : scope->nodes) declare(child);
            }
            return;
        }
        case ASTNodeKind::EmbeddedNode: {
            auto* en = node->as_embedded_node_unsafe();
            ASTNode* repl = embedded_decl_repl_ ? embedded_decl_repl_(en) : nullptr;
            if (repl) {
                en->replacement = repl;
                declare(repl);
            }
            return;
        }
        case ASTNodeKind::EnumDecl:
            return;
        default:
            return;
    }
}

void MIRDeclEmitter::print_decl_list(BufferedWriter& w) {
    const std::string text = decl_list_c(module_, decl_list_);
    w.append(text.data(), text.size());
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
