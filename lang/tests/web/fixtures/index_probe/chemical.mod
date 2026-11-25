application index_probe

source "src"

import std
import page
import html_cbi
import css_cbi
import universal_cbi

// this module's public surface is offered to the "universal" indexer
index_with "universal"
