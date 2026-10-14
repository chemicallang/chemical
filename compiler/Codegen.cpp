// Copyright (c) Chemical Language Foundation 2025.

#ifdef COMPILER_BUILD

#include "Codegen.h"
#include "SelfInvocation.h"
#include "ast/base/GlobalInterpretScope.h"
#include "std/except.h"
#include "utils/CmdUtils2.h"
#include <utility>
#include "llvmimpl.h"
#include <string>
#include <system_error>
#include "lld/Common/Driver.h"
#include "lld/Common/ErrorHandler.h"
#include <cstdlib>
#include <optional>
#include <llvm/TargetParser/Host.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/LLVMDriver.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/OptBisect.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/InitializePasses.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Object/Archive.h>
#include <llvm/Object/ArchiveWriter.h>
#include <llvm/Object/COFF.h>
#include <llvm/Object/COFFImportFile.h>
#include <llvm/Object/COFFModuleDefinition.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/TimeProfiler.h>
#include <llvm/Support/Timer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/CodeGenCWrappers.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Instrumentation/AddressSanitizer.h>
#include <llvm/Transforms/Instrumentation/DataFlowSanitizer.h>
#include <llvm/Transforms/Instrumentation/HWAddressSanitizer.h>
#include <llvm/Transforms/Instrumentation/MemorySanitizer.h>
#include <llvm/Transforms/Instrumentation/ThreadSanitizer.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Utils/AddDiscriminators.h>
#include <llvm/Transforms/Utils/CanonicalizeAliases.h>
#include <llvm/Transforms/Utils/NameAnonGlobals.h>
#include <llvm-c/Core.h>
#include "rang.hpp"
#include "compiler/mangler/NameMangler.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "InvokeUtils.h"

Codegen::Codegen(
        CodegenOptions& options,
        CompilerBinder& binder,
        GlobalInterpretScope& comptime_scope,
        NameMangler& mangler,
        CoreNodes& coreNodes,
        ImplementationsIndex& implsIndex,
        std::string target_triple,
        std::string curr_exe_path,
        bool debug_info,
        ASTAllocator& allocator
) : ASTDiagnoser(comptime_scope.loc_man), binder(binder), options(options), comptime_scope(comptime_scope), allocator(allocator),
    target_triple(std::move(target_triple)),
    di(nullptr, *this, debug_info), mode(comptime_scope.mode),
    mangler(mangler), coreNodes(coreNodes), implsIndex(implsIndex)
{
    // create llvm context
    ctx = std::make_unique<llvm::LLVMContext>();
    // creating a new ir builder
    builder = new llvm::IRBuilder<>(*ctx);
}




bool Codegen::is_arch_64bit(const std::string_view& target_triple) {
    // Parse the target triple string. Normalize so short triples without a
    // vendor (e.g. "aarch64-linux-musl") are parsed the same as 4-part ones.
    auto llvmRef = llvm::StringRef(target_triple);
    llvm::Triple triple(llvm::Triple::normalize(llvmRef));
    return triple.isArch64Bit();
}

void Codegen::module_init(const chem::string_view& scope_name, const chem::string_view& module_name) {
    // final module name = 'scope_name' + '-' + 'module_name'
    std::string final_module_name;
    if(!scope_name.empty()) {
        final_module_name.append(1, '-');
        final_module_name.append(scope_name.view());
    }
    final_module_name.append(module_name.view());

    // creating the modules
    module = std::make_unique<llvm::Module>(final_module_name, *ctx);

    // set the data layout and target triple
    module->setDataLayout(TargetMachine->createDataLayout());
    module->setTargetTriple(llvm::Triple(llvm::Triple::normalize(target_triple)));

    // verify DataLayout has address space 0
    {
        auto& DL = module->getDataLayout();
        (void)DL.getPointerSizeInBits(0);
    }

    // Clear the IRBuilder insert point so it doesn't reference a destroyed module
    builder->ClearInsertionPoint();

    // debug flags must be added otherwise debug information is ignored (or dropped)
    if(di.isEnabled) {
        module->setModuleFlag(
                llvm::Module::ModFlagBehavior::Error,
                "Debug Info Version",
                llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx), llvm::DEBUG_METADATA_VERSION))
        );
        module->setModuleFlag(
                llvm::Module::ModFlagBehavior::Error,
                "CodeView",
                llvm::ConstantAsMetadata::get(llvm::ConstantInt::getBool(*ctx, true))
        );
        module->setModuleFlag(
                llvm::Module::ModFlagBehavior::Warning,
                "Dwarf Version",
                llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx), 4))
        );
    }

    diBuilder = std::make_unique<llvm::DIBuilder>(*module, true);
    di.update_builder(diBuilder.get());

}


void Codegen::print_to_console() {
    module->print(llvm::outs(), nullptr, false, true);
}


Codegen::~Codegen() {
    delete builder;
}

using namespace llvm;
using namespace llvm::sys;

// Configures TargetOptions for a debug build.
// This function enables as much debug-friendly code generation as possible.
// Note: Some options (like disabling fast/global ISel) may increase compile times,
// so they are provided as commented-out alternatives.
void configureDebugTargetOptions(TargetOptions &Opts) {

    // Emit DWARF debug frame section for unwind tables.
    Opts.ForceDwarfFrameSection = 1;

    // --- Debug Entry and Variable Information ---
    // Force production of debug entry values so that variable locations
    // are tracked accurately even if the target doesn't officially support it.
    Opts.EnableDebugEntryValues = 1;

    // Enable experimental variable location tracking for more precise debug info.
    // (May increase compile time slightly.)
    Opts.ValueTrackingVariableLocations = 1;

    // --- Optimization Adjustments ---
    // Tail call optimizations can interfere with proper call-stack reconstruction.
    // Disable guaranteed tail call optimization in debug builds.
    Opts.GuaranteedTailCallOpt = 0;

}


TargetMachine* Codegen::setup_for_target(const std::string &TargetTriple, bool isDebug) {

    // Initialize the target registry etc.
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    std::string Error = "unknown error related to target lookup";
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
        std::cerr << rang::fg::red << "error with target lookup: " << Error << rang::fg::reset << std::endl;
        return nullptr;
    }

    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;

    if(isDebug) {
        configureDebugTargetOptions(opt);
    }

    auto RM = std::optional<Reloc::Model>();

    // we use by default PIE for linux, since that's what the linker expects
    if (!options.no_pie && TargetTriple.find("linux") != std::string::npos) {
        RM = llvm::Reloc::PIC_;
    }

    auto TheTargetMachine = Target->createTargetMachine(llvm::Triple(llvm::Triple::normalize(TargetTriple)), CPU, Features, opt, RM);
    TargetMachine = TheTargetMachine;

    return TheTargetMachine;

}

// LLVM's time profiler can provide a hierarchy view of the time spent
// in each component. It generates JSON report in Chrome's "Trace Event"
// format. So the report can be easily visualized by the Chrome browser.
struct TimeTracerRAII {
    // Granularity in ms
    unsigned TimeTraceGranularity;
    StringRef TimeTraceFile, OutputFilename;
    bool EnableTimeTrace;

    TimeTracerRAII(StringRef ProgramName, StringRef OF)
            : TimeTraceGranularity(500U),
              TimeTraceFile(std::getenv("ZIG_LLVM_TIME_TRACE_FILE")),
              OutputFilename(OF),
              EnableTimeTrace(!TimeTraceFile.empty()) {
        if (EnableTimeTrace) {
            if (const char *G = std::getenv("ZIG_LLVM_TIME_TRACE_GRANULARITY"))
                TimeTraceGranularity = (unsigned)std::atoi(G);

            llvm::timeTraceProfilerInitialize(TimeTraceGranularity, ProgramName);
        }
    }

    ~TimeTracerRAII() {
        if (EnableTimeTrace) {
            if (auto E = llvm::timeTraceProfilerWrite(TimeTraceFile, OutputFilename)) {
                handleAllErrors(std::move(E), [&](const StringError &SE) {
                    errs() << SE.getMessage() << "\n";
                });
                return;
            }
            timeTraceProfilerCleanup();
        }
    }
};

bool save_as_file_type(
        Codegen* gen,
        CodegenEmitterOptions* options,
        char** error_message
) {

    auto& target_machine = *gen->TargetMachine;
    auto& llvm_module = *gen->module;

    raw_fd_ostream *dest_asm_ptr = nullptr;
    raw_fd_ostream *dest_obj_ptr = nullptr;
    raw_fd_ostream *dest_bitcode_ptr = nullptr;

    if (options->asm_path) {
        std::error_code EC;
        dest_asm_ptr = new(std::nothrow) raw_fd_ostream(options->asm_path, EC, sys::fs::OF_None);
        if (EC) {
            *error_message = strdup((const char *)StringRef(EC.message()).bytes_begin());
            return true;
        }
    }
    if (options->obj_path) {
        std::error_code EC;
        dest_obj_ptr = new(std::nothrow) raw_fd_ostream(options->obj_path, EC, sys::fs::OF_None);
        if (EC) {
            *error_message = strdup((const char *)StringRef(EC.message()).bytes_begin());
            return true;
        }
    }
    if (options->bitcode_path) {
        std::error_code EC;
        dest_bitcode_ptr = new(std::nothrow) raw_fd_ostream(options->bitcode_path, EC, sys::fs::OF_None);
        if (EC) {
            *error_message = strdup((const char *)StringRef(EC.message()).bytes_begin());
            return true;
        }
    }

    std::unique_ptr<raw_fd_ostream> dest_asm(dest_asm_ptr),
            dest_obj(dest_obj_ptr),
            dest_bitcode(dest_bitcode_ptr);

    auto PID = sys::Process::getProcessId();
    std::string ProcName = "chem-";
    ProcName += std::to_string(PID);
    TimeTracerRAII TimeTracer(ProcName, options->bitcode_path ? options->bitcode_path : options->obj_path);
    target_machine.setO0WantsFastISel(true);

    // Pipeline configurations
    PipelineTuningOptions pipeline_opts;
    pipeline_opts.LoopUnrolling = !options->is_debug;
    pipeline_opts.SLPVectorization = !options->is_debug;
    pipeline_opts.LoopVectorization = !options->is_debug;
    pipeline_opts.LoopInterleaving = !options->is_debug;
    pipeline_opts.MergeFunctions = !options->is_debug;

    // Instrumentations
    PassInstrumentationCallbacks instr_callbacks;
    StandardInstrumentations std_instrumentations(llvm_module.getContext(), false);
    std_instrumentations.registerCallbacks(instr_callbacks);

    std::optional<PGOOptions> opt_pgo_options = {};
    PassBuilder pass_builder(&target_machine, pipeline_opts,
                             opt_pgo_options, &instr_callbacks);


    LoopAnalysisManager loop_am;
    FunctionAnalysisManager function_am;
    CGSCCAnalysisManager cgscc_am;
    ModuleAnalysisManager module_am;

    // Register the AA manager first so that our version is the one used
    function_am.registerPass([&] {
        return pass_builder.buildDefaultAAPipeline();
    });

    Triple target_triple(llvm_module.getTargetTriple());
    auto tlii = std::make_unique<TargetLibraryInfoImpl>(target_triple);
    function_am.registerPass([&] { return TargetLibraryAnalysis(*tlii); });

    // Initialize the AnalysisManagers
    pass_builder.registerModuleAnalyses(module_am);
    pass_builder.registerCGSCCAnalyses(cgscc_am);
    pass_builder.registerFunctionAnalyses(function_am);
    pass_builder.registerLoopAnalyses(loop_am);
    pass_builder.crossRegisterProxies(loop_am, function_am,
                                      cgscc_am, module_am);

    // IR verification
    if (options->assertions_on) {
        // Verify the input
        pass_builder.registerPipelineStartEPCallback(
                [](ModulePassManager &module_pm, OptimizationLevel OL) {
                    module_pm.addPass(VerifierPass());
                });
        // Verify the output
        pass_builder.registerOptimizerLastEPCallback(
                [](ModulePassManager &module_pm, OptimizationLevel OL, ThinOrFullLTOPhase ltoPhase) {
                    module_pm.addPass(VerifierPass());
                });
    }

    // Passes specific for release build
    if (!options->is_debug) {
        pass_builder.registerPipelineStartEPCallback(
                [](ModulePassManager &module_pm, OptimizationLevel OL) {
                    module_pm.addPass(
                            createModuleToFunctionPassAdaptor(AddDiscriminatorsPass()));
                });
    }

    // Sanitizers
    if (has_sanitizer(options->sanitizers, SanitizerType::Thread)) {
        pass_builder.registerOptimizerLastEPCallback([](ModulePassManager &module_pm, OptimizationLevel level, ThinOrFullLTOPhase ltoPhase) {
            module_pm.addPass(ModuleThreadSanitizerPass());
            module_pm.addPass(createModuleToFunctionPassAdaptor(ThreadSanitizerPass()));
        });
    }
    if (has_sanitizer(options->sanitizers, SanitizerType::Address)) {
        pass_builder.registerOptimizerLastEPCallback([](ModulePassManager &module_pm, OptimizationLevel level, ThinOrFullLTOPhase ltoPhase) {
            AddressSanitizerOptions asan_opts;
            module_pm.addPass(AddressSanitizerPass(asan_opts));
        });
    }
    if (has_sanitizer(options->sanitizers, SanitizerType::Memory)) {
        pass_builder.registerOptimizerLastEPCallback([](ModulePassManager &module_pm, OptimizationLevel level, ThinOrFullLTOPhase ltoPhase) {
            MemorySanitizerOptions msan_opts;
            module_pm.addPass(MemorySanitizerPass(msan_opts));
        });
    }
    if (has_sanitizer(options->sanitizers, SanitizerType::HWAddress)) {
        pass_builder.registerOptimizerLastEPCallback([](ModulePassManager &module_pm, OptimizationLevel level, ThinOrFullLTOPhase ltoPhase) {
            HWAddressSanitizerOptions hwasan_opts;
            module_pm.addPass(HWAddressSanitizerPass(hwasan_opts));
        });
    }
    if (has_sanitizer(options->sanitizers, SanitizerType::DataFlow)) {
        pass_builder.registerOptimizerLastEPCallback([](ModulePassManager &module_pm, OptimizationLevel level, ThinOrFullLTOPhase ltoPhase) {
            std::vector<std::string> abi_files;
            module_pm.addPass(DataFlowSanitizerPass(abi_files));
        });
    }

    ModulePassManager module_pm;
    OptimizationLevel opt_level;
    // Setting up the optimization level
    if (options->is_debug)
        opt_level = OptimizationLevel::O0;
    else if (options->is_small)
        opt_level = OptimizationLevel::Oz;
    else
        opt_level = OptimizationLevel::O3;

    // Initialize the PassManager
    if (opt_level == OptimizationLevel::O0) {
        // TODO: allow controlling post or pre link lto from command line
        module_pm = pass_builder.buildO0DefaultPipeline(opt_level, options->lto ? llvm::ThinOrFullLTOPhase::FullLTOPostLink : llvm::ThinOrFullLTOPhase::None);
    } else if (options->lto) {
        module_pm = pass_builder.buildLTOPreLinkDefaultPipeline(opt_level);
    } else {
        module_pm = pass_builder.buildPerModuleDefaultPipeline(opt_level);
    }

    // Unfortunately we don't have new PM for code generation
    legacy::PassManager codegen_pm;
    codegen_pm.add(
            createTargetTransformInfoWrapperPass(target_machine.getTargetIRAnalysis()));

    if (dest_obj) {
        if (target_machine.addPassesToEmitFile(codegen_pm, *dest_obj, nullptr, CodeGenFileType::ObjectFile)) {
            *error_message = strdup("TargetMachine can't emit an object file");
            return true;
        }
    }
    if (dest_asm) {
        if (target_machine.addPassesToEmitFile(codegen_pm, *dest_asm, nullptr, CodeGenFileType::AssemblyFile)) {
            *error_message = strdup("TargetMachine can't emit an assembly file");
            return true;
        }
    }

    // Optimization phase
    module_pm.run(llvm_module, module_am);

    // Code generation phase
    codegen_pm.run(llvm_module);

    if (options->ir_path) {
        char* message = nullptr;
        if (LLVMPrintModuleToFile(wrap(&llvm_module), options->ir_path, &message)) {
            return false;
        }
    }
    if (options->bitcode_path) {
        WriteBitcodeToFile(llvm_module, *dest_bitcode_ptr);
    }

    if (options->time_report) {
        TimerGroup::printAll(errs());
    }

    return true;

}

void configure_emitter_opts(OutputMode mode, CodegenEmitterOptions* options) {
    switch(mode) {
        case OutputMode::Debug:
        case OutputMode::DebugQuick:
            options->is_debug = true;
            options->lto = false;
            options->is_small = false;
            return;
        case OutputMode::DebugComplete:
            options->is_debug = true;
            options->lto = false;
            options->is_small = false;
            options->assertions_on = true;
            return;
        case OutputMode::ReleaseFast:
            options->is_debug = false;
            options->lto = true;
            options->is_small = false;
            return;
        case OutputMode::ReleaseSmall:
            options->is_debug = false;
            options->lto = true;
            options->is_small = true;
            return;
#ifdef DEBUG
        default:
            CHEM_THROW_RUNTIME("[lab] unknown output mode");
#endif
    }
}

bool Codegen::save_with_options(CodegenEmitterOptions* options) {
    char* error_message = nullptr;
    bool result = save_as_file_type(this, options, &error_message);
    if(error_message) {
        std::cerr << rang::fg::red << "error: " << error_message << ", when emitting files" << rang::fg::reset << std::endl;
    }
    return result;
}

bool Codegen::save_to_assembly_file(std::string &out_path, OutputMode mode) {
    CodegenEmitterOptions options;
    configure_emitter_opts(mode, &options);
    options.asm_path = out_path.data();
    return save_with_options(&options);
}

bool Codegen::save_to_object_file(std::string &out_path, OutputMode mode) {
    CodegenEmitterOptions options;
    configure_emitter_opts(mode, &options);
    options.obj_path = out_path.data();
    return save_with_options(&options);
}

bool Codegen::save_to_bc_file(std::string &out_path, OutputMode mode) {
    CodegenEmitterOptions options;
    configure_emitter_opts(mode, &options);
    options.bitcode_path = out_path.data();
    return save_with_options(&options);
}

bool Codegen::save_to_ll_file_for_debugging(const std::string &out_path) const {
    // This code allows printing llvm ir, even if it's buggy code
    std::error_code errorCode;
    llvm::raw_fd_ostream outLL(out_path, errorCode);
    module->print(outLL, nullptr, false, true);
    outLL.close();
    return true;
}

#ifdef CLANG_LIBS

extern "C" int chemical_clang_main(int argc, char **argv);

extern "C" int ChemLlvmAr_main(int, char **);

int chemical_clang_main2(std::vector<chem::string> &command_args) {
    char** pointers = chem_string_cmd_pointers(command_args);
    // invocation
    auto result = chemical_clang_main(command_args.size(), pointers);
    free_chem_string_cmd_pointers(pointers, command_args.size());
    return result;
}

int llvm_ar_main2(const std::span<chem::string> &command_args) {
    char** pointers = chem_string_cmd_pointers(command_args);
    // invocation
    auto result = ChemLlvmAr_main(command_args.size(), pointers);
    free_chem_string_cmd_pointers(pointers, command_args.size());
    return result;
}

#endif

#ifdef LLD_LIBS

using namespace lld;
using namespace llvm;
using namespace llvm::sys;

LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(elf)
LLD_HAS_DRIVER(mingw)
LLD_HAS_DRIVER(macho)
LLD_HAS_DRIVER(wasm)

int lld_main(int argc, char **argv, const llvm::ToolContext &) {

    sys::Process::UseANSIEscapeCodes(true);

    ArrayRef<const char *> args(argv, argv + argc);

    auto result = lld::lldMain(args, llvm::outs(), llvm::errs(), LLD_ALL_DRIVERS);

    return result.retCode;

}

int invoke_lld(std::vector<chem::string>& command_args, const std::string_view& targetTripleString) {
    // figure out the lld driver
    chem::string lld_driver;
    auto tripleRef = llvm::StringRef(targetTripleString);
    auto triple = llvm::Triple(llvm::Triple::normalize(tripleRef));
    if (triple.isOSDarwin()){
        lld_driver.append(std::string_view("ld64.lld"));
    } else if (triple.isOSWindows()) {
        lld_driver.append(std::string_view("lld-link"));
    } else {
        lld_driver.append(std::string_view("ld.lld"));
    }
    auto pointer = chem_string_cmd_pointers(lld_driver, command_args);
    // invocation
    ToolContext context{};
    const auto command_size = command_args.size() + 1;
    const auto status = lld_main(command_size, pointer, context);
    free_chem_string_cmd_pointers(pointer, command_size);
    return status;
}

#endif

#endif

int lld_link_objects(
        std::vector<chem::string>& linkables,
        const std::string_view& bin_out,
        const std::string_view& comp_exe_path, // our compiler's executable path, needed for self invocation
        const std::vector<chem::string>& link_libs,
        std::vector<chem::string>& lib_search_paths,
        const std::string_view& target_triple,
        LinkFlags& flags
) {

    // creating lld command
    std::vector<chem::string> command;

    // put the linkables into the command
    for(auto& linkable : linkables) {
        command.emplace_back(linkable.to_view());
    }

    // set output
#if defined(_WIN32)
    command.emplace_back("/OUT:");
    command.back().append(bin_out);
#else
    command.emplace_back(chem::string("-o"));
    command.emplace_back(chem::string("./"));
    command.back().append(bin_out);
#endif

    // add library search paths
#ifdef _WIN32
    for (const auto& path : lib_search_paths) {
        command.emplace_back("/LIBPATH:");
        command.back().append(path.data(), path.size());
    }
#else
    for (const auto& path : lib_search_paths) {
        command.emplace_back("-L");
        command.back().append(path.data(), path.size());
    }
#endif

#ifdef _WIN32
    // add libraries user asked us to link
    for(const auto& lib : link_libs) {
        command.emplace_back("/DEFAULTLIB:");
        command.back().append(lib.data(), lib.size());
    }
#else
    // add libraries user asked us to link
    for(const auto& lib : link_libs) {
        command.emplace_back("-l");
        command.back().append(lib.data(), lib.size());
    }
#endif

    if(flags.no_pie) {
#if defined(_WIN32)
        command.emplace_back("/FIXED");
#elif defined(__APPLE__)
        command.emplace_back("-no_pie");
#elif defined(__linux__)
        command.emplace_back("-no-pie");
#endif
    }

    if(flags.debug_info) {
#if defined(_WIN32)
        command.emplace_back("/DEBUG");
#elif defined(__APPLE__)
        command.emplace_back("-g");
#elif defined(__linux__)
        command.emplace_back("-g");
#endif
    }

    if(flags.verbose) {
#if defined(_WIN32)
        command.emplace_back("/VERBOSE");
#elif defined(__APPLE__)
        command.emplace_back("-v");
#elif defined(__linux__)
        command.emplace_back("-v");
#endif
    }

    // Add sanitizer linker flags
    if (has_sanitizer(flags.sanitizers, SanitizerType::Thread)) {
#if defined(_WIN32)
        command.emplace_back("-ltsan");
#elif defined(__APPLE__)
        command.emplace_back("-ltsan");
#elif defined(__linux__)
        command.emplace_back("-ltsan");
#endif
    }
    if (has_sanitizer(flags.sanitizers, SanitizerType::Address)) {
        command.emplace_back("-lasan");
    }
    if (has_sanitizer(flags.sanitizers, SanitizerType::Memory)) {
        command.emplace_back("-lmsan");
    }
    if (has_sanitizer(flags.sanitizers, SanitizerType::Leak)) {
        command.emplace_back("-llsan");
    }
    if (has_sanitizer(flags.sanitizers, SanitizerType::UndefinedBehavior)) {
        command.emplace_back("-lubsan");
    }

#if defined(__APPLE__)
    command.emplace_back("-rpath");
    command.emplace_back("@executable_path");
#elif defined(__linux__)
    command.emplace_back("-rpath");
    command.emplace_back("$ORIGIN");
    command.emplace_back("--enable-new-dtags"); // Use RUNPATH
#endif

    // invoke lld to create executable
    return invoke_lld(command, target_triple);

}

int clang_link_objects(
        std::vector<chem::string>& linkables,
        const std::string_view& bin_out,
        const std::string_view& comp_exe_path, // our compiler's executable path, needed for self invocation
        const std::vector<chem::string>& link_libs,
        std::vector<chem::string>& lib_search_paths,
        const std::string_view& target_triple,
        LinkFlags& flags,
        const std::string_view& resource_dir
) {
    std::vector<chem::string> clang_flags{chem::string(comp_exe_path)};

    // set the target triple
    if (!target_triple.empty()) {
        clang_flags.emplace_back("-target");
        clang_flags.emplace_back(target_triple);
    }

    // handle the resource directory
    clang_flags.emplace_back("-resource-dir=");
    auto& resource_dir_arg = clang_flags.back();
    resource_dir_arg.append(resource_dir);

    if(flags.debug_info) {
        // on windows codeview is being used as .pdb and .ilk are being generated which aren't supported by gdb
        // we can use -gdward-4 which is supported
        // flags.emplace_back("-gdwarf-4");
        clang_flags.emplace_back("-g");
    }
    if(flags.no_pie) {
        clang_flags.emplace_back("-no-pie");
    }
    if(flags.verbose) {
        clang_flags.emplace_back("-v");
    }
    // Add combined sanitizer flag for clang linking
    if (flags.sanitizers != SanitizerType::None) {
        std::string sanitize_flag = "-fsanitize=";
        bool first = true;
        auto append_sanitizer = [&](SanitizerType type, const char* name) {
            if (has_sanitizer(flags.sanitizers, type)) {
                if (!first) sanitize_flag += ",";
                sanitize_flag += name;
                first = false;
            }
        };
        append_sanitizer(SanitizerType::Thread, "thread");
        append_sanitizer(SanitizerType::Address, "address");
        append_sanitizer(SanitizerType::Memory, "memory");
        append_sanitizer(SanitizerType::UndefinedBehavior, "undefined");
        append_sanitizer(SanitizerType::Leak, "leak");
        append_sanitizer(SanitizerType::HWAddress, "hwaddress");
        append_sanitizer(SanitizerType::DataFlow, "dataflow");
        if (!first) {
            clang_flags.emplace_back(sanitize_flag);
        }
    }

    for(auto& linkable : linkables) {
        clang_flags.emplace_back(linkable.to_view());
    }

    // add library search paths
    for (const auto& path : lib_search_paths) {
        clang_flags.emplace_back("-L");
        auto& str = clang_flags.back();
        str.append(path.to_view());
    }

    // add libraries user asked us to link
    for(const auto& lib : link_libs) {
        clang_flags.emplace_back("-l");
        auto& str = clang_flags.back();
        str.append(lib.to_view());
    }

#if defined(_WIN32)
    if(bin_out.ends_with(".dll")) {
        clang_flags.emplace_back("-shared");
    }
#elif defined(__APPLE__)
    if(bin_out.ends_with(".dylib")) {
        clang_flags.emplace_back("-shared");
    }
#elif defined(__linux__)
    if(bin_out.ends_with(".so")) {
        clang_flags.emplace_back("-shared");
    }
#endif

    clang_flags.emplace_back("-o");
    clang_flags.emplace_back(bin_out);

#if defined(__APPLE__)
    clang_flags.emplace_back("-Wl,-rpath,@executable_path");
#elif defined(__linux__)
    clang_flags.emplace_back("-Wl,-rpath,$ORIGIN");
    clang_flags.emplace_back("-Wl,--enable-new-dtags");
#endif

    return chemical_clang_main2(clang_flags);
}

int compile_c_file_to_object(
        const std::string_view& c_file,
        const std::string_view& out_file,
        const std::string_view& comp_exe_path,
        const std::string_view& resource_dir,
        const std::string_view& target_triple,
        const std::vector<chem::string>& include_dirs,
        bool debug_info
) {
    std::vector<chem::string> clang_flags{ chem::string(comp_exe_path) };

    // set the target triple
    if (!target_triple.empty()) {
        clang_flags.emplace_back("-target");
        clang_flags.emplace_back(target_triple);
    }

    // resource dir
    clang_flags.emplace_back("-resource-dir=");
    auto& resource_dir_arg = clang_flags.back();
    resource_dir_arg.append(resource_dir);

    // include directories
    for (const auto& path : include_dirs) {
        clang_flags.emplace_back("-I");
        clang_flags.emplace_back(path.to_view());
    }

    // debug info
    if(debug_info) {
        clang_flags.emplace_back("-g");
    }

    clang_flags.emplace_back("-c");
    clang_flags.emplace_back(c_file);
    clang_flags.emplace_back("-o");
    clang_flags.emplace_back(out_file);

    return chemical_clang_main2(clang_flags);
}