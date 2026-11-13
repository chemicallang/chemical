public namespace server {

public func main(argc : int, argv : **char) : int { unsafe {
    var port = 8080u;
    var root = std::string::make_no_len(".");
    var addr = std::string::make_no_len("0.0.0.0");
    var async_mode = false;

    var i = 1;
    while(i < argc) {
        const arg_str = *argv_offset(argv, i);
        var arg = std::string_view(arg_str, strlen(arg_str));

        if(arg.equals("-p") || arg.equals("--port")) {
            if(i + 1 < argc) {
                var p_str = *argv_offset(argv, i + 1);
                // Simple atoi-like for port
                var p = 0u;
                var j = 0u;
                while(p_str[j] != '\0') {
                    if(p_str[j] >= '0' && p_str[j] <= '9') {
                        p = p * 10u + (p_str[j] as uint - '0' as uint);
                    }
                    j++;
                }
                if (p > 0) port = p;
                i++;
            }
        } else if(arg.equals("-h") || arg.equals("--help")) {
            printf("Usage: server [root_dir] [-p port] [-a address] [--async]\n");
            printf("Default: root='.', port=8080, address=0.0.0.0\n");
            printf("  --async  serve with the coroutine accept loop (async runtime)\n");
            return 0;
        } else if(arg.equals("--async")) {
            async_mode = true;
        } else if(arg.equals("-a") || arg.equals("--addr")) {
            if(i + 1 < argc) {
                addr = std::string::make_no_len(*argv_offset(argv, i + 1));
                i++;
            }
        } else {
            // Assume it's the root directory
            root = std::string::make_no_len(arg_str);
        }
        i++;
    }

    printf("Starting Chemical Server...\n");
    printf("Serving directory: %s\n", root.c_str());
    printf("Listening on: %s:%d\n", addr.c_str(), port);

    var fs_server = std::arc(http.create_file_server(root.c_str()));
    var fs_server_root = fs_server.clone();
    var cfg = server.ServerConfig();
    cfg.addr = addr;

    var S = server.Server(cfg);

    // The router stores handlers for the server's lifetime, so a handler must
    // own its capture and prove `Send + Sync + 'static` (SAFE-SEND-002 rejects a
    // borrowed `&fs_server`). Share the file server through an `Arc` instead.
    unsafe {
        S.router.add("GET", "/:path*", |fs_server|(req, res) => {
            fs_server.get().serve_http(req, res);
        });
        // Also root
        S.router.add("GET", "/", |fs_server_root|(req, res) => {
            fs_server_root.get().serve_http(req, res);
        });
    }

    if(async_mode) {
        // Tier 6: drive the file server from the coroutine accept loop. The
        // synchronous `serve` path stays the default for compatibility.
        async::block_on<int>(serve_files_async(&raw mut S, root.c_str(), port));
        return 0;
    }

    S.serve(port);

    return 0;
 }}

func argv_offset(argv : **char, offset : int) : **char {
    var p = argv as *mut *char;
    var j = 0;
    while(j < offset) {
        unsafe { p = p + 1; }
        j++;
    }
    return p;
}

func strlen(s: *char): usize {
    var len = 0u;
    unsafe {
        while(s[len] != '\0') {
            len++;
        }
    }
    return len;
}

}

protected func main(argc : int, argv : **char) : int {
    return server::main(argc, argv);
}
