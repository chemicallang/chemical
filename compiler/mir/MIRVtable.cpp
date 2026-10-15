// Copyright (c) Chemical Language Foundation 2025.
//
// Shared interface-vtable layout — see MIRVtable.h.

#include "MIRVtable.h"

#include "MIRTypeBuilder.h"
#include "MIRABI.h"

#include "ast/base/BaseType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/FunctionParam.h"

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

} // namespace mir
