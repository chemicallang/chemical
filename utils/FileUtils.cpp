// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 25/02/2024.
//

#include "FileUtils.h"
#include <fstream>
#include <filesystem>
#include <iostream>

bool exists_with_error(const std::string& build_dir) {
    std::error_code ec;
    const auto result = std::filesystem::exists(build_dir, ec);
    if(ec) {
        std::cerr << "error: couldn't check directory '" << build_dir << "' because '" << ec.message() << '\'' << std::endl;
        return false;
    } else {
        return result;
    }
}

bool create_dir_no_check(const std::string& build_dir) {
    std::error_code ec;
    std::filesystem::create_directory(build_dir, ec);
    if(ec) {
        std::cerr << "error: couldn't create directory '" << build_dir << "' because '" << ec.message() << '\'' << std::endl;
        return false;
    } else {
        return true;
    }
}

bool create_dir(const std::string& build_dir) {
    if (!exists_with_error(build_dir)) {
        return create_dir_no_check(build_dir);
    } else {
        return true;
    }
}

void writeToFile(const std::string &path, const std::string_view& text) {
    std::ofstream stream(path,std::ios::trunc);
    stream << text;
    stream.close();
}

void writeAsciiToFile(const std::string &path, const std::string &text) {
    std::ofstream stream(path,std::ios::trunc);
    for(const auto c : text) {
        // Write the original character
        stream << c << "<";
        // Write the ASCII code of the character
        stream << static_cast<int>(c) << ">";
    }
    stream.close();
}

void writeToProjectFile(const std::string &path, const std::string &text) {
    writeToFile("D:/Programming/Chemical/chemical/" + path, text);
}

void writeAsciiToProjectFile(const std::string &path, const std::string &text) {
    writeAsciiToFile("D:/Programming/Chemical/chemical/" + path, text);
}