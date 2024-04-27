// Copyright (c) Qinetik 2024.

#pragma once

#ifdef COMPILER_BUILD

#include <memory>
#include <utility>
#include <vector>
#include <iostream>
#include "ast/utils/Operation.h"
#include "SymbolResolver.h"
#include "llvmfwd.h"
#include "ast/base/AccessSpecifier.h"

class Scope;

class Value;

class Codegen {
public:

    /**
     * nodes that are being traversed to generate the machine code
     */
    std::vector<std::unique_ptr<ASTNode>> nodes;

    /**
     * files that have been imported exist in this unordered map
     * to avoid importing files multiple times, there absolute paths are looked up in this map
     */
    std::unordered_map<std::string, bool> imported;

    /**
     * errors are stored here
     */
    std::vector<std::string> errors = std::vector<std::string>();

    /**
     * constructor
     * @param nodes
     * @param path
     */
    explicit Codegen(std::vector<std::unique_ptr<ASTNode>> nodes, std::string path, std::string target_triple,
                     std::string curr_exe_path);

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
     * when a function ends, this method is called to basically end the block
     * this is because the functions that return void may need a return statement
     * to end current BasicBlock
     */
    void end_function_block();

    /**
     * the method to create a function
     * @param name name of the function
     * @param type type of the function
     * @return
     */
    llvm::Function *create_function(const std::string &name, llvm::FunctionType *type, AccessSpecifier specifier);

    /**
     * create a nested function
     */
    llvm::Function *create_nested_function(const std::string &name, llvm::FunctionType *type, Scope &scope);

    /**
     * gets or inserts a function, similar to declaration
     * @param name
     * @param type
     * @return
     */
    llvm::FunctionCallee declare_function(const std::string &name, llvm::FunctionType *type);

    /**
     * create a function prototype
     * @param name name of the function
     * @param type type of the function
     * @return
     */
    llvm::Function *create_function_proto(const std::string &name, llvm::FunctionType *type, AccessSpecifier specifier);

    /**
     * create a function's basic block, with the given name
     * @param name
     * @param fn
     * @return
     */
    llvm::BasicBlock *createBB(const std::string &name, llvm::Function *fn);

    /**
     * creates a function block, along with setting the insert point to this entry block
     * @param fn
     */
    void createFunctionBlock(llvm::Function *fn);

    /**
     * prints the current module to console
     */
    void print_to_console();

#ifdef FEAT_LLVM_IR_GEN

    /**
     * prints the current module as LLVM IR to a .ll file with given out_path
     * @param out_path
     */
    void save_to_file(const std::string &out_path);

#endif

    /**
     * sets up the module for the given target
     * @param target
     */
    llvm::TargetMachine *setup_for_target(const std::string &TargetTriple);

    /**
     * automatically sets up for the current target triple
     */
    inline llvm::TargetMachine *setup_for_target() {
        return setup_for_target(target_triple);
    }

#ifdef FEAT_JUST_IN_TIME

    /**
     * just in time compilation
     * please note that this takes ownership of the module
     */
    void just_in_time_compile(std::vector<const char *> &args);

#endif

#ifdef FEAT_BITCODE_GEN
    /**
     * save as a bitcode file
     */
    void save_as_bc_file(const std::string &out_path);
#endif

#ifdef FEAT_ASSEMBLY_GEN
    /**
     * saves as assembly file to this path
     * @param TargetTriple
     */
    void save_to_assembly_file(const std::string &out_path);
#endif

    /**
     * saves as object file to this path
     * @param out_path
     */
    void save_to_object_file(const std::string &out_path);

    /**
      * You can invoke lld with this function
      */
    int invoke_lld(const std::vector<std::string> &command_args);

    /**
     * You can invoke clang with this function
     */
    int invoke_clang(const std::vector<std::string> &command_args);

    /**
     * get absolute path to this system header
     */
    std::string abs_header_path(const std::string &header);

    /**
     * get containing system headers directory for the following header
     */
    std::string headers_dir(const std::string &header);

    /**
     * just prints the errors to std out
     */
    void print_errors() {
        for (const auto &err: errors) {
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
    void loop_body_wrap(llvm::BasicBlock *condBlock, llvm::BasicBlock *endBlock);

    /**
     * This sets the insert point to this block
     * Instead of using builder.SetInsertPoint, this function should be
     * used because llvm doesn't support multiple consecutive returns or branches
     * so if we know the block has changed, we can track
     * if this block has ended previously to avoid creating branches and returns
     * @param block
     */
    void SetInsertPoint(llvm::BasicBlock *block);

    /**
     * The safe version of builder.CreateBr
     * this will avoid creating multiple terminator instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted to another block
     * @param block
     */
    void CreateBr(llvm::BasicBlock *block);

    /**
     * The safe version of builder.CreateRet
     * this will avoid creating multiple terminator instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted to another block
     * @param value
     */
    void CreateRet(llvm::Value *value);

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
    llvm::Value *operate(Operation op, Value *lhs, Value *rhs);

    /**
     * report an error when generating a node
     * @param err
     * @param node the node in which error occurred
     */
    void error(const std::string &err, ASTNode* node = nullptr);

    /**
     * destructor takes care of deallocating members
     */
    ~Codegen();

    /**
     * these are the resolved places where system headers paths exist
     * when its empty, its loaded directly by invoking clang (from self)
     * then once we found them we cache them here, for faster invocation next time
     */
    std::vector<std::string> system_headers_paths = {};

    /**
     * path to the current executable, arg[0]
     * this is useful if in the middle of code generation
     * we want to invoke the compiler to get more information !
     */
    std::string curr_exe_path;

    /**
     * root path to the file, the path to file where code gen started
     */
    std::string path;

    /**
     * path to the current file being code_gen
     */
    std::string current_path;

    /**
     * TargetTriple , which we are generating code for !
     */
    std::string target_triple;

    /**
     * The function being compiled currently
     */
    llvm::Function *current_function = nullptr;

    /**
     * This is set by every loop so break statement can exit to this block
     */
    llvm::BasicBlock *current_loop_exit = nullptr;

    /**
     * This is set by every loop so continue statement can continue to this block
     */
    llvm::BasicBlock *current_loop_continue = nullptr;

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
    llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter> *builder;

private:

    /**
     * this is set to true when the branch instruction is executed
     * and set back to false, when a new block begins using SetInsertPoint
     */
    bool has_current_block_ended = false;

};

#endif