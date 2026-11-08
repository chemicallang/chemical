using std::Result;
using std::vector;
using std::string;

// Focused tests for the safe archive surface (`Archive`, `ZipWriter`).
// Digest work is exercised by the legacy tests; these cover ownership,
// `&[u8]` parsing, `&str` lookups, and extraction safety.

func archive_safe_temp_pathbuf(name : *char) : path::PathBuf {
    var s = make_temp_test_path(name)
    return path::PathBuf.from(std::str.make_view(s.to_view()))
}

func archive_safe_view(v : &vector<u8>) : [u8] {
    return unsafe(std::span<u8>(v.data(), v.size()))
}

func archive_safe_fixed_view(p : *u8, n : size_t) : [u8] {
    return unsafe(std::span<u8>(p, n))
}

func archive_safe_bytes_equal(v : &vector<u8>, expected : *u8, n : size_t) : bool {
    if(v.size() != n) { return false }
    var i : size_t = 0
    while(i < n) {
        if(v.get(i) != unsafe(expected[i])) { return false }
        i += 1
    }
    return true
}

func archive_safe_build_zip(name : *char, data : *u8, n : size_t) : vector<u8> {
    var writer = archive::raw::ZipWriter{entries: vector<archive::raw::ZipWriteEntry>(), data: vector<u8>()}
    archive::raw::zip_writer_add_file(&raw mut writer, name, data, n)
    archive::raw::zip_writer_close(&raw mut writer)
    // Copy rather than returning the field: moving a vector field out of a
    // destructor-bearing local would leave the writer's drop glue live.
    var out = vector<u8>()
    var i : size_t = 0
    while(i < writer.data.size()) {
        out.push(writer.data.get(i))
        i += 1
    }
    return out
}

// ═══════════════════════════════════════════════════════════════
// Positive round-trips
// ═══════════════════════════════════════════════════════════════

@test
public func archive_safe_zip_from_bytes_roundtrip(env : &mut TestEnv) {
    var payload : [13]u8 = ['H' as u8, 'e' as u8, 'l' as u8, 'l' as u8, 'o' as u8, ',' as u8, ' ' as u8, 'W' as u8, 'o' as u8, 'r' as u8, 'l' as u8, 'd' as u8, '!' as u8]
    var zip = archive_safe_build_zip("hello.txt\0" as *char, &raw payload[0], 13)

    var opened = archive::Archive.from_bytes(archive_safe_view(&zip))
    if(opened is Result.Err) { env.error("from_bytes should parse ZIP"); return }
    var Ok(handle) = opened else unsafe { unreachable }

    if(handle.entry_count() != 1) { env.error("entry_count should be 1"); return }
    if(!handle.contains("hello.txt")) { env.error("contains should find hello.txt") }

    var entries = handle.entries()
    if(entries.size() != 1) { env.error("entries should have 1 item"); return }
    if(!entries.get_ref(0).name.equals_with_len("hello.txt", 9)) { env.error("entry name mismatch") }

    var read = handle.read_file("hello.txt")
    if(read is Result.Err) { env.error("read_file should succeed"); return }
    var Ok(content) = read else unsafe { unreachable }
    if(!archive_safe_bytes_equal(&content, &raw payload[0], 13)) { env.error("content mismatch") }
}

@test
public func archive_safe_zip_read_entry_and_open(env : &mut TestEnv) {
    var payload : [5]u8 = [0x41, 0x42, 0x43, 0x44, 0x45]
    var zip = archive_safe_build_zip("data.bin\0" as *char, &raw payload[0], 5)

    var zip_path = archive_safe_temp_pathbuf("p24_safe_archive.zip")
    var zs = zip_path.as_string()
    var wr = fs::raw::write_text_file(zs.c_str(), zip.data(), zip.size())
    if(wr is Result.Err) { env.error("failed to write zip fixture"); return }

    var opened = archive::Archive.open(&zip_path.as_path())
    if(opened is Result.Err) { env.error("open should parse ZIP"); return }
    var Ok(handle) = opened else unsafe { unreachable }

    var entry = handle.entry(0)
    if(entry is Result.Err) { env.error("entry(0) should exist"); return }
    var Ok(meta) = entry else unsafe { unreachable }
    if(meta.size != 5) { env.error("entry size should be 5") }

    var by_index = handle.read_entry(0)
    if(by_index is Result.Err) { env.error("read_entry(0) should succeed"); return }
    var Ok(bytes) = by_index else unsafe { unreachable }
    if(!archive_safe_bytes_equal(&bytes, &raw payload[0], 5)) { env.error("read_entry mismatch") }

    var out_of_bounds = handle.read_entry(7)
    if(out_of_bounds is Result.Ok) { env.error("read_entry(7) should fail") }
}

@test
public func archive_safe_tar_from_bytes_roundtrip(env : &mut TestEnv) {
    var payload : [4]u8 = [0x54, 0x45, 0x53, 0x54]
    var tar = build_single_file_tar(string("info.txt"), &raw payload[0], 4)

    var opened = archive::Archive.from_bytes(archive_safe_view(&tar))
    if(opened is Result.Err) { env.error("from_bytes should parse TAR"); return }
    var Ok(handle) = opened else unsafe { unreachable }

    if(handle.entry_count() != 1) { env.error("tar entry_count should be 1") }
    if(!handle.contains("info.txt")) { env.error("tar contains should find info.txt") }

    var read = handle.read_file("info.txt")
    if(read is Result.Err) { env.error("tar read_file should succeed"); return }
    var Ok(content) = read else unsafe { unreachable }
    if(!archive_safe_bytes_equal(&content, &raw payload[0], 4)) { env.error("tar content mismatch") }
}

@test
public func archive_safe_extract_writes_entries(env : &mut TestEnv) {
    var payload : [3]u8 = [0x41, 0x42, 0x43]
    var zip = archive_safe_build_zip("nested/hello.txt\0" as *char, &raw payload[0], 3)

    var opened = archive::Archive.from_bytes(archive_safe_view(&zip))
    if(opened is Result.Err) { env.error("from_bytes should parse ZIP"); return }
    var Ok(handle) = opened else unsafe { unreachable }

    var dest = archive_safe_temp_pathbuf("p24_safe_archive_dest")
    var ds = dest.as_string()
    fs::raw::remove_dir_all_recursive(ds.c_str())
    fs::raw::create_dir_all(ds.c_str())

    var extracted = handle.extract(&dest.as_path())
    if(extracted is Result.Err) { env.error("extract should succeed"); return }

    ds.append_view("/nested/hello.txt")
    var read_back = fs::raw::read_entire_file(ds.c_str())
    if(read_back is Result.Err) { env.error("extracted file should exist"); return }
    var Ok(bytes) = read_back else unsafe { unreachable }
    if(!archive_safe_bytes_equal(&bytes, &raw payload[0], 3)) { env.error("extracted content mismatch") }

    var cleanup = dest.as_string()
    fs::raw::remove_dir_all_recursive(cleanup.c_str())
}

@test
public func archive_safe_zip_writer_roundtrip(env : &mut TestEnv) {
    var payload : [5]u8 = [0x41, 0x42, 0x43, 0x44, 0x45]
    var writer = archive::ZipWriter.new()
    var added = writer.add_bytes("hello.txt", archive_safe_fixed_view(&raw payload[0], 5))
    if(added is Result.Err) { env.error("safe writer add_bytes should succeed"); return }

    var zip = writer.finish()
    var opened = archive::Archive.from_bytes(archive_safe_view(&zip))
    if(opened is Result.Err) { env.error("safe writer output should parse"); return }
    var Ok(handle) = opened else unsafe { unreachable }
    if(handle.entry_count() != 1) { env.error("safe writer entry count") }
    var read = handle.read_file("hello.txt")
    if(read is Result.Err) { env.error("safe writer read_file"); return }
    var Ok(content) = read else unsafe { unreachable }
    if(!archive_safe_bytes_equal(&content, &raw payload[0], 5)) { env.error("safe writer content") }

    // `finish` is idempotent, and adding after finish is rejected instead of
    // appending a second central directory.
    var again = writer.finish()
    if(again.size() != zip.size()) { env.error("finish should be idempotent") }
    var late = writer.add_bytes("late.txt", archive_safe_fixed_view(&raw payload[0], 5))
    if(late is Result.Ok) { env.error("add after finish should fail") }
}

@test
public func archive_safe_zip_writer_save(env : &mut TestEnv) {
    var payload : [3]u8 = [0x58, 0x59, 0x5A]
    var writer = archive::ZipWriter.new()
    var _ = writer.add_bytes("saved.bin", archive_safe_fixed_view(&raw payload[0], 3))

    var p = archive_safe_temp_pathbuf("p24_safe_writer.zip")
    var saved = writer.save(&p.as_path())
    if(saved is Result.Err) { env.error("safe writer save"); return }

    var opened = archive::Archive.open(&p.as_path())
    if(opened is Result.Err) { env.error("saved ZIP should open"); return }
    var Ok(handle) = opened else unsafe { unreachable }
    if(!handle.contains("saved.bin")) { env.error("saved ZIP should contain entry") }
}

@test
public func archive_safe_zip_writer_rejects_nul_name(env : &mut TestEnv) {
    var payload : [1]u8 = [0x01]
    var name_bytes = vector<u8>()
    name_bytes.push('b' as u8)
    name_bytes.push('a' as u8)
    name_bytes.push('d' as u8)
    name_bytes.push(0 as u8)
    name_bytes.push('x' as u8)
    var sv = unsafe(std::string_view(name_bytes.data() as *char, name_bytes.size()))
    var name = std::str.make_view(sv)

    var writer = archive::ZipWriter.new()
    var result = writer.add_bytes(name, archive_safe_fixed_view(&raw payload[0], 1))
    if(result is Result.Ok) { env.error("NUL in entry name must be rejected") }
}

// ═══════════════════════════════════════════════════════════════
// Misuse / hostile input
// ═══════════════════════════════════════════════════════════════

@test
public func archive_safe_bad_magic_rejected(env : &mut TestEnv) {
    var garbage : [16]u8
    var i : size_t = 0
    while(i < 16) { garbage[i] = i as u8; i += 1 }
    var view = archive_safe_fixed_view(&raw garbage[0], 16)
    var opened = archive::Archive.from_bytes(view)
    if(opened is Result.Ok) { env.error("garbage bytes should not parse") }
}

@test
public func archive_safe_truncated_rejected(env : &mut TestEnv) {
    var tiny : [4]u8 = [0x50, 0x4B, 0x03, 0x04]
    var view = archive_safe_fixed_view(&raw tiny[0], 4)
    var opened = archive::Archive.from_bytes(view)
    if(opened is Result.Ok) { env.error("truncated ZIP should not parse") }
}

@test
public func archive_safe_extract_rejects_dotdot(env : &mut TestEnv) {
    var payload : [1]u8 = [0x58]
    var zip = archive_safe_build_zip("../p24_escape.txt\0" as *char, &raw payload[0], 1)

    var opened = archive::Archive.from_bytes(archive_safe_view(&zip))
    if(opened is Result.Err) { env.error("fixture should parse"); return }
    var Ok(handle) = opened else unsafe { unreachable }

    var dest = archive_safe_temp_pathbuf("p24_zip_slip_dest")
    var ds = dest.as_string()
    fs::raw::create_dir_all(ds.c_str())

    var extracted = handle.extract(&dest.as_path())
    if(extracted is Result.Ok) { env.error("path traversal must be rejected"); return }
    var Err(err) = extracted else unsafe { unreachable }
    if(err !is archive::ArchiveError.UnsafePath) { env.error("expected UnsafePath error") }

    fs::raw::remove_dir_all_recursive(ds.c_str())
}

@test
public func archive_safe_extract_rejects_absolute(env : &mut TestEnv) {
    var payload : [1]u8 = [0x58]
    var zip = archive_safe_build_zip("/tmp/p24_absolute.txt\0" as *char, &raw payload[0], 1)

    var opened = archive::Archive.from_bytes(archive_safe_view(&zip))
    if(opened is Result.Err) { env.error("fixture should parse"); return }
    var Ok(handle) = opened else unsafe { unreachable }

    var dest = archive_safe_temp_pathbuf("p24_absolute_dest")
    var ds = dest.as_string()
    fs::raw::create_dir_all(ds.c_str())

    var extracted = handle.extract(&dest.as_path())
    if(extracted is Result.Ok) { env.error("absolute path must be rejected"); return }
    var Err(err) = extracted else unsafe { unreachable }
    if(err !is archive::ArchiveError.UnsafePath) { env.error("expected UnsafePath error") }

    fs::raw::remove_dir_all_recursive(ds.c_str())
}

@test
public func archive_safe_rejects_symlink_escape(env : &mut TestEnv) {
    comptime if(def.windows) {
        // POSIX symlink fixture; Windows extraction is covered by the name
        // checks in the other tests.
    } else {
        var payload : [1]u8 = [0x58]
        var zip = archive_safe_build_zip("link/p24_escape.txt\0" as *char, &raw payload[0], 1)

        var opened = archive::Archive.from_bytes(archive_safe_view(&zip))
        if(opened is Result.Err) { env.error("fixture should parse"); return }
        var Ok(handle) = opened else unsafe { unreachable }

        var dest = archive_safe_temp_pathbuf("p24_symlink_dest")
        var ds = dest.as_string()
        fs::raw::remove_dir_all_recursive(ds.c_str())
        fs::raw::create_dir_all(ds.c_str())

        var target = std::string("/tmp")
        var link = ds.copy()
        link.append_view("/link")
        var _ = unsafe(fs::symlink(target.c_str(), link.c_str()))

        var extracted = handle.extract(&dest.as_path())
        if(extracted is Result.Ok) { env.error("symlink escape must be rejected"); return }
        var Err(err) = extracted else unsafe { unreachable }
        if(err !is archive::ArchiveError.UnsafePath) { env.error("expected UnsafePath for symlink escape") }

        fs::raw::remove_dir_all_recursive(ds.c_str())
    }
}

@test
public func archive_safe_unknown_entry_is_error(env : &mut TestEnv) {
    var payload : [1]u8 = [0x01]
    var zip = archive_safe_build_zip("only.txt\0" as *char, &raw payload[0], 1)

    var opened = archive::Archive.from_bytes(archive_safe_view(&zip))
    if(opened is Result.Err) { env.error("fixture should parse"); return }
    var Ok(handle) = opened else unsafe { unreachable }

    if(handle.contains("missing.txt")) { env.error("missing name must not be contained") }
    var read = handle.read_file("missing.txt")
    if(read is Result.Ok) { env.error("missing name read should fail") }
}
