// Copyright (c) Chemical Language Foundation 2025.

#pragma once

namespace llvm {
    class Instruction;
    class CallInst;
    class UnreachableInst;
    class BranchInst;
    class ReturnInst;
    class AllocaInst;
    class StoreInst;
    class GlobalVariable;
    class Function;
    class FunctionType;
    class FunctionCallee;
    class BasicBlock;
    class TargetMachine;
    class Value;
    class Constant;
    class LLVMContext;
    class Module;
    class Type;
    class ConstantFolder;
    class IRBuilderDefaultInserter;
    class InvokeInst;
    template <typename FolderTy, typename InserterTy>
    class IRBuilder;
    class StructType;

    // debug onces
    class DICompileUnit;
    class DIScope;
    class DIFile;
    class DIBuilder;
    class DILocation;
    class DIType;
    class DICompositeType;

}