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
#include "ast/statements/VarInit.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/base/BaseType.h"
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <functional>
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
        MembersContainer* container_mc =
            impl->struct_type ? impl->struct_type->get_members_container() : nullptr;
        ExtendableMembersContainerNode* container =
            container_mc ? container_mc->as_extendable_members_container_unsafe() : nullptr;
        if (!container) continue;
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
            *gen.module, vts, true, llvm::GlobalValue::InternalLinkage,
            llvm::ConstantStruct::get(vts, init), gname);
        gen.mir_register_named_global(gname, gv);
    }

    // apply literal global initializers from the AST (MIR symbols do not carry
    // initializer values yet)
    for (ASTNode* n : nodes) {
        if (n->kind() != ASTNodeKind::VarInitStmt) continue;
        auto* vi = n->as_var_init();
        // Never initialize an `@extern` global: it is declared (not defined) in
        // this module, and an initializer would make LLVM emit a definition
        // that interposes the real symbol (e.g. libc's `stdout`).
        if (!vi->value || vi->is_comptime() || vi->is_extern()) continue;
        const std::string name = mangle(vi);
        Value* val = vi->value;
        llvm::GlobalVariable* gv = gen.module->getNamedGlobal(name);
        if (getenv("MIR_LLVM_TRACE"))
            std::cerr << "[MIR-LLVM] global init " << name << " found=" << (gv != nullptr)
                      << " vk=" << (val ? static_cast<int>(val->val_kind()) : -1) << "\n";
        if (!gv) continue;
        llvm::Type* gt = gv->getValueType();
        if (gt->isIntegerTy()) {
            switch (val->val_kind()) {
                case ValueKind::IntN:
                    gv->setInitializer(llvm::ConstantInt::get(
                        gt, val->as_int_num_value_unsafe()->get_num_value(), true));
                    break;
                case ValueKind::Bool:
                    gv->setInitializer(llvm::ConstantInt::get(
                        gt, val->as_bool_unsafe()->value ? 1 : 0));
                    break;
                default: break;
            }
        } else if (gt->isFloatTy() && val->val_kind() == ValueKind::Float) {
            gv->setInitializer(llvm::ConstantFP::get(gt, val->as_float_unsafe()->value));
        } else if (gt->isDoubleTy() && val->val_kind() == ValueKind::Double) {
            gv->setInitializer(llvm::ConstantFP::get(gt, val->as_double_unsafe()->value));
        }
    }
    for (const mir::MIRFunction& fn : m.functions) {
        if (!gen.mir_emit_function(m, fn)) {
            std::cerr << "[MIR-LLVM] LLVM emission failed\n";
            return;
        }
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

void ASTProcessor::code_gen_declare(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.declare_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Declare", &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Declare");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::code_gen_compile(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.compile_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Compile", abs_path, &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Compile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::code_gen_external_implement_declare(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.external_implement_declare_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "ExtDeclare", abs_path, &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "ExtDeclare");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::code_gen_external_implement(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.external_implement_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "ExtCompile", abs_path, &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "ExtCompile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::declare_and_compile(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    // create a di compile unit
    if(gen.di.isEnabled) {
        const auto compileUnit = gen.di.createDiCompileUnit(chem::string_view(abs_path.data(), abs_path.size()));
        gen.di.start_di_compile_unit(compileUnit);
        gen.declare_and_compile(nodes_vec);
        gen.di.end_di_compile_unit();
    } else {
        gen.declare_and_compile(nodes_vec);
    }
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Compile", &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Compile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::external_declare_nodes(
        Codegen& gen,
        Scope& import_res,
        const std::string& abs_path
) {
    auto& nodes_vec = import_res.nodes;
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.external_declare_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Declare", &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Declare");
        gen.diagnostics.clear();
    }
}

int ASTProcessor::compile_module(
    Codegen& gen,
    LabModule* module
) {

    // MIR-driven LLVM backend (migration): build MIR and emit LLVM IR from it.
    // The AST-based lowering below is retained only until the MIR path covers
    // every construct, then it is deleted.
    return mir_compile_module(gen, module);

    // let's create a flat vector of direct dependencies, that we want to process
    std::vector<LabModule*> dependencies;
    shallow_dedupe_sorted(dependencies, module->dependencies);

    // creating a compile unit
    const auto compileUnit = gen.di.createDiCompileUnit(module->direct_files.empty() ? chem::string_view("") : chem::string_view(module->direct_files.front().abs_path));
    gen.di.start_di_compile_unit(compileUnit);

    // we will declare the direct dependencies of this module
    for(const auto dep : dependencies) {
        for(auto& metaFile : dep->direct_files) {

            auto& file = *metaFile.result;
            auto& body = file.unit.scope.body;
            auto& abs_path = file.abs_path;

            if (gen.di.isEnabled) {

                // start the file scope
                gen.di.start_file_scope(metaFile.result);

                // declare external nodes
                external_declare_nodes(gen, body, file.abs_path);

                // end the file scope
                gen.di.end_file_scope();

            } else {

                // declare external nodes
                external_declare_nodes(gen, body, file.abs_path);

            }

        }
    }

    // The second loop deals with files that are within current module
    // We just declare the files in current module in this loop that's it, we create prototypes of structs and functions
    // in the next loop we will finally create bodies
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;
        auto& result = file;

        ASTUnit& unit = file.unit;

        // print the benchmark or verbose output received from processing
        if((options->benchmark || options->verbose) && !empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[Declare] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }

        auto& abs_path = file.abs_path;

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(&result);

            // compiling the nodes
            code_gen_declare(gen, unit.scope.body.nodes, abs_path);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // compiling the nodes
            code_gen_declare(gen, unit.scope.body.nodes, abs_path);

        }

        // clear everything we allocated using file allocator to make it re-usable
        file_allocator.clear();

    }

    // these are the generically monomorphized instantiations
    // that were created by this module
    auto& instantiations = container.get_current_module_instantiations();

    // we track for which files we created new di units
    // we must set them to null (so they don't get reused)
    std::vector<ASTFileResult*> newly_created_di_units;

    // declaring the generic instantiations created in this module
    for (const auto node : instantiations) {
        const auto file_scope = node->get_file_scope();
        if (file_scope == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file scope from a generically monomorphized declaration");
#endif
            continue;
        }
        const auto result = file_scope->meta.result;
        if (result == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file result from a generically monomorphized declaration");
#endif
            continue;
        }

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(result);

            // just declare
            node->code_gen_declare(gen);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // just declare
            node->code_gen_declare(gen);

        }

    }

    // implementing the generic instantiations created in this module
    for (const auto node : instantiations) {
        const auto file_scope = node->get_file_scope();
        if (file_scope == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file scope from a generically monomorphized declaration");
#endif
            continue;
        }
        const auto result = file_scope->meta.result;
        if (result == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file result from a generically monomorphized declaration");
#endif
            continue;
        }

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(result);

            // implement the node
            node->code_gen(gen);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // implement the node
            node->code_gen(gen);

        }

    }

    // The fourth loop also only compiles the files that present inside this module
    // This loop will compile the bodies of the functions inside the current module
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;
        auto& result = file;

        ASTUnit& unit = file.unit;

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(&result);

            // compiling the nodes
            code_gen_compile(gen, unit.scope.body.nodes, file.abs_path);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // compiling the nodes
            code_gen_compile(gen, unit.scope.body.nodes, file.abs_path);

        }

        // clear everything we allocated using file allocator to make it re-usable
        file_allocator.clear();

    }

    // end the di compile unit
    // this causes struct types to die
    gen.di.end_di_compile_unit();
    // this is really important
    // it removes cached types, cached scopes
    // if this is not called, freed memory can make into next module's compilation
    gen.di.finalize();
    // clear the generic instantiations for this module in the container
    // so these don't get generated when generating code for the next module
    container.clear_current_module_instantiations();
    // here we store struct types, function callee values to reuse them inside module
    // clearing this after module has compiled prevents reusing function callee's created
    // for this module, forces us to redeclare functions and structs in external module
    gen.mod_ptr_cache.clear();
    // here we store pointers (function callee values) for functions in different implementations
    // llvm pointers for functions inside impl blocks (that implement interfaces for structs) are stored here
    // clearing this has the same advantage of preventing reuse
    gen.trait_impl_func_map.clear();

    return 0;

}