// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include <string>

void writeToFile(const std::string &path, const std::string &text);

/**
 * it will write ascii characters like this character<asciicode>
 * @param path
 * @param text
 */
void writeAsciiToFile(const std::string &path, const std::string &text);

void writeToProjectFile(const std::string &path, const std::string &text);

void writeAsciiToProjectFile(const std::string &path, const std::string &text);