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

Codegen::Codegen(
        std::vector<std::unique_ptr<ASTNode>> nodes,
        std::string path,
        std::string target_triple,
        std::string curr_exe_path,
        bool is_64_bit
) : ASTDiagnoser(std::move(curr_exe_path), std::move(path)), nodes(std::move(nodes)),
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

#endif

#ifdef FEAT_ASSEMBLY_GEN

/**
 * saves as assembly file to this path
 * @param TargetTriple
 */
void Codegen::save_to_assembly_file(const std::string &out_path) {
    save_as_file_type(this, out_path, llvm::CodeGenFileType::CGFT_AssemblyFile);
}

#endif

#ifdef FEAT_BITCODE_GEN

#include <llvm/Bitcode/BitcodeWriter.h>

void Codegen::save_as_bc_file(const std::string &out_path) {

    setup_for_target();

    std::error_code EC;
    raw_fd_ostream dest(out_path, EC, sys::fs::OF_None);

    if (EC) {
        error("Could not open file: " + EC.message());
        return;
    }

    WriteBitcodeToFile(*module, dest);

    dest.flush();
    dest.close();

}

#endif

#ifdef FEAT_JUST_IN_TIME

#include "Codegen.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

typedef int (*MainFuncType)(int, char**);

void Codegen::just_in_time_compile(std::vector<const char*>& args) {

    setup_for_target();

    llvm::EngineBuilder engine_builder(std::move(module));
    std::unique_ptr<llvm::ExecutionEngine> engine(engine_builder.create());

    // Execute main function
    MainFuncType mainFuncPtr = reinterpret_cast<MainFuncType>(engine->getFunctionAddress("main"));

    if (!mainFuncPtr) {
        error("Function 'main' not found in module.\n");
        return;
    }

    int result = mainFuncPtr(args.size(), const_cast<char**>(args.data()));

}

#endif