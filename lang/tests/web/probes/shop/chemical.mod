application shop_probe

// Probe 1 of the web-stack survey: a storefront. Mixed delivery modes (frozen
// pages + per-request renders + shell), sessions, CSRF, POST mutations and a JSON
// endpoint — the shape of a real small shop. Nothing here is a new language
// feature: it is written only against the documented public API, so what had to be
// worked around is exactly what the API is missing.
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
