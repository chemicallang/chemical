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
            const auto numBits = intNType->num_bits();
            const auto isUnsigned = intNType->is_unsigned();
            switch (numBits) {
                case 8:
                    return di.builder->createBasicType(isUnsigned ? "uchar" : "char", 1, isUnsigned ? llvm::dwarf::DW_ATE_unsigned_char : llvm::dwarf::DW_ATE_signed_char);
                case 16:
                    return di.builder->createBasicType(isUnsigned ? "ushort" : "short", 2, isUnsigned ? llvm::dwarf::DW_ATE_unsigned : llvm::dwarf::DW_ATE_signed);
                case 32:
                    return di.builder->createBasicType(isUnsigned ? "uint" : "int", 4, isUnsigned ? llvm::dwarf::DW_ATE_unsigned : llvm::dwarf::DW_ATE_signed);
                case 64:
                    return di.builder->createBasicType(isUnsigned ? "ubigint" : "bigint", 8, isUnsigned ? llvm::dwarf::DW_ATE_unsigned : llvm::dwarf::DW_ATE_signed);
                default:
                    return di.builder->createBasicType("integer", type->as_intn_type_unsafe()->num_bits(), isUnsigned ? llvm::dwarf::DW_ATE_unsigned : llvm::dwarf::DW_ATE_signed);
            }
        }
        case BaseTypeKind::Float:
            return di.builder->createBasicType("float", 32, llvm::dwarf::DW_ATE_float);
        case BaseTypeKind::Double:
            return di.builder->createBasicType("double", 32, llvm::dwarf::DW_ATE_decimal_float);
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
        default:
            return nullptr;
    }
}

void DebugInfoBuilder::info(Value *value) {

}

void DebugInfoBuilder::info(FunctionDeclaration *decl, llvm::Function* func) {
    // TODO must debug info for every function call, before we do function declaration
//    const auto location = loc_node(this, decl->location);
//    llvm::DISubprogram *SP = builder->createFunction(
//            diScope,
//            to_ref(decl->name_view()),
//            decl->runtime_name_str(),  // Linkage name
//            diFile,
//            location.start.line,    // Line number of the function
//            builder->createSubroutineType(builder->getOrCreateTypeArray({})),
//            location.start.character,
//            llvm::DINode::FlagPrototyped,
//            llvm::DISubprogram::SPFlagDefinition
//    );
//    func->setSubprogram(SP);
}

void DebugInfoBuilder::info(StructDefinition *structDefinition) {

}

void DebugInfoBuilder::info(InterfaceDefinition *interfaceDefinition) {

}

void DebugInfoBuilder::info(VarInitStatement *init, llvm::AllocaInst* inst) {
    const auto location = loc_node(this, init->location);
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
            di_loc(location.start),
            inst
    );
}

void DebugInfoBuilder::info(VarInitStatement* init, llvm::GlobalVariable* variable) {

}

void DebugInfoBuilder::info(ImplDefinition *implDefinition) {

}

void DebugInfoBuilder::info(UnionDef *def) {

}

void DebugInfoBuilder::info(VariantDefinition *variant_def) {

}

void DebugInfoBuilder::info(Scope *scope) {

}

void DebugInfoBuilder::info(IfStatement *ifStatement) {

}

void DebugInfoBuilder::info(ValueWrapperNode *node) {

}

void DebugInfoBuilder::info(ForLoop *forLoop) {

}

void DebugInfoBuilder::info(LoopBlock *scope) {

}

void DebugInfoBuilder::info(WhileLoop *whileLoop) {

}

void DebugInfoBuilder::info(DoWhileLoop *doWhileLoop) {

}

void DebugInfoBuilder::info(AssignStatement *assign) {

}

void DebugInfoBuilder::info(SwitchStatement *statement) {

}

void DebugInfoBuilder::info(BreakStatement *breakStatement) {

}

void DebugInfoBuilder::info(ContinueStatement *continueStatement) {

}

void DebugInfoBuilder::info(ReturnStatement *returnStatement) {

}

void DebugInfoBuilder::info(InitBlock *initBlock) {

}

void DebugInfoBuilder::info(StructMemberInitializer *init) {

}

void DebugInfoBuilder::info(DestructStmt *delStmt) {

}

void DebugInfoBuilder::info(UnreachableStmt *stmt) {

}

void DebugInfoBuilder::info(LambdaFunction *func) {

}

void DebugInfoBuilder::info(ExtensionFuncReceiver *receiver) {

}

void DebugInfoBuilder::info(FunctionParam *functionParam) {

}

void DebugInfoBuilder::info(Namespace *ns) {

}