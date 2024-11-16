import "@cstd/stdio.ch"
import "@cstd/stdint.ch"
import "@cstd/string.ch"
import "./ws2def.ch"

@comptime
const WSADESCRIPTION_LEN = 256

@comptime
const WSASYS_STATUS_LEN = 128

typealias SOCKET = uint

typealias ADDRESS_FAMILY = ushort

struct sockaddr {
    // TODO this sa_family field exists in windows nt only
    // u_short sa_family;
    var sa_family : ADDRESS_FAMILY;
    var sa_data : char[14];                   // Up to 14 bytes of direct address.
}

if(def.win64) {
    struct WSAData {
            var wVersion : WORD;
            var wHighVersion : WORD;
            var iMaxSockets : ushort;
            var iMaxUdpDg : ushort;
            var lpVendorInfo : *char;
            var szDescription : char[WSADESCRIPTION_LEN+1];
            var szSystemStatus : char[WSASYS_STATUS_LEN+1];
    }
} else {
    struct WSAData {
            var wVersion : WORD;
            var wHighVersion : WORD;
            var szDescription : char[WSADESCRIPTION_LEN+1];
            var szSystemStatus : char[WSASYS_STATUS_LEN+1];
            var iMaxSockets : ushort;
            var iMaxUdpDg : ushort;
            var lpVendorInfo : *char;
    }
}

@comptime
var INVALID_SOCKET = 0xFFFFFFFF;
@comptime
var SOCKET_ERROR = -1

@comptime
const SOMAXCONN = 0x7fffffff

@comptime
func SOMAXCONN_HINT(b : any) : any {
    return -b;
}

/** process out-of-band data */
const MSG_OOB = 0x1
/** peek at incoming message */
const MSG_PEEK = 0x2
/** send without using routing tables */
const MSG_DONTROUTE = 0x4

typealias BYTE = char
typealias WORD = ushort

@comptime
func MAKEWORD(low : BYTE , high : BYTE) : WORD {
    return ((low as BYTE) | ((high as WORD) << 8)) as WORD;
}

// TODO attribyte std call
func WSAStartup(wVersionRequested : WORD, lpWSAData : *WSAData) : int

// TODO attribyte std call
func WSACleanup() : int

// TODO attribyte std call
func WSAGetLastError() : int

@comptime
const IPPROTO_TCP = 6;

@comptime
const AI_PASSIVE = 0x00000001

@comptime
func ZeroMemory(Destination : *mut void, Length : size_t) : any {
    return compiler::wrap(memset(Destination, 0, Length));
}

struct addrinfo {
    var ai_flags : int;       // AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST
    var ai_family : int;      // PF_xxx
    var ai_socktype : int;    // SOCK_xxx
    var ai_protocol : int;    // 0 or IPPROTO_xxx for IPv4 and IPv6
    var ai_addrlen : size_t;     // Length of ai_addr
    var ai_canonname : *char;   // Canonical name for nodename
    var ai_addr : *sockaddr;        // Binary address
    var ai_next : *addrinfo;
}

func initialize_winsock() : bool {
    var wsaData : WSAData;
    var iResult : int = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return false;
    }
    return true;
}

/* stream socket */
@comptime
/* datagram socket */
const SOCK_STREAM = 1
@comptime
/* raw-protocol interface */
const SOCK_DGRAM = 2
@comptime
/* reliably-delivered message */
const SOCK_RAW = 3
@comptime
/* sequenced packet stream */
const SOCK_RDM = 4
@comptime
const SOCK_SEQPACKET = 5

// TODO attributes dllimport, stdcall
func getaddrinfo(
    pNodeName : *mut char,
    pServiceName : *mut char,
    pHints : *addrinfo,
    ppResult : **mut addrinfo
) : int

// TODO attribute stdcall
func freeaddrinfo(
    pAddrInfo : *mut addrinfo
) : void

// TODO attribute stdcall
func bind(s : SOCKET, name : *sockaddr, namelen : int) : int

func socket(af : int, type : int, protocol : int) : SOCKET

func closesocket(socket : SOCKET) : int

func listen(socket : SOCKET, backlog : int) : int;

func shutdown(socket : SOCKET, how : int) : int

func recv(socket : SOCKET, buf : *mut char, len : int, flags : int) : int

func send(socket : SOCKET, buf : *mut char, len : int, flags : int) : int

func accept(socket : SOCKET, addr : *mut sockaddr, addrlen : *mut int) : int

/*
 * WinSock 2 extension -- manifest constants for return values of the condition function
 */
@comptime
const CF_ACCEPT = 0x0000
@comptime
const CF_REJECT = 0x0001
@comptime
const CF_DEFER = 0x0002

/*
 * WinSock 2 extension -- manifest constants for shutdown()
 */
@comptime
const SD_RECEIVE = 0x00
@comptime
const SD_SEND = 0x01
@comptime
const SD_BOTH = 0x02

func create_server_socket(port : *mut char) : SOCKET {

    var result : *mut addrinfo = null as *mut addrinfo;
    var hints : addrinfo
    var ListenSocket : SOCKET = INVALID_SOCKET;

    const addrinfo_size = #sizeof { addrinfo }
    // if(addrinfo_size != 32) {
    //    printf("size of addr info is actually %d\n", addrinfo_size);
    //}

    ZeroMemory(&hints, addrinfo_size);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    printf("ZERO_MEMORY_SUCCESS\n");

    var iResult = getaddrinfo(null, port, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return INVALID_SOCKET;
    }

    ListenSocket = socket(result.ai_family, result.ai_socktype, result.ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return INVALID_SOCKET;
    }

    iResult = bind(ListenSocket, result.ai_addr, result.ai_addrlen as int);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return INVALID_SOCKET;
    }

    freeaddrinfo(result);

    return ListenSocket;
}

func listen_for_connections(ListenSocket : SOCKET) : bool  {
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return false;
    }
    return true;
}

@comptime
const CHUNK_SIZE = 4096

// this function is just an example
// windows and other OS's perform chunking at an internal (so I've heard)
// the problem is that some times you can't load the entire response in memory and send it
// for example a 4mb file for example, you should send it in chunks of 4kb
// however string responses should be handed straight to send function for fast responses
func send_large_response(ClientSocket : SOCKET, response : *mut char, response_length : int) : bool {
    var bytes_sent = 0;
    while (bytes_sent < response_length) {
        var chunk_size = CHUNK_SIZE;

        // Adjust chunk size if the remaining data is smaller than CHUNK_SIZE
        if (response_length - bytes_sent < CHUNK_SIZE) {
            chunk_size = response_length - bytes_sent;
        }

        // Send the chunk
        var iSendResult = send(ClientSocket, response + bytes_sent, chunk_size, 0);
        if (iSendResult == SOCKET_ERROR) {
            printf("send failed: %d\n", WSAGetLastError());
            return true;
        }

        bytes_sent += iSendResult;
        printf("Chunk sent: %d bytes\n", iSendResult);

    }
    return false;
}

@comptime
const BUFFER_SIZE = 512

func handle_client(ClientSocket : SOCKET) : void {
    var recvbuf : char[BUFFER_SIZE];
    var iResult : int;
    do {
        iResult = recv(ClientSocket, recvbuf, BUFFER_SIZE - 1, 0);
        if (iResult > 0) {

            recvbuf[iResult] = '\0';
            printf("Received request:\n%s\n", recvbuf);

            const response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!";
            const response_length = strlen(response);

            // Sending the large response
            send(ClientSocket, response, response_length, 0);

        } else if (iResult == 0) {
            printf("Connection closing...\n");
            break;
        } else {
            printf("recv failed: %d\n", WSAGetLastError());
            break;
        }
    } while (true);

    closesocket(ClientSocket);
}

// TODO __stdcall
func client_thread(socket : *void) {
    handle_client(socket as SOCKET);
}

func _beginthread(
    _StartAddress : (ptr : *void) => void,
    _StackSize : uint,
    _ArgList : *void
) : uintptr_t

public func main() : int {

    const port = "8080";

    if (!initialize_winsock()) {
        printf("Failed to initialize winsock");
        return 1;
    }

    printf("INITIALIZED_WIN_SOCK\n");

    var ListenSocket = create_server_socket(port);
    if (ListenSocket == INVALID_SOCKET) {
        printf("Created socket is invalid");
        return 1;
    }

    printf("CREATED_SERVER_SOCKET\n");

    if (!listen_for_connections(ListenSocket)) {
        printf("Failed listening to connections");
        return 1;
    }

    printf("Server is listening on port %s\n", port);

    var ClientSocket : SOCKET;
    loop {
        ClientSocket = accept(ListenSocket, null, null)
        if(ClientSocket != INVALID_SOCKET) {
            printf("Accepted connection\n");
            _beginthread(client_thread, 0, ClientSocket as *void);
        } else {
            printf("accept failed: %d\n", WSAGetLastError())
            break;
        }
    }

    closesocket(ListenSocket);
    WSACleanup();

    return 0;
}