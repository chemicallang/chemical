// Copyright (c) Chemical Language Foundation 2025.

#include "ASTProcessor.h"

#include <memory>
#include "parser/Parser.h"
#include "compiler/SymbolResolver.h"
#include "compiler/typeverify/TypeVerifyAPI.h"
#include "preprocess/2c/TranslationContext.h"
#include "utils/Benchmark.h"
#include "utils/JoinedTasks.h"
#include <sstream>
#include "utils/PathUtils.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "std/chem_string.h"
#include "rang.hpp"
#include "preprocess/RepresentationVisitor.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/ComptimeBlock.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/statements/ChildrenMapNode.h"
#include "ast/statements/VarInit.h"
#include <filesystem>
#include <random>

#include "lexer/Lexer.h"
#include "stream/FileInputSource.h"
#include "ast/base/GlobalInterpretScope.h"
#include "preprocess/ImportPathHandler.h"
#include "compiler/lab/mod_conv/ModToLabConverter.h"
#include "ast/statements/Import.h"
#include "compiler/symres/NodeSymbolDeclarer.h"
#include "compiler/symres/GenericInstantiatorPassAPI.h"
#include "compiler/lab/LabGetMethodInjection.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/Namespace.h"
#include "ast/base/BaseType.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/If.h"
#include "ast/structures/Scope.h"

#include "compiler/mir/MIRModule.h"
#include "compiler/mir/MIRArena.h"
#include "compiler/mir/MIRTypeBuilder.h"
#include "compiler/mir/MIRLowerer.h"
#include "compiler/mir/MIRTaskContext.h"
#include "compiler/mir/MIRMangleBuffer.h"
#include "compiler/mir/MIRInterpProgram.h"

#include "ctpl.h"

#include <atomic>
#include <future>
#include <thread>
#include <unordered_map>
#include "compiler/async/AwaitNormalizePass.h"
#include "compiler/mir/MIREmitter.h"
#include "compiler/mir/MIRDeclEmitter.h"
#include "compiler/mir/MIRVtable.h"
#include "compiler/cbi/model/ASTBuilder.h"
#include "compiler/mangler/NameMangler.h"

#ifdef COMPILER_BUILD
#include "compiler/ctranslator/CTranslator.h"
#endif

std::string ASTProcessorOptions::get_resources_path() {
    if(!resources_path.empty()) return resources_path;
    resources_path = resources_path_rel_to_exe(exe_path);
    if(resources_path.empty()) {
        std::cerr << rang::fg::yellow << "warning: " << rang::fg::reset;
        std::cerr << "couldn't locate resources path relative to compiler's executable" << std::endl;
    }
    return resources_path;
}

void getFilesInDirectory(std::vector<std::string>& filePaths, const std::filesystem::path& dirPath) {
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            const auto fPath = entry.path().string();
            if(fPath.ends_with(".ch")) {
                filePaths.emplace_back(canonical_path(fPath));
                if (filePaths.back().empty()) {
                    // will not happen most of the time, since OS is providing us the paths
                    std::cerr << "error: couldn't determine canonical path for the file '" << entry.path().string() << '\'' << std::endl;
                    filePaths.pop_back();
                }
            }
        } else if(entry.is_directory()) {
            getFilesInDirectory(filePaths, entry.path());
        }
    }
}

bool ASTProcessor::empty_diags(ASTFileResult& result) {
    return result.lex_diagnostics.empty() && result.parse_diagnostics.empty();
}

void ASTProcessor::print_results(ASTFileResult& result, const chem::string_view& abs_path, bool benchmark) {
    if(!empty_diags(result)) {
        std::cout << rang::style::bold << rang::fg::magenta << "[Parsed] " << abs_path << rang::fg::reset << rang::style::reset << '\n';
        Diagnoser::print_diagnostics(result.lex_diagnostics, abs_path, "Lexer");
        Diagnoser::print_diagnostics(result.parse_diagnostics, abs_path, "Parser");
    }
    if (benchmark) {
        ASTProcessor::print_benchmarks(std::cout, "Lexer", &result.lex_benchmark);
        ASTProcessor::print_benchmarks(std::cout, "Parser", &result.parse_benchmark);
    }
    std::cout << std::flush;
}

#if defined(DEBUG) || defined(PRINT_FILE_ORDER_SEED)

namespace {

    std::uint64_t file_order_seed() {
        static const std::uint64_t seed = []() -> std::uint64_t {
            if (const char* env = std::getenv("FILE_ORDER_SEED")) {
                return static_cast<std::uint64_t>(std::strtoull(env, nullptr, 10));
            }

            return (static_cast<std::uint64_t>(std::random_device{}()) << 32) |
                   static_cast<std::uint64_t>(std::random_device{}());
        }();

        return seed;
    }

    void print_file_order_seed_once() {
        static const bool printed = [] {
            std::cout << "File order seed: " << file_order_seed()
                      << " (set FILE_ORDER_SEED to reproduce)\n";
            return true;
        }();
        (void)printed;
    }

#ifdef DEBUG
    std::mt19937_64& file_order_rng() {
        static std::mt19937_64 rng(file_order_seed());
        return rng;
    }
#endif

} // namespace

#ifdef DEBUG
void shuffle_files(std::vector<std::string>& files) {
    std::sort(files.begin(), files.end());   // makes the shuffle reproducible across machines
    print_file_order_seed_once();
    std::shuffle(files.begin(), files.end(), file_order_rng());
}
#endif

#endif

void ASTProcessor::determine_module_files(
        ImportPathHandler& path_handler,
        LocationManager& loc_man,
        LabModule* module
) {
    auto& files = module->direct_files;
    if(!files.empty()) {
        // it seems the files have already been determined
        return;
    }
    switch(module->type) {
        case LabModuleType::Files: {
            for (auto& str: module->paths) {
                auto abs_path = canonical(str.to_view());
                if (abs_path.empty()) {
                    std::cerr << rang::fg::red << "error: " << rang::fg::reset << "couldn't determine canonical path for file '" << str.data() << "' in module '" << module->name << '\'' << std::endl;
                    continue;
                }
                auto fileId = loc_man.encodeFile(abs_path);
                // all these files belong to the given module, so it's scope will be used
                files.emplace_back(fileId, &module->module_scope, std::move(abs_path));
            }
            return;
        }
        case LabModuleType::ObjFile:
        case LabModuleType::CFile:
        case LabModuleType::CPPFile:
            return;
        case LabModuleType::Directory:
            for(auto& dir_path : module->paths) {
                auto dir_path_p = (std::filesystem::path) dir_path.to_view();
                if (!std::filesystem::exists(dir_path_p)) {
                    std::cerr << rang::fg::red << "error: " << rang::fg::reset << "directory / file doesn't exist at path'" << dir_path << "' in module '" << *module << '\'' << std::endl;
                    continue;
                }
                if (std::filesystem::is_directory(dir_path_p)) {
                    std::vector<std::string> filePaths;
                    getFilesInDirectory(filePaths, dir_path_p);
                    // why shuffle ? good question.
                    // our compiler doesn't depend on file order, different declarations can be in any order in files (with any order)
                    // our compiler would compile those files properly
                    // but sometimes compiler has bugs in it, that only become visible when files are in certain order
                    // so in debug mode, we always shuffle the files, in hopes to test the compiler in all sorts of file orderings
                    // if there's a compiler crash it can be replicated via this seed we are printing
                    // note: rel-with-deb-info builds do NOT shuffle — they use the system file order like
                    // release, but still print the seed so a crash in CI can be reproduced with a shuffled
                    // debug build using the exact same FILE_ORDER_SEED.
#if defined(DEBUG) || defined(PRINT_FILE_ORDER_SEED)
                    print_file_order_seed_once();
#endif
#ifdef DEBUG
                    shuffle_files(filePaths);
#endif
                    for (auto& abs_path : filePaths) {
                        auto fileId = loc_man.encodeFile(abs_path);
                        files.emplace_back(fileId, &module->module_scope, abs_path);
                    }
                } else if(dir_path.ends_with(".ch")) {
                    auto dir_path_view = dir_path.to_view();
                    auto abs_path = canonical_path(dir_path_view);
                    if(abs_path.empty()) {
                        std::cerr << rang::fg::red << "error: " << rang::fg::reset << "couldn't determine canonical path for file '" << dir_path_view << "' in module '" << *module << '\'' << std::endl;
                        continue;
                    } else {
                        auto fileId = loc_man.encodeFile(abs_path);
                        files.emplace_back(fileId, &module->module_scope, abs_path);
                    }
                } else {
                    std::cerr << rang::fg::red << "error: " << rang::fg::reset << "path '" << dir_path << "' in module '" << *module << "' is not a directory or a chemical file" << std::endl;
                }
            }
            return;
    }
}

bool ASTProcessor::import_module_files_direct(
        ctpl::thread_pool& pool,
        std::vector<ASTFileMetaData>& files,
        LabModule* module
) {
    return import_chemical_files_direct(pool, files);
}

SymbolRange ASTProcessor::sym_res_tld_declare_file(Scope& scope, unsigned int fileId, const std::string& abs_path) {
    // doing stuff
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    const auto range = resolver->tld_declare_file(scope, fileId, abs_path);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:declare", abs_path, &bm_results);
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:declare");
    }
    return range;
}

SymResSignatureResult ASTProcessor::sym_res_link_sig_file(Scope& scope, unsigned int fileId,
                                                          const std::string& abs_path, const SymbolRange& range) {
    // doing stuff
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    auto result = sym_res_signature(*resolver, &scope, range);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:link_sig", abs_path, &bm_results);
    }
    if(!result.diagnostics.empty()) {
        Diagnoser::print_diagnostics(result.diagnostics, chem::string_view(abs_path), "SymRes:link_sig");
    }
    return result;
}

void ASTProcessor::sym_res_generic_instantiation_file(
        Scope& scope,
        unsigned int fileId,
        const std::string& abs_path,
        const SymbolRange& range,
        SymResSignatureResult& sym_res
) {
    // doing stuff
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    auto result = sym_res_generic_instantiation(*resolver, &scope, sym_res, range);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:gen_inst", abs_path, &bm_results);
    }
    if(!result.diagnostics.empty()) {
        Diagnoser::print_diagnostics(result.diagnostics, chem::string_view(abs_path), "SymRes:gen_inst");
    }
}

static void generate_automatic_functions_in_scope(
        Scope& scope,
        ASTAllocator& pub_allocator,
        ASTAllocator& mod_allocator,
        ASTDiagnoser& diagnoser
) {
    for(auto node : scope.nodes) {
        switch(node->kind()) {
            case ASTNodeKind::StructDecl: {
                auto sd = (StructDefinition*)node;
                auto& alloc = sd->specifier() == AccessSpecifier::Public ? pub_allocator : mod_allocator;
                sd->generate_automatic_functions(alloc, diagnoser, sd);
                break;
            }
            case ASTNodeKind::VariantDecl: {
                auto vd = (VariantDefinition*)node;
                auto& alloc = vd->specifier() == AccessSpecifier::Public ? pub_allocator : mod_allocator;
                vd->generate_automatic_functions(alloc, diagnoser, vd);
                break;
            }
            case ASTNodeKind::GenericStructDecl: {
                auto gsd = (GenericStructDecl*)node;
                if(gsd->master_impl) {
                    auto& alloc = gsd->master_impl->specifier() == AccessSpecifier::Public ? pub_allocator : mod_allocator;
                    gsd->master_impl->generate_automatic_functions(alloc, diagnoser, gsd);
                }
                break;
            }
            case ASTNodeKind::GenericVariantDecl: {
                auto gvd = (GenericVariantDecl*)node;
                if(gvd->master_impl) {
                    auto& alloc = gvd->master_impl->specifier() == AccessSpecifier::Public ? pub_allocator : mod_allocator;
                    gvd->master_impl->generate_automatic_functions(alloc, diagnoser, gvd);
                }
                break;
            }
            case ASTNodeKind::NamespaceDecl: {
                auto ns = (Namespace*)node;
                Scope ns_scope(ns, 0);
                ns_scope.nodes = ns->nodes;
                generate_automatic_functions_in_scope(ns_scope, pub_allocator, mod_allocator, diagnoser);
                break;
            }
            default:
                break;
        }
    }
}

static void generate_automatic_functions_for_module(
        LabModule* module,
        ASTAllocator& pub_allocator,
        ASTAllocator& mod_allocator,
        ASTDiagnoser& diagnoser
) {
    for(auto& file_ptr : module->direct_files) {
        auto& file = *file_ptr.result;
        generate_automatic_functions_in_scope(file.unit.scope.body, pub_allocator, mod_allocator, diagnoser);
    }
}

void ASTProcessor::sym_res_after_link_sig_file(
        Scope& scope,
        unsigned int fileId,
        const std::string& abs_path,
        const SymbolRange& range
) {
    // doing stuff
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    sym_res_after_signature(*resolver, &scope);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:after_link_sig", abs_path, &bm_results);
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:after_link_sig");
    }
}

SymResLinkBodyResult ASTProcessor::sym_res_link_file(Scope& scope, unsigned int fileId, const std::string& abs_path, const SymbolRange& range) {
    // doing stuff
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    auto result = sym_res_link_body_pass(*resolver, &scope, range);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:link", abs_path, &bm_results);
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:link");
    }
    return result;
}

void ASTProcessor::sym_res_declare_and_link_file(Scope& scope, unsigned int fileId, const std::string& abs_path) {
    // doing stuff
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    resolver->declare_and_link_file(scope, fileId, abs_path);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "SymRes:link_seq", abs_path, &bm_results);
    }
    if(!resolver->diagnostics.empty()) {
        resolver->print_diagnostics(chem::string_view(abs_path), "SymRes:link_seq");
    }
}

inline static void declareAllSymbols(SymbolResolver& resolver, ChildrenMapNode* children) {
    // user didn't give any alias or symbols
    // declare everything
    for (auto& sym: children->symbols) {
        resolver.declare_or_shadow(sym.first, sym.second);
    }
}

static void declareChildren(SymbolResolver& resolver, ModuleDependency& dep, ChildrenMapNode* children) {
    if(dep.info == nullptr) {
        declareAllSymbols(resolver, children);
        return;
    }
    if(!dep.info->alias.empty()) {
        // does not shadow
        resolver.declare(dep.info->alias, children);
    } else if(!dep.info->symbols.empty()) {
        // declare imported symbols only
        for(auto& sym : dep.info->symbols) {
            resolver.declareImportedSymbol(children, sym.parts, sym.alias, dep.info->location);
        }
    } else {
        declareAllSymbols(resolver, children);
    }
}

static SymResSignatureResult link_sig_file_task(SymbolResolver* resolver, ASTFileResult* file) {
    return sym_res_signature(*resolver, &file->unit.scope.body, file->private_symbol_range);
}

static GenInstSignatureResult gen_inst_file_task(SymbolResolver* resolver, ASTFileResult* file) {
    return sym_res_generic_instantiation(*resolver, &file->unit.scope.body, file->sig_result, file->private_symbol_range);
}

static SymResLinkBodyResult link_body_generic_decls_task(SymbolResolver* resolver, ASTFileResult* file) {
    return sym_res_link_body_generic_decls_pass(*resolver, &file->unit.scope.body, file->private_symbol_range);
}

static SymResLinkBodyResult link_body_task(SymbolResolver* resolver, ASTFileResult* file) {
    return sym_res_link_body_pass(*resolver, &file->unit.scope.body, file->private_symbol_range);
}

namespace {

/**
 * waits for all the given futures, returning true if any of them reported errors
 * the vector is emptied, so a next phase can reuse it
 */
inline bool join_all(std::vector<std::future<bool>>& futures) {
    bool has_errors = false;
    for(auto& future : futures) {
        if(future.get()) {
            has_errors = true;
        }
    }
    futures.clear();
    return has_errors;
}

}

int ASTProcessor::sym_res_module(LabModule* module, ctpl::thread_pool& pool) {

    const auto prev_mod_scope = resolver->current_mod_scope;
    resolver->current_mod_scope = &module->module_scope;
    const auto mod_index = resolver->module_scope_start();

    // declare symbols of directly imported modules
    for(auto& dep : module->dependencies) {

        // checking if already cached
        if(dep.module->children != nullptr) {
            declareChildren(*resolver, dep, dep.module->children);
            continue;
        }

        // creating new children map node
        const auto children = resolver->ast_allocator->allocate<ChildrenMapNode>();
        new (children) ChildrenMapNode(&module->module_scope, module->module_scope.encoded_location());

        // traversing the dependencies to store children into the map
        MapSymbolDeclarer declarer(children->symbols);
        for(auto& file_ptr : dep.module->direct_files) {
            auto& file = *file_ptr.result;
            for(const auto node : file.unit.scope.body.nodes) {
                declare_node(declarer, node, AccessSpecifier::Public);
            }
        }

        // storing the children for caching
        dep.module->children = children;

        // declare children
        declareChildren(*resolver, dep, children);

    }

    // get symbol table
    auto& table = resolver->getSymbolTable();
    auto& symbols = table.get_symbols();
    const auto module_symbols_start = symbols.size();

    // tiny flag for checking error
    bool errored = false;

    // declare symbols for all files once in the module
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        file.private_symbol_range = sym_res_tld_declare_file(file.unit.scope.body, file.file_id, file.abs_path);

        // report and clear diagnostics
        if (resolver->has_errors() && !options->ignore_errors) {
            if(options->stop_on_file_error) return 1;
            errored = true;
        }
        resolver->reset_errors();

    }

    // clear everything allocated during tld declare
    file_allocator.clear();

    if(errored) return 1;

    std::vector<std::future<bool>> futures;
    futures.reserve(module->direct_files.size());
    // every task pushed in this function must be joined before returning, even when a
    // file reports errors and we stop early, because the tasks reference this processor
    // and the allocators owned by our caller's stack frame
    JoinedTasks<bool> joined(futures);

    // link the signature of the files in parallel
    for(auto& file_ptr : module->direct_files) {
        auto& file = *file_ptr.result;
        futures.emplace_back(pool.push([this, &file](int id){
            auto res = link_sig_file_task(resolver, &file);
            if(!res.diagnostics.empty()) {
                std::lock_guard<std::mutex> guard(print_mutex);
                Diagnoser::print_diagnostics(res.diagnostics, chem::string_view(file.abs_path), "SymRes:link_sig");
            }
            auto has_errors = res.has_errors;
            file.sig_result = std::move(res);
            return has_errors;
        }));
    }
    // join every task before deciding, a pending task may not outlive this function
    if(join_all(futures)) {
        if(options->stop_on_file_error) return 1;
        errored = true;
    }

    // clear everything allocated during link signature pass
    file_allocator.clear();

    if(errored) return 1;

    // generate automatic functions (constructors, destructors) for all containers
    // before generic instantiation — this single recursive pass ensures every container
    // gets its automatic functions generated exactly once, with correct depth-first
    // order so member type destructors exist before dependent container destructors.
    generate_automatic_functions_for_module(module, *resolver->ast_allocator, *resolver->mod_allocator, *resolver);

    // generic instantiation pass in parallel
    for(auto& file_ptr : module->direct_files) {
        auto& file = *file_ptr.result;
        futures.emplace_back(pool.push([this, &file](int id){
            auto res = gen_inst_file_task(resolver, &file);
            if(!res.diagnostics.empty()) {
                std::lock_guard<std::mutex> guard(print_mutex);
                Diagnoser::print_diagnostics(res.diagnostics, chem::string_view(file.abs_path), "SymRes:gen_inst");
            }
            return res.has_errors;
        }));
    }
    // join every task before deciding, a pending task may not outlive this function
    if(join_all(futures)) {
        if(options->stop_on_file_error) return 1;
        errored = true;
    }

    // clear everything allocated during gen instantiator pass
    file_allocator.clear();

    if(errored) return 1;

    // after link the signature of the files
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        sym_res_after_link_sig_file(file.unit.scope.body, file.file_id, file.abs_path, file.private_symbol_range);
        // report and clear diagnostics
        if (resolver->has_errors() && !options->ignore_errors) {
            if(options->stop_on_file_error) return 1;
            errored = true;
        }
        resolver->reset_errors();

    }

    // clear everything allocated during after link signature
    file_allocator.clear();

    if(errored) return 1;

    // Two-pass link body: pass 1 resolves generic declaration master bodies,
    // pass 2 does full link body (instantiate generics, resolve function bodies).

    for(auto& file_ptr : module->direct_files) {
        auto& file = *file_ptr.result;
        futures.emplace_back(pool.push([this, &file](int id){
            auto res = link_body_generic_decls_task(resolver, &file);
            if(!res.diagnostics.empty()) {
                std::lock_guard<std::mutex> guard(print_mutex);
                Diagnoser::print_diagnostics(res.diagnostics, chem::string_view(file.abs_path), "SymRes:gen_inst");
            }
            return res.has_errors;
        }));
    }
    // join every task before deciding, a pending task may not outlive this function
    if(join_all(futures)) {
        if(options->stop_on_file_error) return 1;
        errored = true;
    }

    // pass 2: full link body (parallel)
    for(auto& file_ptr : module->direct_files) {
        auto& file = *file_ptr.result;
        futures.emplace_back(pool.push([this, &file](int id){
            auto res = link_body_task(resolver, &file);
            if(!res.diagnostics.empty()) {
                std::lock_guard<std::mutex> guard(print_mutex);
                Diagnoser::print_diagnostics(res.diagnostics, chem::string_view(file.abs_path), "SymRes:link");
            }
            return res.has_errors;
        }));
    }
    // join every task before deciding, a pending task may not outlive this function
    if(join_all(futures)) {
        if(options->stop_on_file_error) return 1;
        errored = true;
    }

    // clear everything allocated during pass 2
    file_allocator.clear();

    if(errored) return 1;

    // link core nodes, only if the module name is core
    if (module->scope_name.empty() && module->name == "core") {
        // this is the core module
        resolver->link_core_nodes();
    }

    // we need to search for main function in each module and make it no_mangle
    // so it won't be mangled (module scope and name gets added, which can cause no entry point error)
    // only do this for application modules - library modules keep main mangled
    if(module->package_kind == PackageKind::Application) {
        const auto main_func = resolver->find("main");
        if(main_func && main_func->kind() == ASTNodeKind::FunctionDecl) {
            if (options->verbose) {
                std::cout << "[lab] " << "making found 'main' function no_mangle" << std::endl;
            }
            main_func->as_function_unsafe()->set_no_mangle(true);
        }
    }

    resolver->module_scope_end(mod_index);
    resolver->stored_file_symbols.clear();
    resolver->current_mod_scope = prev_mod_scope;
    return 0;
}

int ASTProcessor::sym_res_module_seq(LabModule* module) {
    const auto mod_index = resolver->module_scope_start();

    // declare symbols for all files once in the module

    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        sym_res_declare_and_link_file(file.unit.scope.body, file.file_id, file.abs_path);

        // report and clear diagnostics
        if (resolver->has_errors() && !options->ignore_errors) {
            std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
            return 1;
        }
        resolver->reset_errors();

        // clear everything allocated during symbol resolution of current file
        file_allocator.clear();

    }

    resolver->module_scope_end(mod_index);
    return 0;
}

void ASTProcessor::print_benchmarks(std::ostream& stream, const std::string_view& TAG, BenchmarkResults* bm_results) {
    stream << '[' << TAG << ']' << " completed " << bm_results->representation() << std::endl;
}

void ASTProcessor::print_benchmarks(std::ostream& stream, const std::string_view& TAG, const std::string_view& Name, BenchmarkResults* bm_results) {
    stream << '[' << TAG << ']' << ' ' << '\'' << Name << "' completed " << bm_results->representation() << std::endl;
}


// this imports a chemical file without any imports
ASTFileResult* chem_file_concur_importer_direct(
        int id,
        ASTProcessor* processor,
        ASTFileMetaData& fileData,
        bool use_job_allocator
) {
    // import the file
    processor->import_chemical_file(*fileData.result, fileData.file_id, fileData.abs_path, use_job_allocator);

    // print the file results
    processor->print_file_results(*fileData.result, chem::string_view(fileData.abs_path), processor->options->benchmark_files);

    // return the result
    return fileData.result;
}

#ifdef DEBUG
#define DEBUG_FUTURE false
#endif

bool ASTProcessor::import_chemical_files_direct(
        ctpl::thread_pool& pool,
        std::vector<ASTFileMetaData>& files
) {
    std::vector<std::future<ASTFileResult*>> futures;
    futures.reserve(files.size());
    // safety net : no task pushed here may outlive this function
    JoinedTasks<ASTFileResult*> joined(futures);

    // launch all files concurrently
    for(auto& fileData : files) {

        const auto file_id = fileData.file_id;
        const auto& abs_path = fileData.abs_path;

        // we must try to store chem::string_view into the fileData, from the beginning
        const auto ptr = new ASTFileResult(file_id, "", fileData.module);
        // copy the metadata into it
        fileData.result = ptr;
        *static_cast<ASTFileMetaData*>(ptr) = fileData;
        // cache uses std::unique_ptr to destruct it
        // we must store in cache, otherwise we'll have a memory leak
        cache.emplace(file_id, ptr);

#if defined(DEBUG_FUTURE) && DEBUG_FUTURE
        std::promise<bool> promise;
        promise.set_value(chem_file_concur_importer_direct(0, this, fileData, false));
        futures.emplace_back(promise.get_future());
#else
        futures.emplace_back(
                pool.push(chem_file_concur_importer_direct, this, fileData, false)
        );
#endif

    }

    // put each file sequentially into the out_files
    // note : every future is joined, even when a file failed to parse. Returning while
    // tasks are still pending would leave them writing into this processor and into the
    // allocators owned by our caller's stack frame, after that frame is gone
    bool success = true;
    for(auto& wrap : futures) {
        const auto result = wrap.get();
        if(!result->continue_processing) {
            success = false;
        }
    }
    futures.clear();

    return success;
}

struct TypeVerifyFileResult {
    bool has_errors = false;
    std::vector<Diag> diagnostics;
};

TypeVerifyFileResult type_verify_file_task(
    ASTProcessor* processor,
    ASTFileResult* file
) {
    TypeVerifyFileResult result;
    // make a local diagnoser
    ASTDiagnoser diagnoser(processor->loc_man);

    // run verification
    type_verify(processor->resolver->implsIndex, diagnoser, processor->file_allocator, file->unit.scope.body.nodes);

    result.has_errors = diagnoser.has_errors();
    result.diagnostics = std::move(diagnoser.diagnostics);
    return result;
}

bool ASTProcessor::type_verify_module_parallel(ctpl::thread_pool& pool, LabModule* module) {
    std::vector<std::future<TypeVerifyFileResult>> futures;
    futures.reserve(module->direct_files.size());
    // safety net : no task pushed here may outlive this function. The loop below joins
    // every future with get(), but if one of them throws the remaining futures would be
    // destroyed without waiting, leaving tasks running against this processor and the
    // allocators owned by our caller's stack frame
    JoinedTasks<TypeVerifyFileResult> joined(futures);

    for(auto& fileData : module->direct_files) {
        // push task
        futures.emplace_back(pool.push([this, &fileData](int id){
            return type_verify_file_task(this, fileData.result);
        }));
    }

    bool success = true;
    int i = 0;
    for(auto& f : futures) {
        auto res = f.get();
        if(res.has_errors) {
            success = false;
        }
        if(!res.diagnostics.empty()) {
            std::lock_guard<std::mutex> guard(print_mutex);
            // print diagnostics
            Diagnoser::print_diagnostics(res.diagnostics, chem::string_view(module->direct_files[i].abs_path), "TypeCheck");
        }
        i++;
    }

    return success;
}

void ASTProcessor::import_chemical_files_recursive(
        ctpl::thread_pool& pool,
        ConcurrentParsingState& state,
        std::vector<ASTFileMetaData>& files,
        bool use_job_allocator,
        bool in_task
) {

    // the thread that pushes the tasks counts as a task itself : the completion promise
    // must only be fulfilled once the pushing thread has pushed every file, otherwise the
    // counter can reach zero mid loop ( tasks finish while we are still pushing ), the
    // promise gets fulfilled early and the waiting caller proceeds with tasks still running
    const bool is_producer = !in_task;
    if(is_producer) {
        state.pushed_task();
    }

    if(files.empty()) {
        if(is_producer) {
            state.done_task();
        }
        return;
    }

    // if has imports, we import those files
    // launch all files concurrently
    for(auto& fileData : files) {

        const auto file_id = fileData.file_id;
        const auto& abs_path = fileData.abs_path;

        {
            std::lock_guard<std::mutex> guard(import_mutex);
            auto found = cache.find(file_id);
            if (found != cache.end()) {
                fileData.result = found->second.get();
                // inform the import statement about the file result
                if(fileData.stmt) {
                    fileData.stmt->setResult(fileData.result);
                }
                continue;
            }

            // we must try to store chem::string_view into the fileData, from the beginning
            const auto ptr = new ASTFileResult(file_id, "", fileData.module);
            fileData.result = ptr;
            // inform the import statement about the file result
            if(fileData.stmt) {
                fileData.stmt->setResult(ptr);
            }
            *static_cast<ASTFileMetaData*>(ptr) = fileData;
            cache.emplace(file_id, ptr);
        }

        if (in_task) {
            // When called from within a thread pool task, we must process imports sequentially
            // to avoid deadlock. If we push more tasks to the pool while all threads are busy,
            // and those tasks need to wait for subtasks, we deadlock.
            // Instead, process this file and its imports sequentially in the current thread.
            import_chemical_file_recursive(*fileData.result, pool, state, fileData, use_job_allocator);
        } else {
            // this is done inside the mutex lock, so to prevent race conditions when appending
            // to futures vector
            state.pushed_task();
            // the task must not capture a reference into `files`: that vector can be appended to
            // (and therefore reallocate) while the task is still pending, which would leave the
            // captured metadata dangling. The result object is heap allocated and owned by the
            // import cache for the whole run, so it is stable to capture. It already holds a copy
            // of the metadata (`*static_cast<ASTFileMetaData*>(ptr) = fileData` above).
            auto* fileResult = fileData.result;
            pool.push([this, &pool, &state, fileResult, use_job_allocator](int){
                import_chemical_file_recursive(*fileResult, pool, state, *static_cast<ASTFileMetaData*>(fileResult), use_job_allocator, true);
                state.done_task();
            });
        }

    }

    if(is_producer) {
        // done pushing : once every pushed task also finishes, the counter reaches zero
        state.done_task();
    }

}

bool ASTProcessor::figure_out_direct_imports(
        ASTFileMetaData& fileData,
        std::vector<ASTNode*>& fileNodes,
        std::vector<ASTFileMetaData>& outImports
) {
    for(auto node : fileNodes) {

        if(node->kind() == ASTNodeKind::ImportStmt) {

            auto stmt = node->as_import_stmt_unsafe();

            if(!stmt->isLocalFileImport()) {
                // skip native, local or remote module imports
                continue;
            }

            const auto& path = stmt->getSourcePath();

            // resolve the import path of this import statement
            auto replaceResult = path_handler.resolve_import_path(fileData.abs_path, path.view());

            // replaced is empty (this happens when canonical fails)
            if (replaceResult.replaced.empty()) {
                std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't resolve path '" << stmt->getSourcePath() << "' at ";
                std::cerr << loc_man.formatLocation(stmt->encoded_location()) << std::endl;
                return false;
            }

            if(replaceResult.error.empty()) {

                auto fileId = loc_man.encodeFile(replaceResult.replaced);

                auto module = fileData.module;

                // here since this path begins with '@'
                // we must determine which module it belongs to, we index modules based on 'scope_name:module_name' format
                // sometimes the scope is empty, so user can directly use -> import "@std/fs.ch"
                if(path[0] == '@') {

                    // get at directive, which contains module name
                    const auto atDirective = path_handler.get_atDirective(path.view());

                    // determine module
                    const auto found = mod_storage.find_module(atDirective.replaced);
                    if(found) {

                        module = &found->module_scope;

                    } else {

                        // this is an error, because we couldn't determine the module for this file
                        // even though it's path begins with a '@' which means it's external to module

                        // TODO: error out here because we couldn't find the module
                        // error_out(loc_man, stmt->encoded_location(), "couldn't determine module");
                        // return false;

                    }

                }

                outImports.emplace_back(fileId, module, std::move(replaceResult.replaced), stmt);

            } else {

                std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't resolve path '" << stmt->getSourcePath() << "' at ";
                std::cerr << loc_man.formatLocation(stmt->encoded_location()) << std::endl;
                return false;

            }

        } else {

            // import statements are supposed to be at the top
            break;

        }

    }

    return true;

}

bool ASTProcessor::import_mod_file_as_lab(
        ASTFileMetaData& meta,
        ASTFileResult& result,
        bool use_job_allocator,
        InputSource* inp_source
) {

    // get the file id
    auto& modFile = meta.abs_path;
    const auto modFileId = meta.file_id;

    // importing .mod file into data
    ModuleFileData data(meta);
    auto importModFileRes = import_chemical_mod_file(file_allocator, file_allocator, loc_man, data, modFileId, modFile, inp_source);
    if(!importModFileRes) {
        result.continue_processing = false;
        result.lex_diagnostics = std::move(data.lex_diagnostics);
        result.parse_diagnostics = std::move(data.parse_diagnostics);
        return false;
    }

    // lets use it to translate module file into a build.lab and import it
    std::ostringstream stream;
    convertToBuildLab(data, stream);
    const auto labOut = stream.view();
    InputSource labInpSource(labOut.data(), labOut.size());

    // import the file into result
    return import_chemical_file(result, modFileId, modFile, &labInpSource, use_job_allocator);

}

std::optional<FileInputSource> ASTProcessor::make_file_input_source(const char* abs_path, ASTFileResult& result) {
    FileInputSource inp_source(abs_path);
    const auto err = inp_source.error();
    if(err != nullptr) {
        result.continue_processing = false;
        result.read_error = err->format();
        std::cerr << rang::fg::red << "error: " << rang::fg::reset << "reading file '" << abs_path;
        if (result.stmt != nullptr) {
            std::cerr << "' imported with path '" << result.stmt->getSourcePath();
        }
        if(!result.read_error.empty()) {
            std::cerr << "' because " << result.read_error;
        }
        return std::nullopt;
    }
    return inp_source;
}

bool import_file_in_lab(
        ASTProcessor& proc,
        ASTFileMetaData& meta,
        ASTFileResult& result,
        bool use_job_allocator
) {
    auto& abs_path = meta.abs_path;
    const auto fileId = meta.file_id;
    // import the file if it has no error
    FileInputSource inp_source(abs_path.data());
    const auto err = inp_source.error();
    if(err == nullptr) {
        if (abs_path.ends_with(".mod")) {
            return proc.import_mod_file_as_lab(meta, result, use_job_allocator, &inp_source);
        } else {
            // import the file into result (lex and parse)
            return proc.import_chemical_file(result, fileId, abs_path, &inp_source, use_job_allocator);
        }
    }

    // since couldn't import both .lab / .mod we return the original error
    result.continue_processing = false;
    result.read_error = err->format();
    std::cerr << rang::fg::red << "error: " << rang::fg::reset << "reading file '" << abs_path;
    if (result.stmt != nullptr) {
        std::cerr << "' imported with path '" << result.stmt->getSourcePath();
    }
    if(!result.read_error.empty()) {
        std::cerr << "' because " << result.read_error;
    }
    if (result.stmt != nullptr) {
        std::cerr << " at " << proc.loc_man.formatLocation(result.stmt->encoded_location());
    }
    std::cerr << std::endl;
    return false;

}

bool ASTProcessor::import_chemical_file_recursive(
        ASTFileResult& result,
        ctpl::thread_pool& pool,
        ConcurrentParsingState& state,
        ASTFileMetaData& parentFileData,
        bool use_job_allocator,
        bool in_task
) {

    // import the file into result (lex and parse)
    const auto success = import_file_in_lab(*this, parentFileData, result, use_job_allocator);
    if(!success) {
        state.set_has_errors();
        print_file_results(result, chem::string_view(result.abs_path), options->benchmark_files);
        return false;
    }

    // print the file results
    print_file_results(result, chem::string_view(result.abs_path), options->benchmark_files);

    // figure out files imported by this file
    const auto success2 = figure_out_direct_imports(parentFileData, result.unit.scope.body.nodes, result.imports);
    if(!success2) {
        state.set_has_errors();
        return false;
    }

    // handle the imports of this file
    // We're already in a task (or being called from one), so pass in_task=true to avoid
    // submitting more tasks to the pool and causing deadlock.
    import_chemical_files_recursive(pool, state, result.imports, use_job_allocator, true);
    return true;

}

// this imports a chemical file without any imports
std::pair<ASTFileResult*, std::vector<Token>> chem_file_concur_importer_direct_toks(
        int id,
        ASTProcessor* processor,
        ASTFileMetaData& fileData,
        bool use_job_allocator,
        bool keep_comments
) {

    // out tokens
    std::vector<Token> tokens;

    // import the file
    processor->import_chemical_file_with_tokens(*fileData.result, fileData.file_id, fileData.abs_path, use_job_allocator, &tokens, keep_comments);

    // print the file results
    processor->print_file_results(*fileData.result, chem::string_view(fileData.abs_path), processor->options->benchmark_files);

    // return the result
    return { fileData.result, std::move(tokens) };
}

bool ASTProcessor::import_chemical_files_direct_with_tokens(
        ctpl::thread_pool& pool,
        std::vector<ASTFileMetaData>& files,
        std::unordered_map<unsigned int, std::vector<Token>>& token_map,
        bool keep_comments
) {
    std::vector<std::future<std::pair<ASTFileResult*, std::vector<Token>>>> futures;
    futures.reserve(files.size());
    // safety net : no task pushed here may outlive this function
    JoinedTasks<std::pair<ASTFileResult*, std::vector<Token>>> joined(futures);

    // launch all files concurrently
    for(auto& fileData : files) {

        const auto file_id = fileData.file_id;
        const auto& abs_path = fileData.abs_path;

        // we must try to store chem::string_view into the fileData, from the beginning
        const auto ptr = new ASTFileResult(file_id, "", fileData.module);
        // copy the metadata into it
        *static_cast<ASTFileMetaData*>(ptr) = fileData;
        fileData.result = ptr;
        // cache uses std::unique_ptr to destruct it
        // we must store in cache, otherwise we'll have a memory leak
        cache.emplace(file_id, ptr);

#if defined(DEBUG_FUTURE) && DEBUG_FUTURE
        std::promise<bool> promise;
        promise.set_value(chem_file_concur_importer_direct_toks(0, this, fileData, false, keep_comments));
        futures.emplace_back(promise.get_future());
#else
        futures.emplace_back(
                pool.push(chem_file_concur_importer_direct_toks, this, fileData, false, keep_comments)
        );
#endif

    }

    // put each file sequentially into the out_files
    // note : every future is joined, even when a file failed to parse. Returning while
    // tasks are still pending would leave them writing into this processor and into the
    // allocators owned by our caller's stack frame, after that frame is gone
    bool success = true;
    for(auto& wrap : futures) {
        auto result = wrap.get();
        if(!result.first->continue_processing) {
            success = false;
        }
        token_map[result.first->file_id] = std::move(result.second);
    }
    futures.clear();

    return success;
}

void ASTProcessor::print_file_results(ASTFileResult& result, const chem::string_view& abs_path, bool benchmark) {
    std::lock_guard print_lock(print_mutex);
    print_results(result, abs_path, benchmark);
    // after printing we dispose the logs
    auto moved1 = std::move(result.lex_diagnostics);
    auto moved2 = std::move(result.parse_diagnostics);
}

bool ASTProcessor::parse_chemical_file(
        ASTFileResult& result,
        unsigned int fileId,
        const std::string_view& abs_path,
        InputSource* inp_source,
        LocationManager& loc_man,
        AnnotationController& controller,
        CompilerBinder& binder,
        TypeBuilder& typeBuilder,
        ASTAllocator& file_allocator,
        ASTAllocator& mod_allocator,
        ASTAllocator& job_allocator,
        bool use_job_allocator,
        bool is64Bit,
        bool benchmark
) {

    result.abs_path = abs_path;
    result.file_id = fileId;
    result.private_symbol_range = { 0, 0 };
    result.continue_processing = true;

    auto& unit = result.unit;

    Lexer lexer(std::string(abs_path), *inp_source, &binder, file_allocator);
    std::vector<Token> tokens;

    // actual lexing
    if(benchmark) {
        result.lex_benchmark.benchmark_begin();
        lexer.getTokens(tokens);
        result.lex_benchmark.benchmark_end();
    } else {
        lexer.getTokens(tokens);
    }

    const auto total_toks = tokens.size();
    if(total_toks == 0) {
        // empty file
        return true;
    } else if(tokens[total_toks - 1].type == TokenType::Unexpected) {
        auto& last = tokens[total_toks - 1];
        std::string errStr = "unexpected token with value '";
        errStr.append(last.value.view());
        errStr.append("'");
        lexer.diagnoser.diagnostic(chem::string_view(errStr), chem::string_view(result.unit.scope.getAbsPath()), last.position, last.position, DiagSeverity::Error);
    }

    // move lexer diagnostics
    if(!lexer.diagnoser.diagnostics.empty()) {
        result.lex_diagnostics = std::move(lexer.diagnoser.diagnostics);
    }

    // do not continue, if error occurs during lexing
    if(lexer.diagnoser.has_errors()) {
        result.continue_processing = false;
        return false;
    }

    // parse the file
    Parser parser(
            fileId,
            abs_path,
            tokens.data(),
            loc_man,
            controller,
            job_allocator,
            use_job_allocator ? job_allocator : mod_allocator,
            typeBuilder,
            is64Bit,
            &binder
    );

    // setting file scope as parent of all nodes parsed
    parser.parent_node = &result.unit.scope;

    // actual parsing
    if(benchmark) {
        result.parse_benchmark.benchmark_begin();
        parser.parse(unit.scope.body.nodes);
        result.parse_benchmark.benchmark_end();
    } else {
        parser.parse(unit.scope.body.nodes);
    }

    // move parser diagnostics
    if(!parser.diagnostics.empty()) {
        result.parse_diagnostics = std::move(parser.diagnostics);
    }

    // continue
    if(parser.has_errors()) {
        result.continue_processing = false;
        return false;
    } else {
        return true;
    }
}

bool ASTProcessor::import_chemical_file(
        ASTFileResult& result,
        unsigned int fileId,
        const std::string_view& abs_path,
        InputSource* inp_source,
        bool use_job_allocator
) {

    result.abs_path = abs_path;
    result.file_id = fileId;
    result.private_symbol_range = { 0, 0 };
    result.continue_processing = true;

    auto& unit = result.unit;

    Lexer lexer(std::string(abs_path), *inp_source, &binder, file_allocator);
    std::vector<Token> tokens;

    const auto benchmark = options->benchmark_files;

    // actual lexing
    if(benchmark) {
        result.lex_benchmark.benchmark_begin();
        lexer.getTokens(tokens);
        result.lex_benchmark.benchmark_end();
    } else {
        lexer.getTokens(tokens);
    }

    const auto total_toks = tokens.size();
    if(total_toks == 0) {
        // empty file
        return true;
    } else if(tokens[total_toks - 1].type == TokenType::Unexpected) {
        auto& last = tokens[total_toks - 1];
        std::string errStr = "unexpected token with value '";
        errStr.append(last.value.view());
        errStr.append("'");
        lexer.diagnoser.diagnostic(chem::string_view(errStr), chem::string_view(result.unit.scope.getAbsPath()), last.position, last.position, DiagSeverity::Error);
    }

    // move lexer diagnostics
    if(!lexer.diagnoser.diagnostics.empty()) {
        result.lex_diagnostics = std::move(lexer.diagnoser.diagnostics);
    }

    // do not continue, if error occurs during lexing
    if(lexer.diagnoser.has_errors()) {
        result.continue_processing = false;
        return false;
    }

    // parse the file
    Parser parser(
            fileId,
            abs_path,
            tokens.data(),
            resolver->comptime_scope.loc_man,
            controller,
            job_allocator,
            use_job_allocator ? job_allocator : mod_allocator,
            type_builder,
            resolver->is64Bit,
            &binder
    );

    // setting file scope as parent of all nodes parsed
    parser.parent_node = &result.unit.scope;

    // actual parsing
    if(benchmark) {
        result.parse_benchmark.benchmark_begin();
        parser.parse(unit.scope.body.nodes);
        result.parse_benchmark.benchmark_end();
    } else {
        parser.parse(unit.scope.body.nodes);
    }

    // move parser diagnostics
    if(!parser.diagnostics.empty()) {
        result.parse_diagnostics = std::move(parser.diagnostics);
    }

    // continue
    if(parser.has_errors()) {
        result.continue_processing = false;
        return false;
    } else {
        return true;
    }
}

bool ASTProcessor::import_chemical_file_with_tokens(
        ASTFileResult& result,
        unsigned int fileId,
        const std::string_view& abs_path,
        InputSource* inp_source,
        bool use_job_allocator,
        std::vector<Token>* out_tokens,
        bool keep_comments
) {

    result.abs_path = abs_path;
    result.file_id = fileId;
    result.private_symbol_range = { 0, 0 };
    result.continue_processing = true;

    auto& unit = result.unit;

    Lexer lexer(std::string(abs_path), *inp_source, &binder, file_allocator);
    lexer.keep_comments = keep_comments;
    std::vector<Token> tokens;

    const auto benchmark = options->benchmark_files;

    // actual lexing
    if(benchmark) {
        result.lex_benchmark.benchmark_begin();
        lexer.getTokens(tokens);
        result.lex_benchmark.benchmark_end();
    } else {
        lexer.getTokens(tokens);
    }

    const auto total_toks = tokens.size();
    if(total_toks == 0) {
        // empty file
        return true;
    } else if(tokens[total_toks - 1].type == TokenType::Unexpected) {
        auto& last = tokens[total_toks - 1];
        std::string errStr = "unexpected token with value '";
        errStr.append(last.value.view());
        errStr.append("'");
        lexer.diagnoser.diagnostic(chem::string_view(errStr), chem::string_view(result.unit.scope.getAbsPath()), last.position, last.position, DiagSeverity::Error);
    }

    // comments will be disposed as soon as input source dies
    // because source is memory mapped, we will move the comment to job allocator
    if(keep_comments) {
        for(auto& t : tokens) {
            if(t.type == TokenType::SingleLineComment || t.type == TokenType::MultiLineComment) {
                const auto s = t.value.size();
                const auto str_ptr = job_allocator.allocate_str(t.value.data(), s);
                t.value = chem::string_view(str_ptr, s);
            }
        }
    }

    // copy tokens if requested
    if(out_tokens != nullptr) {
        *out_tokens = tokens;
    }

    // move lexer diagnostics
    if(!lexer.diagnoser.diagnostics.empty()) {
        result.lex_diagnostics = std::move(lexer.diagnoser.diagnostics);
    }

    // do not continue, if error occurs during lexing
    if(lexer.diagnoser.has_errors()) {
        result.continue_processing = false;
        return false;
    }

    // we should remove the comment tokens before giving them to the parser
    // parser does not expect comments or whitespaces
    std::vector<Token> clean_tokens;
    Token* tokens_to_parse = tokens.data();

    if(keep_comments) {
        clean_tokens.reserve(tokens.size());
        for(auto& t : tokens) {
            if(t.type != TokenType::SingleLineComment && t.type != TokenType::MultiLineComment && t.type != TokenType::Whitespace) {
                clean_tokens.push_back(t);
            }
        }
        tokens_to_parse = clean_tokens.data();
    }

    // parse the file
    Parser parser(
            fileId,
            abs_path,
            tokens_to_parse,
            resolver->comptime_scope.loc_man,
            controller,
            job_allocator,
            use_job_allocator ? job_allocator : mod_allocator,
            type_builder,
            resolver->is64Bit,
            &binder
    );

    // setting file scope as parent of all nodes parsed
    parser.parent_node = &result.unit.scope;

    // actual parsing
    if(benchmark) {
        result.parse_benchmark.benchmark_begin();
        parser.parse(unit.scope.body.nodes);
        result.parse_benchmark.benchmark_end();
    } else {
        parser.parse(unit.scope.body.nodes);
    }

    // move parser diagnostics
    if(!parser.diagnostics.empty()) {
        result.parse_diagnostics = std::move(parser.diagnostics);
    }

    // continue
    if(parser.has_errors()) {
        result.continue_processing = false;
        return false;
    } else {
        return true;
    }

}

bool ASTProcessor::import_chemical_mod_file(
        ASTAllocator& fileAllocator,
        ASTAllocator& modAllocator,
        LocationManager& loc_man,
        ModuleFileData& data,
        unsigned int fileId,
        const std::string_view& abs_path,
        InputSource* inp_source
) {

    Lexer lexer(std::string(abs_path), *inp_source, nullptr, fileAllocator);
    std::vector<Token> tokens;
    lexer.getTokens(tokens);

    // parse the file
    BasicParser parser(
        loc_man,
        fileId,
        tokens.data()
    );

    // if diagnostics not empty
    if (!lexer.diagnoser.diagnostics.empty()) {
        data.lex_diagnostics = std::move(lexer.diagnoser.diagnostics);
    }


    // if lexer has errors
    if (lexer.diagnoser.has_errors()) {
        return false;
    }

    // setting file scope as parent of all nodes parsed
    parser.parent_node = &data.scope;

    // parse the module file
    parser.parseModuleFile(modAllocator, data);

    // append the diagnostics of parser to data
    if (!parser.diagnostics.empty()) {
        data.parse_diagnostics = std::move(parser.diagnostics);
    }

    return !parser.has_errors();

}

bool ASTProcessor::import_chemical_mod_file(
        ASTAllocator& fileAllocator,
        ASTAllocator& modAllocator,
        LocationManager& loc_man,
        ModuleFileData& data,
        unsigned int fileId,
        const std::string_view& abs_path
) {
    FileInputSource inp_source(abs_path.data());
    const auto err = inp_source.error();
    if(err != nullptr) {
        data.read_error = err->format();
        std::cerr << rang::fg::red << "error: " << rang::fg::reset << "reading file '" << abs_path;
        // TODO: we don't have access to the importer stmt
        // if (result.stmt != nullptr) {
        //     std::cerr << "' imported with path '" << result.stmt->getSourcePath();
        // }
        if(!data.read_error.empty()) {
            std::cerr << "' because " << data.read_error;
        }
        // TODO: we don't have access to the importer stmt
        // if (result.stmt != nullptr) {
        //     std::cerr << " at " << loc_man.formatLocation(result.stmt->encoded_location());
        // }
        std::cerr << rang::fg::reset << std::endl;
        return false;
    }
    return import_chemical_mod_file(fileAllocator, modAllocator, loc_man, data, fileId, abs_path, &inp_source);
}

bool ASTProcessor::print_translation_diagnostics(CTranslationContext& visitor, const std::string& abs_path) {
    if(visitor.diagnostics.empty()) {
        return false;
    }
    visitor.print_diagnostics(chem::string_view(abs_path), "2cTranslation");
    std::cout << std::endl;
    const auto had_errors = visitor.has_errors();
    visitor.reset_errors();
    return had_errors;
}

/**
 * translation errors mean the generated C cannot be trusted, so the caller must
 * stop the build instead of continuing with code that no longer matches the
 * source. Reading the errors out of the visitor also prints them
 */
bool ASTProcessor::print_translation_diagnostics(CTranslationContext& visitor) {
    if(visitor.diagnostics.empty()) {
        return false;
    }
    visitor.print_diagnostics("2cTranslation");
    std::cout << std::endl;
    const auto had_errors = visitor.has_errors();
    visitor.reset_errors();
    return had_errors;
}


/**
 * these two functions have following purpose
 * 1 - we need to process only direct dependencies, so 'outMods' only contains modules that are given to us in 'inMods'
 * 2 - this sorts the modules in the order of independence, so modules that are least dependent are processed first
 *   - this way if module a depends on module b, however both are present in inMods (in any order), we'll process b first since a needs it
 * 3 - a module will only appear once, however inMods could contain the module as many times as it wants
 */
void shallow_dedupe_sorted(std::vector<LabModule*>& outMods, std::unordered_map<LabModule*, bool>& modsMap, LabModule* mod) {
    for(auto& dep : mod->dependencies) {
        shallow_dedupe_sorted(outMods, modsMap, dep.module);
    }
    auto found = modsMap.find(mod);
    if(found != modsMap.end() && found->second) {
        outMods.emplace_back(mod);
        found->second = false;
    }
}
void shallow_dedupe_sorted(std::vector<LabModule*>& outMods, std::vector<ModuleDependency>& deps) {
    std::unordered_map<LabModule*, bool> direct_mods(deps.size());
    // save these modules in direct_mods
    for(auto& mod : deps) {
        direct_mods[mod.module] = true;
    }
    for(auto& dep : deps) {
        shallow_dedupe_sorted(outMods, direct_mods, dep.module);
    }
}


namespace {
std::string mir_mangle_name(NameMangler& mangler, ASTNode* decl);
}

/**
 * Rename an application's `async func main` to the internal symbol
 * `__chx_async_main`, before any prototype is emitted, so the synchronous
 * `int main` trampoline can take the entry name (async-await design Section
 * 5.5). Mirrors the legacy 2c visitor's `rename_async_entry`; the rename is
 * idempotent, and mutating the identifier here means the mangler produces the
 * same name for the prototype, the definition, and every call.
 *
 * The trampoline itself is built by MIR (`MIRLowerer::lower_async_main_trampoline`)
 * when the ramp's module is translated, for both backends.
 */
static void rename_async_entry(ASTNode* node) {
    if (node == nullptr) return;
    switch (node->kind()) {
        case ASTNodeKind::FunctionDecl: {
            auto* decl = node->as_function_unsafe();
            if (decl->is_async() && decl->is_no_mangle() &&
                decl->name_view() == chem::string_view("main")) {
                decl->set_identifier(chem::string_view(mir::MIRLowerer::kAsyncMainInternalName));
            }
            return;
        }
        case ASTNodeKind::NamespaceDecl:
            for (ASTNode* child : node->as_namespace_unsafe()->nodes) rename_async_entry(child);
            return;
        case ASTNodeKind::Scope:
            for (ASTNode* child : node->as_scope_unsafe()->nodes) rename_async_entry(child);
            return;
        case ASTNodeKind::IfStmt: {
            // a top-level `if <comptime condition>` contributes its taken
            // branch's declarations to the module
            auto* stmt = node->as_if_stmt_unsafe();
            if (stmt->is_top_level() && stmt->computed_scope.has_value()) {
                Scope* taken = stmt->computed_scope.value();
                if (taken) for (ASTNode* child : taken->nodes) rename_async_entry(child);
            }
            return;
        }
        default:
            return;
    }
}

int ASTProcessor::declare_module(
    CTranslationContext& c_visitor,
    LabModule* module
) {
    // rename an application's `async func main` before its prototype is written
    for (auto& file_ptr : module->direct_files) {
        if (!file_ptr.result) continue;
        for (ASTNode* node : file_ptr.result->unit.scope.body.nodes) rename_async_entry(node);
    }
    for (ASTNode* node : container.get_current_module_instantiations()) rename_async_entry(node);

    // MIR-driven declaration pass. A throwaway MIR module/types is created here;
    // aggregate names are derived deterministically from the mangler, so the
    // implement pass (which owns its own MIR state) spells them identically.
    mir::MIRModule decl_module;
    mir::MIRTypeBuilder decl_types(decl_module);
    decl_types.set_name_resolver([&c_visitor](ASTNode* n) {
        return mir_mangle_name(c_visitor.mangler, n);
    });
    mir::MIRDeclEmitter decl_emitter(
        decl_module, decl_types, c_visitor.mangler, loc_man,
        c_visitor.comptime_scope, c_visitor.binder, c_visitor.allocator);
    decl_emitter.set_embedded_decl_repl([&c_visitor](EmbeddedNode* node) -> ASTNode* {
        // The MIR->LLVM backend stores this replacement in the `EmbeddedNode`
        // and reads it again after every file has been translated, so it must
        // outlive `file_level_reset()`; the module allocator does.
        ASTAllocator& repl_alloc = c_visitor.embedded_replacement_allocator
            ? *c_visitor.embedded_replacement_allocator : c_visitor.allocator;
        ASTBuilder builder(&repl_alloc, c_visitor.comptime_scope.typeBuilder);
        const auto fn = c_visitor.binder.findHook(node->name, CBIFunctionType::ReplacementNodeDeclare);
        if (!fn) return nullptr;
        return ((EmbeddedNodeReplacementFunc) fn)(&builder, &c_visitor, node);
    });
    decl_emitter.set_static_interface_recorder([&c_visitor](InterfaceDefinition* iface) {
        c_visitor.store_static_interface_for_stub_impl(iface);
    });

    auto& w = c_visitor.writer;
    auto fwd_all = [&](const std::vector<ASTNode*>& nodes) {
        for (ASTNode* node : nodes) decl_emitter.fwd_declare(node);
    };
    auto alias_all = [&](const std::vector<ASTNode*>& nodes) {
        for (ASTNode* node : nodes) decl_emitter.declare_type_alias(node);
    };
    auto declare_all = [&](const std::vector<ASTNode*>& nodes) {
        for (ASTNode* node : nodes) decl_emitter.declare(node);
    };

    // forward declare dependencies & dependencies of dependencies & current module
    for (auto& file_ptr : module->direct_files) {
        auto& file = *file_ptr.result;
        if ((options->benchmark_files || options->verbose) && !empty_diags(file)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[FwdDeclare] "
                      << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }
        fwd_all(file.unit.scope.body.nodes);
    }

    // declare type aliases
    for (auto& file_ptr : module->direct_files) {
        alias_all(file_ptr.result->unit.scope.body.nodes);
    }

    // generics (new instantiations introduced for this module)
    fwd_all(container.get_current_module_instantiations());
    declare_all(container.get_current_module_instantiations());

    bool had_errors = false;

    // declare the files present in this module (prototypes, struct bodies, globals)
    for (auto& file_ptr : module->direct_files) {
        auto& file = *file_ptr.result;
        if ((options->benchmark_files || options->verbose) && !empty_diags(file)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[Declare] "
                      << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }
        declare_all(file.unit.scope.body.nodes);
    }

    // The declaration pass recorded an ordered MIRDeclList; the AST-free
    // `MIRDeclPrinter::decl_list_c` renders it (action A3).
    decl_emitter.print_decl_list(w);

    if (print_translation_diagnostics(c_visitor)) {
        had_errors = true;
    }

    return had_errors ? 1 : 0;
}

namespace {

bool mir_eligible_function(FunctionDeclaration* decl) {
    if (!decl) return false;
    if (!decl->body.has_value()) return false;
    if (decl->is_extern()) return false;
    if (decl->is_comptime()) return false;
    return true;
}

std::string mir_mangle_name(NameMangler& mangler, ASTNode* decl) {
    BufferedWriter& w = mir::mangle_writer();
    mangler.mangle(w, decl);
    return std::string(w.data(), w.getPosition());
}

// Record the top-level globals *this module declares*, i.e. the ones whose
// definition the C backend writes here (`MIRDeclEmitter::implement`). Every
// reference reached from another module's file is a reference to that symbol,
// not a second definition — see `MIRLowerer::add_owned_global`. The traversal
// mirrors `MIRDeclEmitter::declare` (namespace / scope / taken comptime-if
// branch / resolved CBI replacement).
void mir_collect_owned_globals(const std::vector<ASTNode*>& nodes,
                               mir::MIRLowerer& lowerer) {
    for (ASTNode* node : nodes) {
        if (!node) continue;
        switch (node->kind()) {
            case ASTNodeKind::VarInitStmt: {
                auto* vi = node->as_var_init_unsafe();
                if (vi->is_top_level() && !vi->is_comptime()) lowerer.add_owned_global(vi);
                continue;
            }
            case ASTNodeKind::NamespaceDecl: {
                auto* ns = node->as_namespace_unsafe();
                if (!ns->is_comptime()) mir_collect_owned_globals(ns->nodes, lowerer);
                continue;
            }
            case ASTNodeKind::Scope:
                mir_collect_owned_globals(node->as_scope_unsafe()->nodes, lowerer);
                continue;
            case ASTNodeKind::IfStmt: {
                auto* stmt = node->as_if_stmt_unsafe();
                if (stmt->is_top_level() && stmt->computed_scope.has_value()) {
                    Scope* taken = stmt->computed_scope.value();
                    if (taken) mir_collect_owned_globals(taken->nodes, lowerer);
                }
                continue;
            }
            case ASTNodeKind::EmbeddedNode: {
                // the declaration pass resolves the replacement before the
                // implementation pass runs, so a generated top-level global is
                // attributed to the module that hosts the `#macro`/`#universal`
                // block that produced it
                ASTNode* repl = node->as_embedded_node_unsafe()->replacement;
                if (repl) {
                    std::vector<ASTNode*> one{repl};
                    mir_collect_owned_globals(one, lowerer);
                }
                continue;
            }
            default:
                continue;
        }
    }
}

// MIR translation of a file's top-level nodes: eligible functions are lowered
// and their C bodies emitted from MIR; other nodes still go through the legacy
// visitor (declarations and not-yet-ported constructs) for now.
// Lower one function through MIR and fill `task.out` with its C; the driver
// appends `task.out` at the function's source-order slot. This is the C-path
// body of `lower_and_emit`, extracted so a parallel worker can run it against
// its own task context. Returns false on ineligibility or a lowering/emission
// failure (the caller then takes the inline path, which reports the diagnostic).
static bool mir_lower_and_emit_into(mir::MIRTaskContext& task, mir::MIRModule& module,
                                    FunctionDeclaration* decl, bool llvm_collect,
                                    std::string& error) {
    if (!mir_eligible_function(decl)) return false;
    mir::MIRLowerer& lowerer = task.lowerer;
    task.begin_function(!llvm_collect);
    mir::MIRFunction fn;
    if (!lowerer.lower_function(decl, task.arena, fn, error)) {
        lowerer.clear_lambda_defs();
        return false;
    }
    auto flush_lambdas = [&lowerer, &task]() {
        for (std::string& def : lowerer.lambda_defs()) task.out.append(def.data(), def.size());
        lowerer.clear_lambda_defs();
    };
    if (llvm_collect) {
        module.functions.push_back(fn);
        if (decl->is_async()) {
            module.functions.push_back(lowerer.async_poll());
            module.functions.push_back(lowerer.async_drop());
        }
        if (lowerer.is_async_main_entry(decl)) {
            mir::MIRFunction tramp;
            std::string tramp_error;
            if (!lowerer.lower_async_main_trampoline(decl, fn, task.arena, tramp, tramp_error)) {
                error = tramp_error;
                return false;
            }
            module.functions.push_back(tramp);
        }
        for (const mir::MIRFunction& lf : lowerer.lambda_functions()) {
            module.functions.push_back(lf);
        }
        lowerer.clear_lambda_defs();
        return true;
    }
    std::string emit_error;
    if (decl->is_async()) {
        std::string poll_c, poll_e, c;
        if (!mir::emit_function_c(lowerer.async_poll(), module, poll_c, poll_e)) {
            error = poll_e;
            lowerer.clear_lambda_defs();
            return false;
        }
        if (!mir::emit_function_c(fn, module, c, emit_error)) {
            error = emit_error;
            lowerer.clear_lambda_defs();
            return false;
        }
        flush_lambdas();
        task.out.append(lowerer.async_pre_decls());
        task.out.append(poll_c);
        task.out.append(lowerer.async_post_decls());
        task.out.append(c);
        if (lowerer.is_async_main_entry(decl)) {
            mir::MIRFunction tramp;
            std::string tramp_error;
            if (!lowerer.lower_async_main_trampoline(decl, fn, task.arena, tramp, tramp_error)) {
                error = tramp_error;
                return false;
            }
            std::string tramp_c, tramp_emit_error;
            if (!mir::emit_function_c(tramp, module, tramp_c, tramp_emit_error)) {
                error = tramp_emit_error;
                return false;
            }
            task.out.append(tramp_c);
        }
        return true;
    }
    task.body.clear();
    if (!mir::emit_function_c(fn, module, task.body, emit_error)) {
        error = emit_error;
        lowerer.clear_lambda_defs();
        return false;
    }
    flush_lambdas();
    task.out.append(task.body);
    return true;
}

// Collect, in source order, the plain functions `process_nodes` would lower via
// `lower_and_emit` (top-level functions and struct/union/variant methods), so
// they can be lowered in parallel and merged at their slots. Impl methods and
// functions produced by embedded-node replacement are deliberately excluded —
// they stay on the serial path (impl methods need `iface->active_user` set).
static void mir_collect_parallel_functions(const std::vector<ASTNode*>& nodes,
                                           std::vector<FunctionDeclaration*>& out) {
    for (ASTNode* node : nodes) {
        if (!node) continue;
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl: {
                auto* fd = node->as_function();
                if (mir_eligible_function(fd)) out.push_back(fd);
                break;
            }
            case ASTNodeKind::StructDecl:
            case ASTNodeKind::VariantDecl:
            case ASTNodeKind::UnionDecl: {
                auto* def = node->as_extendable_member_container();
                if (!def) break;
                for (ASTNode* c : def->evaluated_nodes()) {
                    if (c && c->kind() == ASTNodeKind::FunctionDecl) {
                        auto* fd = c->as_function_unsafe();
                        if (mir_eligible_function(fd)) out.push_back(fd);
                    }
                }
                break;
            }
            case ASTNodeKind::NamespaceDecl: {
                auto* ns = node->as_namespace_unsafe();
                if (!ns->is_comptime()) mir_collect_parallel_functions(ns->nodes, out);
                break;
            }
            case ASTNodeKind::Scope:
                mir_collect_parallel_functions(node->as_scope_unsafe()->nodes, out);
                break;
            case ASTNodeKind::IfStmt: {
                auto* stmt = node->as_if_stmt_unsafe();
                if (stmt->is_top_level() && stmt->computed_scope.has_value()) {
                    Scope* taken = stmt->computed_scope.value();
                    if (taken) mir_collect_parallel_functions(taken->nodes, out);
                }
                break;
            }
            default:
                break;
        }
    }
}

bool mir_translate_after_declaration(
        CTranslationContext& visitor,
        mir::MIRTaskContext& task,
        mir::MIRDeclEmitter& decl_emitter,
        const std::vector<ASTNode*>& nodes,
        std::recursive_mutex& visitor_mutex,
        bool llvm_collect = false) {
    mir::MIRModule& module = task.module();
    mir::MIRTypeBuilder& types = task.types();
    mir::MIRLowerer& lowerer = task.lowerer;
    // pass 0: name the aggregate types emitted by declare_module so MIR's C
    // type spelling matches the legacy struct/union/variant declarations
    for (ASTNode* node : nodes) {
        BaseType* bt = nullptr;
        switch (node->kind()) {
            case ASTNodeKind::StructDecl: bt = node->as_struct_def()->known_type(); break;
            case ASTNodeKind::VariantDecl: bt = node->as_variant_def()->known_type(); break;
            case ASTNodeKind::EnumDecl: bt = node->as_enum_decl()->known_type(); break;
            default: continue;
        }
        if (!bt) continue;
        const mir::TypeId tid = types.map(bt);
        const std::string name = mir_mangle_name(visitor.mangler, node);
        module.types.set_name(tid, name.data(), static_cast<uint32_t>(name.size()));
    }

    // pass 1: pre-declare mangled symbols so cross-function calls resolve
    for (ASTNode* node : nodes) {
        if (node->kind() != ASTNodeKind::FunctionDecl) continue;
        auto* decl = node->as_function();
        if (!mir_eligible_function(decl)) continue;
        std::string name = mir_mangle_name(visitor.mangler, decl);
        mir::MIRSymbolRecord rec;
        rec.kind = mir::MIRSymbolKind::Function;
        rec.linkage = decl->is_linkage_public() ? mir::MIRLinkage::External
                                                : mir::MIRLinkage::Internal;
        mir::SymbolId sym = module.symbols.add(
            rec, name.data(), static_cast<uint32_t>(name.size()),
            name.data(), static_cast<uint32_t>(name.size()));
        lowerer.set_function_symbol(decl, sym);
    }

    bool had_errors = false;
    const size_t file_start = visitor.writer.getPosition();

    // (C path) Lower the plain functions in parallel and append their buffers at
    // their source-order slots below. The LLVM path must keep
    // `module.functions` in source order, so it stays serial; impl methods and
    // embedded-node-produced functions stay serial too (see the collector).
    std::unordered_map<FunctionDeclaration*, std::string> pre_lowered;
    // The parallel driver is off until the shared name-mangling buffer race is
    // fixed (see compiler/mir/README.md, session 47): with more than one worker
    // the mangled name written into the type/symbol pools is corrupted. With a
    // single worker the output is correct, so the machinery below is kept but
    // gated; the table/pool locks it relies on are always on.
    constexpr bool kEnableParallelLowering = false;
    if (kEnableParallelLowering && !llvm_collect) {
        std::vector<FunctionDeclaration*> parallel_fns;
        mir_collect_parallel_functions(nodes, parallel_fns);
        constexpr size_t kMinParallelFunctions = 24;
        if (parallel_fns.size() >= kMinParallelFunctions) {
            unsigned nworkers = std::thread::hardware_concurrency();
            if (nworkers == 0) nworkers = 4;
            if (nworkers > parallel_fns.size()) nworkers = static_cast<unsigned>(parallel_fns.size());
            std::atomic<size_t> next{0};
            std::vector<std::vector<std::pair<FunctionDeclaration*, std::string>>> results(nworkers);
            ctpl::thread_pool local_pool(static_cast<int>(nworkers));
            std::vector<std::future<void>> futs;
            futs.reserve(nworkers);
            for (unsigned w = 0; w < nworkers; ++w) {
                futs.push_back(local_pool.push([&, w](int) {
                    // One worker context per thread: its own lowerer (wired with
                    // the driver's callbacks), arena and output buffer.
                    mir::MIRTaskContext ctx(module, types);
                    ctx.lowerer.copy_callbacks_from(task.lowerer);
                    auto& r = results[w];
                    for (;;) {
                        const size_t i = next.fetch_add(1);
                        if (i >= parallel_fns.size()) break;
                        FunctionDeclaration* fd = parallel_fns[i];
                        std::string err;
                        if (mir_lower_and_emit_into(ctx, module, fd, false, err)) {
                            r.emplace_back(fd, std::move(ctx.out));
                        }
                    }
                }));
            }
            for (auto& f : futs) f.get();
            for (auto& r : results) {
                for (auto& pr : r) pre_lowered.emplace(pr.first, std::move(pr.second));
            }
        }
    }

    // Lower one function (top-level, method, or generic instantiation) through
    // MIR and append its C. A function pre-lowered above appends its buffer;
    // everything else (impl methods, embedded-produced functions, and the
    // serial/llvm path) lowers here. Returns false when the function cannot be
    // lowered, so the caller can report the diagnostic.
    auto lower_and_emit = [&](FunctionDeclaration* decl) -> bool {
        auto it = pre_lowered.find(decl);
        if (it != pre_lowered.end()) {
            visitor.writer.append(it->second.data(), it->second.size());
            return true;
        }
        std::string error;
        if (!mir_lower_and_emit_into(task, module, decl, llvm_collect, error)) {
            if (mir_eligible_function(decl)) {
                std::cerr << "[MIR] lowering/emission failed (" << decl->name_str()
                          << "): " << error << "\n";
            }
            return false;
        }
        visitor.writer.append(task.out.data(), task.out.size());
        return true;
    };
    // Lower an interface implementation: overridden methods and the interface's
    // default methods through MIR, then emit the vtable definition.
    std::function<bool(ImplDefinition*)> lower_impl = [&](ImplDefinition* impl) -> bool {
        if (!impl) return false;
        InterfaceDefinition* iface = impl->interface_type
            ? impl->interface_type->get_direct_linked_interface() : nullptr;
        // A static interface has no vtable: the impl's methods are emitted as
        // free functions, and the interface no longer needs a stub. MIR lowers
        // both the definition and the call with the ordinary method ABI.
        // Mark the interface (and inherited interfaces) as implemented so the
        // legacy end_translate stub pass does not also emit its default methods.
        std::function<void(InterfaceDefinition*)> mark_static_impl = [&](InterfaceDefinition* i) {
            if (!i) return;
            if (i->is_static()) {
                std::lock_guard<std::recursive_mutex> lk(visitor_mutex);
                visitor.remove_static_interface_for_stub_impl(i);
            }
            for (auto& inherits : i->inherited) {
                if (inherits.type) {
                    mark_static_impl(inherits.type->get_direct_linked_node()->as_interface_def());
                }
            }
        };
        mark_static_impl(iface);
        ExtendableMembersContainerNode* for_decl = nullptr;
        if (impl->struct_type) {
            auto* mc = impl->struct_type->get_members_container();
            if (mc) for_decl = mc->as_extendable_members_container_unsafe();
        }
        // `for_decl` is null for a primitive impl (`impl Hash for char`); the
        // methods still lower through MIR, only the interface's default methods
        // are generated solely for struct impls (matching the legacy visitor).
        ExtendableMembersContainerNode* prev = iface ? iface->active_user : nullptr;
        for (ASTNode* m : impl->evaluated_nodes()) {
            if (!m || m->kind() != ASTNodeKind::FunctionDecl) continue;
            // `active_user` and the mangler are shared translator state: hold the
            // visitor lock across setting it and lowering the method.
            std::lock_guard<std::recursive_mutex> lk(visitor_mutex);
            if (iface) iface->active_user = for_decl;
            const bool ok = lower_and_emit(m->as_function_unsafe());
            if (iface) iface->active_user = prev;
            if (!ok) {
                visitor.error(m) << "MIR lowering of interface method failed";
            }
        }
        if (iface && for_decl) {
            for (FunctionDeclaration* func : iface->instantiated_functions()) {
                if (impl->contains_direct_func(func->name_view())) continue;
                std::lock_guard<std::recursive_mutex> lk(visitor_mutex);
                iface->active_user = for_decl;
                const bool ok = lower_and_emit(func);
                iface->active_user = prev;
                if (!ok) {
                    visitor.error(func) << "MIR lowering of interface default method failed";
                }
            }
        }
        if (iface) {
            decl_emitter.implement(visitor.writer, impl);
        }
        return true;
    };
    // Lower the methods of a struct/union/variant container (its
    // `evaluated_nodes`), falling back to the legacy visitor for any child that
    // is not a MIR-lowered function.
    auto lower_container = [&](ExtendableMembersContainerNode* def) -> bool {
        if (!def) return false;
        for (ASTNode* child : def->evaluated_nodes()) {
            if (!child) continue;
            if (child->kind() == ASTNodeKind::FunctionDecl) {
                auto* fd = child->as_function_unsafe();
                // extern/comptime methods have no MIR body; their prototypes are
                // emitted by the MIR declaration pass.
                if (!mir_eligible_function(fd)) continue;
                if (lower_and_emit(fd)) continue;
            }
            if (child->kind() == ASTNodeKind::ImplDecl &&
                lower_impl(child->as_impl_def_unsafe())) {
                continue;
            }
            // Generic function declarations are templates; their instantiated
            // copies are lowered separately.
            if (child->kind() == ASTNodeKind::GenericFuncDecl) continue;
            {
                std::lock_guard<std::recursive_mutex> lk(visitor_mutex);
                visitor.error(child) << "MIR lowering of container member failed";
            }
        }
        return true;
    };
    std::function<void(const std::vector<ASTNode*>&)> process_nodes =
        [&](const std::vector<ASTNode*>& list) {
        for (ASTNode* node : list) {
            switch (node->kind()) {
                case ASTNodeKind::FunctionDecl: {
                    auto* fd = node->as_function();
                    // extern/comptime functions have no MIR body; their
                    // prototypes are emitted by the MIR declaration pass.
                    if (mir_eligible_function(fd) && !lower_and_emit(fd)) {
                        std::lock_guard<std::recursive_mutex> lk(visitor_mutex);
                        visitor.error(node) << "MIR lowering failed";
                    }
                    continue;
                }
                case ASTNodeKind::StructDecl:
                case ASTNodeKind::VariantDecl:
                case ASTNodeKind::UnionDecl:
                    lower_container(node->as_extendable_member_container());
                    continue;
                case ASTNodeKind::VarInitStmt: {
                    auto* vi = node->as_var_init_unsafe();
                    if (vi->is_top_level() && !vi->is_comptime()) {
                        // lower the initializer to a MIR constant-expression tree
                        // once (action A3), then define the global from MIR
                        const mir::ConstExprId init = lowerer.build_global_initializer(vi);
                        decl_emitter.implement_global(visitor.writer, node, init);
                    }
                    continue;
                }
                case ASTNodeKind::ImplDecl:
                    if (!lower_impl(node->as_impl_def_unsafe())) {
                        std::lock_guard<std::recursive_mutex> lk(visitor_mutex);
                        visitor.error(node) << "MIR lowering of impl failed";
                    }
                    continue;
                case ASTNodeKind::NamespaceDecl: {
                    auto* ns = node->as_namespace_unsafe();
                    if (!ns->is_comptime()) process_nodes(ns->nodes);
                    continue;
                }
                case ASTNodeKind::Scope:
                    process_nodes(node->as_scope_unsafe()->nodes);
                    continue;
                case ASTNodeKind::IfStmt: {
                    // a top-level `if <comptime condition>` contributes its taken
                    // branch's declarations to the module (e.g. the arch/OS split
                    // in `std/concurrency/threadpool.ch`). The taken scope is
                    // resolved during top-level declaration; its members must be
                    // lowered here too, or the module silently loses them.
                    auto* stmt = node->as_if_stmt_unsafe();
                    if (stmt->is_top_level() && stmt->computed_scope.has_value()) {
                        Scope* taken = stmt->computed_scope.value();
                        if (taken) process_nodes(taken->nodes);
                    }
                    continue;
                }
                case ASTNodeKind::EmbeddedNode: {
                    auto* en = node->as_embedded_node_unsafe();
                    ASTNode* repl = visitor.resolve_embedded_node(en);
                    if (!repl) repl = en->replacement;
                    if (repl) {
                        en->replacement = repl;
                        std::vector<ASTNode*> one{repl};
                        process_nodes(one);
                    }
                    continue;
                }
                default:
                    decl_emitter.implement(visitor.writer, node);
                    continue;
            }
        }
    };
    process_nodes(nodes);

    // emit top-level C definitions for inline (anonymous/locally-named) struct
    // types used by the MIR-lowered bodies, and move them before the functions
    if (!module.inline_structs.empty()) {
        const size_t def_start = visitor.writer.getPosition();
        std::unordered_set<std::string> emitted;
        for (auto& entry : module.inline_structs) {
            auto* bt = static_cast<BaseType*>(entry.second);
            if (!bt || bt->kind() != BaseTypeKind::Struct) continue;
            // the C body of a synthesized inline struct comes from MIR (action A3)
            const std::string body = mir::inline_aggregate_c(module, entry.first);
            if (body.empty()) continue;
            const std::string name = module.types.name_of(entry.first);
            if (!emitted.insert(name).second) continue;
            visitor.new_line_and_indent();
            visitor.write_str(body);
        }
        const size_t def_end = visitor.writer.getPosition();
        if (def_end > def_start) {
            visitor.writer.move_range(def_start, def_end, file_start);
        }
        module.inline_structs.clear();
    }
    return had_errors;
}

/**
 * Emit the stubs of the static interfaces that were declared but never
 * implemented (their default methods as free functions, bodyless methods as a
 * trivial `ret 0`), for the MIR→LLVM backend.
 *
 * The C backend emits the whole job as ONE translation unit and therefore runs
 * its own stub pass once, at job end, over every interface that survived
 * unimplemented (`ASTProcessor::emit_static_interface_stubs`). The LLVM backend
 * emits one object per `LabModule` and has no job-end pass, so each module
 * lowers its own stubs here — while the lowering arena and the interface nodes
 * are still alive — and appends them *last* to the MIR module, so a definition
 * this module already has wins the name lookup.
 *
 * A static interface's method and the matching method of every impl mangle to
 * the same symbol; the stubs are flagged `MF_WEAK_STUB` so an implementation in
 * another module's object beats them at link time (the C backend cannot get
 * this wrong: it knows the final implemented-set before writing a byte).
 */
static int mir_lower_static_interface_stubs(CTranslationContext& visitor,
                                            mir::MIRModule& mir_module,
                                            mir::MIRLowerer& mir_lowerer,
                                            mir::MIRArena& mir_arena) {
    if (visitor.unimplemented_static_interfaces.empty()) return 0;

    // names this module already defines; MIR symbol ids are not name-unique but
    // LLVM globals are, so match by name exactly like `Codegen::mir_function`
    std::unordered_set<std::string> defined;
    for (const mir::MIRFunction& fn : mir_module.functions) {
        if (fn.symbol >= mir_module.symbols.size()) continue;
        defined.insert(mir_module.symbols.mangled_name(mir_module.symbols.get(fn.symbol)));
    }

    int status = 0;
    for (auto& pair : visitor.unimplemented_static_interfaces) {
        InterfaceDefinition* iface = pair.first;
        if (!iface) continue;
        for (FunctionDeclaration* func : iface->non_gen_range()) {
            if (!func || func->is_extern() || func->is_comptime()) continue;
            const std::string name = mir_mangle_name(visitor.mangler, func);
            if (name.empty() || defined.count(name)) continue;
            mir::MIRFunction stub;
            std::string error;
            if (!mir_lowerer.lower_static_interface_stub(func, mir_arena, stub, error)) {
                std::cerr << "[MIR] static interface stub lowering failed (" << name
                          << "): " << error << "\n";
                status = 1;
                continue;
            }
            defined.insert(name);
            mir_module.functions.push_back(stub);
            // `lower_function` may have materialized the async poll / lambda
            // bodies of a default method; keep them like `lower_and_emit` does
            if (mir_lowerer.is_async_lowering()) {
                mir_module.functions.push_back(mir_lowerer.async_poll());
                mir_module.functions.push_back(mir_lowerer.async_drop());
            }
            for (const mir::MIRFunction& lf : mir_lowerer.lambda_functions()) {
                mir_module.functions.push_back(lf);
            }
            mir_lowerer.clear_lambda_defs();
        }
    }
    visitor.unimplemented_static_interfaces.clear();
    return status;
}

} // namespace

int ASTProcessor::implement_module(
    CTranslationContext& c_visitor,
    LabModule* module
) {

    // comment
#ifdef DEBUG
    c_visitor.debug_comment(chem::string_view("Implement:Generics " + module->format()));
#endif    // we will implement the direct dependencies of this module
    // this loop will implement new generic instantiations
    // this and the fourth loop generates bodies of functions
    // (generic instantiation bodies are lowered by MIR below, after the MIR
    // module tables are created)

    // translation errors of the generics above are printed along with the first file
    bool had_errors = false;

    // MIR module tables live across the files of this module so cross-function
    // symbols/types are shared. A fresh arena per function is reset in the
    // driver (MIR is emitted immediately, then discarded).
    //
    // The interpretation job passes a job-wide `MIRInterpProgram` instead: the
    // module, types, task and lock are shared across every source module and the
    // arena is never reset, so all lowered functions stay alive to be executed.
    mir::MIRModule mir_module_local;
    mir::MIRTypeBuilder mir_types_local(mir_module_local);
    mir::MIRTaskContext mir_task_local(mir_module_local, mir_types_local);
    std::recursive_mutex mir_mutex_local;
    mir::MIRModule& mir_module = mir_interp_program ? mir_interp_program->module : mir_module_local;
    mir::MIRTypeBuilder& mir_types = mir_interp_program ? *mir_interp_program->types : mir_types_local;
    mir::MIRTaskContext& mir_task = mir_interp_program ? *mir_interp_program->task : mir_task_local;
    std::recursive_mutex& mir_visitor_mutex =
        mir_interp_program ? mir_interp_program->visitor_mutex : mir_mutex_local;
    mir_module.layout.is64Bit = c_visitor.comptime_scope.target_data.is64Bit;
    mir_module.layout.windows = c_visitor.comptime_scope.target_data.windows;
    // Type-name mangling is serialized by the builder's own lock and reads no
    // shared mutable mangler state, so it does not take the visitor lock (which
    // would invert the builder->visitor order).
    mir_types.set_name_resolver([&c_visitor](ASTNode* n) {
        return mir_mangle_name(c_visitor.mangler, n);
    });
    mir::MIRLowerer& mir_lowerer = mir_task.lowerer;
    mir_lowerer.set_visitor_mutex(&mir_visitor_mutex);
    // Source locations and debug metadata (place names, lexical scopes) are only
    // needed for debug info; the common (release / debug_quick) path pays nothing.
    mir_lowerer.set_debug_info(options->debug_info);
    if (options->debug_info) {
        mir_lowerer.set_source_loc_provider([this, &mir_module, &mir_visitor_mutex](uint64_t enc, mir::MIRSourceLoc& out) -> bool {
            std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
            if (enc == 0) return false;
            const SourceLocation loc(enc);
            const LocationManager::LocationPosData pos = loc_man.getLocationPos(loc);
            if (pos.fileId == 0) return false;
            const std::string_view path = loc_man.getPathForFileId(pos.fileId);
            if (path.empty()) return false;
            out.file = mir_module.source_file(std::string(path));
            out.line = pos.start.line;        // 0-based; the emitter adds 1
            out.column = pos.start.character; // 0-based
            return true;
        });
    }
    mir_lowerer.set_mangler([&c_visitor, &mir_visitor_mutex](ASTNode* n) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        return mir_mangle_name(c_visitor.mangler, n);
    });
    mir_lowerer.set_mangler_into([&c_visitor, &mir_visitor_mutex](ASTNode* n, std::string& out) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        BufferedWriter& w = mir::mangle_writer();
        c_visitor.mangler.mangle(w, n);
        out.append(w.data(), w.getPosition());
    });
    mir_lowerer.set_vtable_mangler([&c_visitor, &mir_visitor_mutex](ASTNode* iface, BaseType* impl) -> std::string {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        if (!iface || !impl) return std::string();
        BufferedWriter w(256);
        auto* idef = iface->as_interface_def_unsafe();
        ASTNode* inode = impl->get_direct_linked_canonical_node();
        if (!inode) inode = impl->get_direct_linked_node();
        auto* container = inode ? inode->as_extendable_member_container() : nullptr;
        if (container) {
            c_visitor.mangler.mangle_vtable_name(w, idef, container);
        } else {
            c_visitor.mangler.mangle_vtable_name(w, idef, impl);
        }
        return std::string(w.data(), w.getPosition());
    });
    mir_lowerer.set_comptime_if_resolver([&c_visitor, &mir_visitor_mutex](IfStatement* stmt, std::string& err) -> Scope* {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        auto resolved = stmt->get_or_resolve_scope(c_visitor.comptime_scope, c_visitor);
        if (!resolved.has_value() || resolved.value() == nullptr) {
            err = "failed to resolve comptime if";
            return nullptr;
        }
        return resolved.value();
    });
    mir_lowerer.set_comptime_eval([&c_visitor, &mir_visitor_mutex](FunctionCall* call, FunctionDeclaration* decl) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        return c_visitor.eval_comptime_call(call, decl);
    });
    mir_lowerer.set_comptime_value_eval([&c_visitor, &mir_visitor_mutex](Value* v) -> Value* {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        if (!v) return nullptr;
        return v->evaluated_value(c_visitor.comptime_scope);
    });
    mir_lowerer.set_comptime_ctor_eval([&c_visitor, &mir_visitor_mutex](FunctionDeclaration* ctor, Value* arg) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        return c_visitor.eval_comptime_ctor(ctor, arg);
    });
    mir_lowerer.set_comptime_block_eval([&c_visitor, &mir_visitor_mutex](ASTNode* node) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        auto* block = static_cast<ComptimeBlock*>(node);
        c_visitor.comptime_scope.interpret(&block->body);
    });
    mir_lowerer.set_current_func([&c_visitor, &mir_visitor_mutex](FunctionTypeBody* f) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        c_visitor.current_func_type = f;
    });
    mir_lowerer.set_impls(&c_visitor.implsIndex, &c_visitor.coreNodes);
    mir_lowerer.set_embedded_repl([&c_visitor, &mir_visitor_mutex](EmbeddedNode* node) -> ASTNode* {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        return c_visitor.resolve_embedded_node(node);
    });
    mir_lowerer.set_async_normalize([&c_visitor, &mir_visitor_mutex](FunctionDeclaration* d) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        normalize_async_body(c_visitor.allocator, d);
    });
    mir_lowerer.set_embedded_value_repl([&c_visitor, &mir_visitor_mutex](EmbeddedValue* v) -> Value* {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        return c_visitor.resolve_embedded_value(v);
    });
    mir_lowerer.set_symbol_lookup([this, module, &mir_visitor_mutex](const std::string& name) -> FunctionDeclaration* {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        if (resolver) {
            ASTNode* node = resolver->getSymbolTable().resolve(
                chem::string_view(name.data(), static_cast<unsigned>(name.size())));
            if (node && node->kind() == ASTNodeKind::FunctionDecl) {
                return node->as_function_unsafe();
            }
        }
        // scan this module and its dependencies for a matching top-level function
        std::function<FunctionDeclaration*(LabModule*, const std::string&)> search =
            [&](LabModule* m, const std::string& n) -> FunctionDeclaration* {
            if (!m) return nullptr;
            for (auto& f : m->direct_files) {
                if (!f.result) continue;
                for (ASTNode* node : f.result->unit.scope.body.nodes) {
                    if (node && node->kind() == ASTNodeKind::FunctionDecl) {
                        auto* fd = node->as_function_unsafe();
                        if (fd->name_view() == chem::string_view(n.data(),
                                                                 static_cast<unsigned>(n.size()))) {
                            return fd;
                        }
                    }
                }
            }
            for (auto& dep : m->dependencies) {
                if (FunctionDeclaration* r = search(dep.module, n)) return r;
            }
            return nullptr;
        };
        return search(module, name);
    });
    mir::MIRArena& mir_arena = mir_task.arena;

    // Attribute this module's own top-level globals before any lowering: a
    // global is defined by the module that declares it, and merely referenced
    // by every other module (see `mir_collect_owned_globals`).
    for (auto& file_ptr : module->direct_files) {
        if (file_ptr.result) {
            mir_collect_owned_globals(file_ptr.result->unit.scope.body.nodes, mir_lowerer);
        }
    }
    mir_collect_owned_globals(container.get_current_module_instantiations(), mir_lowerer);

    mir::MIRDeclEmitter mir_decl(mir_module, mir_types, c_visitor.mangler, loc_man,
                                 c_visitor.comptime_scope, c_visitor.binder, c_visitor.allocator);
    mir_decl.set_embedded_decl_repl([&c_visitor](EmbeddedNode* node) -> ASTNode* {
        // The MIR->LLVM backend stores this replacement in the `EmbeddedNode`
        // and reads it again after every file has been translated, so it must
        // outlive `file_level_reset()`; the module allocator does.
        ASTAllocator& repl_alloc = c_visitor.embedded_replacement_allocator
            ? *c_visitor.embedded_replacement_allocator : c_visitor.allocator;
        ASTBuilder builder(&repl_alloc, c_visitor.comptime_scope.typeBuilder);
        const auto fn = c_visitor.binder.findHook(node->name, CBIFunctionType::ReplacementNodeDeclare);
        if (!fn) return nullptr;
        return ((EmbeddedNodeReplacementFunc) fn)(&builder, &c_visitor, node);
    });
    mir_decl.set_value_eval([&c_visitor](Value* v) -> Value* {
        return v ? v->evaluated_value(c_visitor.comptime_scope) : nullptr;
    });

    // The module's own files are re-parsed fresh for this module (a new
    // `ASTFileResult` per file), so a body reached through one of their
    // `FileScope`s is still alive; a body from a dependency module's reused
    // declaration was freed when that module's allocator was cleared. The
    // lowerer needs this to decide whether a constructor body may be inspected.
    std::unordered_set<ASTNode*> current_module_file_scopes;
    current_module_file_scopes.reserve(module->direct_files.size());
    for (auto& file_ptr : module->direct_files) {
        if (file_ptr.result) {
            current_module_file_scopes.insert((ASTNode*) &file_ptr.result->unit.scope);
        }
    }
    mir_lowerer.set_current_module_file_scopes(&current_module_file_scopes);

    // generic instantiations for this module: lowered by MIR (function bodies);
    // non-function nodes still go through the legacy visitor as declarations
    if (mir_translate_after_declaration(c_visitor, mir_task, mir_decl,
                                        container.get_current_module_instantiations(),
                                        mir_visitor_mutex,
                                        mir_llvm_collect)) {
        had_errors = true;
    }

    // The fourth loop deals with generating function bodies present in the current module
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;

        ASTUnit& unit = file.unit;

#ifdef DEBUG
        c_visitor.debug_comment(chem::string_view(("Translate " + file.abs_path)));
#endif

        // translating to c: function bodies via MIR, declarations via legacy
        if(mir_translate_after_declaration(c_visitor, mir_task, mir_decl, unit.scope.body.nodes, mir_visitor_mutex, mir_llvm_collect)) {
            had_errors = true;
        }

        // clear everything we allocated using file allocator to make it re-usable
        // and other stuff to re use memory (makes it performant)
        c_visitor.file_level_reset();
        // the file allocator reset lets a later file reuse addresses, so the
        // BaseType*->TypeId memo must not survive across files
        mir_types.clear_memo();

    }

    // a module without files still collects diagnostics from its generics
    if(print_translation_diagnostics(c_visitor)) {
        had_errors = true;
    }

    // The MIR→LLVM backend consumes the fully-populated MIR module here, while
    // the lowering arena (which backs the collected MIRFunction storage) is
    // still alive. Static-interface stubs go in first so they are appended last
    // (see `mir_lower_static_interface_stubs`); the C backend instead emits them
    // once at job end, from `ASTProcessor::emit_static_interface_stubs`.
    if (mir_llvm_collect) {
        // Build the vtable type/definition records from the module's top-level
        // declarations, so the LLVM driver emits vtables from MIR (action A3/B)
        // instead of walking the AST itself.
        {
            std::vector<ASTNode*> top;
            for (auto& file_ptr : module->direct_files) {
                if (!file_ptr.result) continue;
                for (ASTNode* n : file_ptr.result->unit.scope.body.nodes) top.push_back(n);
            }
            mir::build_module_vtables(c_visitor.mangler, mir_types, mir_module, top,
                                      /*clear=*/mir_interp_program == nullptr);
        }
        if (mir_lower_static_interface_stubs(c_visitor, mir_module, mir_lowerer,
                                             mir_arena) != 0) {
            had_errors = true;
        }
    }
    if (mir_llvm_collect && mir_llvm_finish) {
        mir_llvm_finish(mir_types, mir_module);
    }

    // this will clear current module instantiations
    // so we don't translate them again
    container.clear_current_module_instantiations();

    // resetting c visitor to use with another module
    c_visitor.reset();

    // the generated c cannot be trusted when errors were reported
    return had_errors ? 1 : 0;

}

int ASTProcessor::interpret_module(CTranslationContext& c_visitor, LabModule* module) {
    // Lower every function body into the job-wide interpretation program (collect
    // mode) instead of emitting linkable C. Mirrors `mir_compile_module` minus
    // the LLVM emission; the caller sets `mir_interp_program`.
    const bool prev_collect = mir_llvm_collect;
    mir_llvm_collect = true;
    int result = declare_module(c_visitor, module);
    if (result == 0) result = implement_module(c_visitor, module);
    mir_llvm_collect = prev_collect;
    return result;
}

int ASTProcessor::emit_static_interface_stubs(CTranslationContext& c_visitor) {
    if (c_visitor.unimplemented_static_interfaces.empty()) return 0;

    mir::MIRModule mir_module;
    mir_module.layout.is64Bit = c_visitor.comptime_scope.target_data.is64Bit;
    mir_module.layout.windows = c_visitor.comptime_scope.target_data.windows;
    mir::MIRTypeBuilder mir_types(mir_module);
    // One lock for every access to the shared `c_visitor` (the mangler, the
    // comptime scope, the AST allocator, diagnostics, `iface->active_user`,
    // `current_func_type`). Lowering workers run the pure MIR computation
    // lock-free and take this only around those shared operations. Recursive so
    // a locked operation that calls another (e.g. `intern_method_function`
    // around the mangler) does not deadlock.
    std::recursive_mutex mir_visitor_mutex;
    // Type-name mangling is serialized by the builder's own lock and reads no
    // shared mutable mangler state, so it does not take the visitor lock (which
    // would invert the builder->visitor order).
    mir_types.set_name_resolver([&c_visitor](ASTNode* n) {
        return mir_mangle_name(c_visitor.mangler, n);
    });
    mir::MIRTaskContext mir_task(mir_module, mir_types);
    mir::MIRLowerer& mir_lowerer = mir_task.lowerer;
    mir_lowerer.set_visitor_mutex(&mir_visitor_mutex);
    // Source locations and debug metadata (place names, lexical scopes) are only
    // needed for debug info; the common (release / debug_quick) path pays nothing.
    mir_lowerer.set_debug_info(options->debug_info);
    if (options->debug_info) {
        mir_lowerer.set_source_loc_provider([this, &mir_module, &mir_visitor_mutex](uint64_t enc, mir::MIRSourceLoc& out) -> bool {
            std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
            if (enc == 0) return false;
            const SourceLocation loc(enc);
            const LocationManager::LocationPosData pos = loc_man.getLocationPos(loc);
            if (pos.fileId == 0) return false;
            const std::string_view path = loc_man.getPathForFileId(pos.fileId);
            if (path.empty()) return false;
            out.file = mir_module.source_file(std::string(path));
            out.line = pos.start.line;        // 0-based; the emitter adds 1
            out.column = pos.start.character; // 0-based
            return true;
        });
    }
    mir_lowerer.set_mangler([&c_visitor, &mir_visitor_mutex](ASTNode* n) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        return mir_mangle_name(c_visitor.mangler, n);
    });
    mir_lowerer.set_mangler_into([&c_visitor, &mir_visitor_mutex](ASTNode* n, std::string& out) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        BufferedWriter& w = mir::mangle_writer();
        c_visitor.mangler.mangle(w, n);
        out.append(w.data(), w.getPosition());
    });
    mir_lowerer.set_vtable_mangler([&c_visitor, &mir_visitor_mutex](ASTNode* iface, BaseType* impl) -> std::string {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        if (!iface || !impl) return std::string();
        BufferedWriter w(256);
        auto* idef = iface->as_interface_def_unsafe();
        ASTNode* inode = impl->get_direct_linked_canonical_node();
        if (!inode) inode = impl->get_direct_linked_node();
        auto* container = inode ? inode->as_extendable_member_container() : nullptr;
        if (container) {
            c_visitor.mangler.mangle_vtable_name(w, idef, container);
        } else {
            c_visitor.mangler.mangle_vtable_name(w, idef, impl);
        }
        return std::string(w.data(), w.getPosition());
    });
    mir_lowerer.set_comptime_if_resolver([&c_visitor, &mir_visitor_mutex](IfStatement* stmt, std::string& err) -> Scope* {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        auto resolved = stmt->get_or_resolve_scope(c_visitor.comptime_scope, c_visitor);
        if (!resolved.has_value() || resolved.value() == nullptr) {
            err = "failed to resolve comptime if";
            return nullptr;
        }
        return resolved.value();
    });
    mir_lowerer.set_comptime_eval([&c_visitor, &mir_visitor_mutex](FunctionCall* call, FunctionDeclaration* decl) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        return c_visitor.eval_comptime_call(call, decl);
    });
    mir_lowerer.set_comptime_value_eval([&c_visitor, &mir_visitor_mutex](Value* v) -> Value* {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        if (!v) return nullptr;
        return v->evaluated_value(c_visitor.comptime_scope);
    });
    mir_lowerer.set_comptime_ctor_eval([&c_visitor, &mir_visitor_mutex](FunctionDeclaration* ctor, Value* arg) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        return c_visitor.eval_comptime_ctor(ctor, arg);
    });
    mir_lowerer.set_comptime_block_eval([&c_visitor, &mir_visitor_mutex](ASTNode* node) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        auto* block = static_cast<ComptimeBlock*>(node);
        c_visitor.comptime_scope.interpret(&block->body);
    });
    mir_lowerer.set_current_func([&c_visitor, &mir_visitor_mutex](FunctionTypeBody* f) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        c_visitor.current_func_type = f;
    });
    mir_lowerer.set_impls(&c_visitor.implsIndex, &c_visitor.coreNodes);
    mir_lowerer.set_embedded_repl([&c_visitor, &mir_visitor_mutex](EmbeddedNode* node) -> ASTNode* {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        return c_visitor.resolve_embedded_node(node);
    });
    mir_lowerer.set_async_normalize([&c_visitor, &mir_visitor_mutex](FunctionDeclaration* d) {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        normalize_async_body(c_visitor.allocator, d);
    });
    mir_lowerer.set_embedded_value_repl([&c_visitor, &mir_visitor_mutex](EmbeddedValue* v) -> Value* {
        std::lock_guard<std::recursive_mutex> lk(mir_visitor_mutex);
        return c_visitor.resolve_embedded_value(v);
    });
    mir::MIRDeclEmitter mir_decl(mir_module, mir_types, c_visitor.mangler, loc_man,
                                 c_visitor.comptime_scope, c_visitor.binder, c_visitor.allocator);
    mir_decl.set_embedded_decl_repl([&c_visitor](EmbeddedNode* node) -> ASTNode* {
        // The MIR->LLVM backend stores this replacement in the `EmbeddedNode`
        // and reads it again after every file has been translated, so it must
        // outlive `file_level_reset()`; the module allocator does.
        ASTAllocator& repl_alloc = c_visitor.embedded_replacement_allocator
            ? *c_visitor.embedded_replacement_allocator : c_visitor.allocator;
        ASTBuilder builder(&repl_alloc, c_visitor.comptime_scope.typeBuilder);
        const auto fn = c_visitor.binder.findHook(node->name, CBIFunctionType::ReplacementNodeDeclare);
        if (!fn) return nullptr;
        return ((EmbeddedNodeReplacementFunc) fn)(&builder, &c_visitor, node);
    });
    mir_decl.set_value_eval([&c_visitor](Value* v) -> Value* {
        return v ? v->evaluated_value(c_visitor.comptime_scope) : nullptr;
    });

    std::vector<ASTNode*> stub_nodes;
    auto int_like = [](BaseTypeKind k) -> bool {
        switch (k) {
            case BaseTypeKind::IntN: case BaseTypeKind::Double: case BaseTypeKind::Float:
            case BaseTypeKind::LongDouble: case BaseTypeKind::Pointer: case BaseTypeKind::NullPtr:
            case BaseTypeKind::Reference: case BaseTypeKind::Bool: case BaseTypeKind::Float128:
            case BaseTypeKind::Function: case BaseTypeKind::String:
                return true;
            default:
                return false;
        }
    };
    for (auto& pair : c_visitor.unimplemented_static_interfaces) {
        for (FunctionDeclaration* func : pair.first->non_gen_range()) {
            if (func->body.has_value()) {
                stub_nodes.push_back(func);
                continue;
            }
            // Bodyless method: emit a trivial stub that returns 0 for int-like
            // return types (matching the legacy stub pass).
            auto& w = c_visitor.writer;
            c_visitor.new_line();
            const std::string proto = mir_decl.function_proto(func);
            w.append(proto.data(), proto.size());
            w.append(" { ", 3);
            if (func->returnType && int_like(func->returnType->canonical()->kind())) {
                w.append("return 0; ", 10);
            }
            w.append("}", 1);
        }
    }
    c_visitor.unimplemented_static_interfaces.clear();
    if (stub_nodes.empty()) return 0;
    return mir_translate_after_declaration(c_visitor, mir_task, mir_decl, stub_nodes,
                                           mir_visitor_mutex) ? 1 : 0;
}
