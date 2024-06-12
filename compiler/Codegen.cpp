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

Codegen::Codegen(
        std::vector<std::unique_ptr<ASTNode>> nodes,
        const std::string& path,
        std::string target_triple,
        std::string curr_exe_path,
        bool is_64_bit
) : ASTDiagnoser(path), nodes(std::move(nodes)),
    target_triple(std::move(target_triple)), is64Bit(is_64_bit) {
    module_init();
}

void Codegen::casters_init() {
    comp_casters[caster_index(ValueType::Int, BaseTypeKind::IntN)] = [](Value* value, BaseType* type) -> Value* {
        // TODO
        return nullptr;
    };
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

void Codegen::module_init() {
    // context and module
    ctx = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("TodoName", *ctx);

    // creating a new builder for the module
    builder = new llvm::IRBuilder<>(*ctx);
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
    for(const auto& node : nodes) {
        node->code_gen_declare(*this);
    }
    for (const auto &node: nodes) {
        node->code_gen(*this);
    }
}

void Codegen::compile_end() {
    for(const auto& interface : unimplemented_interfaces) {
        bool has_implemented = false;
        for(const auto& func : interface.second) {
            if(!func.second) {
                has_implemented = true;
            }
        }
        for(const auto& func : interface.second) {
            if(func.second) {
                func.second->removeFromParent();
                if(has_implemented) {
                    info("Method " + func.first + " of Interface " + interface.first + " left unimplemented, has been removed.");
                }
            }
        }
    }
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

llvm::Function *Codegen::create_nested_function(const std::string &name, llvm::FunctionType *type, Scope &scope) {

    auto prev_block_ended = has_current_block_ended;
    auto prev_block = builder->GetInsertBlock();
    auto prev_current_func = current_function;

    SetInsertPoint(nullptr);
    auto nested_function = create_function_proto(name, type, AccessSpecifier::Private);
    current_function = nested_function;
    createFunctionBlock(nested_function);
    scope.code_gen(*this);
    end_function_block();

    has_current_block_ended = prev_block_ended;
    SetInsertPoint(prev_block);
    current_function = prev_current_func;

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
    llvm::verifyFunction(*fn);
    return fn;
}

llvm::BasicBlock *Codegen::createBB(const std::string &name, llvm::Function *fn) {
    return llvm::BasicBlock::Create(*ctx, name, fn);
}

llvm::StructType* Codegen::packed_lambda_type() {
    return llvm::StructType::get(builder->getPtrTy(), builder->getPtrTy());
}

llvm::AllocaInst* Codegen::pack_lambda(llvm::Function* func_ptr, llvm::Value* captured_struct) {
    // create a struct with two pointers
    auto structType = packed_lambda_type();
    auto allocated = builder->CreateAlloca(structType);
    // store lambda function pointer in the first variable
    auto first = builder->CreateStructGEP(structType, allocated, 0);
    builder->CreateStore(func_ptr, first);
    // store a pointer to a struct that contains captured variables in the second variable
    auto second = builder->CreateStructGEP(structType, allocated, 1);
    builder->CreateStore(captured_struct, second);
    return allocated;
}

void Codegen::print_to_console() {
    module->print(llvm::outs(), nullptr, false, true);
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

void Codegen::CreateCondBr(llvm::Value *Cond, llvm::BasicBlock *True, llvm::BasicBlock *FalseMDNode) {
    if (!has_current_block_ended) {
        builder->CreateCondBr(Cond, True, FalseMDNode);
        has_current_block_ended = true;
    }
}

#ifdef FEAT_LLVM_IR_GEN

void Codegen::save_to_file(const std::string &out_path) {
    std::error_code errorCode;
    llvm::raw_fd_ostream outLL(out_path, errorCode);
    module->print(outLL, nullptr, false, true);
    outLL.close();
}

#endif

void Codegen::loop_body_wrap(llvm::BasicBlock *condBlock, llvm::BasicBlock *endBlock) {
    // set current loop exit, so it can be broken
    current_loop_continue = condBlock;
    current_loop_exit = endBlock;
}


Codegen::~Codegen() {
    delete builder;
}

#ifdef FEAT_ASSEMBLY_GEN

/**
 * saves as assembly file to this path
 * @param TargetTriple
 */
void Codegen::save_to_assembly_file(const std::string &out_path) {
    save_as_file_type(this, out_path, llvm::CodeGenFileType::CGFT_AssemblyFile);
}

#endif

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

void save_as_file_type(Codegen* gen, const std::string &out_path, llvm::CodeGenFileType type) {

    auto TheTargetMachine = gen->setup_for_target();
    if(TheTargetMachine == nullptr) {
        return;
    }

    std::error_code EC;
    raw_fd_ostream dest(out_path, EC, sys::fs::OF_None);

    if (EC) {
        gen->error("Could not open file: " + EC.message());
        return;
    }

    legacy::PassManager pass;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, type)) {
        gen->error("TheTargetMachine can't emit a file of this type");
        return;
    }

    pass.run(*gen->module);
    dest.flush();

}

/**
 * saves as object file to this path
 * @param out_path
 */
void Codegen::save_to_object_file(const std::string &out_path) {
    save_as_file_type(this, out_path, llvm::CodeGenFileType::CGFT_ObjectFile);
}

#ifdef CLANG_LIBS

int chemical_clang_main(int argc, char **argv);

int chemical_clang_main2(const std::vector<std::string> &command_args) {
    char** pointers = static_cast<char **>(malloc(command_args.size() * sizeof(char*)));

    // Allocate memory for each argument
    for (size_t i = 0; i < command_args.size(); ++i) {
        pointers[i] = static_cast<char*>(malloc((command_args[i].size() + 1) * sizeof(char)));
        // Copy the argument
        strcpy(pointers[i], command_args[i].c_str());
        pointers[i][command_args[i].size()] = '\0';
    }

    // invocation
    auto result = chemical_clang_main(command_args.size(), pointers);
    for (size_t i = 0; i < command_args.size(); ++i) {
        free(pointers[i]);
    }
    free(pointers);
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

int Codegen::invoke_lld(const std::vector<std::string> &command_args) {
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

#endif

#endif