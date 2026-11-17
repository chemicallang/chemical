application web_split

// The whole point of a web build (lang/docs/web-stack-plan.md §3.1): one
// application, two jobs, split by real module import edges instead of
// annotations. `--web` runs this script twice:
//
//   client pass — `client` is true:  shared + client, JavaScript backend
//   server pass — `server` is true:  shared + server, native executable
//
// `src` (the server entry point and its routes) exists only in the server pass,
// so the client bundle's entry point is `client`'s `main`.

source "src" if server

import cstd
import std
import "./shared"
import "./server" if server
import "./client" if client
