// Copyright (c) Qinetik 2024.

#pragma once

/**
 * You select the release or debug mode by giving this command line parameter
 * -mode value
 * where value can be replaced by one of these debug, debug_quick, release, release_aggressive
 *
 * Release modes allow you to change properties quickly for code generation, The parameters customized
 * by release mode, may have more customized commands separately
 */
enum class OutputMode {

    /**
     * The debug mode, which also contains debug information, about the executable
     * In the case where, invocation is by an IDE, we probably will use this mode, to help you
     * better debug your executable
     */
    Debug,

    /**
     * an executable is outputted without any debug information and also no optimizations
     * for quick executable output, this mode may be preferred
     */
    DebugQuick,

    /**
     * The executable is outputted with debug information, This takes some time to generate
     * the executable because all assertions are on, If you want to check your code thoroughly
     * you should use this mode, This gives us complete guarantee that we can take as much time as
     * we want
     */
    DebugComplete,

    /**
     * The release executable, The default optimizations for a release executable are applied
     * No debug information is added to executable, Optimizations are performed for performance
     */
    ReleaseFast,

    /**
     * This mode is same as release, except the code generated might be a little slow, This mode
     * keeps the executable size down, It may sacrifice performance to do that
     */
    ReleaseSmall,

    /**
     * safe code will be preferred over fast code
     */
    ReleaseSafe

};

inline bool is_debug(OutputMode mode) {
    return mode == OutputMode::Debug || mode == OutputMode::DebugQuick || mode == OutputMode::DebugComplete;
}

inline bool is_release(OutputMode mode) {
    return !is_debug(mode);
}

class CodegenEmitterOptions;

void configure_emitter_opts(OutputMode mode, CodegenEmitterOptions* options);