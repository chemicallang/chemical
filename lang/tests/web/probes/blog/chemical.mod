application blog_probe

// Probe 2 of the web-stack survey: a content site. Forty generated posts, tag
// pages, an archive and an about page, all pre-rendered from one build, with one
// interactive island shared by the index and the tag pages. Everything here is
// public API only, so the work an author has to do by hand is the gap list.
source "src"

import cstd
import std
import page
import html_cbi
import css_cbi
import universal_cbi
import web
import md
