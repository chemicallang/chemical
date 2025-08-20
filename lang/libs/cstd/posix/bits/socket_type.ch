enum __socket_type {
    /* Sequenced, reliable, connection-based byte streams.  */
    SOCK_STREAM = 1,
    /* Connectionless, unreliable datagrams
                   of fixed maximum length.  */
    SOCK_DGRAM = 2,
    /* Raw protocol interface.  */
    SOCK_RAW = 3,
    /* Reliably-delivered messages.  */
    SOCK_RDM = 4,
    /* Sequenced, reliable, connection-based,
                   datagrams of fixed maximum length.  */
    SOCK_SEQPACKET = 5,
    /* Datagram Congestion Control Protocol.  */
    SOCK_DCCP = 6,
    /* Linux specific way of getting packets
                   at the dev level.  For writing rarp and
                   other similar things on the user level. */
                     /* Flags to be ORed into the type parameter of socket and socketpair and
                        used for the flags parameter of paccept.  */
    SOCK_PACKET = 10,
    /* Atomically set close-on-exec flag for the
                   new descriptor(s).  */
    SOCK_CLOEXEC = 02000000,
    /* Atomically mark descriptor(s) as
                   non-blocking.  */
    SOCK_NONBLOCK = 00004000

};

public comptime const SOCK_STREAM = __socket_type.SOCK_STREAM
public comptime const SOCK_DGRAM = __socket_type.SOCK_DGRAM
public comptime const SOCK_RAW = __socket_type.SOCK_RAW
public comptime const SOCK_RDM = __socket_type.SOCK_RDM
public comptime const SOCK_SEQPACKET = __socket_type.SOCK_SEQPACKET
public comptime const SOCK_DCCP = __socket_type.SOCK_DCCP
public comptime const SOCK_PACKET = __socket_type.SOCK_PACKET
public comptime const SOCK_CLOEXEC = __socket_type.SOCK_CLOEXEC
public comptime const SOCK_NONBLOCK = __socket_type.SOCK_NONBLOCK