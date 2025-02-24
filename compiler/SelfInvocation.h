// Copyright (c) Chemical Language Foundation 2025.

#include <vector>
#include <string>

/**
 * invokes command to capture the cout and returns it in output
 */
int invoke_capturing_out(const std::vector<std::string> &command_args, std::string &output);

/**
 * invokes self to get the current system headers paths
 * these paths include directories where to look for c and c++ headers
 */
std::vector<std::string> system_headers_path(const std::string& arg0);

/**
 * given system header search directories and a header
 * it looks for the header in the system headers to resolve its absolute path
 * if not found returns ""
 */
std::string header_abs_path(std::vector<std::string>& system_headers, const std::string& header);

/**
 * get containing headers directory
 */
std::string headers_dir(std::vector<std::string>& system_headers, const std::string& header);