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

void ImportGraphVisitor::visitImport(CompoundCSTToken *cst) {
    imports.emplace_back(
            FlatIGFile { escaped_str_token(cst->tokens[1].get()) },
            Range { cst->start_token()->position, cst->end_token()->position }
    );
}

struct Importer {
    ImportPathHandler* handler;
    Lexer *lexer;
    std::ifstream& stream;
    ImportGraphVisitor *converter;
};

IGFile from_import(
        Importer *importer,
        IGFile *parent,
        const std::string &base_path,
        ImportCollected *importSt
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

std::vector<IGFile> from_tokens(
        Importer *importer,
        IGFile *parent,
        const std::string &abs_path,
        std::vector<std::unique_ptr<CSTToken>>& tokens
) {
    // convert
    importer->converter->imports.clear();
    for(auto& token : tokens) {
        token->accept(importer->converter);
    }

    // take
    importer->stream.close();
    std::vector<IGFile> nested;
    auto nodes = std::move(importer->converter->imports);
    for (auto &node: nodes) {
        nested.emplace_back(
                from_import(
                        importer,
                        parent,
                        abs_path,
                        &node
                )
        );
    }
    return nested;
}

std::vector<IGFile> get_imports(
        Importer *importer,
        IGFile *parent,
        const std::string &abs_path
) {

    // open file
    importer->stream.open(abs_path);
    if(!importer->stream.is_open()) {
        parent->errors.emplace_back(
                Range {0,0,0,0},
                DiagSeverity::Error,
                abs_path,
                "couldn't open the file " + abs_path
        );
        return {};
    }

    // lex
    importer->lexer->tokens.clear();
    importer->lexer->switch_path(abs_path);
    importer->lexer->lexTopLevelMultipleImportStatements();
    if (importer->lexer->has_errors) {
        move_errors(importer->lexer->errors, parent->errors, abs_path);
        importer->lexer->has_errors = false;
    }

    // convert
    return from_tokens(
        importer,
        parent,
        abs_path,
        importer->lexer->tokens
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
            auto resolved = resolve_rel_path_str(base_path, flat_file.abs_path);
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
    file.files = get_imports(importer, &file, file.flat_file.abs_path);
    return file;
}

IGResult determine_import_graph(const std::string& exe_path, std::vector<std::unique_ptr<CSTToken>>& tokens, FlatIGFile asker) {
    std::ifstream file;
    SourceProvider reader(file);
    Lexer lexer(reader, "");
    ImportGraphVisitor visitor;
    ImportPathHandler handler(exe_path);
    IGResult result;
    Importer importer{
            &handler,
            &lexer,
            file,
            &visitor
    };
    result.root = IGFile { nullptr, std::move(asker) };
    result.root.files = from_tokens(&importer, &result.root, result.root.flat_file.abs_path, tokens);
    return result;
}

IGResult determine_import_graph(const std::string &exe_path, const std::string &abs_path) {
    std::ifstream file;
    SourceProvider reader(file);
    Lexer lexer(reader, abs_path);
    ImportGraphVisitor visitor;
    ImportPathHandler handler(exe_path);
    Importer importer{
            &handler,
            &lexer,
            file,
            &visitor
    };
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