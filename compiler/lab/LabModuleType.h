// Copyright (c) Qinetik 2024.

#pragma once

enum class LabModuleType {

    /**
     * a files module is which contains chemical source files
     */
    Files = 0,

    /**
     * a directory with no build.lab file, is considered a directory module
     * files inside are sorted so that independent files that don't depend on other files
     * are compiled first
     */
    Directory = 2

};