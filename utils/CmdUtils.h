// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <iostream>
#include <cstring>
#include "ordered_map.h"
#include "std/except.h"

enum class CmdOptionType {
    // an option that doesn't require a value --help, --version
    NoValue,
    // an option that requires a single value, --mode debug
    SingleValue,
    // an option with multiple values, that will appear multiple times
    // -file a.c -file b.c -file c.c
    MultiValued,
    // a sub command, after this option occurs, every other option is put into a vector of arguments
    SubCommand
};

struct CmdOption {
private:
    union {

        struct {
            std::optional<std::string_view> value;
        } simple;

        struct {
            std::vector<std::string_view> values;
        } multi_value;

    };
public:

    std::string_view large_opt;
    std::string_view small_opt;
    CmdOptionType type;
    std::string_view description;
    bool user_used_large_opt = false;
    bool is_initialized = false;

    CmdOption(std::string_view large_opt, std::string_view small_opt, CmdOptionType type, std::string_view description) : large_opt(large_opt), small_opt(small_opt), type(type), description(description) {

    }

    void initialize() {
        if(is_initialized) return;
        switch(type) {
            case CmdOptionType::NoValue:
            case CmdOptionType::SingleValue:
                new (&simple.value) std::optional<std::string_view>(std::nullopt);
                break;
            case CmdOptionType::MultiValued:
            case CmdOptionType::SubCommand:
                new (&multi_value.values) std::vector<std::string_view>();
                break;
        }
        is_initialized = true;
    }

    inline CmdOption(std::string_view large_opt, CmdOptionType type, std::string_view description) : CmdOption(large_opt, "", type, description) {

    }

    CmdOption(CmdOption&& other) : large_opt(other.large_opt), small_opt(other.small_opt), type(other.type), description(other.description) {
        if(!other.is_initialized) return;
        switch(type) {
            case CmdOptionType::NoValue:
            case CmdOptionType::SingleValue:
                new(&simple.value) std::optional<std::string_view>(other.simple.value);
                break;
            case CmdOptionType::MultiValued:
            case CmdOptionType::SubCommand:
                new(&multi_value.values) std::vector(std::move(other.multi_value.values));
                break;
        }
        is_initialized = true;
    }

    /**
     * this gets the values as strings into the vector
     */
    void get_multi_value_vec(std::vector<std::string>& outArgs) {
        if(!is_initialized) return;
        for(auto& value : multi_value.values) {
            outArgs.emplace_back(value);
        }
    }

    /**
     * puts all multi value
     */
    void get_multi_value_vec(std::vector<chem::string_view>& outArgs) {
        if(!is_initialized) return;
        for(auto& value : multi_value.values) {
            outArgs.emplace_back(value);
        }
    }

    /**
     * calling this method is not recommended if the option is not known to be a multi valued option
     */
    inline bool has_multi_value() {
        return is_initialized;
    }

    /**
     * take sub command if interested
     */
    bool put_subcommand(int& i, int argc, char *argv[]) {
        if(type == CmdOptionType::SubCommand) {
            initialize();
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

    void put_value(const std::string_view& value, bool is_large_opt) {
        initialize();
        switch(type) {
            case CmdOptionType::NoValue:
            case CmdOptionType::SingleValue:
                user_used_large_opt = is_large_opt;
                simple.value = value;
                break;
            case CmdOptionType::MultiValued:
            case CmdOptionType::SubCommand:
                multi_value.values.emplace_back(value);
                break;
        }
    }

    /**
     * get the single option value (if this is a single value opt)
     */
    std::optional<std::string_view>* get_single_opt_value_init() {
        initialize();
        return &simple.value;
    }

    /**
     * get multi opt values (if this is a multi opt)
     */
    std::span<std::string_view> get_multi_opt_values() {
        if(!is_initialized) return { };
        return multi_value.values;
    }

    ~CmdOption() {
        if(!is_initialized) return;
        switch(type) {
            case CmdOptionType::SingleValue:
            case CmdOptionType::NoValue:
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
    std::unordered_map<std::string_view, CmdOption&> data;

    /**
     * arguments, when a value doesn't have an option for example file1.h -include file.h
     * here file1.h is an argument, where file.h is a value for the option -include
     */
    std::vector<std::string_view> arguments;

    /**
     * This must not be empty ! argument keys stored in options map have "" values
     */
    std::string_view defOptValue = "true";
    /**
     * The map of options (--option) and arguments (--option argument)
     * where if another option is encountered after an option, the first is stored with value true
     * if arguments are encountered without options before them, they are stored with "" as values
     */
    tsl::ordered_map<std::string_view, std::string_view> options;

    /**
     * register the given options array
     */
    void register_options(CmdOption options_data[], unsigned size) {
        data.reserve(size);
        unsigned i = 0;
        while(i < size) {
            auto& d = options_data[i];
            if(!d.large_opt.empty()) {
                data.emplace(d.large_opt, d);
            }
            if(!d.small_opt.empty()) {
                data.emplace(d.small_opt, d);
            }
            i++;
        }
    }

    /**
     * this method should only be called, if cmd option is known to exist
     */
    CmdOption& cmd_opt(const std::string_view& opt) {
        return data.find(opt)->second;
    }

    /**
     * get pointer to the single option's value
     */
    CmdOption* opt_val_ptr(const std::string_view& opt, const std::string_view& small_opt) {
        if(!opt.empty()) {
            auto found = data.find(opt);
            if(found != data.end()) {
                return &found->second;
            }
        }
        if(!small_opt.empty()) {
            auto found = data.find(small_opt);
            if(found != data.end()) {
                return &found->second;
            }
        }
#ifdef DEBUG
        CHEM_THROW_RUNTIME("data for option doesn't exist");
#endif
        return nullptr;
    }

    /**
     * get pointer to the single option's value
     */
    std::optional<std::string_view>* single_opt_val_ptr(const std::string_view& opt, const std::string_view& small_opt) {
        const auto ptr = opt_val_ptr(opt, small_opt);
        if(ptr) return ptr->get_single_opt_value_init();
        return nullptr;
    }

    /**
     * check if option has value
     */
    bool has_value(const std::string_view& opt, const std::string_view& small_opt) {
        auto value = single_opt_val_ptr(opt, small_opt);
        return value->has_value();
    }

    /**
     * check if option has value
     */
    bool has_value(const std::string_view& opt) {
        return has_value(opt, "");
    }

    /**
     * check a single option
     */
    std::optional<std::string_view>& option_new(const std::string_view& opt, const std::string_view& small_opt) {
        return *single_opt_val_ptr(opt, small_opt);
    }

    /**
     * check a single option
     */
    std::optional<std::string_view>& option_new(const std::string_view& opt) {
        return *single_opt_val_ptr(opt, "");
    }

    void put_option(const std::string_view& option, bool is_large_opt, const std::string_view& value) {
        auto found = data.find(option);
        if(found != data.end()) {
            found->second.put_value(value, is_large_opt);
        } else {
            options[option] = value;
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
                            if(!opt.put_subcommand(i, argc, argv)) {

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
                        if(!opt.put_subcommand(i, argc, argv)) {

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