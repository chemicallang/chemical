// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <iostream>
#include <cstring>
#include "ordered_map.h"

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

// Custom hash functor
struct StringHash {
    using is_transparent = void; // Enables heterogeneous lookup
    std::size_t operator()(const std::string& str) const noexcept {
        return std::hash<std::string>{}(str);
    }
    std::size_t operator()(const std::string_view& str) const noexcept {
        return std::hash<std::string_view>{}(str);
    }
};

// Custom equality functor
struct StringEqual {
    using is_transparent = void; // Enables heterogeneous lookup
    bool operator()(const std::string& lhs, const std::string& rhs) const noexcept {
        return lhs == rhs;
    }
    bool operator()(const std::string_view& lhs, std::string_view rhs) const noexcept {
        return lhs == rhs;
    }
    bool operator()(const std::string& lhs, std::string_view rhs) const noexcept {
        return lhs == rhs;
    }
    bool operator()(const std::string_view& lhs, const std::string& rhs) const noexcept {
        return lhs == rhs;
    }
};

enum class CmdOptionType {
    // a small option is an option with a smaller key
    // for example -o for output, it has a single dash in front
    SmallOption,
    // a large option is an option with a larger key
    // for example --output for output
    // large options usually have double dashes in front
    LargeOption,
    // an option with multiple values, that will appear multiple times
    // -file a.c -file b.c -file c.c
    MultiValued,
    // a sub command, after this option occurs, every other option is put into a vector of arguments
    SubCommand
};

struct CmdOption {

    CmdOptionType type;
    std::string_view description;

    union {

        struct {
            std::optional<std::string_view> value;
        } simple;

        struct {
            std::vector<std::string_view> values;
            bool has_value;
        } multi_value;

    };

    CmdOption(CmdOptionType type, std::string_view description) : type(type), description(description) {
        switch(type) {
            case CmdOptionType::SmallOption:
            case CmdOptionType::LargeOption:
                new (&simple.value) std::optional<std::string_view>(std::nullopt);
                break;
            case CmdOptionType::MultiValued:
            case CmdOptionType::SubCommand:
                new (&multi_value.values) std::vector<std::string_view>();
                multi_value.has_value = false;
                break;
        }
    }

    CmdOption(CmdOption&& other) : type(other.type), description(other.description) {
        switch(type) {
            case CmdOptionType::SmallOption:
            case CmdOptionType::LargeOption:
                new(&simple.value) std::optional<std::string_view>(other.simple.value);
                break;
            case CmdOptionType::MultiValued:
            case CmdOptionType::SubCommand:
                new(&multi_value.values) std::vector(std::move(other.multi_value.values));
                multi_value.has_value = other.multi_value.has_value;
                break;
        }
    }

    /**
     * TODO: avoid this function
     */
    void put_multi_value_vec(std::vector<std::string>& args) {
        for(auto& value : multi_value.values) {
            args.emplace_back(value);
        }
    }

    /**
     * calling this method is not recommended if the option is not known to be a multi valued option
     */
    inline bool has_multi_value() {
        return multi_value.has_value;
    }

    /**
     * take sub command if interested
     */
    bool take_subcommand(int& i, int argc, char *argv[]) {
        if(type == CmdOptionType::SubCommand) {
            multi_value.has_value = true;
            // skip one argument, this is probably the command name
            i++;
            while(i < argc) {
                multi_value.values.emplace_back(argv[i]);
                i++;
            }
            return true;
        } else {
            return false;
        }
    }

    void put_value(const std::string_view& value) {
        switch(type) {
            case CmdOptionType::SmallOption:
            case CmdOptionType::LargeOption:
                simple.value = value;
                break;
            case CmdOptionType::MultiValued:
            case CmdOptionType::SubCommand:
                multi_value.values.emplace_back(value);
                break;
        }
    }

    ~CmdOption() {
        switch(type) {
            case CmdOptionType::SmallOption:
            case CmdOptionType::LargeOption:
                simple.value.~optional();
                break;
            case CmdOptionType::MultiValued:
            case CmdOptionType::SubCommand:
                multi_value.values.~vector();
                break;
        }
    }

};

struct CmdOptions {

    /**
     * this contains the data for every option
     * when we encounter an option, we check this map to see what kind of option it is
     */
    std::unordered_map<std::string_view, CmdOption> data;

    /**
     * arguments, when a value doesn't have an option for example file1.h -include file.h
     * here file1.h is an argument, where file.h is a value for the option -include
     */
    std::vector<std::string_view> arguments;

    /**
     * This must not be empty ! argument keys stored in options map have "" values
     */
    std::string defOptValue = "true";
    /**
     * The map of options (--option) and arguments (--option argument)
     * where if another option is encountered after an option, the first is stored with value true
     * if arguments are encountered without options before them, they are stored with "" as values
     */
    tsl::ordered_map<std::string, std::string, StringHash, StringEqual> options;

    /**
     * options having multiple values are taken out of options and stored in this map instead
     */
    tsl::ordered_map<std::string, std::vector<std::string>, StringHash, StringEqual> multi_val_options;

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

    std::optional<std::pair<std::string, std::string>> at(unsigned i) {
        const auto itr = options.begin() + i;
        if(itr != options.end()) {
            return std::pair<std::string, std::string> { itr->first, itr->second };
        } else {
            return std::nullopt;
        }
    }

    /**
     * this method should only be called, if cmd option is known to exist
     */
    CmdOption& cmd_opt(const std::string_view& opt) {
        return data.find(opt)->second;
    }

    /**
     * get the single value for the given arg
     */
    std::optional<std::string_view> new_opt(const std::string_view& opt, std::string_view& small_opt) {
        if(!opt.empty()) {
            auto found = data.find(opt);
            if(found != data.end()) {
                return found->second.simple.value;
            } else {
                // todo remove this
                auto next_found = options.find(opt);
                if(next_found != options.end()) {
                    return next_found->second;
                }
            }
        }
        if(!small_opt.empty()) {
            auto found = data.find(small_opt);
            if(found != data.end()) {
                return found->second.simple.value;
            } else {
                // todo remove this
                auto next_found = options.find(small_opt);
                if(next_found != options.end()) {
                    return next_found->second;
                }
            }
        }
        return std::nullopt;
    }

    /**
     * gives the value for a option for example
     * cmd --x file
     * give opt (x) to this function to get the value (file)
     * @param opt the complete option key used with --x (double dashes)
     * @param small_opt the small option key used with -x (single dash)
     * @return
     */
    std::optional<std::string> option(const std::string_view& opt, const std::string_view& small_opt = "", bool consume = true) {
        auto whole = options.find(opt);
        if(whole == options.end()) {
            if(small_opt.empty()) {
                return std::nullopt;
            }
            auto half = options.find(small_opt);
            if(half == options.end()) {
                return std::nullopt;
            } else {
                // take the string forcefully
                auto value = std::move(const_cast<std::string&>(half->second));
                if(consume) options.erase(half);
                return value;
            }
        } else {
            // take the string forcefully
            auto value = std::move(const_cast<std::string&>(whole->second));
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

    void put_option(const std::string_view& option, bool is_large_opt, const std::string_view& value) {
        auto found = data.find(option);
        if(found != data.end()) {
            found->second.put_value(value);
        } else {
            options[std::string(option)] = value;
        }
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
    void parse_cmd_options(int argc, char *argv[], int skip = 0) {

        std::string_view option;
        bool is_option_large_opt = false;

        int i = skip;
        while (i < argc) {

            const auto user_arg = std::string_view(argv[i]);

            const auto has_dash_in_front = user_arg[0] == '-';
            const auto has_at_least_size_2 = user_arg.size() > 1;

            const auto source_opt = has_dash_in_front && !has_at_least_size_2;
            if(source_opt) {
                // cannot handle source option at the moment
                i++;
                continue;
            }

            if(has_at_least_size_2) {

                if(has_dash_in_front) {

                    const auto is_large_opt = user_arg[1] == '-';
                    const auto option_key_offset = is_large_opt ? 2 : 1;
                    const auto option_key = std::string_view(argv[i] + option_key_offset,user_arg.size() - option_key_offset);

                    if (!option.empty()) {
                        // has an option, however user writes another option
                        // put previous option with a default value first
                        put_option(option, is_option_large_opt, defOptValue);
                    }

                    // set this option as current opt
                    option = option_key;
                    is_option_large_opt = is_large_opt;

                } else {
                    if(option.empty()) {
                        auto found = data.find(user_arg);
                        if(found != data.end()) {
                            auto& opt = found->second;
                            if(!opt.take_subcommand(i, argc, argv)) {

                                // TODO: error given option does not take sub arguments

                            }
                        } else {
                            // an argument = has no option, no dash in front, not registered cmd option
                            arguments.emplace_back(user_arg);
                        }
                    } else {
                        // value for an option = has an option, value with no dash in front
                        put_option(option, is_option_large_opt, user_arg);
                        option = "";
                        is_option_large_opt = false;
                    }
                }

            } else {

                if(option.empty()) {

                    // probably a sub command, no dash in front, only size 1, no option
                    auto found = data.find(user_arg);
                    if (found != data.end()) {
                        auto& opt = found->second;
                        if(!opt.take_subcommand(i, argc, argv)) {

                            // TODO: error given option does not take sub arguments

                        }
                    } else {

                        // TODO: error unknown option given

                    }

                } else {

                    put_option(option, is_option_large_opt, user_arg);

                }

            }
            i++;
        }

        if(!option.empty()) {
            put_option(option, is_option_large_opt, defOptValue);
        }

    }

};