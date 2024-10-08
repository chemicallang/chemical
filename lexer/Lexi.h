// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/01/2024.
//

#pragma once

#include <string>
#include <fstream>
#include "Lexer.h"
#include "utils/Benchmark.h"
#include "stream/FileInputSource.h"

/**
 * same as benchLexFile with istream
 * benchmark lexing the filename (relative to in the current project)
 * @return the tokens
 */
void benchLexFile(Lexer* lexer, const std::string &path, BenchmarkResults& results);

/**
 * same as benchLexFile with istream
 * benchmark lexing the filename (relative to in the current project)
 * @return the tokens
 */
void benchLexFile(Lexer* lexer, const std::string &path);

/**
 * same as lexFile with istream
 * lex the file at path (relative to in the current project)
 * @param fileName
 * @return the tokens
 */
void lexFile(Lexer* lexer, const std::string &path);

/**
 * benchmark lexing the given input stream
 * It will print helpful messages like lexing started and time taken by lexing in milli, mico and nano seconds
 * @param file
 * @return tokens
 */
Lexer benchLexFile(InputSource& source);

/**
 * same as benchLexFile with istream
 * benchmark lexing the filename (relative to in the current project)
 * @return the tokens
 */
Lexer benchLexFile(const std::string &path);

/**
 * will lex the file from given istream
 * @return the tokens
 */
Lexer lexFile(InputSource& source);

/**
 * same as lexFile with istream
 * lex the file at path (relative to in the current project)
 * @return the tokens
 */
Lexer lexFile(const std::string &path);