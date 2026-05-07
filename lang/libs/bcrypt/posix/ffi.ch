public namespace bcrypt {

    public func get_random_bytes(buf : *mut u8, len : uint) : bool {
        var fd = open("/dev/urandom", O_RDONLY, 0)
        if(fd < 0) return false
        var bytes_read = read(fd, buf as *mut void, len as ulong)
        close(fd)
        return bytes_read == (len as ssize_t)
    }

}
