import "@std/string_view.ch"

@comptime
func view(str : literal<string>) : std::string_view {
    return std::string_view(str);
}