// Copyright (c) Qinetik 2025.

#include "DebugInfoBuilder.h"
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include "compiler/llvmimpl.h"
#include <filesystem>
#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "cst/LocationManager.h"
#include "compiler/Codegen.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/ExtensionFunction.h"
#include "ast/structures/ExtensionFuncReceiver.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/LoopBlock.h"
#include "ast/structures/ForLoop.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/Return.h"
#include "ast/structures/Namespace.h"
#include "ast/values/LambdaFunction.h"
#include "ast/statements/Unreachable.h"
#include "ast/structures/If.h"
#include "ast/structures/InitBlock.h"
#include "ast/structures/Namespace.h"
#include "ast/types/IntNType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/values/FunctionCall.h"

inline LocationManager::LocationPosData loc_node(DebugInfoBuilder* visitor, SourceLocation loc) {
    return visitor->loc_man.getLocationPos(loc);
}

std::pair<std::string, std::string> splitPath(const chem::string_view &absolutePath) {
    std::filesystem::path path(absolutePath.view());
    std::string fileName = path.filename().string();
    std::string directory = path.parent_path().string();
    return {fileName, directory};
}

inline llvm::StringRef to_ref(const chem::string_view& view) {
    return llvm::StringRef(view.data(), view.size());
}

DebugInfoBuilder::DebugInfoBuilder(
    LocationManager& loc_man,
    llvm::DIBuilder* builder,
    Codegen& gen
)  : loc_man(loc_man), builder(builder), gen(gen) {
    const auto m = gen.mode;
    const auto is_d = is_debug(m);
    isOptimized = !is_d;
    isEnabled = is_d && m != OutputMode::DebugQuick;
}

void DebugInfoBuilder::update_builder(llvm::DIBuilder* new_builder) {
    builder = new_builder;
}

void DebugInfoBuilder::createDiCompileUnit(const chem::string_view& abs_path) {
    unsigned MyCustomLangCode = 0x8001; // Vendor-specific code
    auto [fileName, dirPath] = splitPath(abs_path);
    diFile = builder->createFile(fileName, dirPath);
    diCompileUnit = builder->createCompileUnit(
            MyCustomLangCode,
            diFile,
            "Chemical",
            isOptimized,
            "",
            0 // <--- runtime version
    );
    diScope = diCompileUnit;
}

void DebugInfoBuilder::finalize() {
    builder->finalize();
}

llvm::DILocation* DebugInfoBuilder::di_loc(const Position& position) {
    return llvm::DILocation::get(*gen.ctx, position.line, position.character, diScope);
}

llvm::DIType* to_di_type(DebugInfoBuilder& di, BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::IntN: {
            const auto intNType = type->as_intn_type_unsafe();
            const auto num_bits = intNType->num_bits();
            switch(intNType->IntNKind()) {
                case IntNTypeKind::Char:
                    return di.builder->createBasicType("char", num_bits, llvm::dwarf::DW_ATE_signed_char);
                case IntNTypeKind::Short:
                    return di.builder->createBasicType("short", num_bits, llvm::dwarf::DW_ATE_signed);
                case IntNTypeKind::Int:
                    return di.builder->createBasicType("int", num_bits, llvm::dwarf::DW_ATE_signed);
                case IntNTypeKind::Long:
                    return di.builder->createBasicType("long", num_bits, llvm::dwarf::DW_ATE_signed);
                case IntNTypeKind::BigInt:
                    return di.builder->createBasicType("bigint", num_bits, llvm::dwarf::DW_ATE_signed);
                case IntNTypeKind::Int128:
                    return di.builder->createBasicType("int128", num_bits, llvm::dwarf::DW_ATE_signed);
                case IntNTypeKind::UChar:
                    return di.builder->createBasicType("uchar", num_bits, llvm::dwarf::DW_ATE_unsigned_char);
                case IntNTypeKind::UShort:
                    return di.builder->createBasicType("ushort", num_bits, llvm::dwarf::DW_ATE_unsigned);
                case IntNTypeKind::UInt:
                    return di.builder->createBasicType("uint", num_bits, llvm::dwarf::DW_ATE_unsigned);
                case IntNTypeKind::ULong:
                    return di.builder->createBasicType("ulong", num_bits, llvm::dwarf::DW_ATE_unsigned);
                case IntNTypeKind::UBigInt:
                    return di.builder->createBasicType("ubigint", num_bits, llvm::dwarf::DW_ATE_unsigned);
                case IntNTypeKind::UInt128:
                    return di.builder->createBasicType("int128", num_bits, llvm::dwarf::DW_ATE_unsigned);
            }
        }
        case BaseTypeKind::Float:
            return di.builder->createBasicType("float", 32, llvm::dwarf::DW_ATE_float);
        case BaseTypeKind::Double:
            return di.builder->createBasicType("double", 64, llvm::dwarf::DW_ATE_decimal_float);
        case BaseTypeKind::Pointer: {
            const auto ptrType = type->as_pointer_type_unsafe();
            const auto pointee = to_di_type(di, ptrType->type);
            return di.builder->createPointerType(pointee, 64);
        }
        case BaseTypeKind::Reference: {
            const auto ptrType = type->as_reference_type_unsafe();
            const auto pointee = to_di_type(di, ptrType->type);
            return di.builder->createReferenceType(0, pointee, 64);
        }
        // TODO handle more types
        default:
            return nullptr;
    }
}

void DebugInfoBuilder::instr(llvm::Instruction* inst, const Position& position) {
    if(isEnabled) {
        inst->setDebugLoc(di_loc(position));
    }
}

void DebugInfoBuilder::instr(llvm::Instruction* inst, SourceLocation source_loc) {
    if(inst && isEnabled) {
        const auto location = loc_node(this, source_loc);
        inst->setDebugLoc(di_loc(location.start));
    }
}

void DebugInfoBuilder::info(FunctionType *decl, llvm::Function* func) {
    const auto location = loc_node(this, decl->encoded_location());
    const auto as_func = decl->as_function();
    const auto name_view = as_func ? to_ref(as_func->name_view()) : func->getName();
    llvm::DISubprogram *SP = builder->createFunction(
            diScope,
            name_view,
            func->getName(),  // Linkage name
            diFile,
            location.start.line,    // Line number of the function
            builder->createSubroutineType(builder->getOrCreateTypeArray({})),
            location.start.character,
            llvm::DINode::FlagFwdDecl,
            llvm::DISubprogram::SPFlagDefinition
    );
    func->setSubprogram(SP);
}

void DebugInfoBuilder::info(VarInitStatement *init, llvm::AllocaInst* inst) {
    const auto location = loc_node(this, init->encoded_location());
    const auto loc = di_loc(location.start);
    inst->setDebugLoc(loc);
    llvm::DILocalVariable *Var = builder->createAutoVariable(
            diScope,
            to_ref(init->name_view()),
            diFile,
            location.start.line,
            to_di_type(*this, init->create_value_type(gen.allocator))
    );
    builder->insertDeclare(
            init->llvm_ptr,                                         // Variable allocation
            Var,
            builder->createExpression(),
            loc,
            inst
    );
}

void DebugInfoBuilder::info(FunctionCall* call, llvm::CallInst* callInst) {
    instr(callInst, call->encoded_location());
    const auto location = loc_node(this, call->encoded_location());
    callInst->setDebugLoc(di_loc(location.start));
}