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
#include "CTranslator.h"
#include "ast/types/PointerType.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/StructDefinition.h"

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

BaseType *new_type(CTranslator *translator, clang::QualType *type) {
    auto ptr = type->getTypePtr();
    if(ptr->isBuiltinType()) {
        auto builtIn = static_cast<clang::BuiltinType*>(const_cast<clang::Type*>(ptr));
        auto created = translator->type_makers[builtIn->getKind()](builtIn);
        if(!created) {
            translator->error("builtin type maker failed with kind " + std::to_string(builtIn->getKind()) + " with representation " + builtIn->getName(clang::PrintingPolicy{clang::LangOptions{}}).str());
        }
        return created;
    } else if(ptr->isStructureType()){
        auto str_type = ptr->getAsStructureType();
        auto decl = str_type->getAsRecordDecl();
        std::map<std::string, std::unique_ptr<StructMember>> fields;
        for(auto str : decl->fields()) {
            auto field_type = str->getType();
            fields[str->getNameAsString()] = std::make_unique<StructMember>(str->getNameAsString(), std::unique_ptr<BaseType>(new_type(translator, &field_type)), std::nullopt);
        }
        auto def = new StructDefinition(decl->getNameAsString(), std::nullopt);
        def->variables = std::move(fields);
        translator->before_nodes.emplace_back(def);
        return new ReferencedType(decl->getNameAsString());
    }
//    else if(ptr->isArrayType()) { // couldn't make use of it
//        auto point = ptr->getAsArrayTypeUnsafe();
//        auto elem_type = point->getElementType();
//        auto pointee = new_type(translator, &elem_type);
//        if(!pointee) {
//            return nullptr;
//        }
//        return new ArrayType(std::unique_ptr<BaseType>(pointee), -1);
//    }
    else if(ptr->isPointerType()) {
        auto point = ptr->getPointeeType();
        auto pointee = new_type(translator, &point);
        if(!pointee) {
            return nullptr;
        }
        return new PointerType(std::unique_ptr<BaseType>(pointee));
    } else {
        translator->error("unknown type given to new_type with representation " + type->getAsString());
        return nullptr;
    };
}

void Translate(CTranslator *translator, clang::ASTUnit *unit, std::vector<std::unique_ptr<ASTNode>> &nodes) {
    auto tud = unit->getASTContext().getTranslationUnitDecl();
    for (auto &decl: tud->decls()) {
        if (decl->getKind() == clang::Decl::Kind::Function) {
            auto func_decl = (clang::FunctionDecl *) (decl);
            // Check if the declaration is for the printf function
            // Extract function parameters
            func_params params;
            unsigned index = 0;
            bool skip_fn = false;
            for (const auto *param: func_decl->parameters()) {
                auto type = param->getType();
                auto chem_type = new_type(translator, &type);
                if (!chem_type) {
                    skip_fn = true;
                    continue;
                }
                params.emplace_back(new FunctionParam(
                        param->getNameAsString(),
                        std::unique_ptr<BaseType>(chem_type),
                        index,
                        std::nullopt
                ));
                index++;
            }
            if(skip_fn) {
                continue;
            }
            auto ret_type = func_decl->getReturnType();
            auto chem_type = new_type(translator, &ret_type);
            if (!chem_type) {
                continue;
            }
            for(auto& node : translator->before_nodes) {
                nodes.emplace_back(std::move(node));
            }
            translator->before_nodes.clear();
            nodes.emplace_back(std::make_unique<FunctionDeclaration>(
                    func_decl->getNameAsString(),
                    std::move(params),
                    std::unique_ptr<BaseType>(chem_type),
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
        std::cerr << std::to_string(errors_len) << " errors occurred when loading C files" << std::endl;
        unsigned i = 0;
        ErrorMsg *err;
        while (i < errors_len) {
            err = errors + i;
            std::cerr << err->msg_ptr << " at " << err->filename_ptr << ":" << err->line << ":" << err->column
                      << std::endl;
            i++;
        }
        diags.get()->dump();
        return {};
    }
    std::vector<std::unique_ptr<ASTNode>> nodes;
    CTranslator translator;
    Translate(&translator, unit, nodes);
    delete unit;
    if (!translator.errors.empty()) {
        std::cerr << std::to_string(translator.errors.size()) << " errors occurred when translating C files" << std::endl;
    }
    for (const auto &err: translator.errors) {
        std::cerr << err.message << std::endl;
    }
    return nodes;
}