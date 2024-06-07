// Copyright (c) Qinetik 2024.

#include <string>
#include <vector>

std::string resolve_rel_parent_path_str(const std::string &root_path, const std::string &file_path);

/**
 * result of '@' replacement in path
 */
struct AtReplaceResult {
    std::string replaced;
    std::string error; // empty if no error
};


class ImportPathHandler {
public:

    /**
     * path to the compiler's executable
     */
    std::string compiler_exe_path;

    /**
     * these are the resolved places where system headers paths exist
     * when its empty, its loaded directly by invoking clang (from self)
     * then once we found them we cache them here, for faster invocation next time
     */
    std::vector<std::string> system_headers_paths = {};

    /**
     * constructor
     */
    ImportPathHandler(std::string compiler_exe_path);

    /**
     * get containing system headers directory for the following header
     */
    std::string headers_dir(const std::string &header);

    /**
     * replace '@' in path
     */
    AtReplaceResult replace_at_in_path(const std::string &filePath);


};