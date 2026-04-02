// Copyright (c) Chemical Language Foundation 2026.

#include "server/WorkspaceManager.h"
#include <iostream>

#include "ast/statements/Import.h"
#include "compiler/ASTProcessor.h"

#include "parser/Parser.h"
#include "server/diagnostics/DiagnosticUtils.h"
#include "utils/PathUtils.h"
#include <filesystem>

bool containsModOrLab(const std::string& libDirPath) {
    auto absoluteModPath = resolve_rel_child_path_str(libDirPath, "chemical.mod");

    if (!std::filesystem::exists(absoluteModPath)) {

        auto absoluteLabPath = resolve_rel_child_path_str(libDirPath, "build.lab");

        if (!std::filesystem::exists(absoluteLabPath)) {
            return false;
        }

    }

    return true;

}

void diagnoseModuleFileDataUnit(WorkspaceManager& manager, ModuleFileDataUnit* unit) {

    ASTDiagnoser diagnoser(manager.loc_man);

    // checking source paths, are they valid ?
    for (auto& src : unit->modFileData.sources_list) {
        auto srcPath = resolve_sibling(unit->metaData.abs_path, src.path.view());
        if (!std::filesystem::exists(srcPath)) {
            diagnoser.error(src.location) << "couldn't find the source path '" << srcPath << "'";
        }
    }

    // lets go over imports, if file not found we add diagnostic
    for (const auto node : unit->modFileData.scope.body.nodes) {
        if (node->kind() != ASTNodeKind::ImportStmt) {
            break;
        }

        const auto stmt = node->as_import_stmt_unsafe();

        if (stmt->isNativeLibImport()) {
            // native module, means source path is 'std' or 'cstd'
            // try to find a chemical.mod/build.lab file inside <exe_path>/libs/<module_name>/
            auto libName = stmt->getSourcePath();

            auto libDirPath = manager.pathHandler.resolve_native_lib(libName);

            if (!containsModOrLab(libDirPath)) {
                diagnoser.error(stmt) << "couldn't find native module to import at " << libDirPath;
            }

        } else if (stmt->isLocalImport()) {

            auto libDirPath = resolve_sibling(unit->metaData.abs_path, stmt->getSourcePath().view());

            if (!containsModOrLab(libDirPath)) {
                diagnoser.error(stmt) << "couldn't find native module to import at " << libDirPath;
            }

        }

    }

    if (!diagnoser.diagnostics.empty()) {
        // append all the diagnostics
        unit->modFileData.diagnostics.insert(unit->modFileData.diagnostics.end(), diagnoser.diagnostics.begin(), diagnoser.diagnostics.end());
    }

}

void WorkspaceManager::process_dot_mod_file(const std::string& path) {

#ifdef DEBUG
    std::cout << "[lsp] processing .mod file '" << path << "'" << std::endl;
#endif

    // lex the .mod file
    const auto lexUnit = get_lexed(path, true);

    // put the token into token cache
    tokenCache.put(path, lexUnit);

    auto& all_tokens = lexUnit->tokens;

    // copy the tokens
    std::vector<Token> copied_tokens;
    copied_tokens.reserve(all_tokens.size());

    // index for each token is stored (index in all_tokens)
    std::vector<size_t> originalIndexes;
    originalIndexes.reserve(all_tokens.size());

    // copy the tokens and record the original indexes
    size_t i = 0;
    for(auto& token : all_tokens) {
        if(token.type != TokenType::SingleLineComment && token.type != TokenType::MultiLineComment) {
            copied_tokens.emplace_back(token);
            originalIndexes.emplace_back(i);
        }
        i++;
    }

    // construct a parser
    const auto fileId = loc_man.encodeFile(path);
    BasicParser basicParser(loc_man, fileId, copied_tokens.data());

    // getting the module file data
    ModuleFileDataUnit* unit;
    auto unitPtr = modFileData.get(path);
    if(unitPtr == nullptr) {
        const auto modFileDataUnitShared = std::make_shared<ModuleFileDataUnit>(10000, fileId, lexUnit->abs_path);
        unit = modFileDataUnitShared.get();
        modFileData.put(path, modFileDataUnitShared);
    } else {
        unit = unitPtr->get();
        // clear previous unit and its data (crucial step)
        unit->clear();
    }

    // parse the .mod file
    basicParser.parseModuleFile(unit->allocator, unit->modFileData);

    // now restore the mapping of ast with tokens (using original indexes)
    // the order in which this operation occurs is important because
    // first the copied_tokens must be linked before we can link original tokens
    // which happens after symbol resolution, so this must take place after symbol resolution
    i = 0;
    for(auto& token : copied_tokens) {
        if(token.linked != nullptr) {
            auto original_index = originalIndexes[i];
            auto& originalToken = all_tokens[original_index];
            originalToken.linked = token.linked;
        }
        i++;
    }

    // diagnose the chemical.mod file
    diagnoseModuleFileDataUnit(*this, unit);

    // publish diagnotics of parsing
    std::vector<lsp::Diagnostic> diagnostics;
    add_diagnostics(diagnostics, unit->modFileData.diagnostics);
    publish_diagnostics(path, diagnostics);

}