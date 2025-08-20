func (env : &mut TestEnvImpl) send_message(msg : *char, len : size_t) : int {
    var be : uint32_t = htonl(len);
    if (write_exact(env.fd, &be, sizeof(be)) < 0) return -1;
    if (len == 0) return 0;
    if (write_exact(env.fd, msg, len) < 0) return -1;
    return 0;
}
