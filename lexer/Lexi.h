// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/01/2024.
//

#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <iostream>
#include "stream/SourceProvider.h"
#include "lexer/Lexer.h"
#include "stream/StreamSourceProvider.h"

/**
 * benchmark lexing the given input stream
 * It will print helpful messages like lexing started and time taken by lexing in milli, mico and nano seconds
 * @param file
 * @return tokens
 */
Lexer benchLexFile(std::istream &file, const std::string& path);

/**
 * same as benchLexFile with istream
 * benchmark lexing the filename (relative to in the current project)
 * @param file
 * @return the tokens
 */
Lexer benchLexFile(const std::string &path);

/**
 * will lex the file from given istream
 * @param file
 * @return the tokens
 */
Lexer lexFile(std::istream &file, const std::string& path);

/**
 * same as lexFile with istream
 * lex the file at path (relative to in the current project)
 * @param fileName
 * @return the tokens
 */
Lexer lexFile(const std::string &path);