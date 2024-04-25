// Copyright (c) Qinetik 2024.

#include "chem_clang.h"
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/APValue.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecordLayout.h>
#include "ast/base/ASTNode.h"
#include "ast/base/BaseType.h"
#include <memory>
#include "ast/structures/FunctionParam.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/types/IntNType.h"
#include "ast/types/VoidType.h"
#include "ast/types/CharType.h"
#include "ast/types/FloatType.h"
#include "ast/types/DoubleType.h"

// TODO nothing works in this file
// TODO as the code written is completely garbage at the moment
// TODO this will result in exceptions most probably

void TranslateC(const char *abs_path) {
    std::unique_ptr <std::vector<const char*>> args(new std::vector<const char*>());
    args->push_back(abs_path);
    auto unit = ClangLoadFromCommandLine(
            &(*args)[0],
            &(*args)[0] + args->size(),
            nullptr,
            nullptr,
            nullptr
    );
}

std::unique_ptr<BaseType> new_type(clang::QualType *type) {
    auto type_str = type->getAsString();
    if (type_str == "int") {
        return std::make_unique<IntNType>(32);
    } else if (type_str == "char") {
        return std::make_unique<CharType>();
    } else if (type_str == "char*") {
        return std::make_unique<IntNType>(32);
    } else if (type_str == "float") {
        return std::make_unique<FloatType>();
    } else if (type_str == "double") {
        return std::make_unique<DoubleType>();
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
                        false,
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