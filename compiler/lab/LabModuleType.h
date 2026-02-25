// Copyright (c) Chemical Language Foundation 2025.

#pragma once

enum class LabModuleType : int {

    /**
     * a files module contains directly .ch files in its 'paths', nothing else
     */
    Files = 0,

    /**
     * a c file module corresponds to a single translation unit, a single c
     * file is present in this module
     */
    CFile = 1,

    /**
     * a c++ file module corresponds to a single translation unit, a single c++
     * file is present in this module
     */
    CPPFile = 2,

    /**
     * an obj file module corresponds to a single object file, a single object
     * file is linked with final executable
     */
    ObjFile = 3,

    /**
     * a directory module is created from a chemical.mod or build.lab (module level) file
     * directory module can contain directory paths, file paths, when its a directory
     * we recursively traverse to add files with .ch extension
     */
    Directory = 4,

    /**
     * the last item in the enum, always should equal the last item
     */
    Last = Directory,

};