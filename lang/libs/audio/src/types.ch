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

// Owned PCM audio. The sample buffer and its metadata are private: `frame_total`
// always equals `sample_data.size() / channel_count` once constructed through
// `new` or a decoder. Safe code reads/mutates through the accessors and
// `samples()`/`samples_mut()`.
@direct_init
public struct Audio {
    internal var sample_data : vector<i16>
    internal var sample_rate_hz : u32
    internal var channel_count : u16
    internal var bit_depth : u16
    internal var frame_total : size_t
    internal var loaded_flag : bool

    @make
    public func make() : Audio {
        return Audio {
            sample_data : vector<i16>(),
            sample_rate_hz : 44100,
            channel_count : 1,
            bit_depth : 16,
            frame_total : 0,
            loaded_flag : false
        }
    }

    // ── safe construction ────────────────────────────────────────────────

    /// Build audio from owned interleaved samples. Requires a positive sample
    /// rate, at least one channel, 8- or 16-bit samples, and a sample count
    /// divisible by the channel count. `num_samples()` is derived from the
    /// buffer, so the metadata cannot disagree with the samples.
    public func new(sample_rate : u32, channels : u16, bits_per_sample : u16, samples : vector<i16>) : std::Result<Audio, AudioError> {
        if(sample_rate == 0) {
            return std::Result.Err<Audio, AudioError>(AudioError.InvalidFormat(string("sample rate must be positive")))
        }
        if(channels == 0) {
            return std::Result.Err<Audio, AudioError>(AudioError.InvalidFormat(string("channel count must be positive")))
        }
        if(bits_per_sample != 8 && bits_per_sample != 16) {
            return std::Result.Err<Audio, AudioError>(AudioError.UnsupportedFormat(string("only 8- and 16-bit PCM are supported")))
        }
        if(samples.size() % (channels as size_t) != 0) {
            return std::Result.Err<Audio, AudioError>(AudioError.InvalidFormat(string("sample count is not a multiple of the channel count")))
        }

        var result = Audio.make()
        result.sample_rate_hz = sample_rate
        result.channel_count = channels
        result.bit_depth = bits_per_sample
        result.frame_total = samples.size() / (channels as size_t)
        result.sample_data = samples
        result.loaded_flag = true
        return std::Result.Ok<Audio, AudioError>(result)
    }

    func load(path : &path::Path) : std::Result<Audio, AudioError> {
        return load_from_path(path)
    }

    func from_bytes(data : &[u8]) : std::Result<Audio, AudioError> {
        return decode_any(data.data(), data.size())
    }

    func from_wav(data : &[u8]) : std::Result<Audio, AudioError> {
        return decode_wav(data.data(), data.size())
    }

    // ── safe metadata ────────────────────────────────────────────────────

    public func sample_rate(&self) : u32 { return self.sample_rate_hz }
    public func channels(&self) : u16 { return self.channel_count }
    public func bits_per_sample(&self) : u16 { return self.bit_depth }
    public func num_samples(&self) : size_t { return self.frame_total }
    public func loaded(&self) : bool { return self.loaded_flag }
    public func is_loaded(&self) : bool { return self.loaded_flag }

    func duration_ms(&self) : double {
        return raw::audio_duration_ms(unsafe(self as *mut Audio))
    }

    func duration(&self) : double {
        return raw::audio_duration_ms(unsafe(self as *mut Audio))
    }

    func frame_count(&self) : size_t {
        return self.frame_total
    }

    /// Borrowed interleaved samples. Length is `num_samples() * channels()`.
    public func samples(&self) : &[i16] {
        return unsafe(std::span<i16>(self.sample_data.data(), self.sample_data.size()))
    }

    /// Mutable borrowed interleaved samples for bulk processing.
    public func samples_mut(&mut self) : &mut [i16] {
        return unsafe(std::mut_span<i16>(self.sample_data.mutable_data(), self.sample_data.size()))
    }

    // Compatibility aliases for the original view names.
    func samples_view(&self) : &[i16] { return self.samples() }
    func samples_mut_view(&mut self) : &mut [i16] { return self.samples_mut() }

    // ── safe transformations ─────────────────────────────────────────────

    func trim(&self, start_ms : double, end_ms : double) : Audio {
        return raw::audio_trim(unsafe(self as *mut Audio), start_ms, end_ms)
    }

    // Scale every sample in place by `factor` (clamped to the i16 range).
    func volume(&mut self, factor : float) {
        raw::audio_volume(unsafe(self as *mut Audio), factor)
    }

    func set_volume(&mut self, factor : float) {
        raw::audio_volume(unsafe(self as *mut Audio), factor)
    }

    func mix(&self, other : &Audio) : std::Result<Audio, AudioError> {
        return raw::audio_mix(unsafe(self as *mut Audio), unsafe(other as *mut Audio))
    }

    func append(&self, other : &Audio) : std::Result<Audio, AudioError> {
        return raw::audio_append(unsafe(self as *mut Audio), unsafe(other as *mut Audio))
    }

    func resample(&self, target_rate : u32) : Audio {
        return raw::audio_resample(unsafe(self as *mut Audio), target_rate)
    }

    // ── safe encoding / saving ───────────────────────────────────────────

    func encode_wav(&self) : std::Result<vector<u8>, AudioError> {
        return encode_wav_bytes(self)
    }

    func save(&self, path : &path::Path) : std::Result<std::Unit, AudioError> {
        return save_to_path(self, path)
    }
}

} // end namespace audio
