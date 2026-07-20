using std::Result;

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
public func audio_error_messages_work(env : &mut TestEnv) {
    var err = audio::AudioError.InvalidFormat(std::string("test error"))
    var msg = err.message()
    if(msg.size() == 0) { env.error("error message should not be empty") }
}

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
