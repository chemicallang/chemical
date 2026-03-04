public namespace net {

    @extern protected func inet_pton(
        Family : int,
        pszAddrString : *char,
        pAddrBuf : *void
    ) : int;

    // IPv4 sockaddr structs (C layout)
    public struct in_addr { var s_addr: u32; }
    public struct sockaddr_in {
        var sin_family: u16;
        var sin_port: u16;
        var sin_addr: in_addr;
        var sin_zero: [8]char;
    }

}