public namespace audio {

using std::string;
using std::vector;

public variant AudioError {
    FileNotFound()
    InvalidFormat(msg : string)
    UnsupportedFormat(msg : string)
    IoError(msg : string)
    BufferTooSmall()

    func message(&self) : string {
        switch(self) {
            FileNotFound() => return string("AudioError: file not found")
            InvalidFormat(msg) => {
                var s = string("AudioError: ")
                s.append_string(&msg)
                return s
            }
            UnsupportedFormat(msg) => {
                var s = string("AudioError: unsupported: ")
                s.append_string(&msg)
                return s
            }
            IoError(msg) => {
                var s = string("AudioError: IO error: ")
                s.append_string(&msg)
                return s
            }
            BufferTooSmall() => return string("AudioError: buffer too small")
        }
    }
}

@direct_init
public struct Audio {
    var samples : vector<i16>
    var sample_rate : u32
    var channels : u16
    var bits_per_sample : u16
    var num_samples : size_t
    var loaded : bool

    @make
    func make() : Audio {
        return Audio {
            samples : vector<i16>(),
            sample_rate : 44100,
            channels : 1,
            bits_per_sample : 16,
            num_samples : 0,
            loaded : false
        }
    }

    // ── safe construction ────────────────────────────────────────────────

    func load(path : &path::Path) : std::Result<Audio, AudioError> {
        return audio_safe_load(path)
    }

    func from_bytes(data : &[u8]) : std::Result<Audio, AudioError> {
        return audio_safe_from_bytes(data.data(), data.size())
    }

    func from_wav(data : &[u8]) : std::Result<Audio, AudioError> {
        return audio_safe_from_wav(data.data(), data.size())
    }

    // ── safe metadata ────────────────────────────────────────────────────

    func duration_ms(&self) : double {
        return audio_duration_ms(unsafe(self as *mut Audio))
    }

    func duration(&self) : double {
        return audio_duration_ms(unsafe(self as *mut Audio))
    }

    func frame_count(&self) : size_t {
        return self.num_samples
    }

    // Borrowed sample views.
    func samples_view(&self) : [i16] {
        return unsafe(std::span<i16>(self.samples.data(), self.samples.size()))
    }

    func samples_mut_view(&mut self) : &mut [i16] {
        return unsafe(std::mut_span<i16>(self.samples.mutable_data(), self.samples.size()))
    }

    // ── safe transformations ─────────────────────────────────────────────

    func trim(&self, start_ms : double, end_ms : double) : Audio {
        return audio_trim(unsafe(self as *mut Audio), start_ms, end_ms)
    }

    // Scale every sample in place by `factor` (clamped to the i16 range).
    func volume(&mut self, factor : float) {
        audio_volume(unsafe(self as *mut Audio), factor)
    }

    func set_volume(&mut self, factor : float) {
        audio_volume(unsafe(self as *mut Audio), factor)
    }

    func mix(&self, other : &Audio) : std::Result<Audio, AudioError> {
        return audio_mix(unsafe(self as *mut Audio), unsafe(other as *mut Audio))
    }

    func append(&self, other : &Audio) : std::Result<Audio, AudioError> {
        return audio_append(unsafe(self as *mut Audio), unsafe(other as *mut Audio))
    }

    func resample(&self, target_rate : u32) : Audio {
        return audio_resample(unsafe(self as *mut Audio), target_rate)
    }

    // ── safe encoding / saving ───────────────────────────────────────────

    func encode_wav(&self) : std::Result<vector<u8>, AudioError> {
        return audio_safe_encode_wav(self)
    }

    func save(&self, path : &path::Path) : std::Result<std::Unit, AudioError> {
        return audio_safe_save(self, path)
    }
}

} // end namespace audio
