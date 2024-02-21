//
// Created by ACER on 21/02/2024.
//

#include "FileTracker.h"

std::vector<std::unique_ptr<LexToken>> FileTracker::getLexedFile(const std::string &path, const LexConfig &config) {
    if (overriddenSources.contains(path)) {
        std::istringstream iss(overriddenSources[path]);
        return lexFile(iss, config);
    } else {
        return lexFile(path, config);
    }
}

void FileTracker::onChangedContents(const std::string &path, const std::string &contents) {
    overriddenSources[path] = contents;
}

void
FileTracker::onChangedContents(const std::string &path, const std::vector<lsTextDocumentContentChangeEvent> &changes) {

    // no changes return !
    if (changes.empty()) {
        return;
    }

    std::string source;

    // load the file if it doesn't exit
    if (!overriddenSources.contains(path)) {
        std::ifstream file;
        file.open(path);
        if (!file.is_open()) {
            std::cerr << "Unknown error opening the file" << '\n';
            return;
        }
        source = "";
        while (!file.eof()) {
            source += file.get();
        }
        file.close();
    } else {
        source = overriddenSources[path];
    }

    // make changes to the source code
    for (const auto &change: changes) {
        if (change.range.has_value()) {
            auto start = change.range.get().start;
            auto end = change.range.get().end;
            replaceSafe(source, start.line, start.character, end.line, end.character, change.text);
        }
    }

    // store the overridden sources
    overriddenSources[path] = std::move(source);

}

void FileTracker::onClosedFile(const std::string &path) {
    overriddenSources.erase(path);
}

void replace(
        std::string &source,
        unsigned int lineStart,
        unsigned int charStart,
        unsigned int lineEnd,
        unsigned int charEnd,
        const std::string &replacement
) {

    std::istringstream stream(source);

    auto provider = StreamSourceProvider(stream);

    std::string nextSource;

    while (!provider.eof()) {
        auto c = provider.readCharacter();
        if (provider.getLineNumber() == lineStart && provider.getCharNumber() == charStart) {

            // forwarding to the end without adding character
            while (!provider.eof() && provider.getLineNumber() != lineEnd && provider.getCharNumber() != charEnd) {
                provider.readCharacter();
            }

            // adding replacement
            nextSource += replacement;

        } else {
            nextSource += c;
        }
    }

    source = nextSource;

}

void replaceSafe(std::string &source, unsigned int lineStart, unsigned int charStart, unsigned int lineEnd,
                 unsigned int charEnd, const std::string &replacement) {
    if (lineStart == lineEnd) {
        if (charStart == charEnd) {
            // range is closed, do nothing
        } else if (charStart > charEnd) {
            // if start is larger than end, call replace accurately (swapping start with end)
            replace(source, lineEnd, charEnd, lineStart, charStart, replacement);
        }
    } else if (lineStart > lineEnd) {
        // if start is larger than end, call replace accurately (swapping start with end)
        replace(source, lineEnd, charEnd, lineStart, charStart, replacement);
    }
}