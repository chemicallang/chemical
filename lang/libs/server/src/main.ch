public namespace server {

public func main(argc : int, argv : **char) : int {
    var port = 8080u;
    var root = std::string::make_no_len(".");
    var addr = std::string::make_no_len("0.0.0.0");

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
            printf("Usage: server [root_dir] [-p port] [-a address]\n");
            printf("Default: root='.', port=8080, address=0.0.0.0\n");
            return 0;
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

    var fs_server = http.create_file_server(root.c_str());
    var cfg = server.ServerConfig();
    cfg.addr = addr;

    var S = server.Server(cfg);
    
    // Add catch-all route for static files
    S.router.add("GET", "/:path*", |&fs_server|(req, res) => {
        fs_server.serve_http(req, res);
    });
    // Also root
    S.router.add("GET", "/", |&fs_server|(req, res) => {
        fs_server.serve_http(req, res);
    });

    S.serve(port);

    return 0;
}

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
    while(s[len] != '\0') {
        len++;
    }
    return len;
}

}

protected func main(argc : int, argv : **char) : int {
    return server::main(argc, argv);
}
