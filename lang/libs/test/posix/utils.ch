/* read exactly `count` bytes from fd into buf, handling EINTR and partial reads.
 * returns number of bytes read (should be == count), 0 on EOF (if first read returns 0),
 * -1 on error.
 */
func read_exact(fd : int, buf : *void, count : size_t) : ssize_t {
    var got : size_t = 0;
    while (got < count) {
        var r = read(fd, (buf as *mut char) + got, count - got);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) { // EOF
            return got as ssize_t; // may be 0 for immediate EOF or partial
        }
        got += r as size_t;
    }
    return got as ssize_t;
}

/* write exactly `count` bytes from buf to fd, handling EINTR and partial writes.
 * returns 0 on success, -1 on error.
 */
func write_exact(fd : int, buf : *void, count : size_t) : int {
    var written : size_t = 0;
    while (written < count) {
        var w : ssize_t = write(fd, (buf as *char) + written, count - written);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        written += w as size_t;
    }
    return 0;
}

/* Send a length-prefixed message on fd.
 * Format: 4-byte big-endian length (uint32_t), then payload (len bytes).
 * Returns 0 on success, -1 on error.
 */
func send_message_fd(fd : int, data : *void, len : uint32_t) : int {
    var be : uint32_t = htonl(len);
    if (write_exact(fd, &be, sizeof(be)) < 0) return -1;
    if (len == 0) return 0;
    if (write_exact(fd, data, len) < 0) return -1;
    return 0;
}

/* Convenience: send a NUL-terminated string as a message */
func send_message_str(fd : int, s : *char) : int {
    return send_message_fd(fd, s, strlen(s) as uint32_t);
}
