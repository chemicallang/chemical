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

LocationManager::LocationData loc_node(DebugInfoBuilder* visitor, void* any) {
    const auto loc = ((ASTNode*) any)->encoded_location();
    return visitor->loc_man.getLocation(loc);
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

void DebugInfoBuilder::createDiCompileUnit(const chem::string_view& abs_path) {
    unsigned MyCustomLangCode = 0x8001; // Vendor-specific code
    auto [fileName, dirPath] = splitPath(abs_path);
    diFile = builder->createFile(fileName, dirPath);
    diCompileUnit = builder->createCompileUnit(
            MyCustomLangCode,
            diFile,
            "Chemical",
            context.isOptimized,
            "",
            0 // <--- runtime version
    );
    diScope = diCompileUnit;
}

void DebugInfoBuilder::info(Value *value) {

}

void DebugInfoBuilder::info(FunctionDeclaration *decl) {
    const auto location = loc_node(this, decl);
    llvm::DISubprogram *SP = builder->createFunction(
            diScope,
            to_ref(decl->name_view()),
            decl->runtime_name_str(),  // Linkage name
            diFile,
            location.lineStart,    // Line number of the function
            builder->createSubroutineType(builder->getOrCreateTypeArray({})),
            location.lineStart,
            llvm::DINode::FlagPrototyped,
            llvm::DISubprogram::SPFlagDefinition
    );
    const auto func = decl->llvm_func();
    func->setSubprogram(SP);
}

void DebugInfoBuilder::info(ExtensionFunction *extensionFunc) {

}

void DebugInfoBuilder::info(StructDefinition *structDefinition) {

}

void DebugInfoBuilder::info(InterfaceDefinition *interfaceDefinition) {

}

void DebugInfoBuilder::info(VarInitStatement *init) {

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