// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
#include <vector>
#include <span>
#include "ast/structures/FileScope.h"
#include "core/diag/Diagnostic.h"
#include <string>
#include "ast/utils/IffyConditional.h"

typedef IffyBase ModFileIfBase;
typedef IffyCondId ModFileIfId;
typedef IffyCondExpr ModFileIfExpr;
typedef IffyExprOp ModFileIfExprOp;;

struct ModFileSource {

    chem::string_view path;

    IffyBase* if_cond = nullptr;

};

enum class ModFileLinkLibKind { Name, File };

enum class ModFileLinkLibVisibility { Unknown };

enum class ModFileLinkLibType { Unknown };

struct ModFileLinkLib {

    chem::string_view name;

    ModFileLinkLibKind kind = ModFileLinkLibKind::Name;

    ModFileLinkLibVisibility visibility = ModFileLinkLibVisibility::Unknown;

    ModFileLinkLibType type = ModFileLinkLibType::Unknown;

    IffyBase* if_cond = nullptr;

};

class ModuleFileData {
public:

    /**
     * the scope name in the .mod file's module declaration
     */
    chem::string_view scope_name;

    /**
     * the module name in the .mod file's module declaration
     */
    chem::string_view module_name;

    /**
     * the scope is used to contain the file
     */
    FileScope scope;

    /**
     * sources list tells us directories or files included in compilation
     */
    std::vector<ModFileSource> sources_list;

    /**
     * libraries user asked us to link for this module
     */
    std::vector<ModFileLinkLib> link_libs;

    /**
     * interface declarations in chemical.mod file allowing user to import
     * compiler interfaces easily
     */
    std::vector<chem::string_view> compiler_interfaces;

    /**
     * this is not empty if a ready error occurred
     */
    std::string read_error;

    /**
     * diagnostics for the .mod file
     */
    std::vector<Diag> diagnostics;

    /**
     * the file id is not given and module scope is also null because file
     * doesn't belong to a module
     */
    ModuleFileData(
            ASTFileMetaData& meta
    ) : scope(meta, nullptr) {

    }

};

class ModuleFileDataUnit {
public:

    /**
     * allocator
     */
    ASTAllocator allocator;

    /**
     * module file data
     */
    ModuleFileData modFileData;

};