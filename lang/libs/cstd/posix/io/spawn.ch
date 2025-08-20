// Copyright (c) Chemical Language Foundation 2025.

// TODO: doesn't work at top level
// const seget_n = (1024 / (8 * sizeof(ulong)))
// hardcoding the value
comptime const seget_n = 16

public struct __sigset_t {
    var __val : ulong[seget_n];
}

public type sigset_t = __sigset_t

/* Data structure to describe a process' schedulability.  */
public struct sched_param {
    var sched_priority : int;
};

/* Data structure to contain attributes for thread creation.  */
public struct posix_spawnattr_t {
    var __flags : short;
    var __pgrp : pid_t;
    var __sd : sigset_t;
    var __ss : sigset_t;
    var __sp : sched_param;
    var __policy : int;
    var __cgroup : int;
    var __pad : int[15];
}

public struct posix_spawn_file_actions_t {
    var __allocated : int;
    var __used : int;
    var __actions : *mut void;
    var __pad : [16]int;
};

/* Initialize data structure for file attribute for `spawn' call.  */
@extern
public func posix_spawn_file_actions_init(__file_actions : *mut posix_spawn_file_actions_t) : int

/* Add an action to FILE-ACTIONS which tells the implementation to call
   `dup2' for the given file descriptors during the `spawn' call.  */
@extern
public func posix_spawn_file_actions_adddup2(
    __file_actions : *mut posix_spawn_file_actions_t,
    __fd : int,
    __newfd : int
) : int

/* Free resources associated with FILE-ACTIONS.  */
@extern
public func posix_spawn_file_actions_destroy(
    __file_actions : *mut posix_spawn_file_actions_t
) : int

/* Similar to `posix_spawn' but search for FILE in the PATH.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
@extern
public func posix_spawnp(
    __pid : *mut pid_t, __file : *char,
    __file_actions : *posix_spawn_file_actions_t,
    __attrp : *posix_spawnattr_t,
    __argv : **char,  __envp : **char
) : int