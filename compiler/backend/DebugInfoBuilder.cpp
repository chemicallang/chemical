// Copyright (c) Chemical Language Foundation 2025.
//
// Driven by the MIR → LLVM emitter (`compiler/CodegenMir.cpp`). The former
// AST-shaped methods (struct/functions scopes, `instr`) belonged to the deleted
// AST LLVM lowering; the MIR emitter creates its own `DISubprogram`/`DIType`/
// `DILocation` metadata.

#include "DebugInfoBuilder.h"
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include "compiler/llvmimpl.h"
#include <filesystem>
#include "compiler/Codegen.h"
#include "std/except.h"

std::pair<std::string, std::string> splitPath(const chem::string_view &absolutePath) {
    std::filesystem::path path(absolutePath.view());
    std::string fileName = path.filename().string();
    std::string directory = path.parent_path().string();
    return {fileName, directory};
}

DebugInfoBuilder::DebugInfoBuilder(
    llvm::DIBuilder* builder,
    Codegen& gen,
    bool isEnabled
)  : builder(builder), gen(gen), isEnabled(isEnabled) {
    const auto m = gen.mode;
    const auto is_d = is_debug(m);
    isOptimized = !is_d;
    if(isEnabled) {
        diScopes.reserve(8);
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

void DebugInfoBuilder::end_di_compile_unit() {
    if(isEnabled) {
#ifdef DEBUG
        if(diCompileUnit != diScopes.back()) {
            CHEM_THROW_RUNTIME("current scope is not a compile unit");
        }
#endif
        diCompileUnit = nullptr;
        diScopes.pop_back();
    }
}

void DebugInfoBuilder::finalize() {
    diScopes.clear();
    diCompileUnit = nullptr;
    builder->finalize();
}
