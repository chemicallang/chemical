// Copyright (c) Qinetik 2024.

#pragma once

#include <optional>
#include <string>
#include <vector>
#include "Position.h"
#include "DiagSeverity.h"

class Range {
public:
    // the range in the source file
    Position start, end;

    std::string representation() const {
        return start.representation() + " - " + end.representation();
    }
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

class Diag {
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
    std::string format(const std::string& path, const std::string &tag = "Diagnostic") const {
        return "[" + tag + "] " + message + " at " + path + ':' + range.representation();
    }

    /**
     * returns representation of this diagnostic as string
     * @return
     */
    std::string ansi_representation(const std::string& path, const std::string &tag = "Diagnostic") const {
        return (severity.has_value() ? (color(severity.value())) : "") +
               format(path, tag) +
               (severity.has_value() ? ("\x1b[0m") : "");
    }

    /**
     * returns representation of this diagnostic as a string, also includes file path
     * @param path
     * @return
     */
    std::string representation(const std::string &path, const std::string &tag) const {
        return ansi_representation(path, tag);
    }

};