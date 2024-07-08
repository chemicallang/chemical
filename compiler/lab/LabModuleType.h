// Copyright (c) Qinetik 2024.

#pragma once

enum class LabModuleType {

    /**
     * a root file module means, one file, everything it imports will be merged into a one
     * single module
     */
    RootFile = 0,

    /**
     * a directory with no build.lab file, is considered a directory module
     * files inside are sorted so that independent files that don't depend on other files
     * are compiled first
     */
    Directory = 1

};