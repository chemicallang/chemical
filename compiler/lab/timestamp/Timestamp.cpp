// Copyright (c) Chemical Language Foundation 2025.

#include "Timestamp.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

/**
 * save mod timestamp data (modified date and file size) in a file that can be read later and compared
 * to check if files have changed
 */
void save_mod_timestamp(const std::vector<std::string_view>& files, const std::string_view& output_file) {
    std::ofstream ofs(output_file.data(), std::ios::binary);
    size_t num_files = files.size();
    ofs.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));
    for (const auto& file_abs_path : files) {
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

void save_mod_timestamp(const std::vector<ASTFileMetaData>& files, const std::string_view& output_file) {
    std::vector<std::string_view> paths;
    paths.reserve(files.size());
    for(const auto& f : files) {
        paths.emplace_back(f.abs_path);
    }
    save_mod_timestamp(paths, output_file);
}

void save_mod_timestamp(const std::vector<ASTFileResult*>& files, const std::string_view& output_file) {
    std::vector<std::string_view> paths;
    paths.reserve(files.size());
    for(const auto f : files) {
        // TODO: we should not be putting files that are external to module (imported using '@' usually)
        paths.emplace_back(f->abs_path);
    }
    save_mod_timestamp(paths, output_file);
}

bool compare_mod_timestamp(const std::vector<std::string_view>& files, const std::string_view& prev_timestamp_file) {
    std::ifstream ifs(prev_timestamp_file.data(), std::ios::binary);

    if (!ifs.is_open()) {
        return false;
    }

    size_t prev_num_files;
    ifs.read(reinterpret_cast<char*>(&prev_num_files), sizeof(prev_num_files));

    if (prev_num_files != files.size()) {
        return false;
    }

    for (const auto& file : files) {
        fs::path file_path(file);
        if (fs::exists(file_path)) {
            uintmax_t current_file_size = fs::file_size(file_path);
            auto current_mod_time = fs::last_write_time(file_path);

            size_t file_str_size;
            ifs.read(reinterpret_cast<char*>(&file_str_size), sizeof(file_str_size));

            std::string prev_file_str(file_str_size, '\0');
            ifs.read(&prev_file_str[0], (std::streamsize) file_str_size);

            uintmax_t prev_file_size;
            ifs.read(reinterpret_cast<char*>(&prev_file_size), sizeof(prev_file_size));

            fs::file_time_type prev_mod_time;
            ifs.read(reinterpret_cast<char*>(&prev_mod_time), sizeof(prev_mod_time));

            if (prev_file_str != file_path.string() || prev_file_size != current_file_size || prev_mod_time != current_mod_time) {
                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}

bool compare_mod_timestamp(const std::vector<ASTFileMetaData>& files, const std::string_view& prev_timestamp_file) {
    std::vector<std::string_view> paths;
    paths.reserve(files.size());
    for(const auto& f : files) {
        paths.emplace_back(f.abs_path);
    }
    return compare_mod_timestamp(paths, prev_timestamp_file);
}

bool compare_mod_timestamp(const std::vector<ASTFileResult*>& files, const std::string_view& prev_timestamp_file) {
    std::vector<std::string_view> paths;
    paths.reserve(files.size());
    for(const auto f : files) {
        paths.emplace_back(f->abs_path);
    }
    return compare_mod_timestamp(paths, prev_timestamp_file);
}