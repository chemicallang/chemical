// Copyright (c) Qinetik 2024.

#pragma once

#ifdef COMPILER_BUILD
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

    /**
     * At the moment this stores the position (inside the nodes vector)
     * the position doesn't work with nested nodes, as nested nodes have their own structures
     * for that we may use ASTPointer if required anytime
     */
    unsigned int position = 0;

    /**
     * errors are stored here
     */
    std::vector<std::string> errors = std::vector<std::string>();

    /**
     * similar to ast linker we have allocated
     * which provides a way to link up the currently allocated variables
     */
    std::unordered_map<std::string, llvm::AllocaInst*> allocated;

    /**
     * constructor
     * @param nodes
     * @param path
     */
    explicit Codegen(std::vector<std::unique_ptr<ASTNode>> nodes, std::string path);

    /**
     * initializes the llvm module and context
     */
    void module_init() {
        // context and module
        ctx = std::make_unique<llvm::LLVMContext>();
        module = std::make_unique<llvm::Module>("TodoName", *ctx);

        // creating a new builder for the module
        builder = std::make_unique<llvm::IRBuilder<>>(*ctx);
    }

    /**
     * the actual compile function, when called module, ctx and builder members
     * are used to fill up the IR
     */
    void compile();

    /**
     * the method to create a function
     * @param name name of the function
     * @param type type of the function
     * @return
     */
    llvm::Function* create_function(const std::string& name, llvm::FunctionType* type) {
        current_function = module->getFunction(name);
        if(current_function == nullptr) {
            current_function = create_function_proto(name, type);
        }
        createFunctionBlock(current_function);
        return current_function;
    }

    /**
     * gets or inserts a function, similar to declaration
     * @param name
     * @param type
     * @return
     */
    llvm::FunctionCallee declare_function(const std::string& name, llvm::FunctionType* type) {
        return module->getOrInsertFunction(name, type);
    }

    /**
     * create a function prototype
     * @param name name of the function
     * @param type type of the function
     * @return
     */
    llvm::Function* create_function_proto(const std::string& name, llvm::FunctionType* type) {
        auto fn = llvm::Function::Create(type, llvm::Function::ExternalLinkage, name, *module);
        llvm::verifyFunction(*fn);
        return fn;
    }

    /**
     * create a function's basic block, with the given name
     * @param name
     * @param fn
     * @return
     */
    llvm::BasicBlock* createBB(const std::string& name, llvm::Function* fn) {
        return llvm::BasicBlock::Create(*ctx, name, fn);
    }

    /**
     * creates a function block, along with setting the insert point to this entry block
     * @param fn
     */
    void createFunctionBlock(llvm::Function* fn) {
        auto entry = createBB("entry", fn);
        SetInsertPoint(entry);
    }

    /**
     * prints the current module to console
     */
    void print_to_console() {
        module->print(llvm::outs(), nullptr);
    }

    /**
     * prints the current module as LLVM IR to a .ll file with given out_path
     * @param out_path
     */
    void save_to_file(const std::string &out_path) {
        std::error_code errorCode;
        llvm::raw_fd_ostream outLL(out_path, errorCode);
        module->print(outLL, nullptr);
    }

    /**
     * saves as object file to this path
     * @param out_path
     */
    void save_to_object_file(const std::string &out_path);

    /**
     * This compiles the given object file to executable using clang apis
     */
    void link_object_files_as_executable(std::vector<std::string>& obj_files, const std::string &out_path);

    /**
     * when generating code for the body of the loop, it should be wrapped with this function call
     * before and after the body generation
     * this ensures that break and continue instructions work properly by pointing to the given blocks
     * @param gen
     * @param condBlock
     * @param endBlock
     */
    void loop_body_wrap(llvm::BasicBlock* condBlock, llvm::BasicBlock* endBlock);

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
     * this will avoid creating multiple terminator instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted to another block
     * @param block
     */
    void CreateBr(llvm::BasicBlock* block);

    /**
     * The safe version of builder.CreateRet
     * this will avoid creating multiple terminator instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted to another block
     * @param value
     */
    void CreateRet(llvm::Value* value);

    /**
     * The safe version of builder.CreateCondBr
     * this will avoid creating multiple terminator instructions
     * @param Cond
     * @param True
     * @param False
     * @return
     */
    void CreateCondBr(llvm::Value *Cond, llvm::BasicBlock *True, llvm::BasicBlock *FalseMDNode);

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
#endif