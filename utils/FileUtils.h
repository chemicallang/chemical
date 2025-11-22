// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include <string>

bool exists_with_error(const std::string& build_dir);

bool create_dir_no_check(const std::string& build_dir);

bool create_dir(const std::string& build_dir);

void writeToFile(const std::string &path, const std::string_view& text);

/**
 * it will write ascii characters like this character<asciicode>
 * @param path
 * @param text
 */
void writeAsciiToFile(const std::string &path, const std::string &text);

void writeToProjectFile(const std::string &path, const std::string &text);

void writeAsciiToProjectFile(const std::string &path, const std::string &text);