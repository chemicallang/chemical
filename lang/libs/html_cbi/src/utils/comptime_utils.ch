
comptime func view(str : %literal_string) : std::string_view {
    return std::string_view(str);
}