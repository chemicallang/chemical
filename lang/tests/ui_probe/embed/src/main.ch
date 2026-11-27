// P0.1: embed_file + source location from an imported library module.
using std::string;

public func main() : int {
    var text = ui_probe_embed_lib::asset_text()
    var len = ui_probe_embed_lib::asset_len()
    var raw = ui_probe_embed_lib::here_raw()
    var line = ui_probe_embed_lib::here_line()

    if(len == 47 && text.size() == 47u) {
        println("P0.1 embed_file_from_library PASS len=47")
    } else {
        println("P0.1 embed_file_from_library FAIL")
        return 1
    }
    if(raw != 0u64 && line > 0) {
        println("P0.1 library_source_location PASS")
    } else {
        println("P0.1 library_source_location FAIL")
        return 1
    }
    return 0
}
