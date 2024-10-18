// Copyright (c) Qinetik 2024.

#pragma once

enum class LabJobType : int {

    /**
     * generate executable linking dependent modules
     */
    Executable = 0,

    /**
     * generate dll or shared object linking dependent modules
     */
    Library,

    /**
     * This job basically emits a c file, we translate chemical to c
     */
    ToCTranslation,

    /**
     * This job basically emits a chemical file, we translate c to chemical
     */
    ToChemicalTranslation,

    /**
     * This job basically emits multiple chemical files, dividing
     * the input into multiple parts which is imported
     */
    ToChemicalTranslationInParts,

    /**
     * This means do not link, only process the modules
     */
    ProcessingOnly,

    /**
     * creates a cbi, A CBI is compiled C code that can be called by
     * our compiler to perform actions like handle macros
     */
    CBI = 6,

};