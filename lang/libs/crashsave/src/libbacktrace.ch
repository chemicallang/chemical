struct backtrace_state {}

/* The type of the error callback argument to backtrace functions.
   This function, if not NULL, will be called for certain error cases.
   The DATA argument is passed to the function that calls this one.
   The MSG argument is an error message.  The ERRNUM argument, if
   greater than 0, holds an errno value.  The MSG buffer may become
   invalid after this function returns.

   As a special case, the ERRNUM argument will be passed as -1 if no
   debug info can be found for the executable, or if the debug info
   exists but has an unsupported version, but the function requires
   debug info (e.g., backtrace_full, backtrace_pcinfo).  The MSG in
   this case will be something along the lines of "no debug info".
   Similarly, ERRNUM will be passed as -1 if there is no symbol table,
   but the function requires a symbol table (e.g., backtrace_syminfo).
   This may be used as a signal that some other approach should be
   tried.  */

type backtrace_error_callback = (data : *mut void, msg : *char, errnum : int) => void;

/* Create state information for the backtrace routines.  This must be
   called before any of the other routines, and its return value must
   be passed to all of the other routines.  FILENAME is the path name
   of the executable file; if it is NULL the library will try
   system-specific path names.  If not NULL, FILENAME must point to a
   permanent buffer.  If THREADED is non-zero the state may be
   accessed by multiple threads simultaneously, and the library will
   use appropriate atomic operations.  If THREADED is zero the state
   may only be accessed by one thread at a time.  This returns a state
   pointer on success, NULL on error.  If an error occurs, this will
   call the ERROR_CALLBACK routine.

   Calling this function allocates resources that cannot be freed.
   There is no backtrace_free_state function.  The state is used to
   cache information that is expensive to recompute.  Programs are
   expected to call this function at most once and to save the return
   value for all later calls to backtrace functions.  */

@extern
public func backtrace_create_state (
    filename : *char, threaded : int,
    error_callback : backtrace_error_callback,
    data : *void
) : *mut backtrace_state;

/* The type of the callback argument to the backtrace_full function.
   DATA is the argument passed to backtrace_full.  PC is the program
   counter.  FILENAME is the name of the file containing PC, or NULL
   if not available.  LINENO is the line number in FILENAME containing
   PC, or 0 if not available.  FUNCTION is the name of the function
   containing PC, or NULL if not available.  This should return 0 to
   continuing tracing.  The FILENAME and FUNCTION buffers may become
   invalid after this function returns.  */

type backtrace_full_callback = (
    data : *mut void, pc : uintptr_t,
    filename : *char, lineno : int,
    function : *char
) => int;

/* Get a full stack backtrace.  SKIP is the number of frames to skip;
   passing 0 will start the trace with the function calling
   backtrace_full.  DATA is passed to the callback routine.  If any
   call to CALLBACK returns a non-zero value, the stack backtrace
   stops, and backtrace returns that value; this may be used to limit
   the number of stack frames desired.  If all calls to CALLBACK
   return 0, backtrace returns 0.  The backtrace_full function will
   make at least one call to either CALLBACK or ERROR_CALLBACK.  This
   function requires debug info for the executable.  */

@extern
public func backtrace_full (
    state : *mut backtrace_state, skip : int,
    callback : backtrace_full_callback,
    error_callback : backtrace_error_callback,
    data : *mut void
) : int

/* The type of the callback argument to the backtrace_simple function.
   DATA is the argument passed to simple_backtrace.  PC is the program
   counter.  This should return 0 to continue tracing.  */

type backtrace_simple_callback = (data : *mut void, pc : uintptr_t) => int;

/* Get a simple backtrace.  SKIP is the number of frames to skip, as
   in backtrace.  DATA is passed to the callback routine.  If any call
   to CALLBACK returns a non-zero value, the stack backtrace stops,
   and backtrace_simple returns that value.  Otherwise
   backtrace_simple returns 0.  The backtrace_simple function will
   make at least one call to either CALLBACK or ERROR_CALLBACK.  This
   function does not require any debug info for the executable.  */

@extern
public func backtrace_simple (
    state : *mut backtrace_state,
    skip : int,
    callback : backtrace_simple_callback,
    error_callback : backtrace_error_callback,
    data : *mut void
) : int

/* Print the current backtrace in a user readable format to a FILE.
   SKIP is the number of frames to skip, as in backtrace_full.  Any
   error messages are printed to stderr.  This function requires debug
   info for the executable.  */

@extern
public func backtrace_print(state : *mut backtrace_state, skip : int, file : *mut FILE);

/* Given PC, a program counter in the current program, call the
   callback function with filename, line number, and function name
   information.  This will normally call the callback function exactly
   once.  However, if the PC happens to describe an inlined call, and
   the debugging information contains the necessary information, then
   this may call the callback function multiple times.  This will make
   at least one call to either CALLBACK or ERROR_CALLBACK.  This
   returns the first non-zero value returned by CALLBACK, or 0.  */
@extern
public func backtrace_pcinfo(
    state : *mut backtrace_state,
    pc : uintptr_t,
    callback : backtrace_full_callback,
    error_callback : backtrace_error_callback,
    data : *mut void
) : int;

/* The type of the callback argument to backtrace_syminfo.  DATA and
   PC are the arguments passed to backtrace_syminfo.  SYMNAME is the
   name of the symbol for the corresponding code.  SYMVAL is the
   value and SYMSIZE is the size of the symbol.  SYMNAME will be NULL
   if no error occurred but the symbol could not be found.  */
public type backtrace_syminfo_callback = (
    data : *mut void, pc : uintptr_t,
    symname : *char,
    symval : uintptr_t,
    symsize : uintptr_t
)

/* Given ADDR, an address or program counter in the current program,
   call the callback information with the symbol name and value
   describing the function or variable in which ADDR may be found.
   This will call either CALLBACK or ERROR_CALLBACK exactly once.
   This returns 1 on success, 0 on failure.  This function requires
   the symbol table but does not require the debug info.  Note that if
   the symbol table is present but ADDR could not be found in the
   table, CALLBACK will be called with a NULL SYMNAME argument.
   Returns 1 on success, 0 on error.  */
@extern
public func backtrace_syminfo(
    state : *mut backtrace_state,
    addr : uintptr_t,
    callback : backtrace_syminfo_callback,
    error_callback : backtrace_error_callback,
    data : *mut void
) : int