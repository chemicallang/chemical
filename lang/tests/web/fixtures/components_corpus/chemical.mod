application components_corpus

// Phase 5b corpus fixture. Importing `components` compiles every component in
// the library on both `--web` sides; `components` imports `dom`
// unconditionally, so both jobs build the ambient index and run the body
// checker. The rendered set below is representative, not exhaustive: the body
// check is module-wide, not usage-driven.
source "src" if server

import std
import page
import html_cbi
import css_cbi
import universal_cbi
import components
import "./shared"
import "./client" if client
