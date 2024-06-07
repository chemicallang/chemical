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

bool ImportGraphImporter::prepare_source(const std::string& abs_path, std::vector<Diag>& errors) {
    auto stream = (std::ifstream*) lexer->provider.stream;
    stream->open(abs_path);
    if(!stream->is_open()) {
        errors.emplace_back(
                Range {0,0,0,0},
                DiagSeverity::Error,
                abs_path,
                "couldn't open the file " + abs_path
        );
        return false;
    }
    return true;
}

void ImportGraphImporter::close_source() {
    ((std::ifstream*) lexer->provider.stream)->close();
}

void ImportGraphImporter::lex_source(const std::string& path, std::vector<Diag>& errors) {
    // lex
    lexer->tokens.clear();
    lexer->switch_path(path);
    lexer->lexTopLevelMultipleImportStatements();
    if (lexer->has_errors) {
        move_errors(lexer->errors, errors, path);
        lexer->has_errors = false;
    }
}

void ImportGraphVisitor::visitImport(CompoundCSTToken *cst) {
    imports.emplace_back(
            FlatIGFile { escaped_str_token(cst->tokens[1].get()) },
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
    if(!prepare_source(path, parent->errors)) {
        return {};
    }
    lex_source(path, parent->errors);
    close_source();
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
                        "couldn't find the file to import " + file.flat_file.abs_path
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
    std::ifstream file;
    SourceProvider reader(&file);
    Lexer lexer(reader, asker.abs_path);
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
    std::ifstream file;
    SourceProvider reader(&file);
    Lexer lexer(reader, abs_path);
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

void recursive_dedupe(IGFile* file, std::unordered_map<std::string, bool>& imported, std::vector<FlatIGFile>& imports) {
    for(auto& nested : file->files) {
        recursive_dedupe(&nested, imported, imports);
    }
    auto found = imported.find(file->flat_file.abs_path);
    if(found == imported.end()) {
        imported[file->flat_file.abs_path] = true;
        imports.emplace_back(file->flat_file);
    }
}

std::vector<FlatIGFile> IGFile::flatten_by_dedupe() {
    std::vector<FlatIGFile> imports;
    std::unordered_map<std::string, bool> imported;
    recursive_dedupe(this, imported, imports);
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
            std::cout << err.ansi_representation(file->flat_file.abs_path, "IGGraph") << std::endl;
        }
    }
}