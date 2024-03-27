// Copyright (c) Qinetik 2024.

#include "Codegen.h"
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

typedef int (*MainFuncType)(int, char**);

void Codegen::just_in_time_compile(std::vector<const char*>& args, const std::string& TargetTriple) {

    setup_for_target(TargetTriple);

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