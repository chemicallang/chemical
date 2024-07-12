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

};