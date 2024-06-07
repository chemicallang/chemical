// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <iostream>
#include <cstring>

#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_RESET   "\x1b[0m"

inline std::string cmd_error(const std::string& err) {
    return ANSI_COLOR_RED + err + ANSI_COLOR_RESET;
}

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

    /**
     * just prints the command to cout
     */
    void print(std::ostream& out = std::cout) {
        for(const auto& opt : options) {
            if(opt.second.empty()) {
                out << opt.first << ' ';
            } else {
                out << '-' << opt.first << ' ' << opt.second << ' ';
            }
        }
    }

    /**
     * prints unconsumed options as errs
     */
    void print_unhandled() {
        if(!options.empty()) {
            std::cerr << ANSI_COLOR_RED << "unhandled arguments given -> ";
        }
        print(std::cerr);
        std::cerr << ANSI_COLOR_RESET;
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
    std::optional<std::string> option(const std::string& opt, const std::string& small_opt, bool consume = true) {
        auto whole = options.find(opt);
        if(whole == options.end()) {
            auto half = options.find(small_opt);
            if(half == options.end()) {
                return std::nullopt;
            } else {
                auto value = std::move(half->second);
                if(consume) options.erase(half);
                return value;
            }
        } else {
            auto value = std::move(whole->second);
            if(consume) options.erase(whole);
            return value;
        }
    }

    std::string option_e(const std::string& opt, const std::string& small_opt, bool consume = true) {
        auto got = option(opt, small_opt, consume);
        return got.has_value() ? got.value() : "";
    }

    std::vector<std::string> collect_subcommand(int argc, char *argv[], const std::string& subcommand, int skip = 0, bool consume = true) {
        int i = skip;
        std::vector<std::string> args;
        bool collect = false;
        while (i < argc) {
            auto x = argv[i];
            if (!collect && strncmp(x, subcommand.c_str(), subcommand.size()) == 0) {
                collect = true;
            } else if(collect) {
                args.emplace_back(x);
            }
            i++;
        }
        return args;
    }

    /**
     * parses arguments / options into the options map
     * @param argc arg count
     * @param argv arg pointer
     * @param skip how many to skip before parsing
     * @param defOptValue the default value for an option, for example --print --use --done,
     * all these arguments don't have a value, you can give them default values (default true)
     * for example clang x -o file.o, x here is a argument
     * @return the first args, cmd file.o file.o1, these file.o and file.o1 are returned
     */
    std::vector<std::string> parse_cmd_options(int argc, char *argv[], int skip = 0, const std::vector<std::string>& subcommands = {}, bool add_subcommand = true) {
        int i = skip;
        std::vector<std::string> args;
        std::string option;
        while (i < argc) {
            auto x = argv[i];
            // check if it's a subcommand
            bool found = false;
            for(const auto& sub : subcommands) {
                if(sub == x) {
                    if(add_subcommand) options[sub] = defOptValue;
                    found = true;
                    break;
                }
            }
            if(found) break;
            // check if it's -option
            if (x[0] == '-') {
                if(!option.empty()) {
                    options[option] = defOptValue;
                }
                option = (x[1] == '-') ? (x + 2) : (x + 1);
            } else {
                // it's a value for the previous option
                if(!option.empty()) {
                    options[option] = x;
                    option = "";
                } else {
                    // it's a argument
                    args.emplace_back(x);
                }
            }
            i++;
        }
        if(!option.empty()) {
            options[option] = defOptValue;
        }
        return args;
    }

};