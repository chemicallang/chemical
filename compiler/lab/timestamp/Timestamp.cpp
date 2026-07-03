// Copyright (c) Chemical Language Foundation 2025.

#include "Timestamp.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

/**
 * save mod timestamp data (modified date and file size) in a file that can be read later and compared
 * to check if files have changed
 */
void save_mod_timestamp(const std::vector<std::string_view>& files, const std::string_view& output_file, OutputMode mode) {
    // sort files to ensure deterministic order regardless of source ordering
    std::vector<std::string_view> sorted_files(files.begin(), files.end());
    std::sort(sorted_files.begin(), sorted_files.end());

    std::ofstream ofs(output_file.data(), std::ios::binary);
    size_t num_files = sorted_files.size();
    ofs.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));

    // write the mode
    int mode_int = static_cast<int>(mode);
    ofs.write(reinterpret_cast<const char*>(&mode_int), sizeof(mode_int));

    for (const auto& file_abs_path : sorted_files) {
        fs::path file_path(file_abs_path);
        if (fs::exists(file_path)) {
            uintmax_t file_size = fs::file_size(file_path);
            auto mod_time = fs::last_write_time(file_path);
            size_t file_str_size = file_abs_path.size();
            ofs.write(reinterpret_cast<const char*>(&file_str_size), sizeof(file_str_size));
            ofs.write(file_abs_path.data(), (std::streamsize) file_str_size);
            ofs.write(reinterpret_cast<const char*>(&file_size), sizeof(file_size));
            ofs.write(reinterpret_cast<const char*>(&mod_time), sizeof(mod_time));
        }
    }
}

void save_mod_timestamp(const std::vector<ASTFileMetaData>& files, const std::string_view& output_file, OutputMode mode) {
    std::vector<std::string_view> paths;
    paths.reserve(files.size());
    for(const auto& f : files) {
        paths.emplace_back(f.abs_path);
    }
    save_mod_timestamp(paths, output_file, mode);
}

void save_mod_timestamp(const std::vector<ASTFileResult*>& files, const std::string_view& output_file, OutputMode mode) {
    std::vector<std::string_view> paths;
    paths.reserve(files.size());
    for(const auto f : files) {
        paths.emplace_back(f->abs_path);
    }
    save_mod_timestamp(paths, output_file, mode);
}

bool compare_mod_timestamp(const std::vector<std::string_view>& files, const std::string_view& prev_timestamp_file, OutputMode mode) {
    std::ifstream ifs(prev_timestamp_file.data(), std::ios::binary);
    if (!ifs.is_open()) return false;

    size_t prev_num_files;
    ifs.read(reinterpret_cast<char*>(&prev_num_files), sizeof(prev_num_files));
    if (prev_num_files != files.size()) return false;

    int prev_mode_int;
    ifs.read(reinterpret_cast<char*>(&prev_mode_int), sizeof(prev_mode_int));
    if (prev_mode_int != static_cast<int>(mode)) return false;

    // sort current files to match the saved (sorted) order
    std::vector<std::string_view> sorted(files.begin(), files.end());
    std::sort(sorted.begin(), sorted.end());

    for (size_t i = 0; i < prev_num_files; i++) {
        size_t file_str_size;
        ifs.read(reinterpret_cast<char*>(&file_str_size), sizeof(file_str_size));

        // fast path: skip the string allocation if sizes don't match
        if (sorted[i].size() != file_str_size) return false;

        // read saved path and compare directly with the sorted current path
        std::string saved_path(file_str_size, '\0');
        ifs.read(&saved_path[0], (std::streamsize) file_str_size);
        if (saved_path != sorted[i]) return false;

        uintmax_t saved_size;
        ifs.read(reinterpret_cast<char*>(&saved_size), sizeof(saved_size));

        fs::file_time_type saved_mod_time;
        ifs.read(reinterpret_cast<char*>(&saved_mod_time), sizeof(saved_mod_time));

        // stat the current file
        fs::path file_path(sorted[i]);
        if (!fs::exists(file_path)) return false;

        uintmax_t current_file_size = fs::file_size(file_path);
        auto current_mod_time = fs::last_write_time(file_path);

        if (saved_size != current_file_size || saved_mod_time != current_mod_time) {
            return false;
        }
    }

    return true;
}

bool compare_mod_timestamp(const std::vector<ASTFileMetaData>& files, const std::string_view& prev_timestamp_file, OutputMode mode) {
    std::vector<std::string_view> paths;
    paths.reserve(files.size());
    for(const auto& f : files) {
        paths.emplace_back(f.abs_path);
    }
    return compare_mod_timestamp(paths, prev_timestamp_file, mode);
}

bool compare_mod_timestamp(const std::vector<ASTFileResult*>& files, const std::string_view& prev_timestamp_file, OutputMode mode) {
    std::vector<std::string_view> paths;
    paths.reserve(files.size());
    for(const auto f : files) {
        paths.emplace_back(f->abs_path);
    }
    return compare_mod_timestamp(paths, prev_timestamp_file, mode);
}