// Copyright (c) Qinetik 2024.

#include "ImportGraphMaker.h"
#include "lexer/Lexi.h"
#include "stream/SourceProvider.h"
#include "cst/base/CSTConverter.h"
#include "ast/statements/Import.h"
#include "compiler/ASTDiagnoser.h"
#include "ImportGraphVisitor.h"
#include "cst/base/CompoundCSTToken.h"
#include "cst/utils/CSTUtils.h"
#include "ImportPathHandler.h"
#include "utils/PathUtils.h"

typedef ImportGraphImporter Importer;

void move_errors(std::vector<Diag> &from, std::vector<Diag> &to, const std::string& abs_path) {
    for (auto &dia: from) {
        if (dia.severity.has_value() && dia.severity.value() == DiagSeverity::Error) {
            dia.doc_url.emplace(abs_path);
            to.emplace_back(std::move(dia));
        }
    }
    from.clear();
}

ImportGraphImporter::ImportGraphImporter(ImportPathHandler* handler, Lexer* lexer, ImportGraphVisitor* converter) : handler(handler), lexer(lexer), converter(converter) {

};

void ImportGraphImporter::lex_source(const std::string& path, std::vector<Diag>& errors) {
    // lex
    lexer->tokens.clear();
    lexer->lexTopLevelMultipleImportStatements();
    if (lexer->has_errors) {
        move_errors(lexer->diagnostics, errors, path);
        lexer->has_errors = false;
    }
}

void ImportGraphVisitor::visitImport(CompoundCSTToken *cst) {
    std::string as_identifier;
    if(2 < cst->tokens.size() && is_keyword(cst->tokens[2].get(), "as")) {
        as_identifier = str_token(cst->tokens[3].get());
    }
    imports.emplace_back(
            FlatIGFile { escaped_str_token(cst->tokens[1].get()), escaped_str_token(cst->tokens[1].get()), std::move(as_identifier) },
            Range { cst->start_token()->position, cst->end_token()->position }
    );
}

IGFile from_import(
        Importer *importer,
        IGFile *parent,
        const std::string &base_path,
        ImportCollected *importSt
);

std::vector<IGFile> ImportGraphImporter::from_tokens(
        const std::string &abs_path,
        IGFile* parent,
        std::vector<std::unique_ptr<CSTToken>>& tokens
) {
    // convert
    converter->imports.clear();
    for(auto& token : tokens) {
        token->accept(converter);
    }

    // take
    std::vector<IGFile> nested;
    auto nodes = std::move(converter->imports);
    for (auto &node: nodes) {
        nested.emplace_back(
                from_import(
                        this,
                        parent,
                        abs_path,
                        &node
                )
        );
    }
    return nested;
}

std::vector<IGFile> ImportGraphImporter::process(const std::string &path, IGFile* parent) {
    FileInputSource source(path);
    if(source.has_error()) {
        parent->errors.emplace_back(
                Range {0,0,0,0},
                DiagSeverity::Error,
                path,
                "couldn't open the file " + path
        );
        return {};
    }
    lexer->provider.switch_source(&source);
    lex_source(path, parent->errors);
    return from_tokens(
            path,
            parent,
            lexer->tokens
    );
}

IGFile from_import(
        Importer *importer,
        IGFile *parent,
        const std::string &base_path,
        ImportCollected *importSt
) {
    IGFile file{parent, ""};
    auto& flat_file = file.flat_file;
    if (importSt == nullptr) {
        flat_file.abs_path = base_path;
    } else {
        flat_file.abs_path = importSt->file.abs_path;
        if(!flat_file.abs_path.empty() && flat_file.abs_path[0] == '@') {
            auto result = importer->handler->replace_at_in_path(flat_file.abs_path);
            if(result.error.empty()) {
                flat_file.abs_path = result.replaced;
                flat_file.import_path = importSt->file.abs_path;
            } else {
                parent->errors.emplace_back(
                        importSt->range,
                        DiagSeverity::Error,
                        importSt->file.abs_path,
                        result.error
                );
            }
        }
        if(!flat_file.abs_path.empty()) {
            auto resolved = resolve_rel_parent_path_str(base_path, flat_file.abs_path);
            if (resolved.empty()) {
                parent->errors.emplace_back(
                        importSt->range,
                        DiagSeverity::Error,
                        file.flat_file.abs_path,
                        "couldn't find the file to import " + file.flat_file.abs_path + " relative to base path " +
                                resolve_parent_path(base_path)
                );
            } else {
                file.flat_file.abs_path = resolved;
            }
        }
    }
    file.files = importer->process(file.flat_file.abs_path, &file);
    return file;
}

IGResult determine_import_graph(ImportGraphImporter* importer, std::vector<std::unique_ptr<CSTToken>>& tokens, FlatIGFile &asker) {
    IGResult result;
    result.root = IGFile { nullptr, asker };
    result.root.files = importer->from_tokens(result.root.flat_file.abs_path, &result.root, tokens);
    return result;
}

IGResult determine_import_graph(const std::string& exe_path, std::vector<std::unique_ptr<CSTToken>>& tokens, FlatIGFile &asker) {
    SourceProvider reader(nullptr);
    Lexer lexer(reader);
    ImportGraphVisitor visitor;
    ImportPathHandler handler(exe_path);
    ImportGraphImporter importer(
            &handler,
            &lexer,
            &visitor
    );
    return determine_import_graph(&importer, tokens, asker);
}

IGResult determine_import_graph(const std::string &exe_path, const std::string &abs_path) {
    SourceProvider reader(nullptr);
    Lexer lexer(reader);
    ImportGraphVisitor visitor;
    ImportPathHandler handler(exe_path);
    ImportGraphImporter importer(
            &handler,
            &lexer,
            &visitor
    );
    return IGResult{
            from_import(&importer, nullptr, abs_path, nullptr)
    };
}

//bool IGFile::depth_first(const std::function<bool(IGFile*)>& fn) {
//    for(auto& file : files) {
//        if(!file.depth_first(fn)) return false;
//    }
//    return fn(this);
//}
//
//bool IGFile::breath_first(const std::function<bool(IGFile*)>& fn) {
//    if(fn(this)) {
//        for (auto &file: files) {
//            if (!file.depth_first(fn)) return false;
//        }
//        return true;
//    } else {
//        return false;
//    }
//}

/**
 * TODO
 * 1 - avoid direct cyclic dependencies a depends on b and b depends on a
 * 2 - avoid indirect cyclic dependencies a depends on b and b depends on c and c depends on a
 *
 * recursive dedupe, will go over the imports of the given file recursively, On an import
 * It is checked if has NOT been imported before then put into the 'imports' flat vector
 *
 * The 'imported' map is the one used to keep track of imports, once imported, an index of the file (inside imports vec) is put on the map
 * The 'imports' flat vector is the final imported files
 * The 'file' is a file that contains imports, inside which are files that also contain imports, It is a tree
 *
 * a vector named parent_modify is sent, this is used to keep track of descendant files of a parent file, since descendant files should only
 * be disposed after parent has dealt with them
 *
 */
void recursive_dedupe(IGFile* file, std::unordered_map<std::string, size_t>& imported, std::vector<FlatIGFile>& imports) {
    auto found = imported.find(file->flat_file.abs_path);
    if(found == imported.end()) {
        // the size of the parent, we will only consider any index added to this vector after this size (because it's this file's descendant)
//        const auto parent_size = parent_modify.size();
        // import it's nested files first
        for(auto& nested : file->files) {
            recursive_dedupe(&nested, imported, imports);
        }
        // import it, set its index in the imported vec as it is
        const auto index = imports.size();
        imported[file->flat_file.abs_path] = index;
        imports.emplace_back(file->flat_file);
        // making sure descendants of this file are disposed after this file (in reverse, so we can remove the last)
//        auto i = parent_modify.size() - 1;
//        while(i >= parent_size) {
//            imports[parent_modify[i]].dispose_index = index;
//            // remove last, as we've just processed it
//            parent_modify.pop_back();
//            i--;
//        }
        // ask the parent to set this file's dispose index to its own index, so the file is disposed after the parent
//        parent_modify.emplace_back(index);
    } else {
        // we don't need to import the file, or it's nested files if it already has been imported
        // we just need to make sure it's disposed after the parent has disposed
//        parent_modify.emplace_back(found->second);
    }
}

std::vector<FlatIGFile> IGFile::flatten_by_dedupe() {
    std::vector<FlatIGFile> imports;
    std::unordered_map<std::string, size_t> imported;
    recursive_dedupe(this, imported, imports);
    return imports;
}

std::vector<FlatIGFile> flatten_by_dedupe(std::vector<IGFile>& files) {
    std::vector<FlatIGFile> imports;
    std::unordered_map<std::string, size_t> imported;
    for(auto& file : files) {
        recursive_dedupe(&file, imported, imports);
    }
    return imports;
}

void representation(IGFile& file, std::string& into, unsigned int level) {
    unsigned i = 0;
    while(i < level) {
        into.append("--");
        if(level > 1 && i < level - 1) {
            into.append(1, ' ');
        }
        i++;
    }
    if(level != 0) {
        into.append(1, ' ');
    }
    into.append(file.flat_file.abs_path);
    into.append(1, '\n');
    for(auto& n : file.files) {
        representation(n, into, level + 1);
    }
}

std::string IGFile::representation() {
    std::string rep;
    ::representation(*this, rep, 0);
    return rep;
}

void print_errors(IGFile* file) {
    for(auto& sub_file : file->files) {
        print_errors(&sub_file);
    }
    if(!file->errors.empty()) {
        for (auto& err : file->errors) {
            err.ansi(std::cout, file->flat_file.abs_path, "IGGraph") << std::endl;
        }
    }
}