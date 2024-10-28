// Copyright (c) Qinetik 2024.

#include "ImportGraphMaker.h"
#include "lexer/Lexi.h"
#include "stream/SourceProvider.h"
#include "cst/base/CSTConverter.h"
#include "ast/statements/Import.h"
#include "compiler/ASTDiagnoser.h"
#include "ImportGraphVisitor.h"
#include "cst/base/CSTToken.h"
#include "cst/utils/CSTUtils.h"
#include "ImportPathHandler.h"
#include "utils/PathUtils.h"
#include <sstream>
#include <iostream>

typedef ImportGraphImporter Importer;

void move_errors(std::vector<Diag> &from, std::vector<Diag> &to, const std::string& abs_path) {
    for (auto &dia: from) {
        if (dia.severity.has_value() && dia.severity.value() == DiagSeverity::Error) {
            dia.path_url.emplace(abs_path);
            to.emplace_back(std::move(dia));
        }
    }
    from.clear();
}

ImportGraphImporter::ImportGraphImporter(ImportPathHandler* handler, Lexer* lexer, ImportGraphVisitor* converter) : handler(handler), lexer(lexer), converter(converter) {

};

void ImportGraphImporter::lex_source(const std::string& path, std::vector<Diag>& errors) {
    // lex
    lexer->unit.reset();
    lexer->lexTopLevelMultipleImportStatements();
    if (lexer->has_errors) {
        move_errors(lexer->diagnostics, errors, path);
        lexer->has_errors = false;
    }
}

void ImportGraphVisitor::visitImport(CSTToken *cst) {
    std::string as_identifier;
    if(2 < cst->tokens.size() && is_keyword(cst->tokens[2], "as")) {
        as_identifier = str_token(cst->tokens[3]);
    }
    imports.emplace_back(
            FlatIGFile { escaped_str_token(cst->tokens[1]), escaped_str_token(cst->tokens[1]), std::move(as_identifier) },
            Range { cst->start_token()->position(), cst->end_token()->position() }
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
        std::vector<CSTToken*>& tokens
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

std::vector<IGFile> ImportGraphImporter::process(const std::string &path, const Range& range, IGFile* parent) {
    FileInputSource source(path.c_str());
    if(source.has_error()) {
        parent->errors.emplace_back(
                range,
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
            lexer->unit.tokens
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
        flat_file.range = Range { 0, 0, 0, 0 };
    } else {
        flat_file.range = importSt->range;
        flat_file.import_path = importSt->file.abs_path;
        auto result = importer->handler->resolve_import_path(base_path, importSt->file.abs_path);
        flat_file.abs_path = result.replaced;
        if(!result.error.empty()) {
            parent->errors.emplace_back(
                    importSt->range,
                    DiagSeverity::Error,
                    importSt->file.abs_path,
                    result.error
            );
        }
    }
    file.files = importer->process(flat_file.abs_path, importSt->range, &file);
    return file;
}

IGFile determine_import_graph_file(ImportGraphImporter* importer, std::vector<CSTToken*>& tokens, FlatIGFile &asker) {
    auto root = IGFile { nullptr, asker };
    root.files = importer->from_tokens(root.flat_file.abs_path, &root, tokens);
    return root;
}

IGResult determine_import_graph(ImportGraphImporter* importer, std::vector<CSTToken*>& tokens, FlatIGFile &asker) {
    IGResult result;
    result.root = IGFile { nullptr, asker };
    result.root.files = importer->from_tokens(result.root.flat_file.abs_path, &result.root, tokens);
    return result;
}

IGResult determine_import_graph(
        const std::string& exe_path,
        std::vector<CSTToken*>& tokens,
        FlatIGFile &asker,
        LocationManager& manager
) {
    SourceProvider reader(nullptr);
    Lexer lexer("", reader);
    ImportGraphVisitor visitor;
    ImportPathHandler handler(exe_path);
    ImportGraphImporter importer(
            &handler,
            &lexer,
            &visitor
    );
    return determine_import_graph(&importer, tokens, asker);
}

IGFile determine_ig_file(ImportPathHandler &handler, const std::string &abs_path) {
    SourceProvider reader(nullptr);
    Lexer lexer(abs_path, reader);
    ImportGraphVisitor visitor;
    ImportGraphImporter importer(
            &handler,
            &lexer,
            &visitor
    );
    return from_import(&importer, nullptr, abs_path, nullptr);
}

IGFile determine_ig_file(const std::string &exe_path, const std::string &abs_path) {
    ImportPathHandler handler(exe_path);
    return determine_ig_file(handler, abs_path);
}

IGResult determine_import_graph(ImportPathHandler &path_handler, const std::string &abs_path) {
    return IGResult { determine_ig_file(path_handler, abs_path) };
}

IGResult determine_import_graph(const std::string &exe_path, const std::string &abs_path) {
    return IGResult { determine_ig_file(exe_path, abs_path) };
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
    // if this file's absolute path is empty, we remove it from the imports, which is added by the recursive_dedupe
    // this is done because IGFile can represent multiple files being imported under an anonymous file with no absolute path
    if(flat_file.abs_path.empty()) {
        imports.pop_back();
    }
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

void representation(const IGFile& file, std::ostream& into, unsigned int level) {
    unsigned i = 0;
    while(i < level) {
        into << "--";
        if(level > 1 && i < level - 1) {
            into << ' ';
        }
        i++;
    }
    if(level != 0) {
        into << ' ';
    }
    into << file.flat_file.abs_path << '\n';
    for(auto& n : file.files) {
        representation(n, into, level + 1);
    }
}

std::string IGFile::representation() {
    std::stringstream rep;
    ::representation(*this, rep, 0);
    return rep.str();
}

void IGFile::representation(std::ostream &into) {
    ::representation(*this, into, 0);
}

void representation(std::ostream &into, const std::vector<IGFile>& files) {
    for(auto& file : files) {
        ::representation(file, into, 0);
    }
}

std::string representation(const std::vector<IGFile>& files) {
    std::stringstream rep;
    for(auto& file : files) {
        ::representation(file, rep, 0);
    }
    return rep.str();
}

void print_errors(const IGFile& file) {
    for(auto& sub_file : file.files) {
        print_errors(sub_file);
    }
    if(!file.errors.empty()) {
        for (auto& err : file.errors) {
            err.ansi(std::cout, file.flat_file.abs_path, "IGGraph") << std::endl;
        }
    }
}

void print_errors(const std::vector<IGFile>& files) {
    for(auto& sub_file : files) {
        print_errors(sub_file);
    }
}