// Copyright (c) Qinetik 2024.

#ifdef COMPILER_BUILD

#include "ast/base/ASTNode.h"
#include "Codegen.h"
#include "ast/structures/Scope.h"
#include "SelfInvocation.h"
#include <utility>
#include "llvmimpl.h"

Codegen::Codegen(
        std::vector<std::unique_ptr<ASTNode>> nodes,
        std::string path,
        std::string target_triple,
        std::string curr_exe_path
) : nodes(std::move(nodes)), current_path(path), path(std::move(path)),
    target_triple(std::move(target_triple)),
    curr_exe_path(std::move(curr_exe_path)) {
    module_init();
}

void Codegen::module_init() {
    // context and module
    ctx = std::make_unique<llvm::LLVMContext>();
    module = std::make_unique<llvm::Module>("TodoName", *ctx);

    // creating a new builder for the module
    builder = new llvm::IRBuilder<>(*ctx);
}

void Codegen::compile() {
    for(const auto& node : nodes) {
        node->code_gen_declare(*this);
    }
    for (const auto &node: nodes) {
        node->code_gen(*this);
    }
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
    llvm::verifyFunction(*fn);
    return fn;
}

llvm::BasicBlock *Codegen::createBB(const std::string &name, llvm::Function *fn) {
    return llvm::BasicBlock::Create(*ctx, name, fn);
}

void Codegen::print_to_console() {
    module->print(llvm::outs(), nullptr);
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
    module->print(outLL, nullptr);
    outLL.close();
}

#endif

void Codegen::loop_body_wrap(llvm::BasicBlock *condBlock, llvm::BasicBlock *endBlock) {
    // set current loop exit, so it can be broken
    current_loop_continue = condBlock;
    current_loop_exit = endBlock;
}

void Codegen::info(const std::string &err, ASTNode* node) {
    std::string errStr = "[Codegen] Info\n";
    errStr += "---- message : " + err + "\n";
    errStr += "---- file path : " + current_path;
#ifdef DEBUG
    std::cout << errStr;
    if(node) {
        std::cout << "\n" << "---- node representation : " + node->representation();
    }
    std::cout << std::endl;
#endif
}

void Codegen::error(const std::string &err, ASTNode* node) {
    std::string errStr = "[Codegen] ERROR\n";
    errStr += "---- message : " + err + "\n";
#ifdef DEBUG
    std::cerr << errStr;
    if(node) {
        std::cerr << "\n" << "---- node representation : " + node->representation();
    }
    std::cerr << std::endl;
#endif
    errors.push_back(errStr);
}

std::string Codegen::headers_dir(const std::string& header) {

    if(system_headers_paths.empty()) {
        system_headers_paths = std::move(::system_headers_path(curr_exe_path));
    }

    return ::headers_dir(system_headers_paths, header);

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