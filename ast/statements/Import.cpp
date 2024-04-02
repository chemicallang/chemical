// Copyright (c) Qinetik 2024.

#include <clang/Frontend/CompilerInstance.h>
#include "Import.h"

#ifdef COMPILER_BUILD

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Tooling/Tooling.h"
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/Linker/Linker.h>

void ImportStatement::code_gen(Codegen &gen) {

    // Resolve the containing directory to given header
//    std::string dir = gen.headers_dir(filePath);
//    if(dir.empty()) {
//        gen.error("couldn't resolve headers directory for " + filePath + " for importing");
//        return;
//    }
//
//    // Absolute path to the header
//    std::filesystem::path absPath = std::filesystem::path(dir) / filePath;
//
//    clang::CompilerInstance ci;
//    ci.createDiagnostics();
//
//    clang::HeaderSearchOptions &headerSearchOpts = ci.getHeaderSearchOpts();
//    for(const auto& path : gen.system_headers_paths) {
//        headerSearchOpts.AddPath(path, clang::frontend::Angled, false, false);
//    }
//
//    // Ensure that Clang searches for headers relative to the directory of the file being compiled
//    // headerSearchOpts.AddPath(".", clang::frontend::Angled, false, false);
//
//    // Set the target options to match the current LLVM module
//    ci.getTargetOpts().Triple = gen.target_triple;
//
//    // Set up the compiler instance to use our action
//    std::shared_ptr<clang::TargetOptions> opts(&ci.getTargetOpts());
//    ci.setTarget(clang::TargetInfo::CreateTargetInfo(ci.getDiagnostics(), opts));
//
//    // Create a compiler instance source file
//    clang::InputKind inputKind(clang::Language::CXX, clang::InputKind::Format::Source, false, clang::InputKind::HeaderUnit_System, true);
//    clang::FrontendInputFile input_header_file(absPath.string(), inputKind, true);
//    ci.getFrontendOpts().Inputs.push_back(input_header_file);
//
//    // Create an LLVM-only action
//    std::unique_ptr<clang::EmitLLVMOnlyAction> action(new clang::EmitLLVMOnlyAction(gen.ctx.get()));
//
//    // Create and Execute the llvm only action
//    if (!ci.ExecuteAction(*action)) {
//        // Error handling
//        llvm::errs() << "Failed to import header: " << filePath << "\n";
//        return;
//    }
//
//    // Retrieve the generated LLVM module
//    std::unique_ptr<llvm::Module> headerModule = action->takeModule();
//
//    // Link the imported module into the current module
//    llvm::Linker::linkModules(*gen.module, std::move(headerModule));

}

#endif


ImportStatement::ImportStatement(std::string filePath, std::vector<std::string> identifiers) : filePath(
        std::move(filePath)), identifiers(std::move(identifiers)) {
    this->filePath.shrink_to_fit();
}

void ImportStatement::accept(Visitor &visitor) {
    visitor.visit(this);
}

std::string ImportStatement::representation() const {
    return std::string("import \"" + filePath + "\";");
}

std::filesystem::path ImportStatement::resolve_rel_path(InterpretScope &scope) {
    return (((std::filesystem::path) scope.global->root_path).parent_path() / ((std::filesystem::path) filePath));
}

void ImportStatement::interpret(InterpretScope &scope) {
    auto absolute_path = resolve_rel_path(scope).string();
    std::ifstream stream(absolute_path);
    if (stream.fail()) {
        scope.error("error couldn't import the following file " + absolute_path);
        return;
    }
    StreamSourceProvider provider(stream);
    Lexer lexer(provider, absolute_path);
    lexer.lex();
    Parser parser(std::move(lexer.tokens));
    parser.parse();
    Scope fileScope(std::move(parser.nodes));
    auto prevPath = scope.global->root_path;
    scope.global->root_path = absolute_path;
    fileScope.interpret(scope);
    scope.global->root_path = prevPath;
}