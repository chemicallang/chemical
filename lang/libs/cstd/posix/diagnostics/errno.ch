/**
 * @brief Error number set by system calls on failure.
 */
@extern
public var errno : int;

/**
 * @brief Return human‑readable string for an error code.
 * @param errnum Error number.
 * @return Pointer to descriptive, null‑terminated string.
 */
@extern
public func strerror(errnum : int) : *mut char

public func get_errno() : int {
    return errno
}

public func set_errno(value : int) {
    errno = value;
}

public comptime const EPERM =		 1	/* Operation not permitted */
public comptime const ENOENT =		 2	/* No such file or directory */
public comptime const ESRCH =		 3	/* No such process */
public comptime const EINTR =		 4	/* Interrupted system call */
public comptime const EIO =		 5	/* I/O error */
public comptime const ENXIO =		 6	/* No such device or address */
public comptime const E2BIG =		 7	/* Argument list too long */
public comptime const ENOEXEC =	 8	/* Exec format error */
public comptime const EBADF =		 9	/* Bad file number */
public comptime const ECHILD =		10	/* No child processes */
public comptime const EAGAIN =		11	/* Try again */
public comptime const ENOMEM =		12	/* Out of memory */
public comptime const EACCES =		13	/* Permission denied */
public comptime const EFAULT =		14	/* Bad address */
public comptime const ENOTBLK =	15	/* Block device required */
public comptime const EBUSY =		16	/* Device or resource busy */
public comptime const EEXIST =		17	/* File exists */
public comptime const EXDEV =		18	/* Cross-device link */
public comptime const ENODEV =		19	/* No such device */
public comptime const ENOTDIR =	20	/* Not a directory */
public comptime const EISDIR =		21	/* Is a directory */
public comptime const EINVAL =		22	/* Invalid argument */
public comptime const ENFILE =		23	/* File table overflow */
public comptime const EMFILE =		24	/* Too many open files */
public comptime const ENOTTY =		25	/* Not a typewriter */
public comptime const ETXTBSY =	26	/* Text file busy */
public comptime const EFBIG =		27	/* File too large */
public comptime const ENOSPC =		28	/* No space left on device */
public comptime const ESPIPE =		29	/* Illegal seek */
public comptime const EROFS =		30	/* Read-only file system */
public comptime const EMLINK =		31	/* Too many links */
public comptime const EPIPE =		32	/* Broken pipe */
public comptime const EDOM =		33	/* Math argument out of domain of func */
public comptime const ERANGE =		34	/* Math result not representable */

public comptime const EDEADLK =		35	/* Resource deadlock would occur */
public comptime const ENAMETOOLONG =	36	/* File name too long */
public comptime const ENOLCK =		37	/* No record locks available */

/*
 * This error code is special: arch syscall entry code will return
 * -ENOSYS if users try to call a syscall that doesn't exist.  To keep
 * failures of syscalls that really do exist distinguishable from
 * failures due to attempts to use a nonexistent syscall, syscall
 * implementations should refrain from returning -ENOSYS.
 */
public comptime const	ENOSYS =		38	/* Invalid system call number */

public comptime const ENOTEMPTY =	39	/* Directory not empty */
public comptime const ELOOP =		40	/* Too many symbolic links encountered */
public comptime const EWOULDBLOCK =	EAGAIN	/* Operation would block */
public comptime const ENOMSG =		42	/* No message of desired type */
public comptime const EIDRM =		43	/* Identifier removed */
public comptime const ECHRNG =		44	/* Channel number out of range */
public comptime const EL2NSYNC =	45	/* Level 2 not synchronized */
public comptime const EL3HLT =		46	/* Level 3 halted */
public comptime const EL3RST =		47	/* Level 3 reset */
public comptime const ELNRNG =		48	/* Link number out of range */
public comptime const EUNATCH =	49	/* Protocol driver not attached */
public comptime const ENOCSI =		50	/* No CSI structure available */
public comptime const EL2HLT =		51	/* Level 2 halted */
public comptime const EBADE =		52	/* Invalid exchange */
public comptime const EBADR =		53	/* Invalid request descriptor */
public comptime const EXFULL =		54	/* Exchange full */
public comptime const ENOANO =		55	/* No anode */
public comptime const EBADRQC =	56	/* Invalid request code */
public comptime const EBADSLT =	57	/* Invalid slot */

public comptime const EDEADLOCK =	EDEADLK

public comptime const EBFONT =		59	/* Bad font file format */
public comptime const ENOSTR =		60	/* Device not a stream */
public comptime const ENODATA =		61	/* No data available */
public comptime const ETIME =		62	/* Timer expired */
public comptime const ENOSR =		63	/* Out of streams resources */
public comptime const ENONET =		64	/* Machine is not on the network */
public comptime const ENOPKG =		65	/* Package not installed */
public comptime const EREMOTE =		66	/* Object is remote */
public comptime const ENOLINK =		67	/* Link has been severed */
public comptime const EADV =		68	/* Advertise error */
public comptime const ESRMNT =		69	/* Srmount error */
public comptime const ECOMM =		70	/* Communication error on send */
public comptime const EPROTO =		71	/* Protocol error */
public comptime const EMULTIHOP =	72	/* Multihop attempted */
public comptime const EDOTDOT =		73	/* RFS specific error */
public comptime const EBADMSG =		74	/* Not a data message */
public comptime const EOVERFLOW =	75	/* Value too large for defined data type */
public comptime const ENOTUNIQ =	76	/* Name not unique on network */
public comptime const EBADFD =		77	/* File descriptor in bad state */
public comptime const EREMCHG =		78	/* Remote address changed */
public comptime const ELIBACC =		79	/* Can not access a needed shared library */
public comptime const ELIBBAD =		80	/* Accessing a corrupted shared library */
public comptime const ELIBSCN =		81	/* .lib section in a.out corrupted */
public comptime const ELIBMAX =		82	/* Attempting to link in too many shared libraries */
public comptime const ELIBEXEC =	83	/* Cannot exec a shared library directly */
public comptime const EILSEQ =		84	/* Illegal byte sequence */
public comptime const ERESTART =	85	/* Interrupted system call should be restarted */
public comptime const ESTRPIPE =	86	/* Streams pipe error */
public comptime const EUSERS =		87	/* Too many users */
public comptime const ENOTSOCK =	88	/* Socket operation on non-socket */
public comptime const EDESTADDRREQ =	89	/* Destination address required */
public comptime const EMSGSIZE =	90	/* Message too long */
public comptime const EPROTOTYPE =	91	/* Protocol wrong type for socket */
public comptime const ENOPROTOOPT =	92	/* Protocol not available */
public comptime const EPROTONOSUPPORT =	93	/* Protocol not supported */
public comptime const ESOCKTNOSUPPORT =	94	/* Socket type not supported */
public comptime const EOPNOTSUPP =	95	/* Operation not supported on transport endpoint */
public comptime const EPFNOSUPPORT =	96	/* Protocol family not supported */
public comptime const EAFNOSUPPORT =	97	/* Address family not supported by protocol */
public comptime const EADDRINUSE =	98	/* Address already in use */
public comptime const EADDRNOTAVAIL =	99	/* Cannot assign requested address */
public comptime const ENETDOWN =	100	/* Network is down */
public comptime const ENETUNREACH =	101	/* Network is unreachable */
public comptime const ENETRESET =	102	/* Network dropped connection because of reset */
public comptime const ECONNABORTED =	103	/* Software caused connection abort */
public comptime const ECONNRESET =	104	/* Connection reset by peer */
public comptime const ENOBUFS =		105	/* No buffer space available */
public comptime const EISCONN =		106	/* Transport endpoint is already connected */
public comptime const ENOTCONN =	107	/* Transport endpoint is not connected */
public comptime const ESHUTDOWN =	108	/* Cannot send after transport endpoint shutdown */
public comptime const ETOOMANYREFS =	109	/* Too many references: cannot splice */
public comptime const ETIMEDOUT =	110	/* Connection timed out */
public comptime const ECONNREFUSED =	111	/* Connection refused */
public comptime const EHOSTDOWN =	112	/* Host is down */
public comptime const EHOSTUNREACH =	113	/* No route to host */
public comptime const EALREADY =	114	/* Operation already in progress */
public comptime const EINPROGRESS =	115	/* Operation now in progress */
public comptime const ESTALE =		116	/* Stale file handle */
public comptime const EUCLEAN =		117	/* Structure needs cleaning */
public comptime const ENOTNAM =		118	/* Not a XENIX named type file */
public comptime const ENAVAIL =		119	/* No XENIX semaphores available */
public comptime const EISNAM =		120	/* Is a named type file */
public comptime const EREMOTEIO =	121	/* Remote I/O error */
public comptime const EDQUOT =		122	/* Quota exceeded */

public comptime const ENOMEDIUM =	123	/* No medium found */
public comptime const EMEDIUMTYPE =	124	/* Wrong medium type */
public comptime const ECANCELED =	125	/* Operation Canceled */
public comptime const ENOKEY =		126	/* Required key not available */
public comptime const EKEYEXPIRED =	127	/* Key has expired */
public comptime const EKEYREVOKED =	128	/* Key has been revoked */
public comptime const EKEYREJECTED =	129	/* Key was rejected by service */

/* for robust mutexes */
public comptime const EOWNERDEAD =	130	/* Owner died */
public comptime const ENOTRECOVERABLE =	131	/* State not recoverable */

public comptime const ERFKILL =		132	/* Operation not possible due to RF-kill */

public comptime const EHWPOISON =	133	/* Memory page has hardware error */