// Copyright (c) Chemical Language Foundation 2025.

#include "ParseModDecl.h"
#include <fstream>
#include <cctype>
#include <string>
#include <cstring>

const char* parseModDecl(char* scope_name, char* mod_name, size_t& scopeSizeOut, size_t& modSizeOut, size_t each_buffer_size, const std::string_view& filePath) {

    // fixed buffer: no dynamic allocation
    char buf[100] = {0};

    // open file
    std::ifstream file(filePath.data());
    if (!file.is_open()) {
        return "cannot open file";
    }

    // read up to 100 characters from file
    file.read(buf, sizeof(buf));
    std::streamsize count = file.gcount();
    if(count == 0) {
        return "file is empty";
    }

    const char* p = buf;
    const char* end = buf + count;

    // Skip any leading whitespace (spaces, tabs, newlines)
    while(p < end && std::isspace(static_cast<unsigned char>(*p))) {
        p++;
    }

    // Look for "module"
    const char* keyword = "module";
    for (int i = 0; keyword[i] != '\0'; i++) {
        if (p == end || *p != keyword[i])
            return "bad keyword";
        p++;
    }

    // There must be whitespace after "module"
    if (p < end && !std::isspace(static_cast<unsigned char>(*p))) {
        return "bad keyword";
    }

    // Skip whitespace after keyword
    while(p < end && std::isspace(static_cast<unsigned char>(*p))) {
        p++;
    }

    // Parse the first identifier (alphanumerics and underscore)
    const char* start = p;
    while(p < end && (std::isalnum(static_cast<unsigned char>(*p)) || *p == '_')) {
        p++;
    }
    if (start == p) {
        return "missing module name";
    }

    const char* sep_start = p;

    // Check if there is a separator (dot, colon, or double colon)
    if (p < end && (*p == '.' || *p == ':')) {

        char sep = *p;
        p++; // consume separator

        // For colon, check if next char is also a colon (double colon)
        if (sep == ':' && p < end && *p == ':') {
            p++;
        }

        // Parse second identifier
        const char* start2 = p;
        while(p < end && (std::isalnum(static_cast<unsigned char>(*p)) || *p == '_')) {
            p++;
        }

        if (start2 == p) {
            return "missing module part";
        }

        // Before copying, check if lengths fit in the buffers.
        const auto scope_len = sep_start - start;
        const auto mod_len = p - start2;
        if (scope_len >= each_buffer_size || mod_len >= each_buffer_size) {
            return "module declaration too long";
        }

        // Copy scope and module name; ensure null-termination.
        std::memcpy(scope_name, start, scope_len);
        scope_name[scope_len] = '\0';
        scopeSizeOut = scope_len;

        std::memcpy(mod_name, start2, mod_len);
        mod_name[mod_len] = '\0';
        modSizeOut = mod_len;
    } else {
        // No separator found; the declaration is just "module <name>"
        const auto mod_len = sep_start - start;
        if (mod_len >= each_buffer_size) {
            return "module declaration too long";
        }
        // Clear scope_name since no scope is provided.
        scope_name[0] = '\0';
        std::memcpy(mod_name, start, mod_len);
        mod_name[mod_len] = '\0';
        modSizeOut = mod_len;
    }

    return nullptr;
}