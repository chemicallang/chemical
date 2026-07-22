using std::Result;
using std::vector;
using std::string;

// ═══════════════════════════════════════════════════════════════
// Audio struct creation and properties
// ═══════════════════════════════════════════════════════════════

@test
public func audio_create_empty_works(env : &mut TestEnv) {
    var a = audio::Audio.make()
    if(a.loaded) { env.error("unloaded audio") }
    if(a.sample_rate != 44100) { env.error("default sample rate") }
    if(a.channels != 1) { env.error("default channels") }
    if(a.bits_per_sample != 16) { env.error("default bits per sample") }
}

@test
public func audio_duration_ms_works(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 1
    a.num_samples = 44100
    a.loaded = true
    var dur = audio::audio_duration_ms(&raw mut a)
    if(dur < 999.0 || dur > 1001.0) { env.error("1 second of audio") }
}

@test
public func audio_duration_zero_samples(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.loaded = true
    var dur = audio::audio_duration_ms(&raw mut a)
    if(dur != 0.0) { env.error("zero samples = 0 duration") }
}

@test
public func audio_duration_stereo(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 2
    a.num_samples = 22050
    a.loaded = true
    var dur = audio::audio_duration_ms(&raw mut a)
    if(dur < 499.0 || dur > 501.0) { env.error("0.5 seconds stereo") }
}

// ═══════════════════════════════════════════════════════════════
// Audio volume
// ═══════════════════════════════════════════════════════════════

@test
public func audio_volume_works(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 1
    a.bits_per_sample = 16
    a.num_samples = 10
    a.samples.resize(10)
    var sptr = a.samples.data() as *mut i16
    sptr[0] = 10000
    sptr[1] = -10000
    a.loaded = true
    audio::audio_volume(&raw mut a, 0.5f)
    if(a.samples.get(0) != 5000i16) { env.error("volume 0.5 halves amplitude") }
    if(a.samples.get(1) != -5000i16) { env.error("volume 0.5 halves negative") }
}

@test
public func audio_volume_factor_one(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.samples.resize(3)
    a.loaded = true
    var sptr = a.samples.data() as *mut i16
    sptr[0] = 12345
    sptr[1] = -6789
    audio::audio_volume(&raw mut a, 1.0f)
    if(a.samples.get(0) != 12345i16) { env.error("volume 1.0 unchanged") }
    if(a.samples.get(1) != -6789i16) { env.error("volume 1.0 unchanged neg") }
}

@test
public func audio_volume_factor_zero(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.samples.resize(3)
    a.loaded = true
    var sptr = a.samples.data() as *mut i16
    sptr[0] = 9999
    audio::audio_volume(&raw mut a, 0.0f)
    if(a.samples.get(0) != 0i16) { env.error("volume 0.0 should silence") }
}

@test
public func audio_volume_clipping(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.samples.resize(3)
    a.loaded = true
    var sptr = a.samples.data() as *mut i16
    sptr[0] = 30000
    audio::audio_volume(&raw mut a, 2.0f)
    if(a.samples.get(0) != 32767i16) { env.error("should clip to max") }
}

@test
public func audio_volume_negative_clipping(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.samples.resize(3)
    a.loaded = true
    var sptr = a.samples.data() as *mut i16
    sptr[0] = -30000
    audio::audio_volume(&raw mut a, 2.0f)
    if(a.samples.get(0) != -32768i16) { env.error("should clip to min") }
}

// ═══════════════════════════════════════════════════════════════
// Audio append
// ═══════════════════════════════════════════════════════════════

@test
public func audio_append_works(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 1
    a.bits_per_sample = 16
    a.num_samples = 5
    a.samples.resize(5)
    a.loaded = true

    var b = audio::Audio.make()
    b.sample_rate = 44100
    b.channels = 1
    b.bits_per_sample = 16
    b.num_samples = 3
    b.samples.resize(3)
    b.loaded = true

    var result = audio::audio_append(&raw mut a, &raw mut b)
    if(result is Result.Err) { env.error("append should succeed"); return }
    var Ok(r) = result else unreachable
    if(r.num_samples != 8) { env.error("appended should have 8 samples") }
}

@test
public func audio_append_rate_mismatch(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.loaded = true

    var b = audio::Audio.make()
    b.sample_rate = 22050
    b.loaded = true

    var result = audio::audio_append(&raw mut a, &raw mut b)
    if(result is Result.Ok) { env.error("append with rate mismatch should fail") }
}

@test
public func audio_append_channel_mismatch(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 2
    a.loaded = true

    var b = audio::Audio.make()
    b.sample_rate = 44100
    b.channels = 1
    b.loaded = true

    var result = audio::audio_append(&raw mut a, &raw mut b)
    if(result is Result.Ok) { env.error("append with channel mismatch should fail") }
}

// ═══════════════════════════════════════════════════════════════
// Audio trim
// ═══════════════════════════════════════════════════════════════

@test
public func audio_trim_works(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 1
    a.bits_per_sample = 16
    a.num_samples = 44100
    a.samples.resize(44100)
    a.loaded = true

    var trimmed = audio::audio_trim(&raw mut a, 500.0, 1500.0)
    if(trimmed.num_samples < 44000 || trimmed.num_samples > 44200) { env.error("trimmed sample count") }
}

@test
public func audio_trim_from_start(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 1
    a.num_samples = 44100
    a.samples.resize(44100)
    a.loaded = true

    var trimmed = audio::audio_trim(&raw mut a, 0.0, 1000.0)
    if(trimmed.num_samples != 44100) { env.error("trim from 0 should keep all samples (1000ms = 44100 @ 44100)") }
}

@test
public func audio_trim_beyond_end(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 1
    a.num_samples = 44100
    a.samples.resize(44100)
    a.loaded = true

    var trimmed = audio::audio_trim(&raw mut a, 1000.0, 10000.0)
    // Should clamp end to num_samples
    if(trimmed.num_samples > 40000) { env.error("trim beyond end should clamp") }
}

// ═══════════════════════════════════════════════════════════════
// Audio resample
// ═══════════════════════════════════════════════════════════════

@test
public func audio_resample_works(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 1
    a.bits_per_sample = 16
    a.num_samples = 44100
    a.samples.resize(44100)
    a.loaded = true

    var resampled = audio::audio_resample(&raw mut a, 22050)
    if(resampled.sample_rate != 22050) { env.error("resampled rate") }
    if(resampled.num_samples != 22050) { env.error("resampled count") }
}

@test
public func audio_resample_same_rate(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 1
    a.num_samples = 100
    a.samples.resize(100)
    a.loaded = true

    var resampled = audio::audio_resample(&raw mut a, 44100)
    if(resampled.sample_rate != 44100) { env.error("same rate: sample rate") }
    if(resampled.num_samples != 100) { env.error("same rate: num samples") }
}

@test
public func audio_resample_higher_rate(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 22050
    a.channels = 1
    a.num_samples = 22050
    a.samples.resize(22050)
    a.loaded = true

    var resampled = audio::audio_resample(&raw mut a, 44100)
    if(resampled.sample_rate != 44100) { env.error("upsampled rate") }
    if(resampled.num_samples != 44100) { env.error("upsampled num samples") }
}

// ═══════════════════════════════════════════════════════════════
// Audio copy
// ═══════════════════════════════════════════════════════════════

@test
public func audio_copy_works(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 22050
    a.channels = 2
    a.bits_per_sample = 16
    a.num_samples = 100
    a.samples.resize(200)
    var sptr = a.samples.data() as *mut i16
    sptr[0] = 12345
    a.loaded = true

    var copy = audio::audio_copy(&raw mut a)
    if(copy.sample_rate != 22050) { env.error("copy sample rate") }
    if(copy.channels != 2) { env.error("copy channels") }
    if(copy.num_samples != 100) { env.error("copy num samples") }
    if(copy.samples.get(0) != 12345i16) { env.error("copy sample data") }
}

@test
public func audio_copy_independent(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.samples.resize(1)
    a.loaded = true
    var sptr = a.samples.data() as *mut i16
    sptr[0] = 7777

    var copy = audio::audio_copy(&raw mut a)
    // Modify original
    sptr[0] = 0
    if(copy.samples.get(0) != 7777i16) { env.error("copy should be independent") }
}

// ═══════════════════════════════════════════════════════════════
// Audio mix
// ═══════════════════════════════════════════════════════════════

@test
public func audio_mix_works(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 1
    a.bits_per_sample = 16
    a.num_samples = 10
    a.samples.resize(10)
    var aptr = a.samples.data() as *mut i16
    aptr[0] = 10000
    a.loaded = true

    var b = audio::Audio.make()
    b.sample_rate = 44100
    b.channels = 1
    b.bits_per_sample = 16
    b.num_samples = 10
    b.samples.resize(10)
    var bptr = b.samples.data() as *mut i16
    bptr[0] = 6000
    b.loaded = true

    var result = audio::audio_mix(&raw mut a, &raw mut b)
    if(result is Result.Err) { env.error("mix should succeed"); return }
    var Ok(r) = result else unreachable
    if(r.samples.get(0) != 8000i16) { env.error("mix average") }
}

@test
public func audio_mix_silence(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 1
    a.num_samples = 5
    a.samples.resize(5)
    a.loaded = true

    var b = audio::Audio.make()
    b.sample_rate = 44100
    b.channels = 1
    b.num_samples = 10
    b.samples.resize(10)
    var bptr = b.samples.data() as *mut i16
    bptr[0] = 8000
    b.loaded = true

    var result = audio::audio_mix(&raw mut a, &raw mut b)
    if(result is Result.Err) { env.error("mix with silence should succeed"); return }
    var Ok(r) = result else unreachable
    if(r.samples.get(0) != 4000i16) { env.error("mix silence + signal") }
    if(r.num_samples != 10) { env.error("mix should use max samples") }
}

@test
public func audio_mix_rate_mismatch(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.loaded = true

    var b = audio::Audio.make()
    b.sample_rate = 22050
    b.loaded = true

    var result = audio::audio_mix(&raw mut a, &raw mut b)
    if(result is Result.Ok) { env.error("mix with rate mismatch should fail") }
}

@test
public func audio_mix_channel_mismatch(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 44100
    a.channels = 2
    a.loaded = true

    var b = audio::Audio.make()
    b.sample_rate = 44100
    b.channels = 1
    b.loaded = true

    var result = audio::audio_mix(&raw mut a, &raw mut b)
    if(result is Result.Ok) { env.error("mix with channel mismatch should fail") }
}

// ═══════════════════════════════════════════════════════════════
// AudioError messages
// ═══════════════════════════════════════════════════════════════

@test
public func audio_error_messages_work(env : &mut TestEnv) {
    var err1 = audio::AudioError.FileNotFound()
    var msg1 = err1.message()
    if(msg1.size() == 0) { env.error("FileNotFound message empty") }

    var err2 = audio::AudioError.InvalidFormat(std::string("test error"))
    var msg2 = err2.message()
    if(msg2.size() == 0) { env.error("InvalidFormat message empty") }

    var err3 = audio::AudioError.UnsupportedFormat(std::string("mp3"))
    var msg3 = err3.message()
    if(msg3.size() == 0) { env.error("UnsupportedFormat message empty") }

    var err4 = audio::AudioError.IoError(std::string("write"))
    var msg4 = err4.message()
    if(msg4.size() == 0) { env.error("IoError message empty") }

    var err5 = audio::AudioError.BufferTooSmall()
    var msg5 = err5.message()
    if(msg5.size() == 0) { env.error("BufferTooSmall message empty") }
}

// ═══════════════════════════════════════════════════════════════
// WAV roundtrip and parsing
// ═══════════════════════════════════════════════════════════════

func write_u16_le_wav(out : *mut u8, offset : size_t, val : u16) {
    out[offset] = (val & 0xFFu16) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu16) as u8
}

func write_u32_le_wav(out : *mut u8, offset : size_t, val : u32) {
    out[offset] = (val & 0xFFu32) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu32) as u8
    out[offset + 2] = ((val >> 16) & 0xFFu32) as u8
    out[offset + 3] = ((val >> 24) & 0xFFu32) as u8
}

func build_test_wav(samples : *mut vector<i16>, channels : u16, sample_rate : u32) : vector<u8> {
    var bits_per_sample : u16 = 16
    var bytes_per_sample = (bits_per_sample / 8) as u32
    var data_size = (samples.size() as u32) * bytes_per_sample
    var file_size = 36 + data_size

    var file_data = vector<u8>()
    file_data.resize((file_size + 8) as size_t)
    var fptr = file_data.data() as *mut u8

    fptr[0] = 'R' as u8; fptr[1] = 'I' as u8; fptr[2] = 'F' as u8; fptr[3] = 'F' as u8
    write_u32_le_wav(fptr, 4, file_size)

    fptr[8] = 'W' as u8; fptr[9] = 'A' as u8; fptr[10] = 'V' as u8; fptr[11] = 'E' as u8

    fptr[12] = 'f' as u8; fptr[13] = 'm' as u8; fptr[14] = 't' as u8; fptr[15] = ' ' as u8
    write_u32_le_wav(fptr, 16, 16)
    write_u16_le_wav(fptr, 20, 1)  // PCM
    write_u16_le_wav(fptr, 22, channels)
    write_u32_le_wav(fptr, 24, sample_rate)
    write_u32_le_wav(fptr, 28, sample_rate * (channels as u32) * bytes_per_sample)
    write_u16_le_wav(fptr, 32, channels * bytes_per_sample as u16)
    write_u16_le_wav(fptr, 34, bits_per_sample)

    fptr[36] = 'd' as u8; fptr[37] = 'a' as u8; fptr[38] = 't' as u8; fptr[39] = 'a' as u8
    write_u32_le_wav(fptr, 40, data_size)

    var i : size_t = 0
    while(i < samples.size()) {
        var offset = 44 + i * (bytes_per_sample as size_t)
        if(offset + 1 < file_data.size()) {
            var sample = samples.get(i)
            fptr[offset] = (sample & 0xFF) as u8
            fptr[offset + 1] = ((sample >> 8) & 0xFF) as u8
        }
        i += 1
    }

    return file_data
}

@test
public func audio_wav_parse_16bit_mono(env : &mut TestEnv) {
    var samples = vector<i16>()
    samples.resize(100)
    var sptr = samples.data() as *mut i16
    sptr[0] = 32000
    sptr[50] = -16000

    var wav = build_test_wav(&raw mut samples, 1, 44100)
    var result = audio::parse_wav(wav.data(), wav.size())
    if(result is Result.Err) { env.error("parse 16-bit mono WAV"); return }
    var Ok(a) = result else unreachable
    if(a.sample_rate != 44100) { env.error("sample rate") }
    if(a.channels != 1) { env.error("channels") }
    if(a.bits_per_sample != 16) { env.error("bits per sample") }
    if(a.samples.get(0) != 32000i16) { env.error("first sample") }
    if(a.samples.get(50) != -16000i16) { env.error("sample at 50") }
    if(!a.loaded) { env.error("should be loaded") }
}

@test
public func audio_wav_parse_16bit_stereo(env : &mut TestEnv) {
    var samples = vector<i16>()
    samples.resize(200)
    var sptr = samples.data() as *mut i16
    sptr[0] = 100; sptr[1] = 200

    var wav = build_test_wav(&raw mut samples, 2, 44100)
    var result = audio::parse_wav(wav.data(), wav.size())
    if(result is Result.Err) { env.error("parse 16-bit stereo WAV"); return }
    var Ok(a) = result else unreachable
    if(a.channels != 2) { env.error("stereo channels") }
    if(a.num_samples != 100) { env.error("stereo num_samples") }
    if(a.samples.get(0) != 100i16) { env.error("stereo left") }
    if(a.samples.get(1) != 200i16) { env.error("stereo right") }
}

@test
public func audio_wav_save_roundtrip(env : &mut TestEnv) {
    var a = audio::Audio.make()
    a.sample_rate = 22050
    a.channels = 2
    a.bits_per_sample = 16
    a.num_samples = 50
    a.samples.resize(100)
    var sptr = a.samples.data() as *mut i16
    sptr[0] = 12345
    sptr[1] = -6789
    a.loaded = true

    var save_result = audio::save_wav(&raw mut a, "/tmp/test_roundtrip.wav")
    if(save_result is Result.Err) { env.error("save WAV failed"); return }

    var load_result = audio::load_wav("/tmp/test_roundtrip.wav")
    if(load_result is Result.Err) { env.error("load WAV failed"); return }
    var Ok(loaded) = load_result else unreachable
    if(loaded.sample_rate != 22050) { env.error("roundtrip sample rate") }
    if(loaded.channels != 2) { env.error("roundtrip channels") }
    if(loaded.num_samples != 50) { env.error("roundtrip num samples") }
    if(loaded.samples.get(0) != 12345i16) { env.error("roundtrip sample 0") }
    if(loaded.samples.get(1) != -6789i16) { env.error("roundtrip sample 1") }
}

@test
public func audio_wav_parse_8bit(env : &mut TestEnv) {
    // Build 8-bit WAV manually
    var num_samples : u32 = 10
    var bits_per_sample : u16 = 8
    var bytes_per_sample = 1u32
    var channels : u16 = 1
    var sample_rate : u32 = 8000
    var data_size = num_samples * bytes_per_sample
    var file_size = 36 + data_size

    var file_data = vector<u8>()
    file_data.resize((file_size + 8) as size_t)
    var fptr = file_data.data() as *mut u8

    fptr[0] = 'R'; fptr[1] = 'I'; fptr[2] = 'F'; fptr[3] = 'F'
    write_u32_le_wav(fptr, 4, file_size)

    fptr[8] = 'W'; fptr[9] = 'A'; fptr[10] = 'V'; fptr[11] = 'E'

    fptr[12] = 'f'; fptr[13] = 'm'; fptr[14] = 't'; fptr[15] = ' '
    write_u32_le_wav(fptr, 16, 16)
    write_u16_le_wav(fptr, 20, 1)  // PCM
    write_u16_le_wav(fptr, 22, channels)
    write_u32_le_wav(fptr, 24, sample_rate)
    write_u32_le_wav(fptr, 28, sample_rate * (channels as u32) * bytes_per_sample)
    write_u16_le_wav(fptr, 32, channels * bytes_per_sample as u16)
    write_u16_le_wav(fptr, 34, bits_per_sample)

    fptr[36] = 'd'; fptr[37] = 'a'; fptr[38] = 't'; fptr[39] = 'a'
    write_u32_le_wav(fptr, 40, data_size)

    var i : size_t = 0
    while(i < num_samples as size_t) {
        var offset = 44 + i
        fptr[offset] = (200 + i) as u8  // values 200-209, converted to i16: (v-128)*256
        i += 1
    }

    var result = audio::parse_wav(file_data.data(), file_data.size())
    if(result is Result.Err) { env.error("parse 8-bit WAV should succeed"); return }
    var Ok(a) = result else unreachable
    if(a.bits_per_sample != 8) { env.error("bits per sample") }
    // For 8-bit: sample = ((unsigned byte) - 128) << 8
    if(a.samples.get(0) != (200i16 - 128) << 8) { env.error("8-bit first sample") }
}

// ═══════════════════════════════════════════════════════════════
// WAV error paths
// ═══════════════════════════════════════════════════════════════

@test
public func audio_wav_too_small(env : &mut TestEnv) {
    var data : [10]u8
    var result = audio::parse_wav(&raw data[0], 10)
    if(result is Result.Ok) { env.error("should fail on <44 byte data") }
}

@test
public func audio_wav_not_riff(env : &mut TestEnv) {
    var data : [44]u8
    data[0] = 'N'; data[1] = 'O'; data[2] = 'P'; data[3] = 'E'
    var result = audio::parse_wav(&raw data[0], 44)
    if(result is Result.Ok) { env.error("should fail on non-RIFF") }
}

@test
public func audio_wav_not_wave(env : &mut TestEnv) {
    var data : [44]u8
    data[0] = 'R'; data[1] = 'I'; data[2] = 'F'; data[3] = 'F'
    data[8] = 'N'; data[9] = 'A'; data[10] = 'T'; data[11] = 'V'  // Not WAVE
    var result = audio::parse_wav(&raw data[0], 44)
    if(result is Result.Ok) { env.error("should fail on non-WAVE") }
}

@test
public func audio_wav_non_pcm_format(env : &mut TestEnv) {
    // Build a WAV with format != 1
    var num_samples : u32 = 10
    var data_size = num_samples * 2
    var file_size = 36 + data_size

    var file_data = vector<u8>()
    file_data.resize((file_size + 8) as size_t)
    var fptr = file_data.data() as *mut u8

    fptr[0] = 'R'; fptr[1] = 'I'; fptr[2] = 'F'; fptr[3] = 'F'
    write_u32_le_wav(fptr, 4, file_size)
    fptr[8] = 'W'; fptr[9] = 'A'; fptr[10] = 'V'; fptr[11] = 'E'
    fptr[12] = 'f'; fptr[13] = 'm'; fptr[14] = 't'; fptr[15] = ' '
    write_u32_le_wav(fptr, 16, 16)
    write_u16_le_wav(fptr, 20, 3)  // format = 3 (IEEE float, not PCM)
    write_u16_le_wav(fptr, 22, 1)
    write_u32_le_wav(fptr, 24, 44100)
    write_u32_le_wav(fptr, 28, 88200u32)
    write_u16_le_wav(fptr, 32, 2)
    write_u16_le_wav(fptr, 34, 16)

    fptr[36] = 'd'; fptr[37] = 'a'; fptr[38] = 't'; fptr[39] = 'a'
    write_u32_le_wav(fptr, 40, data_size)
    var i : size_t = 0
    while(i < data_size as size_t) { fptr[44 + i] = 0; i += 1 }

    var result = audio::parse_wav(file_data.data(), file_data.size())
    if(result is Result.Ok) { env.error("should fail on non-PCM format") }
}

@test
public func audio_wav_no_data_chunk(env : &mut TestEnv) {
    // Build a WAV with only fmt chunk, no data chunk
    var file_data = vector<u8>()
    file_data.resize(50)
    var fptr = file_data.data() as *mut u8

    fptr[0] = 'R'; fptr[1] = 'I'; fptr[2] = 'F'; fptr[3] = 'F'
    write_u32_le_wav(fptr, 4, 1000)
    fptr[8] = 'W'; fptr[9] = 'A'; fptr[10] = 'V'; fptr[11] = 'E'
    fptr[12] = 'f'; fptr[13] = 'm'; fptr[14] = 't'; fptr[15] = ' '
    write_u32_le_wav(fptr, 16, 16)
    write_u16_le_wav(fptr, 20, 1)
    write_u16_le_wav(fptr, 22, 1)
    write_u32_le_wav(fptr, 24, 44100)
    write_u32_le_wav(fptr, 28, 88200)
    write_u16_le_wav(fptr, 32, 2)
    write_u16_le_wav(fptr, 34, 16)
    // No 'data' chunk

    var result = audio::parse_wav(file_data.data(), file_data.size())
    if(result is Result.Ok) { env.error("should fail without data chunk") }
}

@test
public func audio_load_wav_nonexistent(env : &mut TestEnv) {
    var result = audio::load_wav("/tmp/nonexistent_audio_file.wav")
    if(result is Result.Ok) { env.error("should fail on nonexistent file") }
}
