// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string_view>
#include <vector>
#include "compiler/processor/ASTFileMetaData.h"
#include "compiler/processor/ASTFileResult.h"
#include "compiler/OutputMode.h"

/**
 * save mod timestamp data (modified date and file size) in a file that can be read later and compared
 * to check if files have changed
 */
void save_mod_timestamp(const std::vector<std::string_view>& files, const std::string_view& output_file, OutputMode mode);

void save_mod_timestamp(const std::vector<ASTFileMetaData>& files, const std::string_view& output_file, OutputMode mode);

void save_mod_timestamp(const std::vector<ASTFileResult*>& files, const std::string_view& output_file, OutputMode mode);

bool compare_mod_timestamp(const std::vector<std::string_view>& files, const std::string_view& prev_timestamp_file, OutputMode mode);

bool compare_mod_timestamp(const std::vector<ASTFileMetaData>& files, const std::string_view& prev_timestamp_file, OutputMode mode);

bool compare_mod_timestamp(const std::vector<ASTFileResult*>& files, const std::string_view& prev_timestamp_file, OutputMode mode);
