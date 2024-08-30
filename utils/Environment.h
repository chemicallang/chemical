// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

bool set_environment_variable(const std::string& name, const std::string& value, bool for_system);

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