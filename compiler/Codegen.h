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
#include "ASTDiagnoser.h"
#include "OutputMode.h"
#include <unordered_map>
#include "CodegenEmitterOptions.h"
#include "ClangCodegen.h"
#include "utils/fwd/functional.h"
#include "std/chem_string_view.h"
#include "backend/DebugInfoBuilder.h"

class ASTAllocator;

class GlobalInterpretScope;

class Codegen;

class Scope;

class Value;

class BaseType;

class FunctionType;

class MembersContainer;

class FunctionCall;

class FunctionDeclaration;

class Codegen : public ASTDiagnoser {
public:

    /**
     * global interpret scope, is the scope used for compile time evaluation
     * of code
     */
    GlobalInterpretScope& comptime_scope;

    /**
     * the clang codegen class, that allows to control clang code generation without importing
     * everything everywhere
     */
    ClangCodegen clang;

    /**
     * allocator is used to allocate ast things during code generation
     */
    ASTAllocator& allocator;

    /**
     * Debug info builder basically just generates the debug information
     * it's conveniently named di so it's easier to access
     */
    DebugInfoBuilder di;

    /**
     * contains references to nodes that must be destructed at return
     */
    std::vector<ASTNode*> destruct_nodes;

    /**
     * when a function is evaluated, it's value is stored on this map, so it can be looked up for destruction
     */
    std::unordered_map<FunctionCall*, Value*> evaluated_func_calls;

    /**
     * implicit arguments are provided using provide as statement
     */
    std::unordered_map<chem::string_view, llvm::Value*> implicit_args;

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
    std::pair<Value*, llvm::Value*> current_assignable = {nullptr, nullptr};

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
     * updated everytime module changes
     */
    std::unique_ptr<llvm::DIBuilder> diBuilder;

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
            GlobalInterpretScope& comptime_scope,
            std::string target_triple,
            std::string curr_exe_path,
            bool is_64_bit, // can be determined using static method is_arch_64bit on Codegen,
            ASTAllocator& allocator,
            const std::string& module_name = "ChemMod"
    );

    /**
     * initializes the llvm module and context
     */
    void module_init(const std::string& module_name);

    /**
     * determine whether the system we are compiling for is 64bit or 32bit
     */
    static bool is_arch_64bit(const std::string_view &target_triple);

    /**
     * provides a caster_index, which can be used to store or retrieve a caster
     * from casters map
     */
//    static constexpr int caster_index(ValueType type, BaseTypeKind kind) {
//        return ((uint8_t) type << 10) | (uint8_t) kind;
//    }

    /**
     * everytime nodes are switched this method must be called, typically
     * when compiling different modules, nodes are changed
     */
    void declare_nodes(std::vector<ASTNode*>& nodes);

    /**
     * compile these nodes after declaring them
     */
    void compile_nodes(std::vector<ASTNode*>& nodes);

    /**
     * everytime nodes are switched this method must be called, typically
     * when compiling different modules, nodes are changed
     */
    void declare_and_compile(std::vector<ASTNode*>& nodes) {
        declare_nodes(nodes);
        compile_nodes(nodes);
    }

    /**
     * this will only declare these nodes
     */
    void external_declare_nodes(std::vector<ASTNode*>& nodes);

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
    llvm::Function* declare_function(const std::string &name, llvm::FunctionType *type, AccessSpecifier specifier);

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
     */
    void assign_dyn_obj_impl(llvm::Value* fat_pointer, llvm::Value* impl);

    /**
     * it finds the implementation for the given value and type automatically
     * returns true if could get implementation and assign it
     */
    bool assign_dyn_obj_impl(Value* value, BaseType* type, llvm::Value* fat_pointer);

    /**
     * the implementation represented by this dynamic object will be assigned, along
     * with the given struct object
     * obj is the pointer to the struct that will be assigned
     * the implementation is calculated based on value and type
     */
    void assign_dyn_obj(llvm::Value* fat_pointer, llvm::Value* obj, llvm::Value* impl);

    /**
     * if you don't know the implementation, you can use this method, it get's the implementation
     * and when found assigns it, along with the obj, returns true if succeeds otherwise false
     */
    bool assign_dyn_obj(Value* value, BaseType* type, llvm::Value* fat_pointer, llvm::Value* obj);

    /**
     * a helper method to call clear function
     */
    void call_clear_fn(Value* value, llvm::Value* llvm_value);

    /**
     * mem copy a struct into the given pointer
     */
    void memcpy_struct(llvm::Type* type, llvm::Value* pointer, llvm::Value* value);

    /**
     * move the value
     * @param type is the type of value, expected
     * @param value is the actual value (not llvm though)
     * @param ptr is where value will be moved to
     * @param movable_value is the movable value that is retrieved by calling movable_value method
     * how this works is, the movable value is mem copied into the given pointer, but movable_value is the
     * memory location of the previous struct, so we call clear function on it, to tell the struct that struct has been
     * freed
     * @return true if move was done, otherwise false, if false is returned, caller should pursue of storing
     * the value inside the given pointer
     */
    void move_by_memcpy(ASTNode* container, Value* value, llvm::Value* ptr, llvm::Value* movable_value);

    /**
     * move the value
     * @param type is the type of value, expected
     * @param value is the actual value (not llvm though)
     * @param ptr is where value will be moved to
     * @param movable_value is the movable value that is retrieved by calling movable_value method
     * how this works is, the movable value is mem copied into the given pointer, but movable_value is the
     * memory location of the previous struct, so we call clear function on it, to tell the struct that struct has been
     * freed
     * @return true if move was done, otherwise false, if false is returned, caller should pursue of storing
     * the value inside the given pointer
     */
    bool move_by_memcpy(BaseType* type, Value* value, llvm::Value* ptr, llvm::Value* movable_value);

    /**
     * this function is used where allocation is necessary, for example
     * in function arguments when value is moved, there's no place to store the moved value
     * if we don't store a copy of the moved value, the function may make changes to the value
     * and they'll reflect in the original pointer, which we don't want
     * This function takes into account owner of the value, and returns the value that you
     * should pass or use, which may or may not be newly allocated struct
     */
    llvm::Value* move_by_allocate(BaseType* type, Value* value, llvm::Value* elem_pointer, llvm::Value* movable_value);

    /**
     * a helper function
     */
    llvm::Value* move_by_allocate(BaseType* type, Value* value, llvm::Value* movable_value) {
        return move_by_allocate(type, value, nullptr, movable_value);
    }

    /**
     * tells whether, the given value should be mem copied into the pointer, because
     * it's an access chain referencing a non move requiring struct
     */
    static bool requires_memcpy_ref_struct(BaseType* known_type, Value* value);

    /**
     * tries to mem copy referenced struct (if there's one) and returns memory pointer if succeeds
     * otherwise nullptr, memory for struct is allocated as required, unless passed
     */
    llvm::Value* memcpy_ref_struct(BaseType* known_type, Value* value, llvm::Value* memory_pointer, llvm::Type* type);

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
    Value*& eval_comptime(FunctionCall* call, FunctionDeclaration* decl);

    /**
     * stores the value, into the pointer, it's an assignment, it takes care of auto dereferences during
     * assignment
     */
    void assign_store(Value* lhs, llvm::Value* pointer, Value* rhs, llvm::Value* value);

    /**
     * determines destructor function for given element type
     */
    FunctionDeclaration* determine_destructor_for(
            BaseType* elem_type,
            llvm::Function*& func_data
    );

    /**
     * determines clear function for given element type
     */
    FunctionDeclaration* determine_clear_fn_for(
            BaseType* elem_type,
            llvm::Function*& func_data
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
            llvm::Function* func_data,
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
     * this ensures that break and continue instructions work properly by pointing to the given blocks
     */
    void loop_body_gen(Scope& body, llvm::BasicBlock *currentBlock, llvm::BasicBlock *endBlock);

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
     * The safe version of builder.CreateUnreachable
     */
    void CreateUnreachable();

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
    * this operates on two values, left and right
    * this is used by expressions to operate on two values
    */
    llvm::Value *operate(
            Operation op,
            Value *lhs,
            Value *rhs,
            BaseType* lhsType,
            BaseType* rhsType,
            llvm::Value* llvm_lhs,
            llvm::Value* llvm_rhs
    );

    /**
     * this operates on two values, left and right
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

};

#endif