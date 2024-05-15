// Copyright (c) Qinetik 2024.

#include "ImportGraphMaker.h"
#include "lexer/Lexi.h"
#include "stream/SourceProvider.h"
#include "cst/base/CSTConverter.h"
#include "ast/statements/Import.h"
#include "compiler/ASTProcessor.h"

struct Importer {
    ASTProcessor *processor;
    Lexer *lexer;
    std::ifstream& stream;
    CSTConverter *converter;
    std::vector<Diag> &errors;
};

IGFile from_import(
        Importer *importer,
        IGFile *parent,
        const std::string &base_path,
        ImportStatement *importSt
);

void move_errors(std::vector<Diag> &from, std::vector<Diag> &to, const std::string& abs_path) {
    for (auto &dia: from) {
        if (dia.severity.has_value() && dia.severity.value() == DiagSeverity::Error) {
            dia.doc_url.emplace(abs_path);
            to.emplace_back(std::move(dia));
        }
    }
    from.clear();
}

std::vector<IGFile> get_imports(
        Importer *importer,
        IGFile *parent,
        const std::string &abs_path
) {

    // open file
    importer->stream.open(abs_path);
    if(!importer->stream.is_open()) {
        importer->errors.push_back(Diag {
                {0,0,0,0},
                DiagSeverity::Error,
                abs_path,
                "couldn't open the file " + abs_path
        });
    }

    // lex
    importer->lexer->tokens.clear();
    importer->lexer->switch_path(abs_path);
    importer->lexer->lexTopLevelMultipleImportStatements();
    if (importer->lexer->has_errors) {
        move_errors(importer->lexer->errors, importer->errors, abs_path);
    }

    // convert
    importer->converter->nodes.clear();
    importer->converter->convert(importer->lexer->tokens);
    if (importer->converter->has_errors) {
        move_errors(importer->converter->diagnostics, importer->errors, abs_path);
    }

    // take
    importer->stream.close();
    std::vector<IGFile> nested;
    auto nodes = std::move(importer->converter->nodes);
    for (auto &node: nodes) {
        nested.emplace_back(
                from_import(
                        importer,
                        parent,
                        abs_path,
                        (ImportStatement*) node.get()
                )
        );
    }

    return nested;
}

IGFile from_import(
        Importer *importer,
        IGFile *parent,
        const std::string &base_path,
        ImportStatement *importSt
) {
    std::string imported_path;
    if (importSt == nullptr) {
        imported_path = base_path;
    } else {
        importSt->replace_at_in_path(importer->processor);
        imported_path = importSt->resolve_rel_path(base_path).string();
    }
    IGFile file{parent, imported_path};
    file.files = get_imports(importer, &file, imported_path);
    return file;
}

IGResult determine_import_graph(const std::string &exe_path, const std::string &abs_path) {
    std::ifstream file;
    SourceProvider reader(file);
    Lexer lexer(reader, abs_path);
    CSTConverter converter(false);
    ASTProcessor processor(exe_path, abs_path);
    std::vector<Diag> errors;
    Importer importer{
            &processor,
            &lexer,
            file,
            &converter,
            errors
    };
    return IGResult{
            from_import(&importer, nullptr, abs_path, nullptr),
            std::move(errors)
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

void recursive_dedupe(IGFile* file, std::unordered_map<std::string, bool>& imported, std::vector<std::string>& imports) {
    for(auto& nested : file->files) {
        recursive_dedupe(&nested, imported, imports);
    }
    auto found = imported.find(file->abs_path);
    if(found == imported.end()) {
        imported[file->abs_path] = true;
        imports.emplace_back(file->abs_path);
    }
}

std::vector<std::string> IGFile::flatten_by_dedupe() {
    std::vector<std::string> imports;
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
    into.append(file.abs_path);
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