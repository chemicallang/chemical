func launch_test(exe_path : *char, id : int, state : &mut TestFunctionState) : int {

    // Build argv for posix_spawnp (must be a NULL-terminated array)
    var id_str = std::string();
    append_integer(id_str, id);

    var sv : int[2]
    if(socketpair(AF_UNIX, SOCK_STREAM as int, 0, sv) < 0) {
        return -1;
    }

    var comm_id_str = std::string()
    append_integer(comm_id_str, sv[1])

    // argv: [exe_path, "--test-id", "<id>", NULL]
    var argv : [6]*char;
    argv[0] = exe_path;
    argv[1] = "--test-id";
    argv[2] = id_str.data();
    argv[3] = "--comm-id";
    argv[4] = comm_id_str.data()
    argv[5] = null;

   // initialize the spawn file actions
   var pid : pid_t
   var actions : posix_spawn_file_actions_t
   var rc = posix_spawn_file_actions_init(&mut actions)
   if(rc != 0) {
        var saved = get_errno();
        close(sv[0]); close(sv[1]);
        set_errno(saved);
        return -1;
   }

   // Close original child-end fd in the child's table (optional, good hygiene)
   posix_spawn_file_actions_addclose(&mut actions, sv[0]);

    // spawn the process
    rc = posix_spawnp(&mut pid, exe_path, &actions, null, argv, __environ)

    // destroy the actions
    posix_spawn_file_actions_destroy(&mut actions);
    if(rc != 0) {
        var saved = get_errno();
        close(sv[0]); close(sv[1]);
        set_errno(rc)
        return -1;
    }

    // parent doesn't need the child's end
    close(sv[1]);

    var parent_fd = sv[0]

    // read loop
    while(true) {
        var be_len : uint32_t;
        var r = read_exact(parent_fd, &be_len, sizeof(be_len))
        if(r < 0) {
            var saved_errno = get_errno();
            close(parent_fd);
            var status : int
            waitpid(pid, &mut status, 0)
            set_errno(saved_errno);
            return -1;
        }
        if(r == 0) {
            // EOF before header — child closed socket cleanly
            break;
        }
        if(r != sizeof(be_len)) {
            break;
        }
        var len = ntohl(be_len)
        var MAX_MSG = 100 * 1024 * 1024;
        if(len > MAX_MSG) {
            close(parent_fd)
            var status : int
            waitpid(pid, &mut status, 0)
            set_errno(EPROTO)
            return -1;
        }
        var buf : uint8_t* = null
        if(len > 0) {
            buf = malloc(len) as *mut uint8_t
            if(!buf) {
                close(parent_fd)
                var status : int
                waitpid(pid, &mut status, 0)
                set_errno(ENOMEM)
                return -1;
            }
            var got = read_exact(parent_fd, buf, len)
            if(got < 0 || got as uint32_t != len) {
                free(buf)
                close(parent_fd)
                var status : int
                waitpid(pid, &mut status, 0)
                if(got >= 0) {
                    set_errno(EPROTO)
                }
                return -1;
            }
        }

        process_message(state, buf as *char)

        free(buf);

    }

    close(parent_fd);

    var status : int
    // try to reap child to avoid zombie
    if(waitpid(pid, &mut status, 0) < 0) {
        return -1;
    }

    // set the exit code in state
    state.exitCode = status;
    if(status != 0 && !state.fn.pass_on_crash) {
        state.has_failed = true;
    }

    return -1;

}
