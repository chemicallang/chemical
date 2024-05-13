// Copyright (c) Qinetik 2024.

#include "ImportGraphMaker.h"
#include "lexer/Lexi.h"
#include "stream/StreamSourceProvider.h"
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
    StreamSourceProvider reader(file);
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

void representation(IGFile& file, std::string& into, unsigned int level) {
    unsigned i = 0;
    while(i < level) {
        into.append("--");
        if(level > 1 && i < level - 1) {
            into.append(1, ' ');
        }
        i++;
    }
    into.append(1, ' ');
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