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
#include "ASTLinker.h"

class Codegen : public ASTLinker {
public:

    unsigned int position = 0;

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
        SetInsertPoint(entry);
    }

    void print_to_console() {
        module->print(llvm::outs(), nullptr);
    }

    void save_to_file(const std::string &out_path) {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(out_path, errorCode);
        module->print(outLL, nullptr);
    }

    /**
     * This sets the insert point to this block
     * Instead of using builder.SetInsertPoint, this function should be
     * used because llvm doesn't support multiple consecutive returns or branches
     * so if we know the block has changed, we can track
     * if this block has ended previously to avoid creating branches and returns
     * @param block
     */
    void SetInsertPoint(llvm::BasicBlock* block);

    /**
     * The safe version of builder.CreateBr
     * this will avoid creating multiple branch instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted
     * @param block
     */
    void CreateBr(llvm::BasicBlock* block);

    /**
     * this operates on two values, left and right
     * this is used by expressions to operate on two values
     * @param op
     * @param lhs
     * @param rhs
     * @return
     */
    llvm::Value* operate(Operation op, llvm::Value* lhs, llvm::Value* rhs);


    /**
     * returns an llvm type for the given type
     * @param gen
     * @param type
     * @return
     */
    llvm::Type* llvm_type(const std::optional<std::string>& type);

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
     * This is set by every loop so break statement can exit to this block
     */
    llvm::BasicBlock* current_loop_exit;

    /**
     * This is set by every loop so continue statement can continue to this block
     */
    llvm::BasicBlock* current_loop_continue;

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

private:

    /**
     * this is set to true when the branch instruction is executed
     * and set back to false, when a new block begins using SetInsertPoint
     */
    bool has_current_block_ended = false;

};