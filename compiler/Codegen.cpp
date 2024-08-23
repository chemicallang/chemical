// Copyright (c) Qinetik 2024.

#ifdef COMPILER_BUILD

#include "ast/base/ASTNode.h"
#include "Codegen.h"
#include "ast/structures/Scope.h"
#include "SelfInvocation.h"
#include <utility>
#include "llvmimpl.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/structures/StructDefinition.h"
#include <string>
#include <system_error>
#include "lld/Common/Driver.h"
#include "lld/Common/ErrorHandler.h"
#include "ast/utils/ExpressionEvaluator.h"
#include "ast/types/IntNType.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/values/NullValue.h"
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
#include <llvm/Transforms/Instrumentation/ThreadSanitizer.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Utils/AddDiscriminators.h>
#include <llvm/Transforms/Utils/CanonicalizeAliases.h>
#include <llvm/Transforms/Utils/NameAnonGlobals.h>
#include <llvm-c/Core.h>

Codegen::Codegen(
        std::vector<std::unique_ptr<ASTNode>> nodes,
        std::string target_triple,
        std::string curr_exe_path,
        bool is_64_bit,
        const std::string& module_name
) : ASTDiagnoser(), comptime_scope(), nodes(std::move(nodes)),
    target_triple(std::move(target_triple)), is64Bit(is_64_bit) {
    // create llvm context
    ctx = std::make_unique<llvm::LLVMContext>();
    // creating a new ir builder
    builder = new llvm::IRBuilder<>(*ctx);
    if(!module_name.empty()) {
        module_init(module_name);
    }
}

bool Codegen::is_arch_64bit(const std::string& target_triple) {
    // Parse the target triple string
    llvm::Triple triple(target_triple);
    // Extract architecture information
    llvm::Triple::ArchType archType = triple.getArch();
    // Check if it's a 32-bit or 64-bit architecture
    return archType == llvm::Triple::ArchType::x86_64 ||
              archType == llvm::Triple::ArchType::ppc64 ||
              archType == llvm::Triple::ArchType::aarch64 ||
              archType == llvm::Triple::ArchType::mips64 ||
              archType == llvm::Triple::ArchType::sparcv9;
}

void Codegen::module_init(const std::string& module_name) {
    module = std::make_unique<llvm::Module>(module_name, *ctx);
}

void Codegen::compile() {
    compile_begin();
    compile_nodes();
    compile_end();
}

void Codegen::compile_begin() {
    // no implementation yet
}

void Codegen::compile_nodes() {
    compile_nodes(nodes);
}

void Codegen::compile_end() {
    // no implementation yet
}

void Codegen::createFunctionBlock(llvm::Function *fn) {
    auto entry = createBB("entry", fn);
    SetInsertPoint(entry);
}

void Codegen::end_function_block() {
    if (!has_current_block_ended) {
        builder->CreateRetVoid();
        has_current_block_ended = true;
    }
}

llvm::Function *Codegen::create_function(const std::string &name, llvm::FunctionType *type, AccessSpecifier specifier) {
    current_function = create_function_proto(name, type, specifier);
    createFunctionBlock(current_function);
    return current_function;
}

llvm::Function *Codegen::create_nested_function(const std::string &name, llvm::FunctionType *type, FunctionType* func_type, Scope &scope) {

    auto prev_destruct_nodes = std::move(destruct_nodes);
    auto prev_destroy_scope = destroy_current_scope;
    auto prev_block_ended = has_current_block_ended;
    auto prev_block = builder->GetInsertBlock();
    auto prev_current_func = current_function;

    destroy_current_scope = true;
    SetInsertPoint(nullptr);
    auto nested_function = create_function_proto(name, type, AccessSpecifier::Private);
    current_function = nested_function;
    const auto destruct_begin = destruct_nodes.size();
    func_type->queue_destruct_params(*this);
    createFunctionBlock(nested_function);
    scope.code_gen(*this, destruct_begin);
    end_function_block();

    has_current_block_ended = prev_block_ended;
    SetInsertPoint(prev_block);
    current_function = prev_current_func;
    destruct_nodes = std::move(prev_destruct_nodes);
    destroy_current_scope = prev_destroy_scope;

    return nested_function;

}

llvm::FunctionCallee Codegen::declare_function(const std::string &name, llvm::FunctionType *type) {
    return module->getOrInsertFunction(name, type);
}

llvm::Function *
Codegen::create_function_proto(const std::string &name, llvm::FunctionType *type, AccessSpecifier specifier) {
    llvm::Function::LinkageTypes linkage;
    switch (specifier) {
        case AccessSpecifier::Private:
            linkage = llvm::Function::PrivateLinkage;
            break;
        case AccessSpecifier::Public:
            linkage = llvm::Function::ExternalLinkage;
            break;
        case AccessSpecifier::Internal:
            linkage = llvm::Function::InternalLinkage;
            break;
    }
    auto fn = llvm::Function::Create(type, linkage, name, *module);
    fn->setDSOLocal(true);
    return fn;
}

std::unique_ptr<Value>& Codegen::eval_comptime(FunctionCall* call, FunctionDeclaration* decl) {
    auto found = evaluated_func_calls.find(call);
    if(found != evaluated_func_calls.end()) {
        return found->second;
    } else {
        auto ret = std::unique_ptr<Value>(decl->call(&comptime_scope, call, nullptr, false));
        if(!ret) {
//            warn("compile time function didn't return a value", decl);
            evaluated_func_calls[call] = std::unique_ptr<Value>(nullptr);
            return evaluated_func_calls[call];
        }
        Value* val;
        auto eval = ret->create_evaluated_value(comptime_scope);
        if(eval) {
            val = eval.release();
        } else {
            val = ret.release();
        }
        evaluated_func_calls[call] = std::unique_ptr<Value>(val);
        return evaluated_func_calls[call];
    }
}

void Codegen::destruct(
        llvm::Value* allocaInst,
        llvm::Value* array_size,
        llvm::Type* elem_type,
        bool check_for_null,
        const std::function<void(llvm::Value*)>& call_destructor
) {

    auto& gen = *this;
    // the pointer to the first element in array
    auto firstEle = allocaInst;
    // the pointer to last element in array
    auto lastEle = builder->CreateGEP(elem_type, firstEle, { array_size }, "", inbounds);
    // the current block in which we are
    auto current_block = builder->GetInsertBlock();
    // the body block contains the loop that calls the destructors
    auto body_block = llvm::BasicBlock::Create(*ctx, "", current_function);
    // the end block is where all destructors have been called, user's code is written...
    auto end_block = llvm::BasicBlock::Create(*ctx, "", current_function);

    if(check_for_null) {
        // check if given pointer is null and send user to end block if it is
        CheckNullCondBr(allocaInst, end_block, body_block);
    } else {
        // sending directly to body block (no null check)
        CreateBr(body_block);
    };

    // generating the code for the body
    SetInsertPoint(body_block);
    auto PHI = builder->CreatePHI(builder->getPtrTy(), 2);
    auto structPtr = builder->CreateGEP(elem_type, PHI, { builder->getInt32(-1) }, "", inbounds);
    // if coming from the current block, last element pointer is taken (the ptr to last element in array)
    PHI->addIncoming(lastEle, current_block);
    // if coming from the body block (next iteration), we take -1 of the previous array (decrementing pointer)
    PHI->addIncoming(structPtr, body_block);

    // calling the destructor
    call_destructor(structPtr);

    // we just called destructor on struct pointer, if it's equal to first element, then we are done
    auto result = builder->CreateICmpEQ(structPtr, firstEle);
    CreateCondBr(result, end_block, body_block);

    // setting end block as insert point so further user code can go into it
    SetInsertPoint(end_block);
}

void Codegen::destruct(
        llvm::Value* allocaInst,
        llvm::FunctionType* destructor_func_type,
        llvm::Value* destructor_func_callee,
        bool pass_self,
        llvm::Value* array_size,
        BaseType* elem_type,
        bool check_for_null,
        const std::function<void(llvm::Value*)>& after_destruct
) {
    destruct(
        allocaInst,
        array_size,
        elem_type->llvm_type(*this),
        check_for_null,
        [&](llvm::Value* struct_pointer) -> void {
            // calling the destructor
            std::vector<llvm::Value*> args;
            if(pass_self) {
                args.emplace_back(struct_pointer);
            }
            builder->CreateCall(destructor_func_type, destructor_func_callee, args, "");
            after_destruct(struct_pointer);
        }
    );
}

FunctionDeclaration* Codegen::determine_destructor_for(
        BaseType* elem_type,
        llvm::FunctionType*& func_type,
        llvm::Value*& func_callee
) {

    if(!elem_type->linked_node() || !elem_type->linked_node()->as_members_container()) {
        return nullptr;
    }
    //checking if struct has a destructor function
    auto structDef = elem_type->linked_node()->as_members_container();
    auto destructorFunc = structDef->destructor_func();
    if(!destructorFunc) {
        return nullptr;
    }

    // determine the function type and callee
    if(!structDef->generic_params.empty()) {
        const auto llvm_data = structDef->llvm_generic_func_data(destructorFunc, structDef->active_iteration, destructorFunc->active_iteration);
        func_type = llvm_data.second;
        func_callee = llvm_data.first;
    } else {
        func_type = destructorFunc->llvm_func_type(*this);
        func_callee = destructorFunc->llvm_pointer(*this);
    }

    return destructorFunc;

}

void Codegen::destruct(
        llvm::Value* allocaInst,
        llvm::Value* array_size,
        BaseType* elem_type,
        bool check_for_null,
        const std::function<void(llvm::Value*)>& after_destruct
) {
    // determining destructor
    llvm::FunctionType* func_type;
    llvm::Value* func_callee;
    auto destructorFunc = determine_destructor_for(elem_type, func_type, func_callee);
    if(!destructorFunc) return;
    // calling destruct
    destruct(
            allocaInst,
            func_type,
            func_callee,
            destructorFunc->has_self_param(),
            array_size,
            elem_type,
            check_for_null,
            after_destruct
    );

}

void Codegen::destruct(
        llvm::Value* allocaInst,
        unsigned int array_size,
        BaseType* elem_type,
        const std::function<void(llvm::Value*)>& after_destruct
) {
    destruct(allocaInst, builder->getInt32(array_size), elem_type, false, after_destruct);
}

llvm::BasicBlock *Codegen::createBB(const std::string &name, llvm::Function *fn) {
    return llvm::BasicBlock::Create(*ctx, name, fn);
}

llvm::StructType* Codegen::fat_pointer_type() {
    return llvm::StructType::get(*ctx, {builder->getPtrTy(), builder->getPtrTy()} );
}

llvm::AllocaInst* Codegen::pack_fat_pointer(llvm::Value* first_ptr, llvm::Value* second_ptr) {
    // create a struct with two pointers
    auto structType = fat_pointer_type();
    auto allocated = builder->CreateAlloca(structType);
    // store lambda function pointer in the first variable
    auto first = builder->CreateGEP(structType, allocated, {builder->getInt32(0), builder->getInt32(0)}, "", inbounds);
    builder->CreateStore(first_ptr, first);
    // store a pointer to a struct that contains captured variables in the second variable
    auto second = builder->CreateGEP(structType, allocated, {builder->getInt32(0), builder->getInt32(1)}, "", inbounds);
    builder->CreateStore(second_ptr, second);
    return allocated;
}

llvm::Value* Codegen::get_dyn_obj_impl(Value* value, BaseType* type) {
    if(!type) return nullptr;
    const auto interface = type->linked_dyn_interface();
    if(!interface) return nullptr;
    if(value->value_type() == ValueType::Struct) {
        const auto linked = value->known_type();
        const auto def = linked->linked_struct_def();
        if(def) {
            const auto found = interface->llvm_global_vtable(*this, def);
            if(found != nullptr) {
                return found;
            } else {
                error("couldn't find the implementation of struct '" + def->name + "' using value '" + value->representation() + "' for interface '" + interface->name + "'");
            }
        } else {
#ifdef DEBUG
            throw std::runtime_error("linked node not StructDefinition");
#endif
        }
    }
    return nullptr;
}

llvm::Value* Codegen::pack_dyn_obj(Value* value, BaseType* type, llvm::Value* llvm_value) {
    auto found = get_dyn_obj_impl(value, type);
    if(found) {
        return pack_fat_pointer(llvm_value, found);
    }
    return llvm_value;
}

llvm::Value* Codegen::allocate_dyn_obj_based_on_type(BaseType* type) {
    if(!type) return nullptr;
    const auto interface = type->linked_dyn_interface();
    if(!interface) return nullptr;
    return builder->CreateAlloca(fat_pointer_type());
}

bool Codegen::assign_dyn_obj_impl(Value* value, BaseType* type, llvm::Value* fat_pointer) {
    auto found = get_dyn_obj_impl(value, type);
    if(found) {
        auto second = builder->CreateGEP(fat_pointer_type(), fat_pointer, {builder->getInt32(0), builder->getInt32(1)}, "", inbounds);
        builder->CreateStore(found, second);
        return true;
    }
    return false;
}

bool Codegen::assign_dyn_obj(Value* value, BaseType* type, llvm::Value* fat_pointer, llvm::Value* obj) {
    auto found = get_dyn_obj_impl(value, type);
    if(found) {
        auto first = builder->CreateGEP(fat_pointer_type(), fat_pointer, {builder->getInt32(0), builder->getInt32(0)}, "", inbounds);
        builder->CreateStore(obj, first);
        auto second = builder->CreateGEP(fat_pointer_type(), fat_pointer, {builder->getInt32(0), builder->getInt32(1)}, "", inbounds);
        builder->CreateStore(found, second);
        return true;
    }
    return false;
}

void Codegen::print_to_console() {
    module->print(llvm::outs(), nullptr, false, true);
}

void Codegen::CheckNullCondBr(llvm::Value* value, llvm::BasicBlock* TrueBlock, llvm::BasicBlock* FalseBlock) {
    auto is_null = builder->CreateICmpEQ(value, NullValue::null_llvm_value(*this));
    CreateCondBr(is_null, TrueBlock, FalseBlock);
}

void Codegen::SetInsertPoint(llvm::BasicBlock *block) {
    has_current_block_ended = false;
    builder->SetInsertPoint(block);
}

void Codegen::CreateBr(llvm::BasicBlock *block) {
    if (!has_current_block_ended) {
        builder->CreateBr(block);
        has_current_block_ended = true;
    }
}

void Codegen::CreateRet(llvm::Value *value) {
    if (!has_current_block_ended) {
        builder->CreateRet(value);
        has_current_block_ended = true;
    }
}

void Codegen::DefaultRet() {
    if(redirect_return) {
        CreateBr(redirect_return);
    } else {
        CreateRet(nullptr);
    }
}

void Codegen::CreateCondBr(llvm::Value *Cond, llvm::BasicBlock *True, llvm::BasicBlock *FalseMDNode) {
    if (!has_current_block_ended) {
        builder->CreateCondBr(Cond, True, FalseMDNode);
        has_current_block_ended = true;
    }
}

void Codegen::loop_body_wrap(llvm::BasicBlock *condBlock, llvm::BasicBlock *endBlock) {
    // set current loop exit, so it can be broken
    current_loop_continue = condBlock;
    current_loop_exit = endBlock;
}

llvm::Value *Codegen::implicit_cast(llvm::Value* value, BaseType* from_type, BaseType* to_type) {
    if(from_type->kind() == BaseTypeKind::IntN && to_type->kind() == BaseTypeKind::IntN) {
        auto from_num_type = (IntNType*) from_type;
        auto to_num_type = (IntNType*) to_type;
        if(from_num_type->num_bits() < to_num_type->num_bits()) {
            if (from_num_type->is_unsigned()) {
                return builder->CreateZExt(value, to_num_type->llvm_type(*this));
            } else {
                return builder->CreateSExt(value, to_num_type->llvm_type(*this));
            }
        } else if(from_num_type->num_bits() > to_num_type->num_bits()) {
            return builder->CreateTrunc(value, to_num_type->llvm_type(*this));
        }
    }
    return value;
}

Codegen::~Codegen() {
    delete builder;
}

using namespace llvm;
using namespace llvm::sys;

TargetMachine * Codegen::setup_for_target(const std::string &TargetTriple) {

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
        error(Error);
        return nullptr;
    }

    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;
    auto RM = std::optional<Reloc::Model>();
    auto TheTargetMachine = Target->createTargetMachine(
            TargetTriple, CPU, Features, opt, RM);

    module->setDataLayout(TheTargetMachine->createDataLayout());
    module->setTargetTriple(TargetTriple);

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

    auto TheTargetMachine = gen->setup_for_target();
    if(TheTargetMachine == nullptr) {
        return false;
    }

    auto& target_machine = *TheTargetMachine;
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
    TheTargetMachine->setO0WantsFastISel(true);

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
                [](ModulePassManager &module_pm, OptimizationLevel OL) {
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

    // Thread sanitizer
    if (options->tsan) {
        pass_builder.registerOptimizerLastEPCallback([](ModulePassManager &module_pm, OptimizationLevel level) {
            module_pm.addPass(ModuleThreadSanitizerPass());
            module_pm.addPass(createModuleToFunctionPassAdaptor(ThreadSanitizerPass()));
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
        module_pm = pass_builder.buildO0DefaultPipeline(opt_level, options->lto);
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
        if (target_machine.addPassesToEmitFile(codegen_pm, *dest_obj, nullptr, CGFT_ObjectFile)) {
            *error_message = strdup("TargetMachine can't emit an object file");
            return true;
        }
    }
    if (dest_asm) {
        if (target_machine.addPassesToEmitFile(codegen_pm, *dest_asm, nullptr, CGFT_AssemblyFile)) {
            *error_message = strdup("TargetMachine can't emit an assembly file");
            return true;
        }
    }

    // Optimization phase
    if(!(options->is_debug && options->ir_path && options->debug_ir)) {
        module_pm.run(llvm_module, module_am);
    }

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
        case OutputMode::ReleaseSmall:
            options->is_debug = false;
            options->lto = true;
            options->is_small = true;
            break;
#ifdef DEBUG
        default:
            throw std::runtime_error("[Compiler] unknown output mode");
#endif
    }
}

bool Codegen::save_with_options(CodegenEmitterOptions* options) {
    char* error_message = nullptr;
    bool result = save_as_file_type(this, options, &error_message);
    if(error_message) error(error_message);
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

static char** convert_to_pointers(const std::vector<std::string> &command_args) {
    char** pointers = static_cast<char **>(malloc(command_args.size() * sizeof(char*)));
    // Allocate memory for each argument
    for (size_t i = 0; i < command_args.size(); ++i) {
        pointers[i] = static_cast<char*>(malloc((command_args[i].size() + 1) * sizeof(char)));
        // Copy the argument
        strcpy(pointers[i], command_args[i].c_str());
        pointers[i][command_args[i].size()] = '\0';
    }
    return pointers;
}

static void free_pointers(char** pointers, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        free(pointers[i]);
    }
    free(pointers);
}

int chemical_clang_main(int argc, char **argv);

int ChemLlvmAr_main(int argc, char **argv);

int chemical_clang_main2(const std::vector<std::string> &command_args) {
    char** pointers = convert_to_pointers(command_args);
    // invocation
    auto result = chemical_clang_main(command_args.size(), pointers);
    free_pointers(pointers, command_args.size());
    return result;
}

int llvm_ar_main2(const std::vector<std::string> &command_args) {
    char** pointers = convert_to_pointers(command_args);
    // invocation
    auto result = ChemLlvmAr_main(command_args.size(), pointers);
    free_pointers(pointers, command_args.size());
    return result;
}

int Codegen::invoke_clang(const std::vector<std::string> &command_args) {
    return chemical_clang_main2(command_args);
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

int invoke_lld(const std::vector<std::string> &command_args) {
    // Convert the vector of strings to an ArrayRef<const char *>
    std::vector<const char *> args_cstr;
    args_cstr.reserve(command_args.size() + 1);
    std::string lld_driver;
    auto triple = llvm::Triple(sys::getDefaultTargetTriple());
    if (triple.isOSDarwin())
        lld_driver = "ld64.lld";
    else if (triple.isOSWindows())
        lld_driver = "lld-link";
    else
        lld_driver = "ld.lld";
    args_cstr.push_back(lld_driver.c_str());
    for (const std::string& arg : command_args) {
        args_cstr.push_back(arg.c_str());
    }
    // invocation
    ToolContext context{};
    return lld_main(args_cstr.size(), const_cast<char**>(args_cstr.data()), context);
}

int Codegen::invoke_lld(const std::vector<std::string> &command_args) {
    return ::invoke_lld(command_args);
}

#endif

#endif

int link_objects(
    std::vector<std::string>& linkables,
    const std::string& bin_out,
    const std::string& comp_exe_path, // our compiler's executable path, needed for self invocation
    const std::vector<std::string>& flags, // passed to clang or lld,
    bool use_lld = false,
    bool libc = true
) {
    if(use_lld) {

        // creating lld command

        // set output
#if defined(_WIN32)
        linkables.emplace_back("/OUT:"+bin_out);
#elif defined(__APPLE__)
        linkables.emplace_back("-o");
        linkables.emplace_back("./"+bin_out);
#elif defined(__linux__)
        linkables.emplace_back("-o");
        linkables.emplace_back("./"+bin_out);
#endif

        // link with standard libc (unless user opts out)
        if(!libc) {
#if defined(_WIN32)
            linkables.emplace_back("-defaultlib:libcmt");
#elif defined(__APPLE__)
            // TODO test linking with libc on apple
            linkables.emplace_back("-lc");
#elif defined(__linux__)
            // TODO test linking with libc on linux
            linkables.emplace_back("-lc");
#endif
        }

        // add user's linker flags
        for(const auto& flag : flags) {
            linkables.emplace_back(flag);
        }

        // invoke lld to create executable
        return invoke_lld(linkables);

    } else {
        // use clang by default
        std::vector<std::string> clang_flags{comp_exe_path};
        for(const auto& cland_fl : flags) {
            clang_flags.emplace_back(cland_fl);
        }
        for(auto& linkable : linkables) {
            clang_flags.emplace_back(linkable);
        }
#if defined(_WINDOWS)
    if(bin_out.ends_with(".dll")) {
            clang_flags.emplace_back("-shared");
    }
#elif defined(__linux__)
        if(bin_out.ends_with(".so")) {
            clang_flags.emplace_back("-shared");
    }
#else
#error "Unknown OS"
#endif
        clang_flags.emplace_back("-o");
        clang_flags.emplace_back(bin_out);
        return chemical_clang_main2(clang_flags);
    }

}

int compile_c_file_to_object(
        const char* c_file,
        const char* out_file,
        const std::string& comp_exe_path,
        const std::vector<std::string>& flags
) {
    std::vector<std::string> clang_flags{comp_exe_path};
    for(const auto& cland_fl : flags) {
        clang_flags.emplace_back(cland_fl);
    }
    clang_flags.emplace_back("-c");
    clang_flags.emplace_back(c_file);
    clang_flags.emplace_back("-o");
    clang_flags.emplace_back(out_file);
    return chemical_clang_main2(clang_flags);
}