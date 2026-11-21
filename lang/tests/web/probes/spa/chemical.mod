application spa_probe

// Probe 3 of the web-stack survey: an SPA. One HTML shell, every view built in
// the browser from a JSON endpoint, client-side navigation without a reload.
//
// Two jobs, split by import edges (`--web`): `client` becomes JavaScript, `server`
// plus `src` becomes the native process that serves the shell and the JSON.
source "src" if server

import cstd
import std
import http if server
import page if server
import web if server
import "./shared"
import "./server" if server
import "./client" if client
