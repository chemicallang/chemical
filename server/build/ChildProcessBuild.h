// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>

class BasicBuildContext;

int report_context_to_parent(BasicBuildContext& context, const std::string& shmName);

int launch_child_build(BasicBuildContext& context, const std::string_view& lspPath, const std::string_view& buildFilePath);