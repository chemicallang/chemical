public comptime const SIGINT =		2	/* Interactive attention signal.  */
public comptime const SIGILL =		4	/* Illegal instruction.  */
public comptime const SIGABRT =		6	/* Abnormal termination.  */
public comptime const SIGFPE =		8	/* Erroneous arithmetic operation.  */
public comptime const SIGSEGV =		11	/* Invalid access to storage.  */
public comptime const SIGTERM =		15	/* Termination request.  */

/* Historical signals specified by POSIX. */
public comptime const SIGHUP =		1	/* Hangup.  */
public comptime const SIGQUIT =		3	/* Quit.  */
public comptime const SIGTRAP =		5	/* Trace/breakpoint trap.  */
public comptime const SIGKILL =		9	/* Killed.  */
public comptime const SIGPIPE =		13	/* Broken pipe.  */
public comptime const SIGALRM =		14	/* Alarm clock.  */

public comptime const SIG_ERR =	    -1	/* Error return.  */
public comptime const SIG_DFL =	     0	/* Default action.  */
public comptime const SIG_IGN =	     1	/* Ignore signal.  */
