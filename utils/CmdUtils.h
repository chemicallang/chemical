// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <iostream>

void print_usage() {
    std::string usage = "chemical <input_file> -o <output_file>\n\n";
    usage += "<input_file> a chemical file path with .ch extension relative to current executable\n";
    usage += "<output_file> a .o (object) file or a .ll (llvm ir) file\n";
    std::cout << usage;
}

struct CmdOptions {

    std::unordered_map<std::string, std::string> options;
    std::vector<std::string> arguments;

    void print() {
        for(const auto& opt : arguments) {
            std::cout << opt << ' ';
        }
        for(const auto& opt : options) {
            std::cout << '-' << opt.first << ' ' << opt.second << ' ';
        }
        std::cout << "\n\n";
    }

    std::optional<std::string> option(const std::string& opt, const std::string& small_opt) {
        auto whole = options.find(opt);
        if(whole == options.end()) {
            auto half = options.find(small_opt);
            if(half == options.end()) {
                return std::nullopt;
            } else {
                return half->second;
            }
        } else {
            return whole->second;
        }
    }

};

CmdOptions parse_cmd_options(int argc, char *argv[], int skip = 0) {
    int i = skip;
    std::string option;
    std::unordered_map<std::string, std::string> options;
    std::vector<std::string> arguments;
    while (i < argc) {
        auto x = argv[i];
        if (x[0] == '-') {
            if(!option.empty()) {
                options[option] = "";
            }
            option = (x[1] == '-') ? x[2] : x[1];
        } else if (!option.empty()) {
            options[option] = x;
            option = "";
        } else {
            arguments.emplace_back(x);
        }
        i++;
    }
    if(!option.empty()) {
        options[option] = "";
    }
    return CmdOptions{ options, arguments };
}