// Copyright (c) Qinetik 2024.

#include <string>

std::string resolve_rel_child_path_str(const std::string& root_path, const std::string& file_path);

std::string resolve_parent_path(const std::string& root_path);

std::string resolve_non_canon_parent_path(const std::string& root_path, const std::string& file_path);

// resolve non canonical sibling path
std::string resolve_sibling(const std::string& rel_to, const std::string& sibling);

std::string resolve_rel_parent_path_str(const std::string& root_path, const std::string& file_path);

std::string resources_path_rel_to_exe(const std::string& exe_path);