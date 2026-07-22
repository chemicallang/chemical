using std::Result;
using std::vector;
using std::string;

// ═══════════════════════════════════════════════════════════════
// CRC32 tests
// ═══════════════════════════════════════════════════════════════

@test
public func crc32_basic_works(env : &mut TestEnv) {
    var data : [5]u8 = ['h' as u8, 'e' as u8, 'l' as u8, 'l' as u8, 'o' as u8]
    var crc = archive::crc32_compute(&raw data[0], 5)
    if(crc != 0x3610a686u32) {
        env.error("CRC32 of 'hello' should be 0x3610a686")
    }
}

@test
public func crc32_empty_works(env : &mut TestEnv) {
    var crc = archive::crc32_compute(null, 0)
    if(crc != 0x00000000u32) {
        env.error("CRC32 of empty data should be 0")
    }
}

@test
public func crc32_different_inputs(env : &mut TestEnv) {
    var data1 : [1]u8 = [0x00]
    var data2 : [1]u8 = [0x01]
    var crc1 = archive::crc32_compute(&raw data1[0], 1)
    var crc2 = archive::crc32_compute(&raw data2[0], 1)
    if(crc1 == crc2) { env.error("different inputs should have different CRCs") }
}

@test
public func crc32_update_basic(env : &mut TestEnv) {
    var part1 : [2]u8 = ['h' as u8, 'e' as u8]
    var part2 : [3]u8 = ['l' as u8, 'l' as u8, 'o' as u8]
    var full : [5]u8 = ['h' as u8, 'e' as u8, 'l' as u8, 'l' as u8, 'o' as u8]

    var full_crc = archive::crc32_compute(&raw full[0], 5)
    var incremental_crc = archive::crc32_compute(&raw part1[0], 2)
    incremental_crc = archive::crc32_update(incremental_crc, &raw part2[0], 3)

    if(incremental_crc != full_crc) { env.error("incremental CRC should match full CRC") }
}

@test
public func crc32_update_empty(env : &mut TestEnv) {
    var data : [3]u8 = [0x41, 0x42, 0x43]
    var crc = archive::crc32_compute(&raw data[0], 3)
    var updated = archive::crc32_update(crc, null, 0)
    if(updated != crc) { env.error("CRC update with empty data should be idempotent") }
}

// ═══════════════════════════════════════════════════════════════
// Endian tests
// ═══════════════════════════════════════════════════════════════

@test
public func endian_u16_le_roundtrip(env : &mut TestEnv) {
    var buf : [2]u8
    archive::write_u16_le(&raw mut buf[0], 0, 0x1234u16)
    if(buf[0] != 0x34u8 || buf[1] != 0x12u8) {
        env.error("u16 LE write failed")
    }
    var val = archive::read_u16_le(&raw buf[0], 0)
    if(val != 0x1234u16) {
        env.error("u16 LE roundtrip failed")
    }
}

@test
public func endian_u32_le_roundtrip(env : &mut TestEnv) {
    var buf : [4]u8
    archive::write_u32_le(&raw mut buf[0], 0, 0xDEADBEEFu32)
    var val = archive::read_u32_le(&raw buf[0], 0)
    if(val != 0xDEADBEEFu32) {
        env.error("u32 LE roundtrip failed")
    }
}

@test
public func endian_u64_le_roundtrip(env : &mut TestEnv) {
    var buf : [8]u8
    archive::write_u64_le(&raw mut buf[0], 0, 0x0123456789ABCDEFu64)
    var val = archive::read_u64_le(&raw buf[0], 0)
    if(val != 0x0123456789ABCDEFu64) {
        env.error("u64 LE roundtrip failed")
    }
}

@test
public func endian_u16_le_read_function(env : &mut TestEnv) {
    var buf : [2]u8
    buf[0] = 0xCD; buf[1] = 0xAB
    var val = archive::read_u16_le(&raw buf[0], 0)
    if(val != 0xABCDu16) { env.error("read_u16_le") }
}

@test
public func endian_u32_le_read_function(env : &mut TestEnv) {
    var buf : [4]u8
    buf[0] = 0x78; buf[1] = 0x56; buf[2] = 0x34; buf[3] = 0x12
    var val = archive::read_u32_le(&raw buf[0], 0)
    if(val != 0x12345678u32) { env.error("read_u32_le") }
}

@test
public func endian_u64_le_read_function(env : &mut TestEnv) {
    var buf : [8]u8
    buf[0] = 0xEF; buf[1] = 0xCD; buf[2] = 0xAB; buf[3] = 0x89
    buf[4] = 0x67; buf[5] = 0x45; buf[6] = 0x23; buf[7] = 0x01
    var val = archive::read_u64_le(&raw buf[0], 0)
    if(val != 0x0123456789ABCDEFu64) { env.error("read_u64_le") }
}

// ═══════════════════════════════════════════════════════════════
// ArchiveEntry struct
// ═══════════════════════════════════════════════════════════════

@test
public func archive_entry_struct_works(env : &mut TestEnv) {
    var entry = archive::ArchiveEntry{name: string(""), size: 0, compressed_size: 0, compression_method: 0, crc32: 0, is_directory: false, offset: 0}
    entry.name = string("test.txt")
    entry.size = 1024
    entry.compression_method = 0
    entry.is_directory = false
    if(entry.size != 1024) { env.error("entry size should be 1024") }
    if(entry.is_directory) { env.error("entry should not be directory") }
}

@test
public func archive_entry_directory_flag(env : &mut TestEnv) {
    var entry = archive::ArchiveEntry{name: string("dir/"), size: 0, compressed_size: 0, compression_method: 0, crc32: 0, is_directory: true, offset: 0}
    if(!entry.is_directory) { env.error("should be directory") }
}

@test
public func compression_method_enum(env : &mut TestEnv) {
    var store = archive::CompressionMethod.Store
    var deflate = archive::CompressionMethod.Deflate
    if(store as u16 != 0) { env.error("Store should be 0") }
    if(deflate as u16 != 8) { env.error("Deflate should be 8") }
}

// ═══════════════════════════════════════════════════════════════
// ZIP writer and reader tests
// ═══════════════════════════════════════════════════════════════

@test
public func zip_roundtrip_works(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var file1_data : [13]u8 = ['H' as u8, 'e' as u8, 'l' as u8, 'l' as u8, 'o' as u8, ',' as u8, ' ' as u8, 'W' as u8, 'o' as u8, 'r' as u8, 'l' as u8, 'd' as u8, '!' as u8]
    archive::zip_writer_add_file(&raw mut writer, "hello.txt\0" as *char, &raw file1_data[0], 13)
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    var zip_result = archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)
    if(zip_result is Result.Err) { env.error("should open ZIP from bytes"); return }

    if(archive::zip_entry_count(&raw mut a) != 1) { env.error("should have 1 entry") }
    if(!archive::zip_contains(&raw mut a, "hello.txt\0" as *char)) { env.error("should contain hello.txt") }

    var content = archive::zip_read_file(&raw mut a, "hello.txt\0" as *char)
    if(content is Result.Err) { env.error("should read file"); return }
    var Ok(data) = content else unreachable
    if(data.size() != 13) { env.error("file should be 13 bytes") }

    var match = true
    var i : size_t = 0
    while(i < 13) {
        if(data.get(i) != file1_data[i]) { match = false }
        i += 1
    }
    if(!match) { env.error("file content should match") }
}

@test
public func zip_multiple_files_works(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var data1 : [5]u8 = ['f' as u8, 'i' as u8, 'l' as u8, 'e' as u8, '1' as u8]
    var data2 : [5]u8 = ['f' as u8, 'i' as u8, 'l' as u8, 'e' as u8, '2' as u8]
    archive::zip_writer_add_file(&raw mut writer, "a.txt\0" as *char, &raw data1[0], 5)
    archive::zip_writer_add_file(&raw mut writer, "b.txt\0" as *char, &raw data2[0], 5)
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    var zip_result = archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)
    if(zip_result is Result.Err) { env.error("should open ZIP"); return }

    if(archive::zip_entry_count(&raw mut a) != 2) { env.error("should have 2 entries") }
    if(!archive::zip_contains(&raw mut a, "a.txt\0" as *char)) { env.error("should contain a.txt") }
    if(!archive::zip_contains(&raw mut a, "b.txt\0" as *char)) { env.error("should contain b.txt") }
}

@test
public func zip_writer_add_bytes_and_store(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var data : [3]u8 = [0xAA, 0xBB, 0xCC]
    archive::zip_writer_add_bytes(&raw mut writer, "f.dat\0" as *char, &raw data[0], 3)
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)
    if(archive::zip_entry_count(&raw mut a) != 1) { env.error("add_bytes should work") }
}

@test
public func zip_writer_empty_zip(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)
    if(archive::zip_entry_count(&raw mut a) != 0) { env.error("empty ZIP should have 0 entries") }
}

@test
public func zip_writer_to_bytes_works(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var d : [4]u8 = [0x01, 0x02, 0x03, 0x04]
    archive::zip_writer_add_file(&raw mut writer, "f.txt\0" as *char, &raw d[0], 4)
    var bytes = archive::zip_writer_to_bytes(&raw mut writer)
    if(bytes.size() == 0) { env.error("to_bytes should return non-empty data") }

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    archive::open_zip_bytes(bytes.data(), bytes.size(), &raw mut a)
    if(archive::zip_entry_count(&raw mut a) != 1) { env.error("to_bytes should produce valid ZIP") }
}

@test
public func zip_large_number_of_files(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var i : int = 0
    while(i < 20) {
        var name = string("file_\0")
        name.append_integer(i)
        var data : [1]u8 = [i as u8]
        archive::zip_writer_add_file(&raw mut writer, name.c_str(), &raw data[0], 1)
        i += 1
    }
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    var result = archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)
    if(result is Result.Err) { env.error("20-file ZIP should open"); return }
    if(archive::zip_entry_count(&raw mut a) != 20) { env.error("should have 20 entries") }
}

@test
public func zip_invalid_data(env : &mut TestEnv) {
    var garbage : [100]u8; var i : size_t = 0
    while(i < 100) { garbage[i] = i as u8; i += 1 }
    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    var result = archive::open_zip_bytes(&raw garbage[0], 100, &raw mut a)
    if(result is Result.Ok) { env.error("garbage data should not parse as ZIP") }
}

@test
public func zip_nonexistent_file(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var d : [3]u8 = [0x41, 0x42, 0x43]
    archive::zip_writer_add_file(&raw mut writer, "f.txt\0" as *char, &raw d[0], 3)
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)

    if(archive::zip_contains(&raw mut a, "nonexistent.txt\0" as *char)) {
        env.error("should not find nonexistent file")
    }
    var result = archive::zip_read_file(&raw mut a, "nonexistent.txt\0" as *char)
    if(result is Result.Ok) { env.error("reading nonexistent file should fail") }
}

@test
public func zip_find_entry_multiple(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var d : [2]u8 = [0xCA, 0xFE]
    archive::zip_writer_add_file(&raw mut writer, "x.txt\0" as *char, &raw d[0], 2)
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)

    var entry1 : archive::ArchiveEntry
    var r1 = archive::zip_find_entry(&raw mut a, "x.txt\0" as *char, &raw mut entry1)
    if(r1 is Result.Err) { env.error("should find x.txt") }

    var entry2 : archive::ArchiveEntry
    var r2 = archive::zip_find_entry(&raw mut a, "x.txt\0" as *char, &raw mut entry2)
    if(r2 is Result.Err) { env.error("should find x.txt again") }
    if(entry2.size != 2) { env.error("second find should give correct size") }
}

@test
public func zip_writer_multiple_roundtrip(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var data : [4]u8 = [0x01, 0x02, 0x03, 0x04]
    archive::zip_writer_add_file(&raw mut writer, "f1.txt\0" as *char, &raw data[0], 4)
    archive::zip_writer_add_file(&raw mut writer, "f2.txt\0" as *char, &raw data[0], 4)
    archive::zip_writer_add_file(&raw mut writer, "f3.txt\0" as *char, &raw data[0], 4)
    archive::zip_writer_add_file(&raw mut writer, "f4.txt\0" as *char, &raw data[0], 4)
    archive::zip_writer_add_file(&raw mut writer, "f5.txt\0" as *char, &raw data[0], 4)
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)
    if(archive::zip_entry_count(&raw mut a) != 5) { env.error("should have 5 entries") }
}

// ═══════════════════════════════════════════════════════════════
// ZIP writer save (to file)
// ═══════════════════════════════════════════════════════════════

@test
public func zip_writer_save_to_file(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var d : [5]u8 = [0x48, 0x65, 0x6C, 0x6C, 0x6F]
    archive::zip_writer_add_file(&raw mut writer, "data.bin\0" as *char, &raw d[0], 5)

    var save_result = archive::zip_writer_save(&raw mut writer, "/tmp/test_writer_save.zip")
    if(save_result is Result.Err) { env.error("should save ZIP to file"); return }

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    var open_result = archive::open_zip("/tmp/test_writer_save.zip", &raw mut a)
    if(open_result is Result.Err) { env.error("should open saved ZIP"); return }
    if(archive::zip_entry_count(&raw mut a) != 1) { env.error("should have 1 entry") }
}

// ═══════════════════════════════════════════════════════════════
// DEFLATE tests
// ═══════════════════════════════════════════════════════════════

@test
public func deflate_empty_input(env : &mut TestEnv) {
    var out_buf : [64]u8
    var result = archive::deflate_decompress(null, 0, &raw mut out_buf[0], 64)
    if(result is Result.Ok) {
        var Ok(len) = result else unreachable
        if(len != 0) { env.error("empty input should produce 0 bytes") }
    }
}

@test
public func deflate_invalid_data(env : &mut TestEnv) {
    var out_buf : [64]u8
    var garbage : [10]u8; var i : size_t = 0
    while(i < 10) { garbage[i] = i as u8; i += 1 }
    var result = archive::deflate_decompress(&raw garbage[0], 10, &raw mut out_buf[0], 64)
    if(result is Result.Ok) { env.error("garbage input should produce error") }
}

@test
public func deflate_stored_block(env : &mut TestEnv) {
    // Create a deflate stored block: bfinal=1, btype=0, len=5, nlen=65530, data=Hello
    var input : [11]u8
    input[0] = 1u8  // bfinal=1, btype=0 (stored)
    input[1] = 5u8; input[2] = 0u8     // len = 5
    input[3] = (5 ^ 0xFFFF) as u8; input[4] = ((5 ^ 0xFFFF) >> 8) as u8  // nlen
    input[5] = 'H' as u8; input[6] = 'e' as u8; input[7] = 'l' as u8
    input[8] = 'l' as u8; input[9] = 'o' as u8
    input[10] = 0u8  // padding

    var out_buf : [64]u8
    var result = archive::deflate_decompress(&raw input[0], 11, &raw mut out_buf[0], 64)
    if(result is Result.Err) { env.error("stored block should decompress"); return }
    var Ok(len) = result else unreachable
    if(len != 5) { env.error("should produce 5 bytes") }
    if(out_buf[0] != 'H' as u8 || out_buf[4] != 'o' as u8) { env.error("stored block content") }
}

@test
public func deflate_decompress_stored_from_zip(env : &mut TestEnv) {
    // Write 7-byte content via zip (which uses STORE), read back
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var content : [7]u8 = [0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47]
    archive::zip_writer_add_file(&raw mut writer, "data.bin\0" as *char, &raw content[0], 7)
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    var zip_result = archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)
    if(zip_result is Result.Err) { env.error("should open ZIP"); return }
    if(archive::zip_entry_count(&raw mut a) != 1) { env.error("should have 1 entry") }

    var result = archive::zip_read_file(&raw mut a, "data.bin\0" as *char)
    if(result is Result.Err) { env.error("should read file"); return }
    var Ok(data) = result else unreachable
    if(data.size() != 7) { env.error("file should be 7 bytes") }
}

// ═══════════════════════════════════════════════════════════════
// TAR tests
// ═══════════════════════════════════════════════════════════════

@test
public func tar_nonexistent_file_rejected(env : &mut TestEnv) {
    var a = archive::TarArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    var result = archive::open_tar("/tmp/does_not_exist_xyz.tar", &raw mut a)
    if(result is Result.Ok) { env.error("nonexistent file should fail") }
}

func build_single_file_tar(filename : string, content : *u8, content_len : size_t) : vector<u8> {
    var tar = vector<u8>()
    // header block (512 bytes)
    var i : size_t = 0
    while(i < 512) { tar.push(0); i += 1 }

    // name (offset 0, 100 bytes)
    var j : size_t = 0
    while(j < filename.size() && j < 100) {
        tar.set(j, filename.get(j) as u8)
        j += 1
    }
    // size (offset 124, 12 bytes, octal)
    var size_val = content_len
    var size_str = string("")
    if(size_val == 0) { size_str.append('0') }
    while(size_val > 0) {
        var digit = (size_val % 8) as u8
        size_str.append(('0' as u8 + digit) as char)
        size_val = size_val / 8
    }
    // Reverse to get proper octal
    var rev = string("")
    var si : size_t = size_str.size()
    while(si > 0) {
        si -= 1
        rev.append(size_str.get(si))
    }
    var k : size_t = 0
    while(k < rev.size() && k < 11) {
        tar.set(124 + k, rev.get(k) as u8)
        k += 1
    }
    // typeflag (offset 156): '0' = regular file
    tar.set(156, '0' as u8)

    // append content (rounded to 512)
    var content_pos : size_t = 512
    var ci : size_t = 0
    while(ci < content_len) {
        tar.push(content[ci])
        ci += 1
    }
    // pad to 512
    var padding = (512 - (content_len % 512)) % 512
    var pi : size_t = 0
    while(pi < padding) { tar.push(0); pi += 1 }

    // end with two zero blocks
    var ti : size_t = 0
    while(ti < 1024) { tar.push(0); ti += 1 }

    return tar
}

@test
public func tar_parse_single_file(env : &mut TestEnv) {
    var data : [4]u8 = [0x54, 0x45, 0x53, 0x54]
    var tar_data = build_single_file_tar(string("test.txt"), &raw data[0], 4)

    var a = archive::TarArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    // Write tar to disk and open
    var write_result = fs::write_text_file("/tmp/test_single_file.tar", tar_data.data(), tar_data.size())
    if(write_result is Result.Err) { env.error("write tar file"); return }

    var open_result = archive::open_tar("/tmp/test_single_file.tar", &raw mut a)
    if(open_result is Result.Err) { env.error("open tar should succeed"); return }

    if(archive::tar_entry_count(&raw mut a) != 1) { env.error("should have 1 entry") }
    if(!archive::tar_contains(&raw mut a, "test.txt\0" as *char)) { env.error("should contain test.txt") }

    var read_result = archive::tar_read_file(&raw mut a, "test.txt\0" as *char)
    if(read_result is Result.Err) { env.error("should read file from TAR"); return }
    var Ok(content) = read_result else unreachable
    if(content.size() != 4) { env.error("file should be 4 bytes") }
    if(content.get(0) != 0x54) { env.error("file content T") }
    if(content.get(3) != 0x54) { env.error("file content T at end") }
}

@test
public func tar_find_entry_not_found(env : &mut TestEnv) {
    var data : [3]u8 = [0xAA, 0xBB, 0xCC]
    var tar_data = build_single_file_tar(string("present.txt"), &raw data[0], 3)

    var write_result = fs::write_text_file("/tmp/test_tar_not_found.tar", tar_data.data(), tar_data.size())
    if(write_result is Result.Err) { env.error("write tar"); return }

    var a = archive::TarArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    archive::open_tar("/tmp/test_tar_not_found.tar", &raw mut a)

    if(archive::tar_contains(&raw mut a, "absent.txt\0" as *char)) {
        env.error("should not contain absent.txt")
    }
    var read_result = archive::tar_read_file(&raw mut a, "absent.txt\0" as *char)
    if(read_result is Result.Ok) { env.error("reading absent file should fail") }
}

@test
public func tar_entries_accessor(env : &mut TestEnv) {
    var data : [2]u8 = [0x01, 0x02]
    var tar_data = build_single_file_tar(string("a.dat"), &raw data[0], 2)

    var write_result = fs::write_text_file("/tmp/test_tar_entries.tar", tar_data.data(), tar_data.size())
    if(write_result is Result.Err) { env.error("write tar"); return }

    var a = archive::TarArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    archive::open_tar("/tmp/test_tar_entries.tar", &raw mut a)

    var entries = archive::tar_entries(&raw mut a)
    if(entries.size() != 1) { env.error("should have 1 entry via accessor") }
}

// ═══════════════════════════════════════════════════════════════
// Archive unified API tests
// ═══════════════════════════════════════════════════════════════

@test
public func archive_open_zip(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var d : [3]u8 = [0x41, 0x42, 0x43]
    archive::zip_writer_add_file(&raw mut writer, "hello.txt\0" as *char, &raw d[0], 3)
    archive::zip_writer_close(&raw mut writer)

    var write_result = fs::write_text_file("/tmp/test_unified_archive.zip", writer.data.data(), writer.data.size())
    if(write_result is Result.Err) { env.error("write zip"); return }

    var  arc_result = archive::open("/tmp/test_unified_archive.zip")
    if( arc_result is Result.Err) { env.error("open unified archive"); return }

    var Ok(arc) =  arc_result else unreachable
    if(archive::entry_count(&raw mut arc) != 1) { env.error("unified entry count") }
    if(!archive::contains(&raw mut arc, "hello.txt\0" as *char)) { env.error("unified contains") }

    var read_result = archive::read(&raw mut arc, "hello.txt\0" as *char)
    if(read_result is Result.Err) { env.error("unified read"); return }
    var Ok(rd) = read_result else unreachable
    if(rd.size() != 3) { env.error("unified read size") }
}

@test
public func archive_open_tar_via_unified(env : &mut TestEnv) {
    var content : [4]u8 = [0x44, 0x41, 0x54, 0x41]
    var tar_data = build_single_file_tar(string("info.txt"), &raw content[0], 4)

    var write_result = fs::write_text_file("/tmp/test_unified_tar.tar", tar_data.data(), tar_data.size())
    if(write_result is Result.Err) { env.error("write tar"); return }

    var archive_result = archive::open("/tmp/test_unified_tar.tar")
    if(archive_result is Result.Err) { env.error("open unified tar"); return }

    var Ok(arc) = archive_result else unreachable
    if(archive::entry_count(&raw mut arc) != 1) { env.error("unified tar entry count") }
    if(!archive::contains(&raw mut arc, "info.txt\0" as *char)) { env.error("unified tar contains") }

    var read_result = archive::read(&raw mut arc, "info.txt\0" as *char)
    if(read_result is Result.Err) { env.error("unified tar read"); return }
    var Ok(rd) = read_result else unreachable
    if(rd.size() != 4) { env.error("unified tar read size") }
}

@test
public func archive_open_invalid_format(env : &mut TestEnv) {
    var garbage : [20]u8; var i : size_t = 0
    while(i < 20) { garbage[i] = i as u8; i += 1 }
    var write_result = fs::write_text_file("/tmp/test_invalid_archive.bin", &raw garbage[0], 20)
    if(write_result is Result.Err) { env.error("write"); return }

    var result = archive::open("/tmp/test_invalid_archive.bin")
    if(result is Result.Ok) { env.error("invalid archive should fail") }
}

@test
public func archive_open_nonexistent(env : &mut TestEnv) {
    var result = archive::open("/tmp/does_not_exist_xyz.arc")
    if(result is Result.Ok) { env.error("nonexistent archive should fail") }
}
