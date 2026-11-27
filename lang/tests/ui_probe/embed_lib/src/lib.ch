// P0.1 probe: `intrinsics::embed_file()` resolves relative to the *calling
// file*, so a library can embed its own asset without the app knowing a path.
public namespace ui_probe_embed_lib {

using std::string;

public func asset_text() : string {
    return string(intrinsics::embed_file("asset.txt"))
}

public func asset_len() : int {
    return string(intrinsics::embed_file("asset.txt")).size() as int
}

// Source location captured inside the library.
public func here_raw() : u64 {
    return intrinsics::get_raw_location() as u64
}

public func here_line() : int {
    return intrinsics::get_line_no() as int
}

}
