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
#include "compiler/mir/MIRVtable.h"
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

    // ── interface vtables (from the MIR vtable records; action A3/B) ────────
    // The slot list, self-erased slot types and resolved callee names all come
    // from `m.vtables`, built by the front end, so this driver reads no AST.
    auto slot_ftys = [&](const std::vector<mir::MIRVtableSlot>& slots,
                         std::vector<llvm::FunctionType*>& out) -> bool {
        out.clear();
        for (const mir::MIRVtableSlot& s : slots) {
            llvm::FunctionType* ft = gen.mir_fn_type(m, s.type);
            if (!ft) return false;
            out.push_back(ft);
        }
        return true;
    };
    auto slot_names = [&](const std::vector<mir::MIRVtableSlot>& slots) {
        std::vector<std::string> names;
        names.reserve(slots.size());
        for (const mir::MIRVtableSlot& s : slots) names.push_back(s.name);
        return names;
    };
    for (const mir::MIRVtableRecord& t : m.vtables.types) {
        std::vector<llvm::FunctionType*> ftys;
        if (!slot_ftys(t.slots, ftys)) continue;
        gen.mir_vtable_struct(t.name, ftys);
        gen.mir_vtable_methods_[t.name] = slot_names(t.slots);
    }
    for (const mir::MIRVtableRecord& d : m.vtables.defs) {
        std::vector<llvm::FunctionType*> ftys;
        if (!slot_ftys(d.slots, ftys)) continue;
        if (ftys.empty()) continue;
        llvm::StructType* vts = gen.mir_vtable_struct(d.type_name, ftys);
        if (vts->getNumElements() != d.slots.size()) continue;
        std::vector<llvm::Constant*> init;
        bool ok = true;
        for (size_t i = 0; i < d.slots.size(); ++i) {
            const mir::MIRVtableSlot& s = d.slots[i];
            llvm::Function* lf =
                s.target.empty() ? nullptr : gen.module->getFunction(s.target);
            if (!lf) {
                // a container impl requires every slot's callee; a primitive impl
                // leaves a non-overridden slot as a null pointer.
                if (d.kind == 1) {
                    ok = false;
                    break;
                }
                init.push_back(llvm::ConstantPointerNull::get(
                    llvm::cast<llvm::PointerType>(vts->getElementType(i))));
            } else {
                init.push_back(lf);
            }
        }
        if (!ok || init.empty()) continue;
        // `DynamicCall` resolves a method name to a slot index via this map; the
        // interface may be declared in another module, so register it if absent.
        if (gen.mir_vtable_methods_.find(d.type_name) == gen.mir_vtable_methods_.end()) {
            gen.mir_vtable_methods_[d.type_name] = slot_names(d.slots);
        }
        // External linkage: the vtable name is program-unique (it embeds both the
        // interface's and the impl type's mangled names), and a `dyn<I>` built in
        // *another* module references it — an internal definition cannot satisfy
        // that reference across per-module LLVM objects.
        auto* gv = new llvm::GlobalVariable(
            *gen.module, vts, true, llvm::GlobalValue::ExternalLinkage,
            llvm::ConstantStruct::get(vts, init), d.name);
        gen.mir_register_named_global(d.name, gv);
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