application tasks_probe

// Probe 4 of the web-stack survey: an authenticated multi-user app (the
// "easytodo" shape). Sign-up and sign-in forms, a protected area, per-user data
// that outlives the session, POST mutations behind CSRF, flash messages, a JSON
// API, and a frozen-chrome account route.
//
// Unlike the storefront, the interesting state is not in the session: it is the
// application's own database. Where that database lives, and what a page handler
// may answer, is what this probe measures. Everything is public API only; each
// workaround is recorded in `lang/docs/web-stack-gaps.md`.
source "src"

import cstd
import std
import http
import page
import html_cbi
import css_cbi
import universal_cbi
import web
import fs
import path
