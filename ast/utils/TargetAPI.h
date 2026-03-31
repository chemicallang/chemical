// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include <string>
#include "compiler/lab/TargetData.h"

/**
 * a target data, that user allocates can be used to get information about the target triple
 */
void prepare_target_data(TargetData& data, const std::string& target_triple);