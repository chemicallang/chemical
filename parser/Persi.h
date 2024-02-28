// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/01/2024.
//

#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <iostream>
#include "SourceProvider.h"
#include "parser/Parser.h"
#include "StreamSourceProvider.h"

/**
 * benchmark lexing the given input stream
 * It will print helpful messages like lexing started and time taken by lexing in milli, mico and nano seconds
 * @param file
 * @return tokens
 */
Parser benchParse(std::vector<std::unique_ptr<LexToken>> tokens);

/**
 * same as lexFile with istream
 * lex the file at path (relative to in the current project)
 * @param fileName
 * @return the tokens
 */
Parser parse(std::vector<std::unique_ptr<LexToken>> tokens);