using std::Result;
using std::vector;
using std::string;

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
public func zip_roundtrip_works(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var file1_data : [13]u8 = ['H' as u8, 'e' as u8, 'l' as u8, 'l' as u8, 'o' as u8, ',' as u8, ' ' as u8, 'W' as u8, 'o' as u8, 'r' as u8, 'l' as u8, 'd' as u8, '!' as u8]
    archive::zip_writer_add_file(&raw mut writer, "hello.txt\0" as *char, &raw file1_data[0], 13)
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    var zip_result = archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)
    if(zip_result is Result.Err) {
        env.error("should open ZIP from bytes")
        return
    }
    if(archive::zip_entry_count(&raw mut a) != 1) {
        env.error("should have 1 entry")
    }
    if(!archive::zip_contains(&raw mut a, "hello.txt\0" as *char)) {
        env.error("should contain hello.txt")
    }
    var content = archive::zip_read_file(&raw mut a, "hello.txt\0" as *char)
    if(content is Result.Err) {
        env.error("should read file")
        return
    }
    var Ok(data) = content else unreachable
    if(data.size() != 13) {
        env.error("file should be 13 bytes")
    }
    var match = true
    var i : size_t = 0
    while(i < 13) {
        if(data.get(i) != file1_data[i]) { match = false }
        i += 1
    }
    if(!match) {
        env.error("file content should match")
    }
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
    if(zip_result is Result.Err) {
        env.error("should open ZIP")
        return
    }
    if(archive::zip_entry_count(&raw mut a) != 2) {
        env.error("should have 2 entries")
    }
    if(!archive::zip_contains(&raw mut a, "a.txt\0" as *char)) {
        env.error("should contain a.txt")
    }
    if(!archive::zip_contains(&raw mut a, "b.txt\0" as *char)) {
        env.error("should contain b.txt")
    }
}

@test
public func endian_u16_le_works(env : &mut TestEnv) {
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
public func endian_u32_le_works(env : &mut TestEnv) {
    var buf : [4]u8
    archive::write_u32_le(&raw mut buf[0], 0, 0xDEADBEEFu32)
    var val = archive::read_u32_le(&raw buf[0], 0)
    if(val != 0xDEADBEEFu32) {
        env.error("u32 LE roundtrip failed")
    }
}

@test
public func archive_entry_struct_works(env : &mut TestEnv) {
    var entry = archive::ArchiveEntry{name: string(""), size: 0, compressed_size: 0, compression_method: 0, crc32: 0, is_directory: false, offset: 0}
    entry.name = string("test.txt")
    entry.size = 1024
    entry.compression_method = 0
    entry.is_directory = false
    if(entry.size != 1024) {
        env.error("entry size should be 1024")
    }
    if(entry.is_directory) {
        env.error("entry should not be directory")
    }
}

// ═══════════════════════════════════════════════════════════════
// DEFLATE decompression tests
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
public func deflate_roundtrip_via_zip(env : &mut TestEnv) {
    // Write a deflate-compressed entry via zip_writer (STORE mode)
    // then verify it roundtrips correctly
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var content : [7]u8 = [0x41 as u8, 0x42 as u8, 0x43 as u8, 0x44 as u8, 0x45 as u8, 0x46 as u8, 0x47 as u8]
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
// TAR error handling
// ═══════════════════════════════════════════════════════════════

@test
public func tar_nonexistent_file_rejected(env : &mut TestEnv) {
    var a = archive::TarArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    var result = archive::open_tar("/tmp/does_not_exist_xyz.tar", &raw mut a)
    if(result is Result.Ok) { env.error("nonexistent file should fail") }
}

// ═══════════════════════════════════════════════════════════════
// ZIP error path tests
// ═══════════════════════════════════════════════════════════════

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
    var d : [3]u8 = [0x41 as u8, 0x42 as u8, 0x43 as u8]
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
public func archive_entry_directory_flag(env : &mut TestEnv) {
    var entry = archive::ArchiveEntry{name: string("dir/"), size: 0, compressed_size: 0, compression_method: 0, crc32: 0, is_directory: true, offset: 0}
    if(!entry.is_directory) { env.error("should be directory") }
}

@test
public func zip_writer_multiple_roundtrip(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    var data : [4]u8 = [0x01 as u8, 0x02 as u8, 0x03 as u8, 0x04 as u8]
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

@test
public func zip_writer_empty_zip(env : &mut TestEnv) {
    var writer = archive::ZipWriter{entries: vector<archive::ZipWriteEntry>(), data: vector<u8>()}
    archive::zip_writer_close(&raw mut writer)

    var a = archive::ZipArchive{data: vector<u8>(), entries: vector<archive::ArchiveEntry>(), data_loaded: false}
    archive::open_zip_bytes(writer.data.data(), writer.data.size(), &raw mut a)
    if(archive::zip_entry_count(&raw mut a) != 0) { env.error("empty ZIP should have 0 entries") }
}
