// Copyright (c) Chemical Language Foundation 2025.

#ifdef COMPILER_BUILD

#include "ast/base/ASTNode.h"
#include "Codegen.h"
#include "ast/structures/Scope.h"
#include "SelfInvocation.h"
#include "utils/CmdUtils2.h"
#include <utility>
#include "llvmimpl.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/GenericType.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/Namespace.h"
#include <string>
#include <system_error>
#include "lld/Common/Driver.h"
#include "lld/Common/ErrorHandler.h"
#include "ast/types/IntNType.h"
#include "ast/types/DynamicType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/statements/VarInit.h"
#include "ast/values/NullValue.h"
#include "ast/types/CapturingFunctionType.h"
#include <cstdlib>
#include <optional>
#include <llvm/TargetParser/Host.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/LLVMDriver.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DiagnosticInfo.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/OptBisect.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/InitializePasses.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Object/Archive.h>
#include <llvm/Object/ArchiveWriter.h>
#include <llvm/Object/COFF.h>
#include <llvm/Object/COFFImportFile.h>
#include <llvm/Object/COFFModuleDefinition.h>
#include <llvm/PassRegistry.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/TimeProfiler.h>
#include <llvm/Support/Timer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/CodeGenCWrappers.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Instrumentation/ThreadSanitizer.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Utils/AddDiscriminators.h>
#include <llvm/Transforms/Utils/CanonicalizeAliases.h>
#include <llvm/Transforms/Utils/NameAnonGlobals.h>
#include <llvm-c/Core.h>
#include "rang.hpp"
#include "compiler/mangler/NameMangler.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "ast/utils/ASTUtils.h"

Codegen::Codegen(
        CodegenOptions& options,
        CompilerBinder& binder,
        GlobalInterpretScope& comptime_scope,
        NameMangler& mangler,
        std::string target_triple,
        std::string curr_exe_path,
        bool is_64_bit,
        bool debug_info,
        ASTAllocator& allocator
) : ASTDiagnoser(comptime_scope.loc_man), binder(binder), options(options), comptime_scope(comptime_scope), allocator(allocator),
    target_triple(std::move(target_triple)), is64Bit(is_64_bit), clang(target_triple),
    di(comptime_scope.loc_man, nullptr, *this, debug_info), llvm(di), mode(comptime_scope.mode),
    mangler(mangler)
{
    // create llvm context
    ctx = std::make_unique<llvm::LLVMContext>();
    // creating a new ir builder
    builder = new llvm::IRBuilder<>(*ctx);
    // updating the builder
    llvm.updateBuilder(builder);
}

void Codegen::declare_nodes(std::vector<ASTNode*>& nodes) {
    auto& gen = *this;
    for(const auto node : nodes) {
        node->code_gen_declare(gen);
    }
}

void Codegen::compile_nodes(std::vector<ASTNode*>& nodes) {
    auto& gen = *this;
    for (const auto node: nodes) {
        node->code_gen(gen);
    }
}

void Codegen::external_declare_nodes(std::vector<ASTNode*>& nodes_vec) {
    auto& gen = *this;
    for(const auto node : nodes_vec) {
        node->code_gen_external_declare(gen);
    }
}

void Codegen::external_implement_nodes(std::vector<ASTNode*>& nodes) {
    auto& gen = *this;
    for(const auto node : nodes) {
        switch(node->kind()) {
            case ASTNodeKind::GenericFuncDecl:
            case ASTNodeKind::GenericStructDecl:
            case ASTNodeKind::GenericUnionDecl:
            case ASTNodeKind::GenericInterfaceDecl:
            case ASTNodeKind::GenericVariantDecl:
                node->code_gen_declare(gen);
                node->code_gen(gen);
                break;
            case ASTNodeKind::NamespaceDecl:
                external_implement_nodes(node->as_namespace_unsafe()->nodes);
                break;
            default:
                break;
        }
    }
}

bool Codegen::is_arch_64bit(const std::string_view& target_triple) {
    // Parse the target triple string
    llvm::Triple triple(target_triple);
    return triple.isArch64Bit();
}

void Codegen::module_init(const chem::string_view& scope_name, const chem::string_view& module_name) {

    // final module name = 'scope_name' + '-' + 'module_name'
    std::string final_module_name;
    if(!scope_name.empty()) {
        final_module_name.append(1, '-');
        final_module_name.append(scope_name.view());
    }
    final_module_name.append(module_name.view());

    // creating the modules
    module = std::make_unique<llvm::Module>(final_module_name, *ctx);
    diBuilder = std::make_unique<llvm::DIBuilder>(*module, true);
    llvm.di.update_builder(diBuilder.get());

}

void Codegen::createFunctionBlock(llvm::Function *fn) {
    auto entry = createBB("entry", fn);
    SetInsertPoint(entry);
}

llvm::Function::LinkageTypes to_linkage_type(AccessSpecifier specifier) {
    switch (specifier) {
        case AccessSpecifier::Private:
        case AccessSpecifier::Protected:
            return llvm::Function::PrivateLinkage;
        case AccessSpecifier::Public:
            return llvm::Function::ExternalLinkage;
        case AccessSpecifier::Internal:
            return llvm::Function::InternalLinkage;
        default:
#ifdef DEBUG
            throw "unknown access specifier in to linkage type";
#endif
            return llvm::Function::InternalLinkage;

    }
}

/**
 * all functions must be created through this
 * because it creates the function according to the mode
 */
llvm::Function* create_func(
        Codegen& gen,
        const std::string_view &name,
        llvm::FunctionType *type,
        llvm::Function::LinkageTypes linkage
) {
    auto fn = llvm::Function::Create(type, linkage, name, *gen.module);
    fn->setDSOLocal(true);
    // we add the uwtable attribute in debug mode, because we need it to be generated (at least on windows, I think)
    // this will generate the unwind tables, which are required for call stack information
    if(is_debug_or_compl(gen.mode) && !gen.options.fno_unwind_tables) {
        fn->addFnAttr(llvm::Attribute::UWTable);
    } else {
        fn->addFnAttr(llvm::Attribute::NoUnwind);
    }
    return fn;
}

bool get_has_move(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::VarInitStmt:
            return node->as_var_init_unsafe()->get_has_move();
        case ASTNodeKind::FunctionParam:
            return node->as_func_param_unsafe()->get_has_move();
        default:
            return false;
    }
}

llvm::Value* createDropFlag(Codegen & gen, SourceLocation location) {
    // create a boolean flag
    const auto instr = gen.builder->CreateAlloca(gen.builder->getInt1Ty());
    gen.di.instr(instr, location);

    // store true in it, that this value should be destructed
    const auto storeIns = gen.builder->CreateStore(gen.builder->getInt1(true), instr);
    gen.di.instr(storeIns, location);

    // return instruction
    return instr;
}

Destructible create_destructible(
        llvm::Value* pointer,
        llvm::Value* dropFlag,
        ASTNode* node,
        ASTNode* containerNode
) {
    return Destructible{
            .kind = DestructibleKind::Single,
            .initializer = node,
            .dropFlag = dropFlag,
            .pointer = pointer,
            .container = containerNode
    };
}

Destructible create_arr_destructible(
        llvm::Value* pointer,
        llvm::Value* dropFlag,
        ASTNode* node,
        unsigned int arrSize,
        BaseType* elemType
) {
    return Destructible{
            .kind = DestructibleKind::Array,
            .initializer = node,
            .dropFlag = dropFlag,
            .pointer = pointer,
            .array = {
                    .arrSize = arrSize,
                    .elem_type = elemType
            }
    };
}

std::optional<Destructible> create_destructible_with_drop_flag(
        Codegen& gen,
        llvm::Value* pointer,
        ASTNode* node,
        ASTNode* containerNode,
        SourceLocation location,
        llvm::Value* oldDropFlag
) {
    const auto container = containerNode->get_members_container();
    if(container) {
        const auto destructor = container->destructor_func();
        if(destructor) {
            const auto dropFlag = oldDropFlag ? oldDropFlag : get_has_move(node) ? createDropFlag(gen, location) : nullptr;
            return create_destructible(
                    pointer, dropFlag, node, containerNode
            );
        }
    } else if(containerNode->kind() == ASTNodeKind::VariantMember) {
        return create_destructible_with_drop_flag(gen, pointer, node, (ASTNode*) containerNode->as_variant_member_unsafe()->parent(), location, oldDropFlag);
    }
    return std::nullopt;
}

ASTNode* get_destructible_type_node(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::CapturingFunction:{
            const auto instType = type->as_capturing_func_type_unsafe()->instance_type;
            return get_destructible_type_node(instType);
        }
        case BaseTypeKind::Linked:
            return type->as_linked_type_unsafe()->linked;
        case BaseTypeKind::Generic:
            return type->as_generic_type_unsafe()->referenced->linked;
        case BaseTypeKind::Array:
            return get_destructible_type_node(type->as_array_type_unsafe()->elem_type->canonical());
        default:
            return nullptr;
    }
}

std::optional<Destructible> create_destructible_for(
        Codegen& gen,
        ASTNode* node,
        BaseType* nonCanonType,
        llvm::Value* pointer,
        llvm::Value* oldDropFlag
) {
    const auto type = nonCanonType->canonical();
    const auto location = node->encoded_location();
    const auto linkedNode = get_destructible_type_node(type);
    if(linkedNode) {
        if(type->kind() == BaseTypeKind::Array) {
            const auto arrType = type->as_array_type_unsafe();
            const auto container = linkedNode->get_members_container();
            if(container->destructor_func() == nullptr) {
                return std::nullopt;
            }
            const auto dropFlag = oldDropFlag ? oldDropFlag : get_has_move(node) ? createDropFlag(gen, node->encoded_location()) : nullptr;
            return create_arr_destructible(
                    pointer, dropFlag, node, arrType->get_array_size(), linkedNode->known_type()
            );
        } else {
            return create_destructible_with_drop_flag(gen, pointer, node, linkedNode, node->encoded_location(), oldDropFlag);
        }
    }
    return std::nullopt;
}

std::optional<Destructible> Codegen::create_destructible_for(ASTNode* node, llvm::Value* oldDropFlag) {
    switch(node->kind()) {
        case ASTNodeKind::VarInitStmt: {
            const auto init = node->as_var_init_unsafe();
            const auto type = init->known_type();
            return ::create_destructible_for(*this, node, type, init->llvm_ptr, oldDropFlag);
        }
        case ASTNodeKind::FunctionParam:{
            const auto param = node->as_func_param_unsafe();
            const auto argInd = param->calculate_c_or_llvm_index(current_func_type);
            const auto ptr = current_function->getArg(argInd);
            return ::create_destructible_for(*this, node, param->type, ptr, oldDropFlag);
        }
        default:
            return std::nullopt;
    }
}

void Codegen::enqueue_destructible(BaseType* nonCanonType, ASTNode* node, llvm::Value* pointer) {
    auto destructible = ::create_destructible_for(*this, node, nonCanonType, pointer, nullptr);
    if(destructible.has_value()) {
        destruct_nodes.emplace_back(destructible.value());
    }
}

llvm::Function* Codegen::getFreeFn() {
    const auto previousFunc = module->getFunction("free");
    if(previousFunc != nullptr) {
        return previousFunc;
    } else {
        const auto paramType = builder->getPtrTy();
        const auto type = llvm::FunctionType::get(builder->getVoidTy(), { paramType }, false);
        return create_func(*this, "free", type, llvm::Function::LinkageTypes::ExternalLinkage);
    }
}

llvm::Function* Codegen::getMallocFn() {
    const auto previousFunc = module->getFunction("malloc");
    if(previousFunc != nullptr) {
        return previousFunc;
    } else {
        auto& data = comptime_scope.target_data;
        const auto paramType = (data.is_win64 || data.is_64Bit) ? builder->getInt64Ty() : builder->getInt32Ty();
        const auto retTy = builder->getPtrTy();
        const auto type = llvm::FunctionType::get(retTy, { paramType }, false);
        return create_func(*this, "malloc", type, llvm::Function::LinkageTypes::ExternalLinkage);
    }
}

llvm::Function *Codegen::create_function(const std::string_view &name, llvm::FunctionType *type, AccessSpecifier specifier) {
    const auto fn = create_func(*this, name, type, to_linkage_type(specifier));
    current_function = fn;
    createFunctionBlock(current_function);
    return current_function;
}

llvm::Function *Codegen::create_nested_function(const std::string_view &name, llvm::FunctionType *type, FunctionTypeBody* func_type, Scope &scope) {

    auto prev_destruct_nodes = std::move(destruct_nodes);
    const auto prev_destroy_scope = destroy_current_scope;
    const auto prev_block_ended = has_current_block_ended;
    const auto prev_block = builder->GetInsertBlock();
    const auto prev_current_func = current_function;
    const auto prev_func_type = current_func_type;

    destroy_current_scope = true;
    SetInsertPoint(nullptr);
    auto nested_function = create_func(*this, name, type, llvm::Function::PrivateLinkage);
    current_function = nested_function;
    current_func_type = func_type;
    const auto destruct_begin = destruct_nodes.size();
    // this begins the function scope by creating a di subprogram
    di.start_nested_function_scope(func_type, nested_function);
    createFunctionBlock(nested_function);
    // this will queue the destruction of parameters that have been moved into the function
    func_type->queue_destruct_params(*this);
    // this will emit without creating a scope, so direct instructions use the function di subprogram as parent scope
    scope.code_gen(*this, destruct_begin);
    end_function_block(scope.encoded_location());
    // this will end the function scope we started by creating a di subprogram above
    di.end_function_scope();
    has_current_block_ended = prev_block_ended;
    SetInsertPoint(prev_block);
    current_func_type = prev_func_type;
    current_function = prev_current_func;
    destruct_nodes = std::move(prev_destruct_nodes);
    destroy_current_scope = prev_destroy_scope;

    return nested_function;

}

llvm::Function* Codegen::declare_function(const std::string_view &name, llvm::FunctionType *type, FunctionType* func_type, AccessSpecifier specifier) {
    const auto previousFunc = module->getFunction(name);
    if(previousFunc != nullptr) {
        return previousFunc;
    } else {
        const auto fn = create_func(*this, name, type, to_linkage_type(specifier));
        di.declare(func_type, fn);
        return fn;
    }
}

llvm::Function* Codegen::declare_weak_function(const std::string_view& name, llvm::FunctionType* type, FunctionTypeBody* func_type, bool is_exported, SourceLocation location) {
    auto fn = create_func(*this, name, type, llvm::Function::WeakAnyLinkage);
    // if there's no implementation, a stub implementation is required, so if a strong implementation exists it can override it later
//    if(!is_exported) {
        // this will create a di subprogram and set it as current scope
        di.start_function_scope(func_type, fn);
        di.start_scope(location);
        // what happens is an error when there's not a single implementation for an interface
        // because on windows, it requires a stub implementation
        createFunctionBlock(fn);
        // generating an empty return
        const auto returnType = type->getReturnType();
        llvm::Type* retType = type->getReturnType();
        if (retType->isVoidTy()) {
            CreateRet(nullptr, location);
        } else if (retType->isIntegerTy()) {
            CreateRet(llvm::ConstantInt::get(retType, 0), location);
        } else if (retType->isFloatingPointTy()) {
            CreateRet(llvm::ConstantFP::get(retType, 0.0), location);
        } else if (retType->isPointerTy()) {
            CreateRet(llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(retType)), location);
        } else {
            // For other return types (e.g. structs), return an undefined value.
            CreateRet(llvm::UndefValue::get(retType), location);
        }
        // this will end the di subprogram
        di.end_scope();
        di.end_function_scope();
//    }
    return fn;
}

Value*& Codegen::eval_comptime(FunctionCall* call, FunctionDeclaration* decl) {
    auto found = evaluated_func_calls.find(call);
    if(found != evaluated_func_calls.end()) {
        return found->second;
    } else {
        auto prev = comptime_scope.current_func_type;
        comptime_scope.current_func_type = current_func_type;
        auto ret = decl->call(&comptime_scope, allocator, call, nullptr, false);
        comptime_scope.current_func_type = prev;
        if(!ret) {
//            warn("compile time function didn't return a value", decl);
            evaluated_func_calls[call] = nullptr;
            return evaluated_func_calls[call];
        }
        auto eval = ret->evaluated_value(comptime_scope);
        evaluated_func_calls[call] = eval;
        return evaluated_func_calls[call];
    }
}

bool has_allocated_storage(Value* value) {
    if(value->is_func_call()) {
        // values (references) returned from a function call, do not have
        // allocated storage
        return false;
    }
    const auto linked = value->linked_node();
    if(!linked) return true;
    switch(linked->kind()) {
        case ASTNodeKind::FunctionParam:
            return false;
        default:
            return true;
    }
}

void Codegen::assign_store(Value* lhs, llvm::Value* pointer, Value* rhs, llvm::Value* value, SourceLocation location) {
    if(lhs) {
        const auto lhsType = lhs->create_type(allocator)->canonical();
        if (!assign_dyn_obj(rhs, lhsType, pointer, value, location)) {

            const auto value_pure = rhs->create_type(allocator);
            const auto derefType = value_pure->getAutoDerefType(lhsType);

            // this handles the case where value is a reference, however
            // user is assigning it to a non reference, we auto deref the reference
            auto Val = value;
            if(derefType) {
                const auto loadInst = builder->CreateLoad(derefType->llvm_type(*this), value);
                di.instr(loadInst, rhs);
                Val = loadInst;
            } else {

                // now we check if we need to auto deref the lhs type
                // for example user is assigning to lhsType = &mut int
                // so we need to load the pointer, before we use it as lhs
                // and do a store instruction on it
                // in rhs we have just int which doesn't matter
                // we'll handle this by reversing the logic of auto dereference
                if(lhsType->kind() == BaseTypeKind::Reference && has_allocated_storage(lhs)) {
                    const auto loadInst = builder->CreateLoad(builder->getPtrTy(), pointer);
                    di.instr(loadInst, lhs);
                    pointer = loadInst;
                }

                // can the value be casted to lhs
                // for example i64 is the lhs, however value is i32
                // so we extend that value
                Val = implicit_cast(Val, lhsType, lhsType->llvm_type(*this));

            }


            // TODO not using the correct location for debugging
            const auto storeInst = builder->CreateStore(Val, pointer);
            di.instr(storeInst, lhs);
        }
    } else {
        // TODO not using the correct location for debugging
        const auto storeInst = builder->CreateStore(value, pointer);
        di.instr(storeInst, rhs);
    }
}

LLVMArrayDestructor::~LLVMArrayDestructor() {
    const auto builder = gen.builder;
    // we just called destructor on struct pointer, if it's equal to first element, then we are done
    auto result = builder->CreateICmpEQ(structPtr, firstEle);
    builder->CreateCondBr(result, end_block, body_block);
    // setting end block as insert point so further user code can go into it
    builder->SetInsertPoint(end_block);
}

LLVMArrayDestructor Codegen::loop_array_destructor(
        llvm::Value* allocaInst,
        llvm::Value* array_size,
        llvm::Type* elem_type,
        bool check_for_null,
        SourceLocation location
) {
    auto& gen = *this;
    // initial variables
    const auto builder = gen.builder;
    auto& ctx = *gen.ctx;
    const auto inbounds = gen.inbounds;
    const auto current_function = gen.current_function;

    // the pointer to the first element in array
    auto firstEle = allocaInst;
    // the pointer to last element in array
    auto lastEle = builder->CreateGEP(elem_type, firstEle, { array_size }, "", inbounds);
    // the current block in which we are
    auto current_block = builder->GetInsertBlock();
    // the body block contains the loop that calls the destructors
    auto body_block = llvm::BasicBlock::Create(ctx, "", current_function);
    // the end block is where all destructors have been called, user's code is written...
    auto end_block = llvm::BasicBlock::Create(ctx, "", current_function);

    if(check_for_null) {
        // check if given pointer is null and send user to end block if it is
        gen.CheckNullCondBr(allocaInst, end_block, body_block, location);
    } else {
        // sending directly to body block (no null check)
        gen.CreateBr(body_block, location);
    };

    // generating the code for the body
    gen.SetInsertPoint(body_block);
    auto PHI = builder->CreatePHI(builder->getPtrTy(), 2);
    auto structPtr = builder->CreateGEP(elem_type, PHI, { builder->getInt32(-1) }, "", inbounds);
    // if coming from the current block, last element pointer is taken (the ptr to last element in array)
    PHI->addIncoming(lastEle, current_block);
    // if coming from the body block (next iteration), we take -1 of the previous array (decrementing pointer)
    PHI->addIncoming(structPtr, body_block);

    // returning array destructor
    return LLVMArrayDestructor(gen, structPtr, firstEle, body_block, end_block);

}

LLVMArrayDestructor Codegen::destruct(
        llvm::Value* allocaInst,
        llvm::Function* destr_func_data,
        bool pass_self,
        llvm::Value* array_size,
        BaseType* elem_type,
        bool check_for_null,
        SourceLocation location
) {
    const auto finalize = loop_array_destructor(allocaInst, array_size, elem_type->llvm_type(*this), check_for_null, location);
    // calling the destructor
    std::vector<llvm::Value*> args;
    if(pass_self) {
        args.emplace_back(finalize.structPtr);
    }
    const auto callInst = builder->CreateCall(destr_func_data, args, "");
    di.instr(callInst, location);
    return finalize;
}

FunctionDeclaration* Codegen::determine_destructor_for(
        BaseType* elem_type,
        llvm::Function*& func_data
) {

    //checking if struct has a destructor function
    auto container = elem_type->get_members_container();
    if(!container) {
        return nullptr;
    }

    auto chosenFunc = container->destructor_func();
    if(!chosenFunc) {
        return nullptr;
    }

    // determine the function type and callee
    func_data = chosenFunc->llvm_func(*this);

    return chosenFunc;
}

void Codegen::destruct(
        llvm::Value* allocaInst,
        llvm::Value* array_size,
        BaseType* elem_type,
        bool check_for_null,
        SourceLocation location
) {
    // determining destructor
    llvm::Function* func_data;
    auto destructorFunc = determine_destructor_for(elem_type, func_data);
    if(!destructorFunc) return;
    // calling destruct
    const auto finalize = destruct(
            allocaInst,
            func_data,
            destructorFunc->has_self_param(),
            array_size,
            elem_type,
            check_for_null,
            location
    );
}

void Codegen::destruct(
        llvm::Value* allocaInst,
        unsigned int array_size,
        BaseType* elem_type,
        SourceLocation location
) {
    destruct(allocaInst, builder->getInt32(array_size), elem_type, false, location);
}

llvm::BasicBlock *Codegen::createBB(const std::string &name, llvm::Function *fn) {
    return llvm::BasicBlock::Create(*ctx, name, fn);
}

llvm::Value* Codegen::mutate_capturing_function(BaseType* pure_type, Value* value, llvm::Value* pointer) {
    // function takes a capturing function type, and we have a lambda
    if(pure_type->kind() == BaseTypeKind::CapturingFunction && value->kind() == ValueKind::LambdaFunc) {
        const auto capFuncTy = pure_type->as_capturing_func_type_unsafe();
        const auto instanceNode = capFuncTy->instance_type->get_direct_linked_canonical_node();
        if(instanceNode->kind() == ASTNodeKind::StructDecl) {
            const auto makeFn = instanceNode->as_struct_def_unsafe()->child("make");
            if(makeFn->kind() == ASTNodeKind::FunctionDecl) {
                const auto makeFnDecl = makeFn->as_function_unsafe();
                if(makeFnDecl->is_comptime()) {
                    auto called = call_with_arg(makeFnDecl, value, pure_type, allocator, *this);
                    const auto evaluated = eval_comptime(called, makeFnDecl);
                    if(pointer && evaluated->kind() == ValueKind::FunctionCall) {
                        const auto call = evaluated->as_func_call_unsafe();
                        return call->llvm_chain_value(*this, pointer);
                    } else {
                        return evaluated->llvm_value(*this);
                    }
                }
            }
        }
    }
    return nullptr;
}

llvm::StructType* Codegen::fat_pointer_type() {
    return llvm::StructType::get(*ctx, {builder->getPtrTy(), builder->getPtrTy()} );
}

llvm::AllocaInst* Codegen::pack_fat_pointer(llvm::Value* first_ptr, llvm::Value* second_ptr, SourceLocation location) {
    // create a struct with two pointers
    auto structType = fat_pointer_type();
    const auto allocated = builder->CreateAlloca(structType);
    di.instr(allocated, location);
    // store lambda function pointer in the first variable
    auto first = builder->CreateGEP(structType, allocated, {builder->getInt32(0), builder->getInt32(0)}, "", inbounds);
    const auto storeInst1 = builder->CreateStore(first_ptr, first);
    di.instr(storeInst1, location);
    // store a pointer to a struct that contains captured variables in the second variable
    auto second = builder->CreateGEP(structType, allocated, {builder->getInt32(0), builder->getInt32(1)}, "", inbounds);
    const auto storeInst2 = builder->CreateStore(second_ptr, second);
    di.instr(storeInst2, location);
    return allocated;
}

llvm::Value* Codegen::get_dyn_obj_impl(Value* value, BaseType* type) {
    if(!type) return nullptr;
    const auto interface = type->linked_dyn_interface();
    if(!interface) return nullptr;
    const auto linked = value->known_type();
    if(linked->isStructLikeType()) {
        const auto def = linked->linked_struct_def();
        if(def) {
            const auto found = interface->llvm_global_vtable(*this, def);
            if(found != nullptr) {
                return found;
            } else {
                error(value) << "couldn't find the implementation of struct '" << def->name_view() << "' using value '" << value->representation() << "' for interface '" << interface->name_view() << "'";
            }
        }
    }
    return nullptr;
}

llvm::Value* Codegen::allocate_dyn_obj_based_on_type(BaseType* type, SourceLocation loc) {
    if(!type) return nullptr;
    const auto interface = type->linked_dyn_interface();
    if(!interface) return nullptr;
    const auto allocaInst = builder->CreateAlloca(fat_pointer_type());
    di.instr(allocaInst, loc);
    return allocaInst;
}

void Codegen::assign_dyn_obj_impl(llvm::Value* fat_pointer, llvm::Value* impl, SourceLocation location) {
    auto second = builder->CreateGEP(fat_pointer_type(), fat_pointer, {builder->getInt32(0), builder->getInt32(1)}, "", inbounds);
    const auto storeInst = builder->CreateStore(impl, second);
    di.instr(storeInst, location);
}

bool Codegen::assign_dyn_obj_impl(Value* value, BaseType* type, llvm::Value* fat_pointer, SourceLocation location) {
    auto found = get_dyn_obj_impl(value, type);
    if(found) {
        assign_dyn_obj_impl(fat_pointer, found, location);
        return true;
    }
    return false;
}

void Codegen::assign_dyn_obj(llvm::Value* fat_pointer, llvm::Value* obj, llvm::Value* impl, SourceLocation location) {
    auto first = builder->CreateGEP(fat_pointer_type(), fat_pointer, {builder->getInt32(0), builder->getInt32(0)}, "", inbounds);
    const auto storeInst1 = builder->CreateStore(obj, first);
    di.instr(storeInst1, location);
    auto second = builder->CreateGEP(fat_pointer_type(), fat_pointer, {builder->getInt32(0), builder->getInt32(1)}, "", inbounds);
    const auto storeInst2 = builder->CreateStore(impl, second);
    di.instr(storeInst2, location);
}

bool Codegen::assign_dyn_obj(Value* value, BaseType* type, llvm::Value* fat_pointer, llvm::Value* obj, SourceLocation location) {
    auto found = get_dyn_obj_impl(value, type);
    if(found) {
        assign_dyn_obj(fat_pointer, obj, found, location);
        return true;
    }
    return false;
}

void Codegen::memcpy_struct(llvm::Type* type, llvm::Value* pointer, llvm::Value* value, SourceLocation location) {
    llvm::MaybeAlign m;
    const auto alloc_size = module->getDataLayout().getTypeAllocSize(type);
    const auto callInst = builder->CreateMemCpy(pointer, m, value, m, alloc_size);
    di.instr(callInst, location);
}

//void Codegen::move_by_memcpy(ASTNode* node, Value* value_ptr, llvm::Value* elem_ptr, llvm::Value* movable_value) {
//    const auto node_kind = node->kind();
//    if(node_kind == ASTNodeKind::UnnamedStruct || node_kind == ASTNodeKind::UnnamedUnion) {
//#ifdef DEBUG
//        throw std::runtime_error("requires implementing the union or struct move when unnamed");
//#endif
//    }
//    const auto container = node->as_members_container();
//    if(!container) {
//        return;
//    }
//    move_by_memcpy(container, value_ptr, elem_ptr, movable_value);
//}

//void Codegen::move_by_memcpy(MembersContainer* container, Value* value_ptr, llvm::Value* elem_ptr, llvm::Value* movable_value) {
//    auto& gen = *this;
//    auto& value = *value_ptr;
//    auto pre_move_func = container->pre_move_func();
//    if(pre_move_func) {
//        auto linked = value.linked_node();
//        auto k = linked->kind();
//        if (k == ASTNodeKind::VarInitStmt || k == ASTNodeKind::FunctionParam) {
//            gen.memcpy_struct(container->llvm_type(gen), elem_ptr, movable_value, value_ptr->encoded_location());
//            return;
//        }
//        const auto callInst = gen.builder->CreateCall(pre_move_func->llvm_func(), { elem_ptr, movable_value });
//        gen.di.instr(callInst, value_ptr);
//        return;
//    }
//    gen.memcpy_struct(container->llvm_type(gen), elem_ptr, movable_value, value_ptr->encoded_location());
//    auto linked = value.linked_node();
//    auto k = linked->kind();
//    if (k == ASTNodeKind::VarInitStmt || k == ASTNodeKind::FunctionParam) {
//        return;
//    }
//    auto clear_func = container->clear_func();
//    if (clear_func) {
//        // now we can move the previous arg, since we copied it's contents
//        ::call_clear_fn(gen, clear_func, movable_value, value_ptr->encoded_location());
//    }
//}

//bool Codegen::move_by_memcpy(BaseType* type, Value* value_ptr, llvm::Value* elem_ptr, llvm::Value* movable_value) {
//    auto& value = *value_ptr;
//    auto known_t = value.pure_type_ptr();
//    auto movable = known_t->get_direct_linked_node();
//    if(!movable || !movable->isStoredStructType(movable->kind())) {
//        return false;
//    }
//    move_by_memcpy(movable, value_ptr, elem_ptr, movable_value);
//    return true;
//}

//llvm::Value* Codegen::move_by_allocate(BaseType* type, Value* value, llvm::Value* elem_pointer, llvm::Value* movable_value) {
//    const auto linked = value->linked_node();
//    const auto linked_kind = linked->kind();
//    if((linked_kind == ASTNodeKind::VarInitStmt && !linked->as_var_init_unsafe()->is_const()) || linked_kind == ASTNodeKind::FunctionParam) {
//        // we can pass directly, as the original nodes are in-accessible after this move
//        return movable_value;
//    }
//    const auto pure = value->pure_type_ptr();
//    const auto kind = pure->kind();
//    const auto linked_node = pure->get_direct_linked_node();
//    if(!linked_node) {
//        // we can pass directly, as there's no node, it's probably a native type like int or long, where generic is being moved, and int is used in generic type parameter
//        return movable_value;
//    }
//    const auto linked_node_kind = linked_node->kind();
//    if(!ASTNode::isStoredStructType(linked_node_kind)) {
//        // we can pass directly, as the node is not a stored struct type, what could it be except typealias
//        return movable_value;
//    }
//
//    auto new_struct = elem_pointer;
//    if(!new_struct) {
//        const auto allocaInst = builder->CreateAlloca(type->llvm_type(*this));
//        di.instr(allocaInst, type);
//        new_struct = allocaInst;
//    }
//    move_by_memcpy(linked_node, value, new_struct, movable_value);
//    return new_struct;
//}

void Codegen::print_to_console() {
    module->print(llvm::outs(), nullptr, false, true);
}

void Codegen::CheckNullCondBr(llvm::Value* value, llvm::BasicBlock* TrueBlock, llvm::BasicBlock* FalseBlock, SourceLocation location) {
    auto is_null = builder->CreateICmpEQ(value, NullValue::null_llvm_value(*this));
    CreateCondBr(is_null, TrueBlock, FalseBlock, location);
}

void Codegen::SetInsertPoint(llvm::BasicBlock *block) {
    has_current_block_ended = false;
    builder->SetInsertPoint(block);
}

void Codegen::CreateBr(llvm::BasicBlock *block, SourceLocation location) {
    if (!has_current_block_ended) {
        const auto brInst = builder->CreateBr(block);
        di.instr(brInst, location);
        has_current_block_ended = true;
    }
}

void Codegen::CreateUnreachable(SourceLocation location) {
    if(!has_current_block_ended) {
        const auto unrInst = builder->CreateUnreachable();
        di.instr(unrInst, location);
        has_current_block_ended = true;
    }
}

void Codegen::CreateRet(llvm::Value *value, SourceLocation location) {
    if (!has_current_block_ended) {
#ifdef DEBUG
        if(current_function) {
            if (value == nullptr && !current_function->getReturnType()->isVoidTy()) {
                warn("creating a return void when the function's return ", location);
            } else if (value != nullptr && current_function->getReturnType()->getTypeID() != value->getType()->getTypeID()) {
                warn("return value type is different than the function's return type", location);
            }
        }
#endif
        const auto retInst = builder->CreateRet(value);
        di.instr(retInst, location);
        has_current_block_ended = true;
    }
}

void Codegen::DefaultRet(SourceLocation location) {
    if(redirect_return) {
        CreateBr(redirect_return, location);
    } else {
        CreateRet(nullptr, location);
    }
}

void Codegen::CreateCondBr(llvm::Value *Cond, llvm::BasicBlock *True, llvm::BasicBlock *FalseMDNode, SourceLocation location) {
    if (!has_current_block_ended) {
        const auto condType = Cond->getType();
#ifdef DEBUG
        if(!condType->isIntegerTy()) {
            throw std::runtime_error("only integer / boolean values can be used as condition");
        }
#endif
        const auto bitWidth = condType->getIntegerBitWidth();
        if(bitWidth != 1) {
            // since its a different integer bitwidth, let's compare with zero as condition
            Cond = builder->CreateICmpNE(Cond, builder->getIntN(bitWidth, 0));
        }
        const auto brInst = builder->CreateCondBr(Cond, True, FalseMDNode);
        di.instr(brInst, location);
        has_current_block_ended = true;
    }
}

void Codegen::loop_body_gen(Scope& body, llvm::BasicBlock *currentBlock, llvm::BasicBlock *endBlock) {
    auto prev_loop_continue = current_loop_continue;
    auto prev_loop_exit = current_loop_exit;
    current_loop_continue = currentBlock;
    current_loop_exit = endBlock;
    body.code_gen(*this);
    current_loop_continue = prev_loop_continue;
    current_loop_exit = prev_loop_exit;
}

llvm::Value *Codegen::implicit_cast(llvm::Value* value, BaseType* to_type, llvm::Type* to_type_llvm) {
    const auto value_type = value->getType();
    const auto exp_type = to_type_llvm;
    if(value_type->isIntegerTy() && exp_type->isIntegerTy()) {
        const auto fromIntTy = (llvm::IntegerType*) value_type;
        const auto toIntTy = (llvm::IntegerType*) exp_type;
        if(fromIntTy->getBitWidth() < toIntTy->getBitWidth()) {
            if (to_type->kind() == BaseTypeKind::IntN && to_type->as_intn_type_unsafe()->is_unsigned()) {
                return builder->CreateZExt(value, toIntTy);
            } else {
                return builder->CreateSExt(value, toIntTy);
            }
        } else if(fromIntTy->getBitWidth() > toIntTy->getBitWidth()) {
            return builder->CreateTrunc(value, toIntTy);
        }
    }
    return value;
}

Codegen::~Codegen() {
    delete builder;
}

using namespace llvm;
using namespace llvm::sys;

// Configures TargetOptions for a debug build.
// This function enables as much debug-friendly code generation as possible.
// Note: Some options (like disabling fast/global ISel) may increase compile times,
// so they are provided as commented-out alternatives.
void configureDebugTargetOptions(TargetOptions &Opts) {

    // Emit DWARF debug frame section for unwind tables.
    Opts.ForceDwarfFrameSection = 1;

    // --- Debug Entry and Variable Information ---
    // Force production of debug entry values so that variable locations
    // are tracked accurately even if the target doesn't officially support it.
    Opts.EnableDebugEntryValues = 1;

    // Enable experimental variable location tracking for more precise debug info.
    // (May increase compile time slightly.)
    Opts.ValueTrackingVariableLocations = 1;

    // --- Optimization Adjustments ---
    // Tail call optimizations can interfere with proper call-stack reconstruction.
    // Disable guaranteed tail call optimization in debug builds.
    Opts.GuaranteedTailCallOpt = 0;

}


TargetMachine * Codegen::setup_for_target(const std::string &TargetTriple, bool isDebug) {

    // Initialize the target registry etc.
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    std::string Error = "unknown error related to target lookup";
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
        std::cerr << rang::fg::red << "error with target lookup: " << Error << rang::fg::reset << std::endl;
        return nullptr;
    }

    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;

    if(isDebug) {
        configureDebugTargetOptions(opt);
    }

    auto RM = std::optional<Reloc::Model>();

    // we use by default PIE for linux, since that's what the linker expects
    if (!options.no_pie && TargetTriple.find("linux") != std::string::npos) {
        RM = llvm::Reloc::PIC_;
    }

    auto TheTargetMachine = Target->createTargetMachine(
            TargetTriple, CPU, Features, opt, RM);

    module->setDataLayout(TheTargetMachine->createDataLayout());
    module->setTargetTriple(TargetTriple);

    return TheTargetMachine;

}

// LLVM's time profiler can provide a hierarchy view of the time spent
// in each component. It generates JSON report in Chrome's "Trace Event"
// format. So the report can be easily visualized by the Chrome browser.
struct TimeTracerRAII {
    // Granularity in ms
    unsigned TimeTraceGranularity;
    StringRef TimeTraceFile, OutputFilename;
    bool EnableTimeTrace;

    TimeTracerRAII(StringRef ProgramName, StringRef OF)
            : TimeTraceGranularity(500U),
              TimeTraceFile(std::getenv("ZIG_LLVM_TIME_TRACE_FILE")),
              OutputFilename(OF),
              EnableTimeTrace(!TimeTraceFile.empty()) {
        if (EnableTimeTrace) {
            if (const char *G = std::getenv("ZIG_LLVM_TIME_TRACE_GRANULARITY"))
                TimeTraceGranularity = (unsigned)std::atoi(G);

            llvm::timeTraceProfilerInitialize(TimeTraceGranularity, ProgramName);
        }
    }

    ~TimeTracerRAII() {
        if (EnableTimeTrace) {
            if (auto E = llvm::timeTraceProfilerWrite(TimeTraceFile, OutputFilename)) {
                handleAllErrors(std::move(E), [&](const StringError &SE) {
                    errs() << SE.getMessage() << "\n";
                });
                return;
            }
            timeTraceProfilerCleanup();
        }
    }
};

bool save_as_file_type(
        Codegen* gen,
        CodegenEmitterOptions* options,
        char** error_message
) {

    auto TheTargetMachine = gen->setup_for_target(gen->target_triple, options->is_debug);
    if(TheTargetMachine == nullptr) {
        return false;
    }

    auto& target_machine = *TheTargetMachine;
    auto& llvm_module = *gen->module;

    raw_fd_ostream *dest_asm_ptr = nullptr;
    raw_fd_ostream *dest_obj_ptr = nullptr;
    raw_fd_ostream *dest_bitcode_ptr = nullptr;

    if (options->asm_path) {
        std::error_code EC;
        dest_asm_ptr = new(std::nothrow) raw_fd_ostream(options->asm_path, EC, sys::fs::OF_None);
        if (EC) {
            *error_message = strdup((const char *)StringRef(EC.message()).bytes_begin());
            return true;
        }
    }
    if (options->obj_path) {
        std::error_code EC;
        dest_obj_ptr = new(std::nothrow) raw_fd_ostream(options->obj_path, EC, sys::fs::OF_None);
        if (EC) {
            *error_message = strdup((const char *)StringRef(EC.message()).bytes_begin());
            return true;
        }
    }
    if (options->bitcode_path) {
        std::error_code EC;
        dest_bitcode_ptr = new(std::nothrow) raw_fd_ostream(options->bitcode_path, EC, sys::fs::OF_None);
        if (EC) {
            *error_message = strdup((const char *)StringRef(EC.message()).bytes_begin());
            return true;
        }
    }

    std::unique_ptr<raw_fd_ostream> dest_asm(dest_asm_ptr),
            dest_obj(dest_obj_ptr),
            dest_bitcode(dest_bitcode_ptr);

    auto PID = sys::Process::getProcessId();
    std::string ProcName = "chem-";
    ProcName += std::to_string(PID);
    TimeTracerRAII TimeTracer(ProcName, options->bitcode_path ? options->bitcode_path : options->obj_path);
    TheTargetMachine->setO0WantsFastISel(true);

    // Pipeline configurations
    PipelineTuningOptions pipeline_opts;
    pipeline_opts.LoopUnrolling = !options->is_debug;
    pipeline_opts.SLPVectorization = !options->is_debug;
    pipeline_opts.LoopVectorization = !options->is_debug;
    pipeline_opts.LoopInterleaving = !options->is_debug;
    pipeline_opts.MergeFunctions = !options->is_debug;

    // Instrumentations
    PassInstrumentationCallbacks instr_callbacks;
    StandardInstrumentations std_instrumentations(llvm_module.getContext(), false);
    std_instrumentations.registerCallbacks(instr_callbacks);

    std::optional<PGOOptions> opt_pgo_options = {};
    PassBuilder pass_builder(&target_machine, pipeline_opts,
                             opt_pgo_options, &instr_callbacks);


    LoopAnalysisManager loop_am;
    FunctionAnalysisManager function_am;
    CGSCCAnalysisManager cgscc_am;
    ModuleAnalysisManager module_am;

    // Register the AA manager first so that our version is the one used
    function_am.registerPass([&] {
        return pass_builder.buildDefaultAAPipeline();
    });

    Triple target_triple(llvm_module.getTargetTriple());
    auto tlii = std::make_unique<TargetLibraryInfoImpl>(target_triple);
    function_am.registerPass([&] { return TargetLibraryAnalysis(*tlii); });

    // Initialize the AnalysisManagers
    pass_builder.registerModuleAnalyses(module_am);
    pass_builder.registerCGSCCAnalyses(cgscc_am);
    pass_builder.registerFunctionAnalyses(function_am);
    pass_builder.registerLoopAnalyses(loop_am);
    pass_builder.crossRegisterProxies(loop_am, function_am,
                                      cgscc_am, module_am);

    // IR verification
    if (options->assertions_on) {
        // Verify the input
        pass_builder.registerPipelineStartEPCallback(
                [](ModulePassManager &module_pm, OptimizationLevel OL) {
                    module_pm.addPass(VerifierPass());
                });
        // Verify the output
        pass_builder.registerOptimizerLastEPCallback(
                [](ModulePassManager &module_pm, OptimizationLevel OL) {
                    module_pm.addPass(VerifierPass());
                });
    }

    // Passes specific for release build
    if (!options->is_debug) {
        pass_builder.registerPipelineStartEPCallback(
                [](ModulePassManager &module_pm, OptimizationLevel OL) {
                    module_pm.addPass(
                            createModuleToFunctionPassAdaptor(AddDiscriminatorsPass()));
                });
    }

    // Thread sanitizer
    if (options->tsan) {
        pass_builder.registerOptimizerLastEPCallback([](ModulePassManager &module_pm, OptimizationLevel level) {
            module_pm.addPass(ModuleThreadSanitizerPass());
            module_pm.addPass(createModuleToFunctionPassAdaptor(ThreadSanitizerPass()));
        });
    }

    ModulePassManager module_pm;
    OptimizationLevel opt_level;
    // Setting up the optimization level
    if (options->is_debug)
        opt_level = OptimizationLevel::O0;
    else if (options->is_small)
        opt_level = OptimizationLevel::Oz;
    else
        opt_level = OptimizationLevel::O3;

    // Initialize the PassManager
    if (opt_level == OptimizationLevel::O0) {
        module_pm = pass_builder.buildO0DefaultPipeline(opt_level, options->lto);
    } else if (options->lto) {
        module_pm = pass_builder.buildLTOPreLinkDefaultPipeline(opt_level);
    } else {
        module_pm = pass_builder.buildPerModuleDefaultPipeline(opt_level);
    }

    // Unfortunately we don't have new PM for code generation
    legacy::PassManager codegen_pm;
    codegen_pm.add(
            createTargetTransformInfoWrapperPass(target_machine.getTargetIRAnalysis()));

    if (dest_obj) {
        if (target_machine.addPassesToEmitFile(codegen_pm, *dest_obj, nullptr, CGFT_ObjectFile)) {
            *error_message = strdup("TargetMachine can't emit an object file");
            return true;
        }
    }
    if (dest_asm) {
        if (target_machine.addPassesToEmitFile(codegen_pm, *dest_asm, nullptr, CGFT_AssemblyFile)) {
            *error_message = strdup("TargetMachine can't emit an assembly file");
            return true;
        }
    }

    // Optimization phase
    module_pm.run(llvm_module, module_am);

    // Code generation phase
    codegen_pm.run(llvm_module);

    if (options->ir_path) {
        char* message = nullptr;
        if (LLVMPrintModuleToFile(wrap(&llvm_module), options->ir_path, &message)) {
            return false;
        }
    }
    if (options->bitcode_path) {
        WriteBitcodeToFile(llvm_module, *dest_bitcode_ptr);
    }

    if (options->time_report) {
        TimerGroup::printAll(errs());
    }

    return true;

}

void configure_emitter_opts(OutputMode mode, CodegenEmitterOptions* options) {
    switch(mode) {
        case OutputMode::Debug:
        case OutputMode::DebugQuick:
            options->is_debug = true;
            options->lto = false;
            options->is_small = false;
            return;
        case OutputMode::DebugComplete:
            options->is_debug = true;
            options->lto = false;
            options->is_small = false;
            options->assertions_on = true;
            return;
        case OutputMode::ReleaseFast:
            options->is_debug = false;
            options->lto = true;
            options->is_small = false;
        case OutputMode::ReleaseSmall:
            options->is_debug = false;
            options->lto = true;
            options->is_small = true;
            break;
#ifdef DEBUG
        default:
            throw std::runtime_error("[lab] unknown output mode");
#endif
    }
}

bool Codegen::save_with_options(CodegenEmitterOptions* options) {
    char* error_message = nullptr;
    bool result = save_as_file_type(this, options, &error_message);
    if(error_message) {
        std::cerr << rang::fg::red << "error: " << error_message << ", when emitting files" << rang::fg::reset << std::endl;
    }
    return result;
}

bool Codegen::save_to_assembly_file(std::string &out_path, OutputMode mode) {
    CodegenEmitterOptions options;
    configure_emitter_opts(mode, &options);
    options.asm_path = out_path.data();
    return save_with_options(&options);
}

bool Codegen::save_to_object_file(std::string &out_path, OutputMode mode) {
    CodegenEmitterOptions options;
    configure_emitter_opts(mode, &options);
    options.obj_path = out_path.data();
    return save_with_options(&options);
}

bool Codegen::save_to_bc_file(std::string &out_path, OutputMode mode) {
    CodegenEmitterOptions options;
    configure_emitter_opts(mode, &options);
    options.bitcode_path = out_path.data();
    return save_with_options(&options);
}

bool Codegen::save_to_ll_file_for_debugging(const std::string &out_path) const {
    // This code allows printing llvm ir, even if it's buggy code
    std::error_code errorCode;
    llvm::raw_fd_ostream outLL(out_path, errorCode);
    module->print(outLL, nullptr, false, true);
    outLL.close();
    return true;
}

#ifdef CLANG_LIBS

int chemical_clang_main(int argc, char **argv);

int ChemLlvmAr_main(int argc, char **argv);

int chemical_clang_main2(const std::vector<std::string> &command_args) {
    char** pointers = convert_to_pointers(command_args);
    // invocation
    auto result = chemical_clang_main(command_args.size(), pointers);
    free_pointers(pointers, command_args.size());
    return result;
}

int llvm_ar_main2(const std::span<chem::string_view> &command_args) {
    char** pointers = convert_to_pointers(command_args);
    // invocation
    auto result = ChemLlvmAr_main(command_args.size(), pointers);
    free_pointers(pointers, command_args.size());
    return result;
}

int Codegen::invoke_clang(const std::vector<std::string> &command_args) {
    return chemical_clang_main2(command_args);
}

#endif

#ifdef LLD_LIBS

using namespace lld;
using namespace llvm;
using namespace llvm::sys;

LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(elf)
LLD_HAS_DRIVER(mingw)
LLD_HAS_DRIVER(macho)
LLD_HAS_DRIVER(wasm)

int lld_main(int argc, char **argv, const llvm::ToolContext &) {

    sys::Process::UseANSIEscapeCodes(true);

    ArrayRef<const char *> args(argv, argv + argc);

    auto result = lld::lldMain(args, llvm::outs(), llvm::errs(), LLD_ALL_DRIVERS);

    return result.retCode;

}

int invoke_lld(const std::vector<std::string> &command_args, const std::string_view& targetTripleString) {
    // Convert the vector of strings to an ArrayRef<const char *>
    std::vector<const char *> args_cstr;
    args_cstr.reserve(command_args.size() + 1);
    std::string lld_driver;
    auto triple = llvm::Triple(targetTripleString);
    if (triple.isOSDarwin())
        lld_driver = "ld64.lld";
    else if (triple.isOSWindows())
        lld_driver = "lld-link";
    else
        lld_driver = "ld.lld";
    args_cstr.push_back(lld_driver.c_str());
    for (const std::string& arg : command_args) {
        args_cstr.push_back(arg.c_str());
    }
    // invocation
    ToolContext context{};
    return lld_main(args_cstr.size(), const_cast<char**>(args_cstr.data()), context);
}

int Codegen::invoke_lld(const std::vector<std::string> &command_args) {
    return ::invoke_lld(command_args, target_triple);
}

#endif

#endif

int link_objects(
    std::vector<std::string>& linkables,
    const std::string& bin_out,
    const std::string& comp_exe_path, // our compiler's executable path, needed for self invocation
    const std::vector<std::string>& flags, // passed to clang or lld,
    const std::string_view& target_triple,
    bool use_lld = false,
    bool libc = true
) {
    if(use_lld) {

        // creating lld command

        // set output
#if defined(_WIN32)
        linkables.emplace_back("/OUT:"+bin_out);
#else
        linkables.emplace_back("-o");
        linkables.emplace_back("./"+bin_out);
#endif

        // link with standard libc (unless user opts out)
        if(!libc) {
#if defined(_WIN32)
            linkables.emplace_back("-defaultlib:libcmt");
#elif defined(__APPLE__)
            linkables.emplace_back("-lc");
#elif defined(__linux__)
            linkables.emplace_back("-lc");
#endif
        }

        // add user's linker flags
        for(const auto& flag : flags) {
            linkables.emplace_back(flag);
        }

        // invoke lld to create executable
        return invoke_lld(linkables, target_triple);

    } else {
        // use clang by default

        std::vector<std::string> clang_flags{comp_exe_path};
        clang_flags.emplace_back("-target");
        clang_flags.emplace_back(target_triple);

        for(const auto& cland_fl : flags) {
            clang_flags.emplace_back(cland_fl);
        }
        for(auto& linkable : linkables) {
            clang_flags.emplace_back(linkable);
        }
#if defined(_WINDOWS)
    if(bin_out.ends_with(".dll")) {
            clang_flags.emplace_back("-shared");
    }
#elif defined(__APPLE__)
    if(bin_out.ends_with(".dylib")) {
            clang_flags.emplace_back("-shared");
    }
#elif defined(__linux__)
    if(bin_out.ends_with(".so")) {
            clang_flags.emplace_back("-shared");
    }
#endif
        clang_flags.emplace_back("-o");
        clang_flags.emplace_back(bin_out);
        return chemical_clang_main2(clang_flags);
    }

}

int compile_c_file_to_object(
        const char* c_file,
        const char* out_file,
        const std::string& comp_exe_path,
        const std::vector<std::string>& flags
) {
    std::vector<std::string> clang_flags{comp_exe_path};
    for(const auto& cland_fl : flags) {
        clang_flags.emplace_back(cland_fl);
    }
    clang_flags.emplace_back("-c");
    clang_flags.emplace_back(c_file);
    clang_flags.emplace_back("-o");
    clang_flags.emplace_back(out_file);
    return chemical_clang_main2(clang_flags);
}