// Copyright (c) Qinetik 2024.

#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/APValue.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/AST/RecordLayout.h>
#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"
#include <memory>
#include "ast/structures/FunctionParam.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/types/IntNType.h"
#include "ast/types/IntType.h"
#include "ast/types/VoidType.h"
#include "ast/types/CharType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"
#include "ast/types/LongType.h"
#include "ast/types/BigIntType.h"

namespace clang {
    class ASTUnit;
}

struct ErrorMsg {
    const char *filename_ptr; // can be null
    unsigned long filename_len;
    const char *msg_ptr;
    unsigned long msg_len;
    const char *source; // valid until the ASTUnit is freed. can be null
    unsigned line; // 0 based
    unsigned column; // 0 based
    unsigned offset; // byte offset into source
};

#if __GNUC__ >= 8
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif

#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/APValue.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecordLayout.h>

#if __GNUC__ >= 8
#pragma GCC diagnostic pop
#endif

std::unique_ptr<BaseType> new_type(clang::QualType *type) {
    auto type_str = type->getAsString();
    if (type_str == "int") {
        return std::make_unique<IntType>();
    } else if (type_str == "char") {
        return std::make_unique<CharType>();
    } else if (type_str == "float") {
        return std::make_unique<FloatType>();
    } else if (type_str == "double") {
        return std::make_unique<DoubleType>();
    } else if (type_str == "long") {
        return std::make_unique<LongType>(sizeof(long) == 8);
    } else if (type_str == "long long") {
        return std::make_unique<BigIntType>();
    } else if (type_str == "void") {
        return std::make_unique<VoidType>();
    } else {
        throw std::runtime_error("type not supported");
    }
}


void Translate(clang::ASTUnit *unit, std::vector<std::unique_ptr<ASTNode>> &nodes) {
    auto tud = unit->getASTContext().getTranslationUnitDecl();
    for (auto &decl: tud->decls()) {
        if (decl->getKind() == clang::Decl::Kind::Function) {
            auto func_decl = (clang::FunctionDecl *) (decl);
            func_decl->getName();
            // Check if the declaration is for the printf function
            // Extract function parameters
            func_params params;
            unsigned index = 0;
            for (const auto *param: func_decl->parameters()) {
                auto type = param->getType();
                params.emplace_back(new FunctionParam(
                        param->getNameAsString(),
                        new_type(&type),
                        index,
                        std::nullopt
                ));
                index++;
            }
            auto ret_type = func_decl->getReturnType();
            nodes.emplace_back(std::make_unique<FunctionDeclaration>(
                    func_decl->getNameAsString(),
                    std::move(params),
                    new_type(&ret_type),
                    func_decl->isVariadic(),
                    std::nullopt
            ));
        }
    }
}

clang::ASTUnit *ClangLoadFromCommandLine(
        const char **args_begin,
        const char **args_end,
        struct ErrorMsg **errors_ptr,
        unsigned long *errors_len,
        const char *resources_path,
        clang::IntrusiveRefCntPtr<clang::DiagnosticsEngine> diags
) {

    std::shared_ptr<clang::PCHContainerOperations> pch_container_ops = std::make_shared<clang::PCHContainerOperations>();

    bool only_local_decls = true;
    bool user_files_are_volatile = true;
    bool allow_pch_with_compiler_errors = false;
    bool single_file_parse = false;
    bool for_serialization = false;
    bool retain_excluded_conditional_blocks = false;
    bool store_preambles_in_memory = false;
    llvm::StringRef preamble_storage_path = llvm::StringRef();
    clang::ArrayRef<clang::ASTUnit::RemappedFile> remapped_files = std::nullopt;
    std::unique_ptr<clang::ASTUnit> err_unit;
    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS = nullptr;
    std::optional<llvm::StringRef> ModuleFormat = std::nullopt;
    std::unique_ptr<clang::ASTUnit> ast_unit_unique_ptr = clang::ASTUnit::LoadFromCommandLine(
            args_begin, args_end,
            pch_container_ops,
            diags,
            resources_path,
            store_preambles_in_memory,
            preamble_storage_path,
            only_local_decls,
            clang::CaptureDiagsKind::All,
            remapped_files,
            true, // remapped files keep original name
            0, // precompiled preable after n parses
            clang::TU_Complete,
            false, // cache code completion results
            false, // include brief comments in code completion
            allow_pch_with_compiler_errors,
            clang::SkipFunctionBodiesScope::None,
            single_file_parse,
            user_files_are_volatile,
            for_serialization,
            retain_excluded_conditional_blocks,
            ModuleFormat,
            &err_unit,
            VFS);
    clang::ASTUnit *ast_unit = ast_unit_unique_ptr.release();

    *errors_len = 0;

    // Early failures in LoadFromCommandLine may return with ErrUnit unset.
    if (!ast_unit && !err_unit) {
        return nullptr;
    }

    if (diags->hasErrorOccurred()) {
        // Take ownership of the err_unit ASTUnit object so that it won't be
        // free'd when we return, invalidating the error message pointers
        clang::ASTUnit *unit = ast_unit ? ast_unit : err_unit.release();
        ErrorMsg *errors = nullptr;

        for (clang::ASTUnit::stored_diag_iterator it = unit->stored_diag_begin(),
                     it_end = unit->stored_diag_end(); it != it_end; ++it) {
            switch (it->getLevel()) {
                case clang::DiagnosticsEngine::Ignored:
                case clang::DiagnosticsEngine::Note:
                case clang::DiagnosticsEngine::Remark:
                case clang::DiagnosticsEngine::Warning:
                    continue;
                case clang::DiagnosticsEngine::Error:
                case clang::DiagnosticsEngine::Fatal:
                    break;
            }

            llvm::StringRef msg_str_ref = it->getMessage();

            *errors_len += 1;
            errors = reinterpret_cast<ErrorMsg *>(realloc(errors, sizeof(ErrorMsg) * *errors_len));
            if (errors == nullptr) abort();
            ErrorMsg *msg = &errors[*errors_len - 1];
            memset(msg, 0, sizeof(*msg));

            msg->msg_ptr = (const char *) msg_str_ref.bytes_begin();
            msg->msg_len = msg_str_ref.size();

            clang::FullSourceLoc fsl = it->getLocation();

            // Ensure the source location is valid before expanding it
            if (fsl.isInvalid()) {
                std::cerr << "FullSourceLoc is invalid" << std::endl;
                continue;
            }
            // Expand the location if possible
            fsl = fsl.getFileLoc();

            // The only known way to obtain a Loc without a manager associated
            // to it is if you have a lot of errors clang emits "too many errors
            // emitted, stopping now"
            if (fsl.hasManager()) {
                const clang::SourceManager &SM = fsl.getManager();

                clang::PresumedLoc presumed_loc = SM.getPresumedLoc(fsl);
                assert(!presumed_loc.isInvalid());

                msg->line = presumed_loc.getLine() - 1;
                msg->column = presumed_loc.getColumn() - 1;

                clang::StringRef filename = presumed_loc.getFilename();
                if (!filename.empty()) {
                    msg->filename_ptr = (const char *) filename.bytes_begin();
                    msg->filename_len = filename.size();
                }

                bool invalid;
                clang::StringRef buffer = fsl.getBufferData(&invalid);

                if (!invalid) {
                    msg->source = (const char *) buffer.bytes_begin();
                    msg->offset = SM.getFileOffset(fsl);
                }
            } else {
                std::cerr << "NO Filemanager associated" << std::endl;
            }
        }

        *errors_ptr = errors;

        return nullptr;
    }

    return ast_unit;
}

// Function to convert std::vector<std::string> to char**
void convertToCharPointers(const std::vector<std::string> &args, char ***begin, char ***end) {
    // Allocate memory for the array of char pointers
    char **argv = new char *[args.size()];

    // Copy each string from the vector to the array
    for (size_t i = 0; i < args.size(); ++i) {
        // Allocate memory for the C-style string and copy the content
        argv[i] = new char[args[i].size() + 1]; // +1 for null terminator
        std::strcpy(argv[i], args[i].c_str());
    }

    // Set begin and end pointers
    *begin = argv;
    *end = argv + args.size();
}

// Function to free memory allocated for char** pointers
void freeCharPointers(char **begin, char **end) {
    for (char **it = begin; it != end; ++it) {
        delete[] *it; // Free memory for each C-style string
    }
    delete[] begin; // Free memory for the array of char pointers
}

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path) {
    std::vector<std::string> args;
    args.emplace_back(exe_path);
    args.emplace_back(abs_path);
    clang::IntrusiveRefCntPtr<clang::DiagnosticsEngine> diags(
            clang::CompilerInstance::createDiagnostics(new clang::DiagnosticOptions));
    ErrorMsg *errors;
    unsigned long errors_len = 0;

    char **args_begin;
    char **args_end;

    // Convert vector to char** pointers
    convertToCharPointers(args, &args_begin, &args_end);

    auto unit = ClangLoadFromCommandLine(
            const_cast<const char **>(args_begin),
            const_cast<const char **>(args_end),
            &errors,
            &errors_len,
            resources_path,
            diags
    );
    freeCharPointers(args_begin, args_end);
    if (!unit) {
        std::cerr << "Errors occurred during translation, Length : " << std::to_string(errors_len) << std::endl;
        diags.get()->dump();
        return {};
    }
    std::vector<std::unique_ptr<ASTNode>> nodes;
    Translate(unit, nodes);
    delete unit;
    return nodes;
}