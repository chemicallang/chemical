// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#include "FileUtils.h"
#include <fstream>

void writeToFile(const std::string &path, const std::string &text) {
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