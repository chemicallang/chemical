// Copyright (c) Chemical Language Foundation 2025.

#include "Codegen.h"
#include "utils/Benchmark.h"
#include "ASTProcessor.h"
#include "compiler/SymbolResolver.h"
#include "compiler/mir/MIRModule.h"
#include "compiler/mir/MIRTypeBuilder.h"
#include "compiler/mir/MIRLowerer.h"
#include "compiler/mir/MIRArena.h"
#include "compiler/mir/MIRDump.h"
#include "compiler/mangler/NameMangler.h"
#include "preprocess/2c/BufferedWriter.h"
#include "preprocess/2c/TranslationContext.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/If.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/AccessChain.h"
#include "ast/statements/VarInit.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/Scope.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/StructValue.h"
#include "ast/values/CastedValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/values/Negative.h"
#include "ast/values/NullValue.h"
#include "ast/structures/VariantMember.h"
#include "ast/base/BaseType.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include "rang.hpp"
#include <iostream>

// ── MIR → LLVM: emit the module's LLVM IR from the collected MIR module ─────
// The MIR module is produced by the *shared* MIR construction used by the C
// backend (`ASTProcessor::implement_module` -> `mir_translate_after_declaration`),
// so the LLVM backend needs no AST traversal of its own. This function only
// turns the fully-populated MIR module into LLVM IR.


static void mir_emit_llvm_module(Codegen& gen, LabModule* module,
                                 mir::MIRTypeBuilder& types, mir::MIRModule& m) {
    auto mangle = [&gen](ASTNode* n) -> std::string {
        BufferedWriter w(256);
        gen.mangler.mangle(w, n);
        return std::string(w.data(), w.getPosition());
    };

    std::vector<ASTNode*> nodes;
    for (auto& f : module->direct_files) {
        for (ASTNode* n : f.result->unit.scope.body.nodes) nodes.push_back(n);
    }
    if (getenv("MIR_LLVM_TRACE"))
        std::cerr << "[MIR-LLVM] emitting module " << module->format()
                  << " functions=" << m.functions.size() << "\n";

    gen.mir_reset_caches();
    // Debug info: one DICompileUnit per module (the MIR path used to emit none).
    if (gen.di.isEnabled) {
        const std::string cu_path =
            module->direct_files.empty() ? std::string() : module->direct_files.front().abs_path;
        llvm::DICompileUnit* cu = gen.di.createDiCompileUnit(chem::string_view(cu_path));
        gen.di.start_di_compile_unit(cu);
        gen.mir_di_file_ = cu ? cu->getFile() : nullptr;
    }
    gen.mir_declare_globals(m);
    // pre-declare all functions so vtable initializers can reference them
    for (const mir::MIRFunction& fn : m.functions) gen.mir_function(m, fn.symbol);

    // ── interface vtables ───────────────────────────────────────────────────
    auto vtable_type_name = [&](InterfaceDefinition* iface) -> std::string {
        return "__chx_" + mangle(iface) + "_vt_t";
    };
    auto is_agg = [&](mir::TypeId t) -> bool {
        if (t >= m.types.size()) return false;
        const mir::MIRTypeRecord& r = m.types.get(t);
        if (r.flags & mir::TF_TYPEDEF) return false;
        return r.kind == mir::MIRTypeKind::Struct || r.kind == mir::MIRTypeKind::Union ||
               r.kind == mir::MIRTypeKind::Variant;
    };
    std::function<std::vector<FunctionDeclaration*>(InterfaceDefinition*)> slots_of =
        [&](InterfaceDefinition* iface) -> std::vector<FunctionDeclaration*> {
        std::vector<FunctionDeclaration*> out;
        for (auto& inh : iface->inherited) {
            InterfaceDefinition* inherited =
                inh.type ? inh.type->get_direct_linked_interface() : nullptr;
            if (inherited) {
                auto s = slots_of(inherited);
                out.insert(out.end(), s.begin(), s.end());
            }
        }
        for (FunctionDeclaration* f : iface->instantiated_functions()) out.push_back(f);
        return out;
    };
    auto slot_ftype = [&](FunctionDeclaration* func) -> llvm::FunctionType* {
        FunctionParam* self = func->get_self_param();
        mir::TypeId ret = func->returnType ? types.map(func->returnType) : types.void_type();
        const bool sret = is_agg(ret);
        std::vector<mir::TypeId> ptypes;
        if (self) ptypes.push_back(types.pointer_type(types.void_type(), true));
        if (sret) ptypes.push_back(types.pointer_type(ret, true));
        bool skipped_self = false;
        for (FunctionParam* p : func->params) {
            if (p == self && !skipped_self) { skipped_self = true; continue; }
            mir::TypeId pt = p && p->type ? types.map(p->type) : types.opaque_type();
            if (pt < m.types.size()) {
                if (m.types.get(pt).kind == mir::MIRTypeKind::Array) {
                    pt = types.pointer_type(m.types.get(pt).element, true);
                } else if (is_agg(pt)) {
                    pt = types.pointer_type(pt, true);
                }
            }
            ptypes.push_back(pt);
        }
        mir::TypeId stype = types.function_signature(sret ? types.void_type() : ret, ptypes,
                                                     func->isVariadic());
        return gen.mir_fn_type(m, stype);
    };
    for (ASTNode* n : nodes) {
        if (n->kind() != ASTNodeKind::InterfaceDecl) continue;
        auto* iface = n->as_interface_def_unsafe();
        const std::string tn = vtable_type_name(iface);
        auto slots = slots_of(iface);
        std::vector<llvm::FunctionType*> ftys;
        std::vector<std::string> names;
        for (FunctionDeclaration* f : slots) {
            ftys.push_back(slot_ftype(f));
            names.push_back(std::string(f->name_view().data(), f->name_view().size()));
        }
        gen.mir_vtable_struct(tn, ftys);
        gen.mir_vtable_methods_[tn] = names;
    }
    for (ASTNode* n : nodes) {
        if (n->kind() != ASTNodeKind::ImplDecl) continue;
        auto* impl = n->as_impl_def_unsafe();
        InterfaceDefinition* iface =
            impl->interface_type ? impl->interface_type->get_direct_linked_interface() : nullptr;
        if (!iface) continue;
        if (iface->is_static()) continue; // static interfaces have no vtable
        MembersContainer* container_mc =
            impl->struct_type ? impl->struct_type->get_members_container() : nullptr;
        ExtendableMembersContainerNode* container =
            container_mc ? container_mc->as_extendable_members_container_unsafe() : nullptr;
        if (!container) {
            // Primitive impl (`impl Interface for int`): there is no members
            // container, so the vtable is built from the impl's own methods —
            // mirroring `MIRDeclEmitter::create_vtable_primitive`. Without this
            // the `dyn<I>` fat pointer's `second` slot had no global to point at
            // and the store was dropped (a garbage vtable → SIGSEGV on dispatch).
            if (!iface->generates_vtable()) continue;
            const std::string tn = vtable_type_name(iface);
            std::string gname;
            {
                BufferedWriter w(256);
                gen.mangler.mangle_vtable_name(w, iface, impl->struct_type);
                gname.assign(w.data(), w.getPosition());
            }
            auto slots = slots_of(iface);
            std::vector<llvm::FunctionType*> ftys;
            bool ok = true;
            for (FunctionDeclaration* f : slots) {
                llvm::FunctionType* sft = slot_ftype(f);
                if (!sft) { ok = false; break; }
                ftys.push_back(sft);
            }
            if (!ok || ftys.empty()) continue;
            llvm::StructType* vts = gen.mir_vtable_struct(tn, ftys);
            if (vts->getNumElements() != slots.size()) continue;
            std::vector<llvm::Constant*> init;
            for (size_t si = 0; si < slots.size(); ++si) {
                FunctionDeclaration* f = slots[si];
                llvm::Function* lf = nullptr;
                if (f->has_self_param()) {
                    FunctionDeclaration* overrider = impl->direct_child_function(f->name_view());
                    if (overrider) lf = gen.module->getFunction(mangle(overrider));
                }
                init.push_back(lf ? static_cast<llvm::Constant*>(lf)
                                  : llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(
                                        vts->getElementType(si))));
            }
            if (gen.mir_vtable_methods_.find(tn) == gen.mir_vtable_methods_.end()) {
                std::vector<std::string> slot_names;
                for (FunctionDeclaration* f : slots)
                    slot_names.emplace_back(f->name_view().data(), f->name_view().size());
                gen.mir_vtable_methods_[tn] = std::move(slot_names);
            }
            // External linkage: the vtable name is program-unique (it embeds
            // both the interface's and the impl type's mangled names), and a
            // `dyn<I>` built in *another* module references it — an internal
            // definition cannot satisfy that reference across per-module LLVM
            // objects (the C backend emits one translation unit, so `static`
            // was enough there).
            auto* gv = new llvm::GlobalVariable(
                *gen.module, vts, true, llvm::GlobalValue::ExternalLinkage,
                llvm::ConstantStruct::get(vts, init), gname);
            gen.mir_register_named_global(gname, gv);
            continue;
        }
        const std::string tn = vtable_type_name(iface);
        std::string gname;
        {
            BufferedWriter w(256);
            gen.mangler.mangle_vtable_name(w, iface, container);
            gname.assign(w.data(), w.getPosition());
        }
        auto slots = slots_of(iface);
        std::vector<llvm::Constant*> init;
        bool ok = true;
        for (FunctionDeclaration* f : slots) {
            FunctionDeclaration* overrider = container->direct_child_function(f->name_view());
            std::string fname;
            if (overrider) {
                fname = mangle(overrider);
            } else {
                ExtendableMembersContainerNode* prev = iface->active_user;
                iface->active_user = container;
                fname = mangle(f);
                iface->active_user = prev;
            }
            llvm::Function* lf = gen.module->getFunction(fname);
            if (!lf) { ok = false; break; }
            init.push_back(lf);
        }
        if (!ok || init.empty()) continue;
        // Build (or fetch) the vtable struct with its real slot types. The
        // interface declaration may live in a *different* module than the impl,
        // so the type must be registered here from the impl's interface.
        std::vector<llvm::FunctionType*> ftys;
        for (FunctionDeclaration* f : slots) {
            llvm::FunctionType* sft = slot_ftype(f);
            if (!sft) { ok = false; break; }
            ftys.push_back(sft);
        }
        if (!ok) continue;
        llvm::StructType* vts = gen.mir_vtable_struct(tn, ftys);
        if (vts->getNumElements() != init.size()) continue;
        // `DynamicCall` resolves the method name to a slot index via this map;
        // the interface may be declared in another module, so make sure the
        // slot-name list is registered for the impl's interface too.
        if (gen.mir_vtable_methods_.find(tn) == gen.mir_vtable_methods_.end()) {
            std::vector<std::string> slot_names;
            for (FunctionDeclaration* f : slots) {
                slot_names.emplace_back(f->name_view().data(), f->name_view().size());
            }
            gen.mir_vtable_methods_[tn] = std::move(slot_names);
        }
        auto* gv = new llvm::GlobalVariable(
            *gen.module, vts, true, llvm::GlobalValue::ExternalLinkage,
            llvm::ConstantStruct::get(vts, init), gname);
        gen.mir_register_named_global(gname, gv);
    }

    // Apply the global initializers from MIR (action A3): each `MIRGlobal`
    // carries a target-neutral constant-expression tree produced by the shared
    // front-end lowering, so no AST is read here.
    gen.mir_apply_global_initializers(m);
    for (const mir::MIRFunction& fn : m.functions) {
        if (!gen.mir_emit_function(m, fn)) {
            std::cerr << "[MIR-LLVM] LLVM emission failed\n";
            return;
        }
    }
    if (gen.di.isEnabled) {
        gen.di.end_di_compile_unit();
        if (gen.diBuilder) gen.diBuilder->finalize();
    }
    if (getenv("MIR_LLVM_DUMP_IR")) {
        std::string path = std::string(getenv("MIR_LLVM_DUMP_IR"));
        std::error_code ec;
        llvm::raw_fd_ostream os(path, ec);
        gen.module->print(os, nullptr);
    }
}

int ASTProcessor::mir_compile_module(Codegen& gen, LabModule* module) {
    // Drive the *same* MIR construction the C backend uses (declarations +
    // implementations, including impls, generic instantiations, CBI embedded
    // nodes and async), but collect the lowered functions instead of emitting C
    // bodies; the collected MIR is then emitted as LLVM IR by the finish hook.
    CTranslationContext c_visitor(gen.binder, gen.comptime_scope, gen.mangler, gen.allocator,
                                  loc_man, gen.coreNodes, gen.implsIndex,
                                  options->debug_info, /*minify=*/false);
    // CBI macro replacements are stored on their `EmbeddedNode` and re-read at
    // the end of the module (in `mir_emit_llvm_module`), after `file_level_reset()`
    // has cleared the file allocator. Allocate them in the module allocator, which
    // lives until the module has been fully emitted.
    c_visitor.embedded_replacement_allocator = &mod_allocator;
    c_visitor.prepare_translate();

    mir_llvm_collect = true;
    mir_llvm_finish = [&](mir::MIRTypeBuilder& types, mir::MIRModule& m) {
        mir_emit_llvm_module(gen, module, types, m);
    };

    int result = declare_module(c_visitor, module);
    if (result == 0) result = implement_module(c_visitor, module);

    mir_llvm_collect = false;
    mir_llvm_finish = nullptr;
    return result;
}

int ASTProcessor::compile_module(
    Codegen& gen,
    LabModule* module
) {

    // MIR-driven LLVM backend: build MIR from the resolved AST and emit LLVM IR
    // from it. The AST-based lowering has been deleted.
    return mir_compile_module(gen, module);
}