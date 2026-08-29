public namespace fs {

public func filetime_to_unix(ft : FILETIME) : i64 {
    const SECS_BETWEEN_EPOCHS : i64 = 11644473600;
    const HUNDRED_NANOSECONDS_PER_SEC : i64 = 10000000;

    // Reconstruct 64-bit Windows time from FILETIME parts
    var windows_time : u64 = ((ft.dwHighDateTime as u64) << 32) | (ft.dwLowDateTime as u64);

    // Convert to seconds and adjust epoch
    var unix_time : i64 = (windows_time / HUNDRED_NANOSECONDS_PER_SEC) - SECS_BETWEEN_EPOCHS;

    return unix_time;
}

public func unix_to_filetime(unix_time : i64) : FILETIME {
    const SECS_BETWEEN_EPOCHS : i64 = 11644473600;   // seconds between 1601 and 1970
    const HUNDRED_NANOSECONDS_PER_SEC : i64 = 10000000;

    var ft : FILETIME;

    // Convert unix seconds to Windows ticks
    var windows_time : u64 = ((unix_time + SECS_BETWEEN_EPOCHS) * HUNDRED_NANOSECONDS_PER_SEC) as u64;

    ft.dwLowDateTime = (windows_time & 0xFFFFFFFF) as u32;
    ft.dwHighDateTime = ((windows_time >> 32) & 0xFFFFFFFF) as u32;

    return ft;
}

}