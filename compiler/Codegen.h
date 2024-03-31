// Copyright (c) Qinetik 2024.

#pragma once

#ifdef COMPILER_BUILD
#include <memory>
#include <utility>
#include <vector>
#include <iostream>
#include "ast/utils/Operation.h"
#include "ASTLinker.h"
#include "llvmfwd.h"

class Codegen {
public:

    /**
     * nodes that are being traversed to generate the machine code
     */
    std::vector<std::unique_ptr<ASTNode>> nodes;

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
     * constructor
     * @param nodes
     * @param path
     */
    explicit Codegen(std::vector<std::unique_ptr<ASTNode>> nodes, std::string path);

    /**
     * initializes the llvm module and context
     */
    void module_init();

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
    llvm::Function* create_function(const std::string& name, llvm::FunctionType* type);

    /**
     * gets or inserts a function, similar to declaration
     * @param name
     * @param type
     * @return
     */
    llvm::FunctionCallee declare_function(const std::string& name, llvm::FunctionType* type);

    /**
     * create a function prototype
     * @param name name of the function
     * @param type type of the function
     * @return
     */
    llvm::Function* create_function_proto(const std::string& name, llvm::FunctionType* type);

    /**
     * create a function's basic block, with the given name
     * @param name
     * @param fn
     * @return
     */
    llvm::BasicBlock* createBB(const std::string& name, llvm::Function* fn);

    /**
     * creates a function block, along with setting the insert point to this entry block
     * @param fn
     */
    void createFunctionBlock(llvm::Function* fn);

    /**
     * prints the current module to console
     */
    void print_to_console();

    /**
     * prints the current module as LLVM IR to a .ll file with given out_path
     * @param out_path
     */
    void save_to_file(const std::string &out_path, const std::string& TargetTriple);

    /**
     * sets up the module for the given target
     * @param target
     */
    llvm::TargetMachine * setup_for_target(const std::string& TargetTriple);

    /**
     * just in time compilation
     * please note that this takes ownership of the module
     */
    void just_in_time_compile(std::vector<const char*>& args, const std::string& TargetTriple);

    /**
     * save file as file type
     * @param object_file when true object file is generated, otherwise assembly file is generated
     */
    void save_as_file_type(const std::string &out_path, const std::string& TargetTriple, bool object_file = true);

    /**
     * save as a bitcode file
     */
    void save_as_bc_file(const std::string &out_path, const std::string& TargetTriple);

    /**
     * saves as assembly file to this path
     * @param TargetTriple
     */
    inline void save_to_assembly_file(const std::string &out_path, const std::string& TargetTriple) {
        save_as_file_type(out_path, TargetTriple, false);
    }

    /**
     * saves as object file to this path
     * @param out_path
     */
    inline void save_to_object_file(const std::string &out_path, const std::string& TargetTriple) {
        save_as_file_type(out_path, TargetTriple, true);
    }

    /**
      * You can invoke clang with this function
      */
    int invoke_lld(const std::vector<std::string>& command_args);

    /**
     * You can invoke clang with this function
     */
    int invoke_clang(const std::vector<std::string>& command_args);

    /**
     * this can be used to capture output of clang in the console
     */
    int invoke_clang(const std::vector<std::string>& command_args, std::string& clang_output);

    /**
     * get system headers directory path for searching headers
     */
    std::vector<std::string> system_headers_path(const std::string& argv1);

    /**
     * just prints the errors to std out
     */
    void print_errors() {
        for(const auto& err : errors) {
            std::cerr << err << std::endl;
        }
    }

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
     * destructor takes care of deallocating members
     */
    ~Codegen();

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
    llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>* builder;

private:

    /**
     * this is set to true when the branch instruction is executed
     * and set back to false, when a new block begins using SetInsertPoint
     */
    bool has_current_block_ended = false;

};
#endif