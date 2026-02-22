// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>
#include "ast/utils/IffyConditional.h"
#include "ast/base/ASTNode.h"
#include "std/except.h"

class Diag;
class ASTDiagnoser;
struct ASTFileResult;
struct LabModule;

/**
 * Represents a specific item being imported.
 * e.g., "import { some.path as alias } from ..."
 */
struct ImportItem {
    std::vector<chem::string_view> parts; // The path to the symbol (e.g., ["another", "symbol"])
    chem::string_view alias;              // The "as" alias for this specific symbol
};

enum class ImportStatementKind : uint8_t {
    NativeLib, // example: import std // std is stored in the source path
    LocalOrRemote, // example: import "./local" or import "org/repo"
};

enum class ImportResultKind : uint8_t {
    None,
    File,
    Module
};

struct ImportStatementAttributes {

    bool force_empty_import_items = false;

};

class ImportStatement final : public ASTNode {
private:

    /**
     * what kind of import statement is this
     */
    ImportStatementKind import_kind;

    /**
     * set once the import has been processed by the compiler
     */
    ImportResultKind result_kind = ImportResultKind::None;

    /**
     * the attributes on the import statement
     */
    ImportStatementAttributes attrs;

    // --- New Internal State ---
    chem::string_view m_sourcePath;       // "std", "./local", or "org/repo"
    chem::string_view m_topLevelAlias;    // the 'as' at the very end of the statement

    /**
     * import item is a single symbol with an optional alias
     * when specified, we only import these symbols
     */
    std::vector<ImportItem> m_importItems;

    // Remote Metadata
    chem::string_view m_version;
    chem::string_view m_subdir;
    chem::string_view m_branch;
    chem::string_view m_commit;

    // only pointers should be contained in the union (size should be 8 bytes)
    union {
        /**
         * we set this if it imports a file and once its parsed
         */
        ASTFileResult* file = nullptr;
        /**
         * we set this if it imports a module
         */
        LabModule* module;
    } result;

public:
    /**
     * contains an if condition that is used as a guard for this import
     */
    IffyBase* if_condition = nullptr;

    /**
     * constructor
     */
    constexpr ImportStatement(
            ImportStatementKind import_kind,
            chem::string_view sourcePath,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::ImportStmt, parent_node, location), import_kind(import_kind), m_sourcePath(sourcePath) {}

    // dangerously
    void setImportStmtKindDangerously(ImportStatementKind k) {
        import_kind = k;
    }

    inline bool is_force_empty_import_items() {
        return attrs.force_empty_import_items;
    }

    void setSourcePath(chem::string_view path) { m_sourcePath = path; }
    const chem::string_view& getSourcePath() const { return m_sourcePath; }

    void addImportItem(ImportItem item) { m_importItems.push_back(std::move(item)); }
    const std::vector<ImportItem>& getImportItems() const { return m_importItems; }

    void setTopLevelAlias(chem::string_view alias) { m_topLevelAlias = alias; }
    const chem::string_view& getTopLevelAlias() const { return m_topLevelAlias; }

    // Remote metadata setters/geters
    void setVersion(chem::string_view v) { m_version = v; }
    void setSubdir(chem::string_view s) { m_subdir = s; }
    void setBranch(chem::string_view b) { m_branch = b; }
    void setCommit(chem::string_view c) { m_commit = c; }

    const chem::string_view& getVersion() const { return m_version; }
    const chem::string_view& getSubdir() const { return m_subdir; }
    const chem::string_view& getBranch() const { return m_branch; }
    const chem::string_view& getCommit() const { return m_commit; }

    /**
     * check if its a native lib import
     */
    inline bool isNativeLibImport() {
        return import_kind == ImportStatementKind::NativeLib;
    }

    /**
     * tries its best to predict its a remote import from given data
     */
    bool isRemoteImportPredict() {
        // if any of these is given, it's a remote import
        if(!m_version.empty() || !m_commit.empty() || !m_branch.empty() || !m_subdir.empty()) return true;
        // user could be importing a local module using relative path: import "./rel_path" as relMod
        if(m_sourcePath.starts_with("./") || m_sourcePath.starts_with("../")) return false;
        // user could be importing a remote .lab or .mod file: import "http:// place.com/build.lab" as onlineScript
        if(m_sourcePath.starts_with("http://") || m_sourcePath.starts_with("https://") || m_sourcePath.starts_with("git@")) return true;
        return true;
    }

    /**
     * tries its best to predict its a remote import from given data
     */
    bool isLocalImportPredict() {
        // if any of these is given, it's a remote import
        if(!m_version.empty() || !m_commit.empty() || !m_branch.empty() || !m_subdir.empty()) return false;
        // user could be importing a local module using relative path: import "./rel_path" as relMod
        if(m_sourcePath.starts_with("./") || m_sourcePath.starts_with("../")) return true;
        // user could be importing a remote .lab or .mod file: import "http:// place.com/build.lab" as onlineScript
        if(m_sourcePath.starts_with("http://") || m_sourcePath.starts_with("https://") || m_sourcePath.starts_with("git@")) return false;
        return true;
    }

    /**
     * checks if a import is a remote import
     */
    inline bool isRemoteImport() {
        return !isNativeLibImport() && isRemoteImportPredict();
    }

    /**
     * check if imports a .lab or .mod file
     */
    inline bool isLabOrModFileImport() {
        return m_sourcePath.ends_with(".lab") || m_sourcePath.ends_with(".mod");
    }

    /**
     * check if imports a .ch, .lab or .mod file
     */
    inline bool isFileImport() {
        return isLabOrModFileImport() || m_sourcePath.ends_with(".ch");
    }

    /**
     * check import is a remote module import (definitely a module, not a file, and is remote)
     */
    inline bool isRemoteModuleImport() {
        return isRemoteImport() && !isLabOrModFileImport();
    }

    inline bool isLocalImport() {
        return !isRemoteImport();
    }

    inline bool isLocalModuleImport() {
        return isNativeLibImport() || (!isRemoteImportPredict() && !isLabOrModFileImport());
    }

    // checks if the import statement is like import '@lab/build.lab' as labMod for example
    bool isExternalModuleLabImport() {
        return m_sourcePath[0] == '@' && (m_sourcePath.ends_with(".lab") || m_sourcePath.ends_with(".mod"));
    }

    bool isLocalFileImport() {
        return !isNativeLibImport() && isFileImport() && isLocalImportPredict();
    }

    ImportStatement* copy(ASTAllocator &allocator) override {
        const auto stmt = new (allocator.allocate<ImportStatement>()) ImportStatement(
                import_kind, m_sourcePath, parent(), encoded_location()
        );
        stmt->m_importItems = m_importItems;
        stmt->m_topLevelAlias = m_topLevelAlias;
        stmt->m_version = m_version;
        stmt->m_subdir = m_subdir;
        stmt->m_branch = m_branch;
        stmt->m_commit = m_commit;
        stmt->if_condition = if_condition; // Note: You might want to copy_iffy here
        return stmt;
    }

    // ---------- Result

    inline bool hasResult() {
        return result_kind != ImportResultKind::None;
    }

    inline bool hasNoResult() {
        return result_kind == ImportResultKind::None;
    }

    inline bool isFileResult() {
        return result_kind == ImportResultKind::File;
    }

    inline bool isModuleResult() {
        return result_kind == ImportResultKind::Module;
    }

    inline ImportResultKind getResultKind() {
        return result_kind;
    }

    inline void setResult(ASTFileResult* fileResult) {
        result_kind = ImportResultKind::File;
        result.file = fileResult;
    }

    inline void setResult(LabModule* modulePtr) {
        result_kind = ImportResultKind::Module;
        result.module = modulePtr;
    }

    inline void unsetResult() {
        result_kind = ImportResultKind::None;
        result.file = nullptr;
    }

    inline ASTFileResult* getFileResult() {
#ifdef DEBUG
        if(result_kind != ImportResultKind::File) {
            CHEM_THROW_RUNTIME("expected result kind to be of file");
        }
#endif
        return result.file;
    }

    inline LabModule* getModuleResult() {
#ifdef DEBUG
        if(result_kind != ImportResultKind::Module) {
            CHEM_THROW_RUNTIME("expected result kind to be of module");
        }
#endif
        return result.module;
    }

};