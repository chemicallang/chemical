// Copyright (c) Chemical Language Foundation 2025.
//
// Shared interface-vtable layout and declaration records — see MIRVtable.h.

#include "MIRVtable.h"

#include "MIRMangleBuffer.h"

#include "MIRTypeBuilder.h"
#include "MIRABI.h"

#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/MembersContainer.h"
#include "compiler/mangler/NameMangler.h"
#include "preprocess/2c/BufferedWriter.h"

namespace mir {

TypeId mir_vtable_slot_type(MIRTypeBuilder& types, MIRModule& module,
                            FunctionDeclaration* func) {
    FunctionParam* self = func->get_self_param();
    const TypeId ret = func->returnType ? types.map(func->returnType) : types.void_type();
    const bool sret = abi::needs_sret(module, ret);
    std::vector<TypeId> ptypes;
    // vtable slots are called as `fn(void* self, ...)`; the interface's `self`
    // is an opaque pointer regardless of the concrete impl.
    if (self) ptypes.push_back(types.pointer_type(types.void_type(), true));
    if (sret) ptypes.push_back(types.pointer_type(ret, true));
    bool skipped_self = false;
    for (FunctionParam* p : func->params) {
        if (p == self && !skipped_self) {
            skipped_self = true;
            continue;
        }
        TypeId pt = p && p->type ? types.map(p->type) : types.opaque_type();
        if (pt != MIR_INVALID_ID && module.types.get(pt).kind == MIRTypeKind::Array) {
            pt = types.pointer_type(module.types.get(pt).element, true);
        } else if (abi::is_aggregate(module, pt)) {
            pt = types.pointer_type(pt, true);
        }
        ptypes.push_back(pt);
    }
    return types.function_signature(sret ? types.void_type() : ret, ptypes,
                                    func->isVariadic());
}

namespace {

// Append a mangled node name straight into a record's string (one allocation —
// the record's own — instead of a `std::string` per slot that is then copied).
void append_mangled(NameMangler& mangler, ASTNode* node, std::string& out) {
    BufferedWriter& w = mangle_writer();
    mangler.mangle(w, node);
    out.append(w.data(), w.getPosition());
}

void append_vtable_type_name(NameMangler& mangler, InterfaceDefinition* iface,
                             std::string& out) {
    out += "__chx_";
    append_mangled(mangler, iface, out);
    out += "_vt_t";
}

void collect_vtable_slots(MIRTypeBuilder& types, MIRModule& module, InterfaceDefinition* iface,
                          std::vector<MIRVtableSlot>& out,
                          std::vector<FunctionDeclaration*>* out_funcs) {
    if (!iface) return;
    for (auto& inh : iface->inherited) {
        InterfaceDefinition* inherited =
            inh.type ? inh.type->get_direct_linked_interface() : nullptr;
        collect_vtable_slots(types, module, inherited, out, out_funcs);
    }
    for (FunctionDeclaration* func : iface->instantiated_functions()) {
        if (!func) continue;
        MIRVtableSlot slot;
        const chem::string_view nv = func->name_view();
        slot.name.assign(nv.data(), nv.size());
        slot.type = mir_vtable_slot_type(types, module, func);
        out.push_back(std::move(slot));
        if (out_funcs) out_funcs->push_back(func);
    }
}

} // namespace

void mir_vtable_slots(MIRTypeBuilder& types, MIRModule& module, InterfaceDefinition* iface,
                      std::vector<MIRVtableSlot>& out,
                      std::vector<FunctionDeclaration*>* out_funcs) {
    collect_vtable_slots(types, module, iface, out, out_funcs);
}

MIRVtableRecord build_vtable_def_record(NameMangler& mangler, MIRTypeBuilder& types,
                                        MIRModule& module, InterfaceDefinition* iface,
                                        ImplDefinition* impl) {
    MembersContainer* mc =
        impl->struct_type ? impl->struct_type->get_members_container() : nullptr;
    ExtendableMembersContainerNode* container =
        mc ? mc->as_extendable_members_container_unsafe() : nullptr;
    std::vector<FunctionDeclaration*> funcs;
    MIRVtableRecord rec;
    append_vtable_type_name(mangler, iface, rec.type_name);
    mir_vtable_slots(types, module, iface, rec.slots, &funcs);
    if (container) {
        rec.kind = 1;
        {
            BufferedWriter& w = mangle_writer();
            mangler.mangle_vtable_name(w, iface, container);
            rec.name.assign(w.data(), w.getPosition());
        }
        for (size_t i = 0; i < funcs.size(); ++i) {
            FunctionDeclaration* f = funcs[i];
            // Mirror the declaration pass exactly. A `self` method prefers the
            // container's own override when it is a *direct* child; otherwise —
            // for either a self or a non-self method — the interface method is
            // mangled with the *declaring interface* temporarily set to the
            // container as its active user, so the slot names the interface's
            // instantiated implementation for this container. An inherited
            // method's declaring interface is the base, not `iface`, so the
            // active user must be set on `f->parent()`, not on `iface`.
            ASTNode* parent = f->parent();
            InterfaceDefinition* owner =
                (parent && parent->kind() == ASTNodeKind::InterfaceDecl)
                    ? parent->as_interface_def_unsafe() : nullptr;
            bool resolved = false;
            if (f->get_self_param()) {
                FunctionDeclaration* overrider = container->direct_child_function(f->name_view());
                if (overrider) {
                    append_mangled(mangler, overrider, rec.slots[i].target);
                    resolved = true;
                }
            }
            if (!resolved && owner) {
                ExtendableMembersContainerNode* prev = owner->active_user;
                owner->active_user = container;
                append_mangled(mangler, f, rec.slots[i].target);
                owner->active_user = prev;
                resolved = true;
            }
            // `!resolved` leaves the target empty: `vtable_def_c` writes
            // `__chx_unresolved`, exactly as the declaration pass did.
        }
    } else {
        // a primitive impl (`impl Interface for int`): only a `self` method has a
        // callee; the rest are null slots.
        rec.kind = 2;
        {
            BufferedWriter& w = mangle_writer();
            mangler.mangle_vtable_name(w, iface, impl->struct_type);
            rec.name.assign(w.data(), w.getPosition());
        }
        for (size_t i = 0; i < funcs.size(); ++i) {
            FunctionDeclaration* f = funcs[i];
            if (!f->has_self_param()) continue;
            FunctionDeclaration* overrider = impl->direct_child_function(f->name_view());
            if (overrider) append_mangled(mangler, overrider, rec.slots[i].target);
        }
    }
    return rec;
}

void build_module_vtables(NameMangler& mangler, MIRTypeBuilder& types, MIRModule& module,
                          const std::vector<ASTNode*>& nodes, bool clear) {
    if (clear) module.vtables.clear();

    // One struct type per interface, whether or not it is implemented here (the
    // interface may be declared in this module and implemented in another).
    for (ASTNode* n : nodes) {
        if (!n || n->kind() != ASTNodeKind::InterfaceDecl) continue;
        auto* iface = n->as_interface_def_unsafe();
        MIRVtableRecord rec;
        rec.kind = 0;
        append_vtable_type_name(mangler, iface, rec.name);
        rec.type_name = rec.name;
        mir_vtable_slots(types, module, iface, rec.slots);
        module.vtables.types.push_back(std::move(rec));
    }

    // One definition per (non-static, generating) impl. The callee names are
    // resolved by the shared `build_vtable_def_record`, exactly as the C
    // definition emitter does for the same impl.
    for (ASTNode* n : nodes) {
        if (!n || n->kind() != ASTNodeKind::ImplDecl) continue;
        auto* impl = n->as_impl_def_unsafe();
        InterfaceDefinition* iface =
            impl->interface_type ? impl->interface_type->get_direct_linked_interface() : nullptr;
        if (!iface || iface->is_static() || !iface->generates_vtable()) continue;
        if (!impl->struct_type) continue;
        module.vtables.defs.push_back(
            build_vtable_def_record(mangler, types, module, iface, impl));
    }
}

} // namespace mir
