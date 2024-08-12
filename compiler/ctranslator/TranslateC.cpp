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
#include "ast/structures/EnumDeclaration.h"
#include "CTranslator.h"
#include "ast/types/PointerType.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/VarInit.h"

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

BaseType* CTranslator::make_type(clang::QualType* type) {
    auto canonical = type->getCanonicalType();
    auto ptr = type->getTypePtr();
    auto canon_ptr = canonical.getTypePtr();
    if(type->isConstQualified()) {
        type->removeLocalConst();
    }
    if(ptr->isPointerType()) {
        auto point = ptr->getPointeeType();
        auto pointee = make_type(&point);
        if(!pointee) {
            return nullptr;
        }
        return new PointerType(std::unique_ptr<BaseType>(pointee));
    }
    if(canon_ptr != ptr) { // reference
        if(canon_ptr->isStructureType()) {
            return make_type(&canonical);
        }
        return new ReferencedType(type->getAsString());
    }
    if(ptr->isBuiltinType()) {
        auto builtIn = static_cast<clang::BuiltinType*>(const_cast<clang::Type*>(ptr));
        auto created = type_makers[builtIn->getKind()](builtIn);
        if(!created) {
            error("builtin type maker failed with kind " + std::to_string(builtIn->getKind()) + " with representation " + builtIn->getName(clang::PrintingPolicy{clang::LangOptions{}}).str() + " with actual " + type->getAsString());
        }
        return created;
    } else if(ptr->isStructureType()){
        auto str_type = ptr->getAsStructureType();
        auto decl = str_type->getAsRecordDecl();
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
    else {
        error("unknown type given to new_type with representation " + type->getAsString());
        return nullptr;
    };
}

EnumDeclaration* CTranslator::make_enum(clang::EnumDecl* decl) {
    auto enum_decl = new EnumDeclaration(decl->getNameAsString(), {}, parent_node);
    std::unordered_map<std::string, std::unique_ptr<EnumMember>> members;
    unsigned index = 0;
    for(auto mem : decl->enumerators()) {
        enum_decl->members[mem->getNameAsString()] = std::make_unique<EnumMember>(mem->getNameAsString(), index, enum_decl);
        index++;
    }
    return enum_decl;
}

StructDefinition* CTranslator::make_struct(clang::RecordDecl* decl) {
    auto def = new StructDefinition(decl->getNameAsString(), parent_node);
    for(auto str : decl->fields()) {
        auto field_type = str->getType();
        auto field_type_conv = make_type(&field_type);
        if(!field_type_conv) {
            return nullptr;
        }
        def->variables[str->getNameAsString()] = std::make_unique<StructMember>(str->getNameAsString(), std::unique_ptr<BaseType>(field_type_conv), std::nullopt, def);
    }
    return def;
}

TypealiasStatement* CTranslator::make_typealias(clang::TypedefDecl* decl) {
    auto decl_type = decl->getUnderlyingType();
    auto type = make_type(&decl_type);
    if(type == nullptr) return nullptr;
    return new TypealiasStatement(decl->getNameAsString(), std::unique_ptr<BaseType>(type), parent_node);
}

Expression* CTranslator::make_expr(clang::Expr* expr) {
    error("UNIMPLEMENTED: cannot create an expression at the moment");
    return nullptr;
}

VarInitStatement* CTranslator::make_var_init(clang::VarDecl* decl) {
    auto type = decl->getType();
    auto made_type = make_type(&type);
    std::optional<std::unique_ptr<Value>> initial = std::nullopt;
    auto initial_value = (Value*) make_expr(decl->getInit());
    if(initial_value) {
        initial.emplace(initial_value);
    }
    return new VarInitStatement(false, decl->getNameAsString(), std::unique_ptr<BaseType>(made_type), std::move(initial), parent_node);
}

FunctionDeclaration* CTranslator::make_func(clang::FunctionDecl* func_decl) {
    // Check if the declaration is for the printf function
    // Extract function parameters
    std::vector<std::unique_ptr<FunctionParam>> params;
    unsigned index = 0;
    bool skip_fn = false;
    for (const auto *param: func_decl->parameters()) {
        auto type = param->getType();
        auto chem_type = make_type(&type);
        if (!chem_type) {
            skip_fn = true;
            continue;
        }
        auto param_name = param->getNameAsString();
        if(param_name.empty()) {
            param_name = "_";
        }
        params.emplace_back(new FunctionParam(
                param_name,
                std::unique_ptr<BaseType>(chem_type),
                index,
                std::nullopt
        ));
        index++;
    }
    if(skip_fn) {
        return nullptr;
    }
    auto ret_type = func_decl->getReturnType();
    auto chem_type = make_type(&ret_type);
    if (!chem_type) {
        return nullptr;
    }
    dispatch_before();
    auto decl = new FunctionDeclaration(
            func_decl->getNameAsString(),
            std::move(params),
            std::unique_ptr<BaseType>(chem_type),
            func_decl->isVariadic(),
            parent_node,
            std::nullopt
    );
    decl->assign_params();
    return decl;
}

void Translate(CTranslator *translator, clang::ASTUnit *unit) {
    auto tud = unit->getASTContext().getTranslationUnitDecl();
    for (auto decl: tud->decls()) {
        auto node = translator->node_makers[decl->getKind()](translator, decl);
        if(node) {
            translator->nodes.emplace_back(node);
        } else {
            translator->error("couldn't convert decl with kind " + std::to_string(decl->getKind()) + " & kind name " + decl->getDeclKindName());
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
//    std::cout << "[TranslateC] Processing " << abs_path << " with resources " << resources_path << " & compiler at "<< exe_path << std::endl;
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
    CTranslator translator;
    Translate(&translator, unit);
    delete unit;
    if (!translator.errors.empty()) {
        std::cerr << std::to_string(translator.errors.size()) << " errors occurred when translating C files" << std::endl;
    }
    for (const auto &err: translator.errors) {
        std::cerr << err.message << std::endl;
    }
    return std::move(translator.nodes);
}