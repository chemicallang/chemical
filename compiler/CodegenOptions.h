// Copyright (c) Chemical Language Foundation 2025.

#pragma once

/**
 * allow to control the code generated
 * all options are false by default
 */
struct CodegenOptions {

    bool fno_unwind_tables = false;

    bool fno_asynchronous_unwind_tables = false;

    bool no_pie = false;

};