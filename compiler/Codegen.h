// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#ifdef COMPILER_BUILD

#include <memory>
#include <utility>
#include <vector>
#include "ast/utils/Operation.h"
#include "llvmfwd.h"
#include "ast/base/AccessSpecifier.h"
#include "ast/base/BaseTypeKind.h"
#include "ASTDiagnoser.h"
#include "OutputMode.h"
#include <unordered_map>
#include "CodegenEmitterOptions.h"
#include "ClangCodegen.h"
#include "std/chem_string_view.h"
#include "backend/DebugInfoBuilder.h"
#include "compiler/backend/include/LLVMArrayDestructor.h"
#include "CodegenOptions.h"
#include "compiler/backend/LLVMGen.h"

class ASTAllocator;

class CompilerBinder;

class GlobalInterpretScope;

class Codegen;

class Scope;

class NameMangler;

class Value;

class BaseType;

class FunctionType;

class MembersContainer;

class VariablesContainer;

class ExtendableMembersContainerNode;

class FunctionCall;

class FunctionDeclaration;

enum class DestructibleKind {
    Single,
    Array
};

struct Destructible {

    DestructibleKind kind;

    ASTNode* initializer = nullptr;

    llvm::Value* dropFlag = nullptr;

    llvm::Value* pointer = nullptr;

    union {

        ASTNode* container;

        struct {

            unsigned int arrSize;

            BaseType* elem_type;

        } array;

    };

    inline ASTNode* getInitializer() {
        return initializer;
    }

    inline llvm::Value* getDropFlag() {
        return dropFlag;
    }

};

class Codegen : public ASTDiagnoser {
public:

    /**
     * used to get the functions invoked for embedded nodes and values
     */
    CompilerBinder& binder;

    /**
     * global interpret scope, is the scope used for compile time evaluation
     * of code
     */
    GlobalInterpretScope& comptime_scope;

    /**
     * the codegen options determine what kind of code is generated
     */
    CodegenOptions& options;

    /**
     * the name mangler is used to mangle names
     */
    NameMangler& mangler;

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
     * the code generation mode
     */
    OutputMode mode;

    /**
     * the debug info builder
     */
    DebugInfoBuilder di;

    /**
     * llvm generator
     */
    LLVMGen llvm;

    /**
     * contains references to nodes that must be destructed at return
     * the second llvm value is a optional boolean flag, that is checked to see if the
     * destruction should be performed at runtime, because certain values become moved
     * and therefore no destruction is performed on them
     */
    std::vector<Destructible> destruct_nodes;

    /**
     * when a function is evaluated, it's value is stored on this map, so it can be looked up for destruction
     */
    std::unordered_map<FunctionCall*, Value*> evaluated_func_calls;

    /**
     * implicit arguments are provided using provide as statement
     */
    std::unordered_map<chem::string_view, llvm::Value*> implicit_args;

    /**
     * this is the storage place for structs / function llvm values
     * structs store their global type, functions store their callee values
     * this cache is cleared when the module has compiled, prompting
     * declarations to recreate llvm values by external declaring to avoid
     * reusing pointers that have become invalid
     */
    std::unordered_map<ASTNode*, llvm::Value*> mod_ptr_cache;

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
    FunctionTypeBody* current_func_type = nullptr;

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
     */
    Codegen(
            CodegenOptions& options,
            CompilerBinder& binder,
            GlobalInterpretScope& comptime_scope,
            NameMangler& mangler,
            std::string target_triple,
            std::string curr_exe_path,
            bool is_64_bit, // can be determined using static method is_arch_64bit on Codegen,
            bool debug_info,
            ASTAllocator& allocator
    );

    /**
     * implicit cast constant to given type
     */
    static llvm::ConstantInt* implicit_cast_constant(llvm::ConstantInt* value, BaseType* to_type, llvm::Type* to_type_llvm);

    /**
     * initializes the llvm module and context
     */
    void module_init(const chem::string_view& scope_name, const chem::string_view& module_name);

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
     * this will only declare these imported nodes
     */
    void external_declare_nodes(std::vector<ASTNode*>& nodes);

    /**
     * this will implement these imported nodes, please note first declare the external module, then
     * declare current module, only then you can implement the external module
     */
    void external_implement_nodes(std::vector<ASTNode*>& nodes);

    /**
     * when a function ends, this method is called to basically end the block
     * this is because the functions that return void may need a return statement
     * to end current BasicBlock
     */
    void end_function_block(SourceLocation location) {
        DefaultRet(location);
    }

    /**
     * this gets the free function or declares it in the current module
     * @return
     */
    llvm::Function* getFreeFn();

    /**
     * this gets the malloc function or declares it in the current module
     */
    llvm::Function* getMallocFn();

    /**
     * the method to create a function
     */
    llvm::Function *create_function(const std::string_view &name, llvm::FunctionType *type, AccessSpecifier specifier);

    /**
     * the method to create a function
     */
    llvm::Function *create_function(const std::string_view &name, llvm::FunctionType *type, FunctionType* func_type, AccessSpecifier specifier) {
        return create_function(name, type, specifier);
    }

    /**
     * create a nested function
     */
    llvm::Function *create_nested_function(const std::string_view &name, llvm::FunctionType *type, FunctionTypeBody* func_type, Scope &scope);

    /**
     * gets or inserts a function, similar to declaration
     */
    llvm::Function* declare_function(const std::string_view &name, llvm::FunctionType *type, FunctionType* func_type, AccessSpecifier specifier);

    /**
     * get or insert a function prototype for weak symbol, is exported is required
     */
    llvm::Function* declare_weak_function(const std::string_view& name, llvm::FunctionType* type, FunctionTypeBody* func_type, bool is_exported, SourceLocation location);

    /**
     * create a function's basic block, with the given name
     */
    llvm::BasicBlock *createBB(const std::string &name, llvm::Function *fn);

    /**
     * default initialize inherited variables of the given container
     */
    void default_initialize_inherited(VariablesContainer* container, llvm::Type* parent_type, llvm::Value* inst, Value* parent_value);

    /**
     * default initialize the struct inside the given ptr, the parent_value is probably the debug_value
     */
    void default_initialize_struct(ExtendableMembersContainerNode* decl, llvm::Value* ptr, Value* parent_value);

    /**
     * capturing lambdas need to be mutated to capturing function types
     */
    llvm::Value* mutate_capturing_function(BaseType* pure_type, Value* value, llvm::Value* pointer = nullptr);

    /**
     * a struct type with two pointers inside
     */
    llvm::StructType* fat_pointer_type();

    /**
     * packs two pointers into a fat pointer (a struct containing two pointers)
     */
    llvm::AllocaInst* pack_fat_pointer(llvm::Value* first_ptr, llvm::Value* second_ptr, SourceLocation location);

    /**
     * gets the implementation using value and type, where value is a struct value or ref struct
     * and type is a type to an interface, that is implemented by ref struct
     */
    llvm::Value* get_dyn_obj_impl(Value*, BaseType* type);

    /**
     * if this type corresponds to a dynamic object, a fat pointer type will be allocated and returned
     * otherwise nullptr is returned
     */
    llvm::Value* allocate_dyn_obj_based_on_type(BaseType* type, SourceLocation loc);

    /**
     * to change the implementation of dynamic object, this function can be used
     * fat_pointer already having valid struct object for the implementation
     * the implementation is calculated based on the given Value* pointer and type
     */
    void assign_dyn_obj_impl(llvm::Value* fat_pointer, llvm::Value* impl, SourceLocation location);

    /**
     * it finds the implementation for the given value and type automatically
     * returns true if could get implementation and assign it
     */
    bool assign_dyn_obj_impl(Value* value, BaseType* type, llvm::Value* fat_pointer, SourceLocation location);

    /**
     * the implementation represented by this dynamic object will be assigned, along
     * with the given struct object
     * obj is the pointer to the struct that will be assigned
     * the implementation is calculated based on value and type
     */
    void assign_dyn_obj(llvm::Value* fat_pointer, llvm::Value* obj, llvm::Value* impl, SourceLocation location);

    /**
     * if you don't know the implementation, you can use this method, it get's the implementation
     * and when found assigns it, along with the obj, returns true if succeeds otherwise false
     */
    bool assign_dyn_obj(Value* value, BaseType* type, llvm::Value* fat_pointer, llvm::Value* obj, SourceLocation location);

    /**
     * mem copy a struct into the given pointer
     */
    void memcpy_struct(llvm::Type* type, llvm::Value* pointer, llvm::Value* value, SourceLocation location);

    /**
     * writes the return statement for the given return value, also accounting
     * for destructors and other stuff
     */
    void writeReturnStmtFor(Value* returnValue, SourceLocation location);

//    /**
//     * move the value
//     * @param type is the type of value, expected
//     * @param value is the actual value (not llvm though)
//     * @param ptr is where value will be moved to
//     * @param movable_value is the movable value that is retrieved by calling movable_value method
//     * how this works is, the movable value is mem copied into the given pointer, but movable_value is the
//     * memory location of the previous struct, so we call clear function on it, to tell the struct that struct has been
//     * freed
//     * @return true if move was done, otherwise false, if false is returned, caller should pursue of storing
//     * the value inside the given pointer
//     */
//    void move_by_memcpy(ASTNode* container, Value* value, llvm::Value* ptr, llvm::Value* movable_value);

//    /**
//     * move the value
//     * @param type is the type of value, expected
//     * @param value is the actual value (not llvm though)
//     * @param ptr is where value will be moved to
//     * @param movable_value is the movable value that is retrieved by calling movable_value method
//     * how this works is, the movable value is mem copied into the given pointer, but movable_value is the
//     * memory location of the previous struct, so we call clear function on it, to tell the struct that struct has been
//     * freed
//     * @return true if move was done, otherwise false, if false is returned, caller should pursue of storing
//     * the value inside the given pointer
//     */
//    void move_by_memcpy(MembersContainer* container, Value* value, llvm::Value* ptr, llvm::Value* movable_value);

//    /**
//     * move the value
//     * @param type is the type of value, expected
//     * @param value is the actual value (not llvm though)
//     * @param ptr is where value will be moved to
//     * @param movable_value is the movable value that is retrieved by calling movable_value method
//     * how this works is, the movable value is mem copied into the given pointer, but movable_value is the
//     * memory location of the previous struct, so we call clear function on it, to tell the struct that struct has been
//     * freed
//     * @return true if move was done, otherwise false, if false is returned, caller should pursue of storing
//     * the value inside the given pointer
//     */
//    bool move_by_memcpy(BaseType* type, Value* value, llvm::Value* ptr, llvm::Value* movable_value);

//    /**
//     * this function is used where allocation is necessary, for example
//     * in function arguments when value is moved, there's no place to store the moved value
//     * if we don't store a copy of the moved value, the function may make changes to the value
//     * and they'll reflect in the original pointer, which we don't want
//     * This function takes into account owner of the value, and returns the value that you
//     * should pass or use, which may or may not be newly allocated struct
//     */
//    llvm::Value* move_by_allocate(BaseType* type, Value* value, llvm::Value* elem_pointer, llvm::Value* movable_value);

//    /**
//     * a helper function
//     */
//    llvm::Value* move_by_allocate(BaseType* type, Value* value, llvm::Value* movable_value) {
//        return move_by_allocate(type, value, nullptr, movable_value);
//    }

//    /**
//     * tells whether, the given value should be mem copied into the pointer, because
//     * it's an access chain referencing a non move requiring struct
//     */
//    static bool requires_memcpy_ref_struct(BaseType* known_type, Value* value);

//    /**
//     * tries to mem copy referenced struct (if there's one) and returns memory pointer if succeeds
//     * otherwise nullptr, memory for struct is allocated as required, unless passed
//     */
//    llvm::Value* memcpy_ref_struct(BaseType* known_type, Value* value, llvm::Value* memory_pointer, llvm::Type* type);

    /**
     * will only perform a copy of the given value, if the type has a implicit constructor
     */
    llvm::Value* memcpy_shallow_copy(BaseType* known_type, Value* value, llvm::Value* llvm_value);

    /**
     * copy or move struct basically tries to copy a referenced struct, call appropriate functions like premove and postmove
     */
    bool copy_or_move_struct(BaseType* known_type, Value* value, llvm::Value* memory_pointer);

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
    llvm::TargetMachine *setup_for_target(const std::string &TargetTriple, bool isDebug);

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
    void assign_store(Value* lhs, llvm::Value* pointer, Value* rhs, llvm::Value* value, SourceLocation location);

    /**
     * determines destructor function for given element type
     */
    FunctionDeclaration* determine_destructor_for(
            BaseType* elem_type,
            llvm::Function*& func_data
    );

    /**
     * this returns llvm array destructor, which finishes the job
     * you are supposed to call destructor before the returned object destructs on the
     * struct pointer present inside the llvm array destructor,
     * otherwise no actual destruction would take place
     */
    LLVMArrayDestructor loop_array_destructor(
            llvm::Value* allocaInst,
            llvm::Value* array_size,
            llvm::Type* elem_type,
            bool check_for_null,
            SourceLocation location
    );

    /**
     * destruct given alloca instruction, the array size is llvm value
     * destructs an array, you can take the object into a variable, and put extra logic
     * after the destruction
     */
    LLVMArrayDestructor destruct(
            llvm::Value* allocaInst,
            llvm::Function* func_data,
            bool pass_self,
            llvm::Value* array_size,
            BaseType* elem_type,
            bool check_for_null,
            SourceLocation location
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
            SourceLocation location
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
            SourceLocation location
    );

    /**
     * find destruct ref for the given node, node should be var init / function param
     */
    llvm::Value* find_drop_flag(ASTNode* node);

    /**
     * set drop flag for given node, node should be var init / function param
     */
    bool set_drop_flag_for_node(ASTNode* node, bool flag, SourceLocation loc);

    /**
     * this creates the destructible for the given node or none if couldn't create it
     * this node must be the initializer
     */
    std::optional<Destructible> create_destructible_for(ASTNode* node, llvm::Value* oldDropFlag);

    /**
     * this is a better method for enqueuing destructible
     */
    void enqueue_destructible(BaseType* type, ASTNode* initializer, llvm::Value* pointer);

    /**
     * this will execute the job, meaning destruct it
     * @param destructible
     */
    void destruct(Destructible& destructible, SourceLocation location);

    /**
     * conditional destruction of the pair, if this is return statement
     * give the return value, so it can be checked (if node returned, it won't be destructed)
     */
    void conditional_destruct(
            Destructible& pair,
            Value* returnValue,
            SourceLocation location
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
    void CheckNullCondBr(llvm::Value* value, llvm::BasicBlock* TrueBlock, llvm::BasicBlock* FalseBlock, SourceLocation location);

    /**
     * The safe version of builder.CreateBr
     * this will avoid creating multiple terminator instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted to another block
     */
    void CreateBr(llvm::BasicBlock *block, SourceLocation location);

    /**
     * The safe version of builder.CreateUnreachable
     */
    void CreateUnreachable(SourceLocation location);

    /**
     * The safe version of builder.CreateRet
     * this will avoid creating multiple terminator instructions
     * once you call this, no longer can you create branch, or return instructions
     * because you've already shifted to another block
     */
    void CreateRet(llvm::Value *value, SourceLocation location);

    /**
     * creates a default return, it returns void
     * this takes into account the redirect_return required for cleanup blocks in
     * destructors, it should be used instead of CreateRet(nullptr) or CreateRetVoid()
     */
    void DefaultRet(SourceLocation location);

    /**
     * The safe version of builder.CreateCondBr
     * this will avoid creating multiple terminator instructions
     */
    void CreateCondBr(llvm::Value *Cond, llvm::BasicBlock *True, llvm::BasicBlock *FalseMDNode, SourceLocation location);

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
    llvm::Value *implicit_cast(llvm::Value* value, BaseType* to_type, llvm::Type* to_type_llvm);

    /**
     * destructor takes care of deallocating members
     */
    ~Codegen();

};

#endif