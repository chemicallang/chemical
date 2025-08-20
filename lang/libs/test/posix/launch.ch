func launch_test(exe_path : *char, id : int, state : &mut TestFunctionState) : int {

   var cmd = std::string()
   cmd.append_char_ptr(exe_path)
   cmd.append(' ');
   cmd.append_char_ptr("--test-id ");
   append_integer(cmd, id);

   var sv : int[2]
   if(socketpair(AF_UNIX, SOCK_STREAM as int, 0, sv) < 0) {
        return -1;
   }

   // initialize the spawn file actions
   var pid : pid_t
   var actions : posix_spawn_file_actions_t
   var rc = posix_spawn_file_actions_init(&mut actions)
   if(rc != 0) {
        var saved = errno;
        close(sv[0]); close(sv[1]);
        errno = saved;
        return -1;
   }

   // Duplicate child end to FD_CHILD in the child
   posix_spawn_file_actions_adddup2(&mut actions, sv[1], FD_CHILD);
   // Close original child-end fd in the child's table (optional, good hygiene)
   posix_spawn_file_actions_addclose(&mut actions, sv[1]);

    // spawn the process
    rc = posix_spawnp(&mut pid, exe_path, &actions, null as **char, cmd.data(), environ)

    // destroy the actions
    posix_spawn_file_actions_destroy(&mut actions);
    if(rc != 0) {
        var saved = errno;
        close(sv[0]); close(sv[1]);
        errno = rc;
        errno = rc;
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
            var saved_errno = errno;
            close(parent_fd);
            var status : int
            waitpid(pid, &status, 0)
            errno = saved_errno;
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
            waitpid(pid, &status, 0)
            errno = EPROTO
            return -1;
        }
        var buf : uint8_t* = null
        if(len > 0) {
            buf = malloc(len) as *mut uint8_t
            if(!buf) {
                close(parent_fd)
                var status : int
                waitpid(pid, &mut status, 0)
                errno = ENOMEM
                return -1;
            }
            var got = read_exact(parent_fd, buf, len)
            if(got < 0 || got as uint32_t != len) {
                free(buf)
                close(parent_fd)
                var status : int
                waitpid(pid, &mut status, 0)
                if(got >= 0) {
                    errno = EPROTO
                }
                return -1;
            }
        }

        process_message(state, buf)

        free(buf);

    }

    close(parent_fd);

    var status : int
    // try to reap child to avoid zombie
    if(waitpid(pid, &mut status, 0)) {
        return -1;
    }
    return -1;

}
