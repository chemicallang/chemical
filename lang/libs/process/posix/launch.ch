func launch_exe(path : &std::string_view) : int {

    var comm = std::string()
    comm.append_view(path)

    // argv: [exe_path]
    var argv : [6]*char;
    argv[0] = comm.data();
    argv[1] = null;

   // initialize the spawn file actions
   var pid : pid_t
   var actions : posix_spawn_file_actions_t
   var rc = posix_spawn_file_actions_init(&mut actions)
   if(rc != 0) {
        var saved = get_errno();
        set_errno(saved);
        return -1;
   }

    // spawn the process
    rc = posix_spawnp(&mut pid, exe_path, &actions, null, argv, __environ)

    // destroy the actions
    posix_spawn_file_actions_destroy(&mut actions);
    if(rc != 0) {
        var saved = get_errno();
        set_errno(rc)
        return -1;
    }

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
