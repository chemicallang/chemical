// url — URL encoding (percent-encoding) and decoding utilities.
//
// Raw pointer/out-buffer entry points. The canonical owned-buffer API lives at
// `encoding::url_encode` / `encoding::url_decode` (see `safe.ch`).

public namespace encoding {

using std::Result;

// ---------------------------------------------------------------------------
// Percent-encoding (URL encoding)
// ---------------------------------------------------------------------------

/// Check if a character is unreserved (doesn't need percent-encoding).
func is_unreserved(c : char) : bool {
        unsafe {
        // unreserved = ALPHA / DIGIT / '-' / '.' / '_' / '~'
        if(c >= 'a' && c <= 'z') { return true; }
        if(c >= 'A' && c <= 'Z') { return true; }
        if(c >= '0' && c <= '9') { return true; }
        if(c == '-' || c == '.' || c == '_' || c == '~') { return true; }
        return false;

        }
    }


} // end namespace encoding
