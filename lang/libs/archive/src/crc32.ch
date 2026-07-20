public namespace archive {

func crc32_table_entry(i : u32) : u32 {
    var crc = i;
    var j : u32 = 0;
    while(j < 8) {
        if(crc & 1u32 != 0) {
            crc = (crc >> 1) ^ 0xEDB88320u32;
        } else {
            crc = crc >> 1;
        }
        j += 1;
    }
    return crc;
}

public func crc32_compute(data : *u8, data_len : size_t) : u32 {
    var crc : u32 = 0xFFFFFFFFu32;
    var i : size_t = 0;
    while(i < data_len) {
        var idx = (crc ^ (data[i] as u32)) & 0xFFu32;
        var table_val = crc32_table_entry(idx);
        crc = table_val ^ (crc >> 8);
        i += 1;
    }
    return crc ^ 0xFFFFFFFFu32;
}

public func crc32_update(crc : u32, data : *u8, data_len : size_t) : u32 {
    var c = crc ^ 0xFFFFFFFFu32;
    var i : size_t = 0;
    while(i < data_len) {
        var idx = (c ^ (data[i] as u32)) & 0xFFu32;
        var table_val = crc32_table_entry(idx);
        c = table_val ^ (c >> 8);
        i += 1;
    }
    return c ^ 0xFFFFFFFFu32;
}

} // end namespace archive
