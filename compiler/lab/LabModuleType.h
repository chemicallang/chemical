// Copyright (c) Qinetik 2024.

#pragma once

enum class LabModuleType : int {

    /**
     * a files module is which contains chemical source files, if any c files
     * are present, they are converted to c file modules and taken as
     * dependencies on current module
     */
    Files = 0,

    /**
     * a c file module corresponds to a single translation unit, a single c
     * file is present in this module
     */
    CFile = 0,

    /**
     * a directory with no build.lab file, is considered a directory module
     * files inside are sorted so that independent files that don't depend on other files
     * are compiled first
     */
    Directory = 2

};