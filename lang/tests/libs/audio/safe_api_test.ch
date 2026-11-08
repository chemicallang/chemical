using std::Result;
using std::vector;
using std::string;

// Focused tests for the additive safe audio surface: `Audio::load`,
// `from_wav`, `encode_wav`, `save`, sample views, and transformations.

func audio_safe_temp_pathbuf(name : *char) : path::PathBuf {
    var s = make_temp_test_path(name)
    return path::PathBuf.from(std::str.make_view(s.to_view()))
}

func audio_safe_view(v : &vector<u8>) : [u8] {
    return unsafe(std::span<u8>(v.data(), v.size()))
}

func audio_safe_fixed_view(p : *u8, n : size_t) : [u8] {
    return unsafe(std::span<u8>(p, n))
}

func audio_safe_make_tone(rate : u32, sample_count : int) : audio::Audio {
    var a = audio::Audio.make()
    a.sample_rate = rate
    a.channels = 1
    a.bits_per_sample = 16
    var i : int = 0
    while(i < sample_count) {
        a.samples.push((i * 100 - 500) as i16)
        i += 1
    }
    a.num_samples = a.samples.size()
    a.loaded = true
    return a
}

// ═══════════════════════════════════════════════════════════════
// Positive round-trips
// ═══════════════════════════════════════════════════════════════

@test
public func audio_safe_wav_roundtrip(env : &mut TestEnv) {
    var a = audio_safe_make_tone(8000, 8)

    var encoded = a.encode_wav()
    if(encoded is Result.Err) { env.error("encode_wav should succeed"); return }
    var Ok(bytes) = encoded else unsafe { unreachable }
    if(bytes.size() < 44) { env.error("WAV should have a header"); return }

    var parsed = audio::Audio.from_wav(audio_safe_view(&bytes))
    if(parsed is Result.Err) { env.error("from_wav should succeed"); return }
    var Ok(loaded) = parsed else unsafe { unreachable }

    if(loaded.sample_rate != 8000) { env.error("sample rate roundtrip") }
    if(loaded.samples.size() != 8) { env.error("sample count roundtrip") }
    if(loaded.duration_ms() <= 0.0) { env.error("duration should be positive") }

    var view = loaded.samples_view()
    if(view.size() != 8) { env.error("samples_view size") }
}

@test
public func audio_safe_sample_views_are_mutable(env : &mut TestEnv) {
    var a = audio_safe_make_tone(44100, 4)
    var view = a.samples_mut_view()
    view.set(0, 1234)
    view.set(1, -5678)
    if(a.samples.get(0) != 1234) { env.error("samples_mut_view write 0") }
    if(a.samples.get(1) != -5678) { env.error("samples_mut_view write 1") }
}

@test
public func audio_safe_load_and_save(env : &mut TestEnv) {
    var a = audio_safe_make_tone(16000, 6)
    var p = audio_safe_temp_pathbuf("p24_safe_audio.wav")

    var saved = a.save(&p)
    if(saved is Result.Err) { env.error("save should succeed"); return }

    var loaded = audio::Audio.load(&p)
    if(loaded is Result.Err) { env.error("load should succeed"); return }
    var Ok(read_back) = loaded else unsafe { unreachable }
    if(read_back.sample_rate != 16000) { env.error("loaded sample rate") }
    if(read_back.samples.size() != 6) { env.error("loaded sample count") }
}

@test
public func audio_safe_transformations(env : &mut TestEnv) {
    var a = audio_safe_make_tone(8000, 8)
    var before = a.samples.get(7)
    a.volume(0.5f)
    var after = a.samples.get(7)
    if(before > 0 && after >= before) { env.error("volume should attenuate positive samples") }

    var trimmed = a.trim(0.0, 0.25)
    if(trimmed.samples.size() > a.samples.size()) { env.error("trim should not grow audio") }

    var b = audio_safe_make_tone(8000, 8)
    var mixed = a.mix(&b)
    if(mixed is Result.Err) { env.error("mix should succeed") }

    var appended = a.append(&b)
    if(appended is Result.Err) { env.error("append should succeed"); return }
    var Ok(longer) = appended else unsafe { unreachable }
    if(longer.samples.size() != a.samples.size() + b.samples.size()) { env.error("append sample count") }

    var resampled = a.resample(16000)
    if(resampled.sample_rate != 16000) { env.error("resample target rate") }
}

// ═══════════════════════════════════════════════════════════════
// Misuse
// ═══════════════════════════════════════════════════════════════

@test
public func audio_safe_bad_magic_rejected(env : &mut TestEnv) {
    var garbage : [16]u8
    var i : size_t = 0
    while(i < 16) { garbage[i] = 0xAA; i += 1 }
    var view = audio_safe_fixed_view(&raw garbage[0], 16)
    var result = audio::Audio.from_wav(view)
    if(result is Result.Ok) { env.error("bad WAV magic must be rejected") }
}

@test
public func audio_safe_truncated_rejected(env : &mut TestEnv) {
    var tiny : [4]u8 = [0x52, 0x49, 0x46, 0x46]
    var view = audio_safe_fixed_view(&raw tiny[0], 4)
    var result = audio::Audio.from_wav(view)
    if(result is Result.Ok) { env.error("truncated WAV must be rejected") }
}

@test
public func audio_safe_riff_without_wave_rejected(env : &mut TestEnv) {
    var riff : [12]u8 = [0x52, 0x49, 0x46, 0x46, 0, 0, 0, 0, 0x41, 0x42, 0x43, 0x44]
    var view = audio_safe_fixed_view(&raw riff[0], 12)
    var result = audio::Audio.from_bytes(view)
    if(result is Result.Ok) { env.error("RIFF without WAVE must be rejected") }
}

@test
public func audio_safe_load_nonexistent(env : &mut TestEnv) {
    var p = audio_safe_temp_pathbuf("p24_does_not_exist.wav")
    var result = audio::Audio.load(&p)
    if(result is Result.Ok) { env.error("missing file must fail") }
}
