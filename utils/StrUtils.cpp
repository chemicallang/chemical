// Copyright (c) Qinetik 2024.

#include "StrUtils.h"

bool endsWith(const std::string &fullString, const std::string &ending) {
    if (fullString.length() >= ending.length()) {
        return (fullString.compare(fullString.length() - ending.length(), ending.length(), ending) == 0);
    } else {
        return false;
    }
}