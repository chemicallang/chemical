// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <optional>
#include <string>
#include <vector>
#include <cinttypes>
#include "Range.h"
#include "DiagSeverity.h"
#include "std/chem_string_view.h"

enum class DiagTag : uint8_t {
    // redundant code tag
    Unnecessary = (1),
    // deprecated code tag
    Deprecated = (2),
};

class DiagRelatedInfo {
public:
    // location where the related information exists
    Position location;
    // message about the related information
    std::string message;
};

class TextReplacement {
public:
    Range range;
    std::string new_text;
};

class Diag {
public:

    // The range at which the message applies.
    Range range;

    // severity. Can be omitted. If omitted it is up to the
    // client to interpret diagnostics as error, warning, info or hint.
    std::optional<DiagSeverity> severity;

    // a documentation url where this diagnostic can be explained
    std::optional<std::string> path_url;

    // message, This is the most important thing in this
    std::string message;

    // Non-serialized set of fixits.
    std::vector<TextReplacement> fixits_;

    // Additional metadata about the diagnostic
    std::optional<std::vector<DiagTag>> tags;

    // An array of related diagnostic information, e.g. when symbol-names within a scope collide
    // all definitions can be marked via this property.
    std::optional<std::vector<DiagRelatedInfo>> relatedInformation;

    /**
     * this format function directly prints the diagnostic to output stream
     */
    void format(std::ostream& os, const chem::string_view& path, const chem::string_view& tag) const;

    /**
     * prints the current diagnostic to console
     */
    std::ostream& ansi(std::ostream& os, const chem::string_view& path, const chem::string_view& tag = "Diagnostic") const;

    /**
     * helper function to append a char pointer
     */
    constexpr Diag& operator<<(const char* str) {
        const auto size = str ?
        // GCC 7 doesn't have constexpr char_traits. Fall back to __builtin_strlen.
        #if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 8
        __builtin_strlen(Str)
        #else
        std::char_traits<char>::length(str)
        #endif
            : 0;
        message.append(str, size);
        return *this;
    }

    /**
     * append a string view
     */
    inline Diag& operator<<(const chem::string_view &value) {
        message.append(value.data(), value.size());
        return *this;
    }

    /**
     * append a string view
     */
    inline Diag& operator<<(const std::string_view& value) {
        message.append(value);
        return *this;
    }

    /**
     * append a string value
     */
    inline Diag& operator<<(std::string& value) {
        message.append(value);
        return *this;
    }

    /**
     * append a character value
     */
    inline Diag& operator<<(char value) {
        message.append(1, value);
        return *this;
    }

    /**
     * check if a vector of diagnostics has a error diagnostic in them
     */
    static bool has_errors(std::vector<Diag>& diags) {
        for(auto& diag : diags) {
            if(diag.severity == DiagSeverity::Error) {
                return true;
            }
        }
        return false;
    }

};