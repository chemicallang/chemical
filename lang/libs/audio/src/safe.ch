// Safe audio surface: `&[u8]` WAV decoding, `&path::Path` loading/saving,
// `Result` errors, and owned `Audio` values. The legacy raw functions in
// `wav.ch` remain available; new code should use the methods on `Audio` or the
// helpers here (no `*mut Audio` in the new surface).
public namespace audio {

using std::Result;
using std::string;
using std::vector;

// ── loading ───────────────────────────────────────────────────────────────

func audio_safe_load(path : &path::Path) : Result<Audio, AudioError> {
    var ps = path.as_string()
    var read = fs::read_entire_file(ps.c_str())
    if(read is Result.Err) {
        return Result.Err(AudioError.FileNotFound())
    }
    var Ok(data) = read else unsafe { unreachable }
    return audio_safe_from_bytes(data.data(), data.size())
}

func audio_safe_from_bytes(data : *u8, len : size_t) : Result<Audio, AudioError> {
    unsafe {
        if(len >= 12) {
            if(data[0] == 'R' as u8 && data[1] == 'I' as u8 && data[2] == 'F' as u8 && data[3] == 'F' as u8 &&
               data[8] == 'W' as u8 && data[9] == 'A' as u8 && data[10] == 'V' as u8 && data[11] == 'E' as u8) {
                return audio_safe_from_wav(data, len)
            }
        }
    }
    return Result.Err(AudioError.InvalidFormat(string("not a WAV file")))
}

func audio_safe_from_wav(data : *u8, len : size_t) : Result<Audio, AudioError> {
    return parse_wav(data, len)
}

// ── encoding / saving ─────────────────────────────────────────────────────

func audio_safe_encode_wav(a : &Audio) : Result<vector<u8>, AudioError> {
    return encode_wav(unsafe(a as *mut Audio))
}

func audio_safe_save(a : &Audio, path : &path::Path) : Result<std::Unit, AudioError> {
    var encoded = audio_safe_encode_wav(a)
    if(encoded is Result.Err) {
        var Err(e) = encoded else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(bytes) = encoded else unsafe { unreachable }

    var ps = path.as_string()
    var written = fs::write_text_file(ps.c_str(), bytes.data(), bytes.size())
    if(written is Result.Err) {
        return Result.Err(AudioError.IoError(string("failed to write WAV file")))
    }
    return Result.Ok(std::Unit{})
}

} // end namespace audio
