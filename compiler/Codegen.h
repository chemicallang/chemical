// Copyright (c) Qinetik 2024.

#pragma once

#ifdef COMPILER_BUILD

#include <memory>
#include <utility>
#include <vector>
#include <iostream>
#include "ast/utils/Operation.h"
#include "llvmfwd.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/base/BaseTypeKind.h"
#include "ast/base/ValueType.h"
#include "ASTDiagnoser.h"
#include "OutputMode.h"
#include <unordered_map>
#include "CodegenEmitterOptions.h"
#include "ast/base/GlobalInterpretScope.h"

class Codegen;

class Scope;

class Value;

class BaseType;

class FunctionType;

class MembersContainer;

class FunctionCall;

class FunctionDeclaration;

/**
 * A caster fn takes a pointer to a value, and a pointer to a type
 * the function then returns a new value by casting the given value to the given type
 * the value is created a new on the heap
 */
//typedef Value*(*CasterFn)(Codegen* gen, Value* val, BaseType* type);

class Codegen : public ASTDiagnoser {
public:

    /**
     * global interpret scope, is the scope used for compile time evaluation
     * of code
     */
    GlobalInterpretScope comptime_scope;

    /**
     * nodes that are being traversed to generate the machine code
     */
    std::vector<std::unique_ptr<ASTNode>> nodes;

    /**
     * casters that take a value and cast them to a different value
     * it should be known that value created by caster is on the heap
     * the caller has the ownership and must manage memory
     */
//    std::unordered_map<int, CasterFn> casters;

    /**
     * contains references to nodes that must be destructed at return
     */
    std::vector<ASTNode*> destruct_nodes;

    /**
     * when a function is evaluated, it's value is stored on this map, so it can be looked up for destruction
     */
    std::unordered_map<FunctionCall*, std::unique_ptr<Value>> evaluated_func_calls;

    /**
     * All get element pointer instructions use this to state that the element pointer is inbounds
     * If true, results in undefined behavior when accessing element out of bounds, which is the default
     */
    bool inbounds = true;

    /**
     * whether the system we are compiling for is 64 bit
     * when compile method is called, this is determined
     */
    bool is64Bit = false;

    /**
     * constructor
     * @param nodes
     * @param path
     */
    explicit Codegen(
            std::vector<std::unique_ptr<ASTNode>> nodes,
            std::string target_triple,
            std::string curr_exe_path,
            bool is_64_bit, // can be determined using static method is_arch_64bit on Codegen
            const std::string& module_name = "ChemMod"
    );

    /**
     * initializes the llvm module and context
     */
    void module_init(const std::string& module_name);

    /**
     * determine whether the system we are compiling for is 64bit or 32bit
     */
    static bool is_arch_64bit(const std::string &target_triple);

    /**
     * provides a caster_index, which can be used to store or retrieve a caster
     * from casters map
     */
//    static constexpr int caster_index(ValueType type, BaseTypeKind kind) {
//        return ((uint8_t) type << 10) | (uint8_t) kind;
//    }

    /**
     * before compilation begins (calling compile_nodes for the first time), this should be called
     * this must be called once for a single module
     */
    void compile_begin();

    /**
     * everytime nodes are switched this method must be called, typically
     * when compiling different modules, nodes are changed
     */
    template<typename NodesVec>
    void compile_nodes(NodesVec& nodes);

    /**
     * everytime nodes are switched this method must be called, typically
     * when compiling different modules, nodes are changed
     */
    void compile_nodes();

    /**
     * after compilation has finished, of all files, this method is called
     * this must be called once, for a single module
     */
    void compile_end();

    /**
     * the actual compile function, when called module, ctx and builder members
     * are used to fill up the IR
     *
     * this just calls compile_begin, compile_nodes and compile_end
     * this should be used if compiling a single file, whose nodes are present in nodes vector
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
     */
    llvm::Function *create_function(const std::string &name, llvm::FunctionType *type, AccessSpecifier specifier);

    /**
     * create a nested function
     */
    llvm::Function *create_nested_function(const std::string &name, llvm::FunctionType *type, FunctionType* func_type, Scope &scope);

    /**
     * gets or inserts a function, similar to declaration
     */
    llvm::FunctionCallee declare_function(const std::string &name, llvm::FunctionType *type);

    /**
     * create a function prototype
     * @param name name of the function
     * @param type type of the function
     */
    llvm::Function *create_function_proto(const std::string &name, llvm::FunctionType *type, AccessSpecifier specifier);

    /**
     * create a function's basic block, with the given name
     */
    llvm::BasicBlock *createBB(const std::string &name, llvm::Function *fn);

    /**
     * a struct type with two pointers inside
     */
    llvm::StructType* fat_pointer_type();

    /**
     * packs two pointers into a fat pointer (a struct containing two pointers)
     */
    llvm::AllocaInst* pack_fat_pointer(llvm::Value* first_ptr, llvm::Value* second_ptr);

    /**
     * gets the implementation using value and type, where value is a struct value or ref struct
     * and type is a type to an interface, that is implemented by ref struct
     */
    llvm::Value* get_dyn_obj_impl(Value*, BaseType* type);

    /**
     * it checks whether the given type is a dynamic type and the given value is a struct
     * if the given value is a struct that implements the given dynamic type, it's packed into
     * a fat pointer where the first pointer is the llvm_value expected to be the object reference
     * retrieved via value->llvm_value(), the second pointer is the implementation retrieved by
     * finding the interface which stores all implementations for the structs
     * IF it can't pack because it's not a dynamic type, it will return llvm_value
     */
    llvm::Value* pack_dyn_obj(Value* value, BaseType* type, llvm::Value* llvm_value);

    /**
     * if this type corresponds to a dynamic object, a fat pointer type will be allocated and returned
     * otherwise nullptr is returned
     */
    llvm::Value* allocate_dyn_obj_based_on_type(BaseType* type);

    /**
     * to change the implementation of dynamic object, this function can be used
     * fat_pointer already having valid struct object for the implementation
     * the implementation is calculated based on the given Value* pointer and type
     * returns true if could get implementation and assign it
     */
    bool assign_dyn_obj_impl(Value* value, BaseType* type, llvm::Value* fat_pointer);

    /**
     * the implementation represented by this dynamic object will be assigned, along
     * with the given struct object
     * obj is the pointer to the struct that will be assigned
     * the implementation is calculated based on value and type
     * returns true if could get implementation and assign both
     */
    bool assign_dyn_obj(Value* value, BaseType* type, llvm::Value* fat_pointer, llvm::Value* obj);

    /**
     * creates a function block, along with setting the insert point to this entry block
     */
    void createFunctionBlock(llvm::Function *fn);

    /**
     * prints the current module to console
     */
    void print_to_console();

    /**
     * sets up the module for the given target
     */
    llvm::TargetMachine *setup_for_target(const std::string &TargetTriple);

    /**
     * automatically sets up for the current target triple
     */
    inline llvm::TargetMachine *setup_for_target() {
        return setup_for_target(target_triple);
    }

    /**
     * options will be used to save to files
     */
    bool save_with_options(CodegenEmitterOptions* options);

    /**
     * prints the current module as LLVM IR to a .ll file with given out_path
     */
    bool save_to_ll_file_for_debugging(const std::string &out_path) const;

    /**
     * save as a bitcode file
     */
    bool save_to_bc_file(std::string &out_path, OutputMode mode);

    /**
     * saves as assembly file to this path
     */
    bool save_to_assembly_file(std::string &out_path, OutputMode mode);

    /**
     * saves as object file to this path
     */
    bool save_to_object_file(std::string &out_path, OutputMode mode);

#ifdef LLD_LIBS

    /**
      * You can invoke lld with this function
      */
    int invoke_lld(const std::vector<std::string> &command_args);

#endif

#ifdef CLANG_LIBS

    /**
     * You can invoke clang with this function
     */
    int invoke_clang(const std::vector<std::string> &command_args);

#endif

    /**
     * the given call is linked with given comptime function decl, that is evaluated to receive the return value
     * using this function, the evaluation is done once, so this function caches the return value
     */
    std::unique_ptr<Value>& eval_comptime(FunctionCall* call, FunctionDeclaration* decl);

    /**
     * determines destructor function for given element type
     */
    FunctionDeclaration* determine_destructor_for(
            BaseType* elem_type,
            llvm::FunctionType*& func_type,
            llvm::Value*& func_callee
    );

    /**
     * destruct given alloca instruction, the array size is llvm value
     * destructs an array, also allows to add extra logic after destruction via lambda
     * the llvm::Value* provided in lambda is of the struct present in the array
     */
    void destruct(
            llvm::Value* allocaInst,
            llvm::Value* array_size,
            llvm::Type* elem_type,
            bool check_for_null,
            const std::function<void(llvm::Value*)>& call_destructor
    );

    /**
     * destruct given alloca instruction, the array size is llvm value
     * destructs an array, also allows to add extra logic after destruction via lambda
     * the llvm::Value* provided in lambda is of the struct present in the array
     */
    void destruct(
            llvm::Value* allocaInst,
            llvm::FunctionType* destructor_func_type,
            llvm::Value* destructor_func_callee,
            bool pass_self,
            llvm::Value* array_size,
            BaseType* elem_type,
            bool check_for_null,
            const std::function<void(llvm::Value*)>& after_destruct
    );

    /**
     * destruct given alloca instruction, the array size is llvm value
     * destructs an array, also allows to add extra logic after destruction via lambda
     * the llvm::Value* provided in lambda is of the struct present in the array
     */
    void destruct(
            llvm::Value* allocaInst,
            llvm::Value* array_size,
            BaseType* elem_type,
            bool check_for_null,
            const std::function<void(llvm::Value*)>& after_destruct
    );

    /**
     * destructs an array, also allows to add extra logic after destruction via lambda
     * the llvm::Value* provided in lambda is of the struct present in the array
     *
     * no null check is made, the caller must ensure that given allocaInst is valid
     * Since this method is mainly used for stack based arrays containing structs
     */
    void destruct(
            llvm::Value* allocaInst,
            unsigned int array_size,
            BaseType* elem_type,
            const std::function<void(llvm::Value*)>& after_destruct
    );

    /**
     * when generating code for the body of the loop, it should be wrapped with this function call
     * before and after the body generation
     * this ensures that break and continue instructions work properly by pointing to the given blocks
     */
    void loop_body_wrap(llvm::BasicBlock *condBlock, llvm::BasicBlock *endBlock);

    /**
     * This sets the insert point to this block
     * Instead of using builder.SetInsertPoint, this function should be
     * used because llvm doesn't support multiple consecutive returns or branches
     * so if we know the block has changed, we can track
     * if this block has ended previously to avoid creating branches and returns
     */
    void SetInsertPoint(llvm::BasicBlock *block);

    /**
     * if the value is null, go to true block, otherwise false block
     */
    void CheckNullCondBr(llvm::Value* value, llvm::BasicBlock* TrueBlock, llvm::BasicBlock* FalseBlock);

    /**
     * The safe version of builder.CreateBr
     * this will avoid creating multiple terminator instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted to another block
     */
    void CreateBr(llvm::BasicBlock *block);

    /**
     * The safe version of builder.CreateRet
     * this will avoid creating multiple terminator instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted to another block
     */
    void CreateRet(llvm::Value *value);

    /**
     * creates a default return, it returns void
     * this takes into account the redirect_return required for cleanup blocks in
     * destructors, it should be used instead of CreateRet(nullptr) or CreateRetVoid()
     */
    void DefaultRet();

    /**
     * The safe version of builder.CreateCondBr
     * this will avoid creating multiple terminator instructions
     */
    void CreateCondBr(llvm::Value *Cond, llvm::BasicBlock *True, llvm::BasicBlock *FalseMDNode);

    /**
     * tag
     */
    std::string TAG() override {
        return "Codegen";
    }

    /**
    * this operates on two values, left and right
    * this is used by expressions to operate on two values
    */
    llvm::Value *operate(Operation op, Value *lhs, Value *rhs, BaseType* lhsType, BaseType* rhsType);

    /**
     * this operates on two values, left and right
     * this is used by expressions to operate on two values
     */
    llvm::Value *operate(Operation op, Value *lhs, Value *rhs);

    /**
     * implicitly cast given value from type to to type, if no cast is available value is returned
     */
    llvm::Value *implicit_cast(llvm::Value* value, BaseType* from_type, BaseType* to_type);

    /**
     * destructor takes care of deallocating members
     */
    ~Codegen();

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
     * the statements being generated are from this function type
     */
    FunctionType* current_func_type = nullptr;

    /**
     * when user say's var x = if(condition) value else other_value
     * the if statement here set's x to this variable as "assignable)
     * so when if statement or switch or any value that would like to assign
     * it's implicitly returned value assigns it to this current_assignable
     */
    llvm::Value* current_assignable = nullptr;

    /**
     * When given, return's are shifted to this block
     * So when a return statement is detected a branch instruction is made to this block
     * The return must be void for this to work
     */
    llvm::BasicBlock *redirect_return = nullptr;

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

    /**
     * this is set to true when the branch instruction is executed
     * and set back to false, when a new block begins using SetInsertPoint
     */
    bool has_current_block_ended = false;

    /**
     * by default every scope is destroyed, however a return statement in current scope
     * sets this to true, because return destroys everything in a function
     */
    bool destroy_current_scope = true;

};

template<typename NodesVec>
void Codegen::compile_nodes(NodesVec& nodes_vec) {
    for(const auto& node : nodes_vec) {
        node->code_gen_declare(*this);
    }
    for (const auto &node: nodes_vec) {
        node->code_gen(*this);
    }
}

#endif