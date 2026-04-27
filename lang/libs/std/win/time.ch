@extern
protected struct FILETIME {
    var dwLowDateTime : u32;
    var dwHighDateTime : u32;
}

@extern
public func GetSystemTimeAsFileTime(lpSystemTimeAsFileTime : *mut FILETIME) : void

public namespace std {
public func now_milli() : i64 {
    var ft = zeroed<FILETIME>();
    GetSystemTimeAsFileTime(&mut ft);
    var time = (ft.dwHighDateTime as u64 << 32u64) | (ft.dwLowDateTime as u64);
    // 100-nanosecond intervals since Jan 1, 1601 to Jan 1, 1970
    return ((time as i64 - 116444736000000000i64) / 10000i64);
}
}