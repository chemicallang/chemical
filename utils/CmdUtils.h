// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <map>
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

    /**
     * This must not be empty ! argument keys stored in options map have "" values
     */
    std::string defOptValue = "true";
    /**
     * The map of options (--option) and arguments (--option argument)
     * where if another option is encountered after an option, the first is stored with value true
     * if arguments are envountered without options before them, they are stored with "" as values
     */
    std::map<std::string, std::string> options;

    void print() {
        for(const auto& opt : options) {
            std::cout << '-' << opt.first << ' ' << opt.second << ' ';
        }
    }

    /**
     * counts only the arguments
     * @return
     */
    unsigned int count_args(){
        unsigned int i = 0;
        for(const auto& x : options) {
            if(x.second.empty()) {
                i++;
            }
        }
        return i;
    }

    /**
     * when a argument of multi is encountered for example
     * cmd -m file.o file.o1
     * when -m is encountered file.o and file.o1 are collected into the vector and returned
     * @param multi
     * @return
     */
    std::vector<std::string> collect_multi(const std::string& multi) {
        std::vector<std::string> args;
        auto found = options.find(multi);
        while(found != options.end()) {
            if(found->second.empty()) {
                args.emplace_back(found->first);
            } else {
                break;
            }
            found++;
        }
        args.shrink_to_fit();
        return args;
    }

    /**
     * gives the value for a option for example
     * cmd --x file
     * give opt (x) to this function to get the value (file)
     * @param opt the complete option key used with --x (double dashes)
     * @param small_opt the small option key used with -x (single dash)
     * @return
     */
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

    /**
     * parses arguments / options into the options map
     * @param argc arg count
     * @param argv arg pointer
     * @param skip how many to skip before parsing
     * @param defOptValue the default value for an option, for example --print --use --done,
     * all these arguments don't have a value, you can give them default values (default true)
     * @param defArgValue the default value for a argument (not followed by a -- option)
     * for example clang x -o file.o, x here is a argument
     * @return
     */
    void parse_cmd_options(int argc, char *argv[], int skip = 0) {
        int i = skip;
        std::string option;
        while (i < argc) {
            auto x = argv[i];
            if (x[0] == '-') {
                if(!option.empty()) {
                    options[option] = defOptValue;
                }
                option = (x[1] == '-') ? (x + 2) : (x + 1);
            } else if (!option.empty()) {
                options[option] = x;
                option = "";
            } else {
                options[x] = "";
            }
            i++;
        }
        if(!option.empty()) {
            options[option] = defOptValue;
        }
    }

};