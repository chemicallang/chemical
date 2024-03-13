// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <utility>
#include <vector>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Verifier.h>
#include "ast/utils/Operation.h"

class ASTNode;

class Codegen {
public:

    unsigned int position = 0;

    std::vector<std::unique_ptr<ASTNode>> nodes;

    std::vector<std::string> errors = std::vector<std::string>();

    explicit Codegen(std::vector<std::unique_ptr<ASTNode>> nodes, std::string path);

    void module_init() {
        // context and module
        ctx = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("TodoName", *ctx);

        // creating a new builder for the module
        builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
    }

    void compile();

    llvm::Function* create_function(const std::string& name, llvm::FunctionType* type) {
        current_function = module->getFunction(name);
        if(current_function == nullptr) {
            current_function = create_function_proto(name, type);
        }
        createFunctionBlock(current_function);
        return current_function;
    }

    llvm::FunctionCallee declare_function(const std::string& name, llvm::FunctionType* type) {
        return module->getOrInsertFunction(name, type);
    }

//    template <typename... ArgsTy>
//    llvm::Function* declare_function(const std::string& name, llvm::FunctionType* type, ArgsTy... args) {
//        module->getOrInsertFunction(name, type, args...);
//    }

    llvm::Function* create_function_proto(const std::string& name, llvm::FunctionType* type) {
        auto fn = llvm::Function::Create(type, llvm::Function::ExternalLinkage, name, *module);
        llvm::verifyFunction(*fn);
        return fn;
    }

    llvm::BasicBlock* createBB(const std::string& name, llvm::Function* fn) {
        return llvm::BasicBlock::Create(*ctx, name, fn);
    }

    void createFunctionBlock(llvm::Function* fn) {
        auto entry = createBB("entry", fn);
        builder->SetInsertPoint(entry);
    }

    void print_to_console() {
        module->print(llvm::outs(), nullptr);
    }

    void save_to_file(const std::string &out_path) {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(out_path, errorCode);
        module->print(outLL, nullptr);
    }

    llvm::Value* operate(Operation op, llvm::Value* lhs, llvm::Value* rhs);

    /**
     * report an error when generating a node
     * @param err
     */
    void error(const std::string& err);

    /**
     * path to the file
     */
    std::string path;

    /**
     * The function being compiled currently
     */
    llvm::Function* current_function;

    /**
     * LLVM context that holds modules
     */
    std::unique_ptr<llvm::LLVMContext> ctx;

    /**
     * module holds functions, global vars and stuff
     */
    std::unique_ptr<llvm::Module> module;

    /**
     * the builder that builds ir
     */
    std::unique_ptr<llvm::IRBuilder<>> builder;

};