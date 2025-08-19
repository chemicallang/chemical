
/**
 * We will only declare errors that are relevant to us
 * because winerror file is too large, it would
 * slow down compilation
 */


/**
 * MessageId: ERROR_NO_MORE_FILES
 *
 * MessageText:
 *
 * There are no more files.
 */
public comptime const ERROR_NO_MORE_FILES = 18L

//
// MessageId: ERROR_PIPE_CONNECTED
//
// MessageText:
//
// There is a process on other end of the pipe.
//
public comptime const ERROR_PIPE_CONNECTED = 535L

//
// MessageId: ERROR_PIPE_BUSY
//
// MessageText:
//
// All pipe instances are busy.
//
public comptime const ERROR_PIPE_BUSY = 231L

//
// MessageId: ERROR_BROKEN_PIPE
//
// MessageText:
//
// The pipe has been ended.
//
public comptime const ERROR_BROKEN_PIPE = 109L

//
// MessageId: ERROR_MORE_DATA
//
// MessageText:
//
// More data is available.
//
public comptime const ERROR_MORE_DATA = 234L    // dderror