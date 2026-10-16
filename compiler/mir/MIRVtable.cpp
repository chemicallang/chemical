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

std::string mangle_node(NameMangler& mangler, ASTNode* node) {
    BufferedWriter& w = mangle_writer();
    mangler.mangle(w, node);
    return std::string(w.data(), w.getPosition());
}

std::string vtable_type_name(NameMangler& mangler, InterfaceDefinition* iface) {
    return "__chx_" + mangle_node(mangler, iface) + "_vt_t";
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
        rec.name = vtable_type_name(mangler, iface);
        rec.type_name = rec.name;
        mir_vtable_slots(types, module, iface, rec.slots);
        module.vtables.types.push_back(std::move(rec));
    }

    // One definition per (non-static, generating) impl. The callee names are
    // resolved here, mirroring what `mir_emit_llvm_module` used to do inline.
    for (ASTNode* n : nodes) {
        if (!n || n->kind() != ASTNodeKind::ImplDecl) continue;
        auto* impl = n->as_impl_def_unsafe();
        InterfaceDefinition* iface =
            impl->interface_type ? impl->interface_type->get_direct_linked_interface() : nullptr;
        if (!iface || iface->is_static() || !iface->generates_vtable()) continue;
        MembersContainer* mc =
            impl->struct_type ? impl->struct_type->get_members_container() : nullptr;
        ExtendableMembersContainerNode* container =
            mc ? mc->as_extendable_members_container_unsafe() : nullptr;
        std::vector<FunctionDeclaration*> funcs;
        MIRVtableRecord rec;
        rec.type_name = vtable_type_name(mangler, iface);
        mir_vtable_slots(types, module, iface, rec.slots, &funcs);
        if (container) {
            rec.kind = 1;
            {
                BufferedWriter w(256);
                mangler.mangle_vtable_name(w, iface, container);
                rec.name.assign(w.data(), w.getPosition());
            }
            for (size_t i = 0; i < funcs.size(); ++i) {
                FunctionDeclaration* f = funcs[i];
                FunctionDeclaration* overrider = container->direct_child_function(f->name_view());
                if (overrider) {
                    rec.slots[i].target = mangle_node(mangler, overrider);
                } else {
                    ExtendableMembersContainerNode* prev = iface->active_user;
                    iface->active_user = container;
                    rec.slots[i].target = mangle_node(mangler, f);
                    iface->active_user = prev;
                }
            }
        } else {
            // a primitive impl (`impl Interface for int`): only a `self` method
            // has a callee; the rest are null slots.
            rec.kind = 2;
            {
                BufferedWriter w(256);
                mangler.mangle_vtable_name(w, iface, impl->struct_type);
                rec.name.assign(w.data(), w.getPosition());
            }
            for (size_t i = 0; i < funcs.size(); ++i) {
                FunctionDeclaration* f = funcs[i];
                if (!f->has_self_param()) continue;
                FunctionDeclaration* overrider = impl->direct_child_function(f->name_view());
                if (overrider) rec.slots[i].target = mangle_node(mangler, overrider);
            }
        }
        module.vtables.defs.push_back(std::move(rec));
    }
}

} // namespace mir
