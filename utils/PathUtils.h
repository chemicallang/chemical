// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string_view>

// argv[0] is not complete, when user executes from the environment variable
std::string getExecutablePath();

std::string resolve_rel_child_path_str(const std::string_view& root_path, const std::string_view& file_path);

std::string resolve_parent_path(const std::string_view& root_path);

std::string resolve_non_canon_parent_path(const std::string_view& root_path, const std::string_view& file_path);

// resolve non canonical sibling path
std::string resolve_sibling(const std::string_view& rel_to, const std::string_view& sibling);

std::string resolve_rel_parent_path_str(const std::string_view& root_path, const std::string_view& file_path);

std::string resources_path_rel_to_exe(const std::string_view& exe_path);

// get canonical path or empty for this given path
std::string canonical(const std::string_view& path);

inline std::string canonical_path(const std::string_view& path) {
    return canonical(path);
}

// resolve relative paths (relative to current working directory)
std::string absolute_path(const std::string_view& relative);

/**
 * get our actual compiler (clang based) executable path relative to tiny cc
 */
std::string compiler_exe_path_relative_to_tcc(const std::string_view& exe_path);