// Copyright (c) Qinetik 2024.

#pragma once

namespace llvm {
    class AllocaInst;
    class GlobalVariable;
    class Function;
    class FunctionType;
    class FunctionCallee;
    class BasicBlock;
    class TargetMachine;
    class Value;
    class LLVMContext;
    class Module;
    class Type;
    class ConstantFolder;
    class IRBuilderDefaultInserter;
    class InvokeInst;
    template <typename FolderTy, typename InserterTy>
    class IRBuilder;
    class StructType;
}