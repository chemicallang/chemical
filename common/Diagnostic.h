// Copyright (c) Qinetik 2024.

#pragma once

#include <optional>
#include <string>
#include <vector>

class Position {
public:

    // line and character correspond to position in the source file
    unsigned int line, character;

    // representation of the position
    std::string representation() {
        return std::to_string(line) + ':' + std::to_string(character);
    }

};

class Range {
public:
    // the range in the source file
    Position start, end;
};

enum class DiagSeverity : uint8_t {
    // Reports an error.
    Error = 1,
    // Reports a warning.
    Warning = 2,
    // Reports an information.
    Information = 3,
    // Reports a hint.
    Hint = 4
};

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

class Diagnostic {
public:

    // The range at which the message applies.
    Range range;

    // severity. Can be omitted. If omitted it is up to the
    // client to interpret diagnostics as error, warning, info or hint.
    std::optional<DiagSeverity> severity;

    // a documentation url where this diagnostic can be explained
    std::optional<std::string> doc_url;

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
     * returns representation of this diagnostic as string
     * @return
     */
    std::string representation() const {
        return "[Diagnostic] " + message;
    }

    /**
     * returns representation of this diagnostic as a string, also includes file path
     * @param path
     * @return
     */
    std::string representation(const std::string &path) const {
        return representation();
    }

};