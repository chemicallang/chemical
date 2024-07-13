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
    Library = 1,

    /**
     * This job basically emits a c file, we translate chemical to c
     */
    ToCTranslation = 2,

    /**
     * This job basically emits a chemical file, we translate c to chemical
     */
    ToChemicalTranslation = 3,

    /**
     * This means do not link, only process the modules
     */
    ProcessingOnly = 4,

};