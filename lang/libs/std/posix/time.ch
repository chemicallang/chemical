public namespace std {
    public func now_milli() : i64 {
        var ts = zeroed<timespec>();
        clock_gettime(0, &mut ts); // 0 is CLOCK_REALTIME on most posix
        return (ts.tv_sec as i64 * 1000i64) + (ts.tv_nsec as i64 / 1000000i64);
    }
}