// Copyright (c) Qinetik 2024.

#include "DynamicType.h"

DynamicType::DynamicType(std::unique_ptr<BaseType> referenced) : referenced(std::move(referenced)) {

}