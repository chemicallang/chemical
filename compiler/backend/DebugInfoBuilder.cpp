// Copyright (c) Chemical Language Foundation 2025.

#include "DebugInfoBuilder.h"
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include "compiler/llvmimpl.h"
#include <filesystem>
#include "ast/base/ASTNode.h"
#include "compiler/mangler/NameMangler.h"
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "core/source/LocationManager.h"
#include "compiler/Codegen.h"
#include "ast/structures/FunctionDeclaration.h"
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
#include "ast/structures/Namespace.h"
#include "ast/types/IntNType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/values/FunctionCall.h"
#include "compiler/processor/ASTFileMetaData.h"
#include "compiler/processor/ASTFileResult.h"

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
    Codegen& gen,
    bool isEnabled
)  : loc_man(loc_man), builder(builder), gen(gen), isEnabled(isEnabled) {
    const auto m = gen.mode;
    const auto is_d = is_debug(m);
    isOptimized = !is_d;
    if(isEnabled) {
        diScopes.reserve(20);
        cachedTypes.reserve(60);
    }
}

void DebugInfoBuilder::update_builder(llvm::DIBuilder* new_builder) {
    builder = new_builder;
}

llvm::DICompileUnit* DebugInfoBuilder::createDiCompileUnit(const chem::string_view& abs_path) {
    unsigned MyCustomLangCode = 0x8001; // Vendor-specific code
    auto [fileName, dirPath] = splitPath(abs_path);
    diCompileUnit = builder->createCompileUnit(
            MyCustomLangCode,
            builder->createFile(fileName, dirPath),
            "Chemical",
            isOptimized,
            "",
            0 // <--- runtime version
    );
    diScopes.clear();
    return diCompileUnit;
}



void DebugInfoBuilder::start_di_compile_unit(llvm::DICompileUnit* unit) {
    if(isEnabled) {
#ifdef DEBUG
        if(!unit) {
            CHEM_THROW_RUNTIME("cannot start an empty di compile unit");
        }
#endif
        diCompileUnit = unit;
        diScopes.push_back(unit);
    }
}

llvm::DILocation* DebugInfoBuilder::di_loc(const Position& position) {
#ifdef DEBUG
    if(diScopes.empty()) {
        CHEM_THROW_RUNTIME("expected a scope to be present, when creating a di location");
    }
    if(diScopes.back() == diCompileUnit) {
        CHEM_THROW_RUNTIME("scope for a location cannot be a di compile unit");
    }
#endif
    return llvm::DILocation::get(*gen.ctx, position.line + 1, position.character + 1, diScopes.back());
}

void DebugInfoBuilder::end_di_compile_unit() {
    if(isEnabled) {
#ifdef DEBUG
        if(diCompileUnit != diScopes.back()) {
            CHEM_THROW_RUNTIME("current scope is not a compile unit");
        }
#endif
#ifdef DEBUG
        if(!replaceAbleTypes.empty()) {
            CHEM_THROW_RUNTIME("replaceable types isn't empty");
        }
#endif
        diCompileUnit = nullptr;
        diScopes.pop_back();
    }
}

void DebugInfoBuilder::finalize() {
    cachedTypes.clear();
    replaceAbleTypes.clear();
    diScopes.clear();
    diCompileUnit = nullptr;
    fileScopes.clear();
    builder->finalize();
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








