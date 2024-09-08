// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

/**
 * will set the given environment, caller must have permission required to set the env variable
 */
bool set_environment_variable(const std::string& name, const std::string& value, bool for_system);

/**
 * will add the given path to PATH environment variable
 */
bool add_to_PATH(const std::string& new_path, bool for_system);

/**
 * will use the command and read it's output and return it
 */
std::string cmd_read_out(const std::string& command);

/**
 * will read stdout and redirect stderr to null
 */
inline std::string cmd_read_out_stderr_to_null(const std::string& command) {
#ifdef _WIN32
    return cmd_read_out(command + " 2>NUL");
#else
    return cmd_read_out(command + " 2>/dev/null");
#endif
}

/**
 * will read stdout and redirect stderr to stdout as well
 */
inline std::string cmd_read_out_stderr_to_out(const std::string& command) {
    return cmd_read_out(command + " 2>&1");
}

/**
 * this will get existing chemical executable's complete path
 * this executable path is located inside the PATH environment variable
 */
inline std::string chemical_exe_PATH_name() {
#ifdef DEBUG
#ifdef _WIN32
    return "Compiler.exe";
#else
    return "Compiler";
#endif
#else
#ifdef _WIN32
    return "chemical.exe";
#else
    return "chemical";
#endif
#endif
}

std::string get_chemical_version_in_PATH();

#ifdef _WIN32

/**
 * check on windows if has administrator privileges
 */
bool isAdmin();

/**
 * relaunch the executable as administrator on windows
 * returns false if does not succeed, when succeeds the current
 * process is terminated
 */
bool relaunchAsAdmin();

#else

/**
 * check if this is sudo in linux
 */
bool isSudo();

/**
 * try requesting sudo, retruns true if gains sudo priveleges
 */
bool requestSudo();

#endif