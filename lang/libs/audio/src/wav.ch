public namespace audio {

using std::Result;
using std::string;
using std::vector;

func read_u16_le(data : *u8, offset : size_t) : u16 {
    return (data[offset] as u16) | ((data[offset + 1] as u16) << 8)
}

func read_u32_le(data : *u8, offset : size_t) : u32 {
    return (data[offset] as u32) | ((data[offset + 1] as u32) << 8) |
           ((data[offset + 2] as u32) << 16) | ((data[offset + 3] as u32) << 24)
}

func write_u16_le(out : *mut u8, offset : size_t, val : u16) {
    out[offset] = (val & 0xFFu16) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu16) as u8
}

func write_u32_le(out : *mut u8, offset : size_t, val : u32) {
    out[offset] = (val & 0xFFu32) as u8
    out[offset + 1] = ((val >> 8) & 0xFFu32) as u8
    out[offset + 2] = ((val >> 16) & 0xFFu32) as u8
    out[offset + 3] = ((val >> 24) & 0xFFu32) as u8
}

public func load_wav(path : *char) : std::Result<Audio, AudioError> {
    var read_result = fs::read_entire_file(path)
    if(read_result is Result.Err) {
        return std.Result.Err(AudioError.FileNotFound())
    }
    var Ok(data) = read_result else unreachable
    return parse_wav(data.data(), data.size())
}

public func parse_wav(data : *u8, data_len : size_t) : std::Result<Audio, AudioError> {
    if(data_len < 44) {
        return std.Result.Err(AudioError.InvalidFormat(string("WAV file too small")))
    }

    if(data[0] != 'R' as u8 || data[1] != 'I' as u8 || data[2] != 'F' as u8 || data[3] != 'F' as u8) {
        return std.Result.Err(AudioError.InvalidFormat(string("not a RIFF file")))
    }

    if(data[8] != 'W' as u8 || data[9] != 'A' as u8 || data[10] != 'V' as u8 || data[11] != 'E' as u8) {
        return std.Result.Err(AudioError.InvalidFormat(string("not a WAVE file")))
    }

    var audio = Audio.make()
    var pos : size_t = 12

    while(pos + 8 <= data_len) {
        var chunk_id = read_u32_le(data, pos)
        var chunk_size = read_u32_le(data, pos + 4) as size_t

        if(chunk_id == 0x666D7420u32) {
            if(chunk_size < 16 || pos + 24 > data_len) {
                return std.Result.Err(AudioError.InvalidFormat(string("fmt chunk too small")))
            }
            var audio_format = read_u16_le(data, pos + 8)
            audio.channels = read_u16_le(data, pos + 10)
            audio.sample_rate = read_u32_le(data, pos + 12)
            var byte_rate = read_u32_le(data, pos + 16)
            var block_align = read_u16_le(data, pos + 20)
            audio.bits_per_sample = read_u16_le(data, pos + 22)

            if(audio_format != 1) {
                return std.Result.Err(AudioError.UnsupportedFormat(string("only PCM WAV supported")))
            }

            pos += 8 + chunk_size
            if(chunk_size % 2 != 0) { pos += 1 }
        } else if(chunk_id == 0x64617461u32) {
            var data_size = chunk_size
            var bytes_per_sample = (audio.bits_per_sample / 8) as size_t
            var total_samples = data_size / bytes_per_sample

            audio.samples.resize(total_samples)
            audio.num_samples = total_samples / (audio.channels as size_t)

            var src_pos = pos + 8
            var sptr = audio.samples.data() as *mut i16
            var i : size_t = 0
            while(i < total_samples && src_pos + bytes_per_sample <= data_len) {
                if(audio.bits_per_sample == 16) {
                    var lo = data[src_pos] as i16
                    var hi = (data[src_pos + 1] as i16) << 8
                    sptr[i] = lo | hi
                } else if(audio.bits_per_sample == 8) {
                    sptr[i] = ((data[src_pos] as i16) - 128) << 8
                }
                src_pos += bytes_per_sample
                i += 1
            }

            audio.loaded = true
            break
        } else {
            pos += 8 + chunk_size
            if(chunk_size % 2 != 0) { pos += 1 }
        }
    }

    if(!audio.loaded) {
        return std.Result.Err(AudioError.InvalidFormat(string("no data chunk found")))
    }

    return std.Result.Ok(audio)
}

public func save_wav(audio : *mut Audio, path : *char) : std::Result<std::Unit, AudioError> {
    var bytes_per_sample = (audio.bits_per_sample / 8) as u32
    var data_size = (audio.samples.size() as u32) * bytes_per_sample
    var file_size = 36 + data_size

    var file_data = vector<u8>()
    file_data.resize((file_size + 8) as size_t)
    var fptr = file_data.data() as *mut u8

    fptr[0] = 'R' as u8
    fptr[1] = 'I' as u8
    fptr[2] = 'F' as u8
    fptr[3] = 'F' as u8
    write_u32_le(fptr, 4, file_size)

    fptr[8] = 'W' as u8
    fptr[9] = 'A' as u8
    fptr[10] = 'V' as u8
    fptr[11] = 'E' as u8

    fptr[12] = 'f' as u8
    fptr[13] = 'm' as u8
    fptr[14] = 't' as u8
    fptr[15] = ' ' as u8
    write_u32_le(fptr, 16, 16)
    write_u16_le(fptr, 20, 1)
    write_u16_le(fptr, 22, audio.channels)
    write_u32_le(fptr, 24, audio.sample_rate)
    write_u32_le(fptr, 28, audio.sample_rate * (audio.channels as u32) * bytes_per_sample)
    write_u16_le(fptr, 32, audio.channels * (audio.bits_per_sample / 8))
    write_u16_le(fptr, 34, audio.bits_per_sample)

    fptr[36] = 'd' as u8
    fptr[37] = 'a' as u8
    fptr[38] = 't' as u8
    fptr[39] = 'a' as u8
    write_u32_le(fptr, 40, data_size)

    var i : size_t = 0
    while(i < audio.samples.size()) {
        var offset = 44 + i * (bytes_per_sample as size_t)
        if(offset + 1 < file_data.size()) {
            var sample = audio.samples.get(i)
            if(audio.bits_per_sample == 16) {
                fptr[offset] = (sample & 0xFF) as u8
                fptr[offset + 1] = ((sample >> 8) & 0xFF) as u8
            } else if(audio.bits_per_sample == 8) {
                fptr[offset] = ((sample >> 8) + 128) as u8
            }
        }
        i += 1
    }

    var write_result = fs::write_text_file(path, file_data.data(), file_data.size())
    if(write_result is Result.Err) {
        return std.Result.Err(AudioError.IoError(string("failed to write WAV file")))
    }

    return std.Result.Ok(std::Unit{})
}

public func audio_duration_ms(audio : *mut Audio) : double {
    if(audio.sample_rate == 0) { return 0.0 }
    return (audio.num_samples as double) * 1000.0 / (audio.sample_rate as double)
}

public func audio_trim(audio : *mut Audio, start_ms : double, end_ms : double) : Audio {
    var start_sample = ((start_ms * (audio.sample_rate as double)) / 1000.0) as size_t
    var end_sample = ((end_ms * (audio.sample_rate as double)) / 1000.0) as size_t

    if(start_sample >= audio.num_samples) { start_sample = audio.num_samples - 1 }
    if(end_sample > audio.num_samples) { end_sample = audio.num_samples }

    var result = Audio.make()
    result.sample_rate = audio.sample_rate
    result.channels = audio.channels
    result.bits_per_sample = audio.bits_per_sample
    result.num_samples = end_sample - start_sample

    var ch = audio.channels as size_t
    var total = result.num_samples * ch
    result.samples.resize(total)

    var src_offset = start_sample * ch
    var sptr = result.samples.data() as *mut i16
    var i : size_t = 0
    while(i < total) {
        sptr[i] = audio.samples.get(src_offset + i)
        i += 1
    }

    result.loaded = true
    return result
}

public func audio_volume(audio : *mut Audio, factor : float) {
    var sptr = audio.samples.data() as *mut i16
    var i : size_t = 0
    while(i < audio.samples.size()) {
        var sample = (audio.samples.get(i) as float * factor) as i32
        if(sample > 32767) { sample = 32767 }
        if(sample < -32768) { sample = -32768 }
        sptr[i] = sample as i16
        i += 1
    }
}

public func audio_mix(a : *mut Audio, b : *mut Audio) : std::Result<Audio, AudioError> {
    if(a.sample_rate != b.sample_rate) {
        return std.Result.Err(AudioError.InvalidFormat(string("sample rate mismatch")))
    }
    if(a.channels != b.channels) {
        return std.Result.Err(AudioError.InvalidFormat(string("channel count mismatch")))
    }

    var max_samples : size_t
    if(a.num_samples > b.num_samples) {
        max_samples = a.num_samples
    } else {
        max_samples = b.num_samples
    }
    var ch = a.channels as size_t

    var result = Audio.make()
    result.sample_rate = a.sample_rate
    result.channels = a.channels
    result.bits_per_sample = a.bits_per_sample
    result.num_samples = max_samples
    result.samples.resize(max_samples * ch)

    var rptr = result.samples.data() as *mut i16
    var i : size_t = 0
    while(i < max_samples * ch) {
        var sa : i32 = 0
        var sb : i32 = 0
        if(i < a.samples.size()) { sa = a.samples.get(i) as i32 }
        if(i < b.samples.size()) { sb = b.samples.get(i) as i32 }
        var mixed = (sa + sb) / 2
        if(mixed > 32767) { mixed = 32767 }
        if(mixed < -32768) { mixed = -32768 }
        rptr[i] = mixed as i16
        i += 1
    }

    result.loaded = true
    return std.Result.Ok(result)
}

public func audio_append(a : *mut Audio, b : *mut Audio) : std::Result<Audio, AudioError> {
    if(a.sample_rate != b.sample_rate) {
        return std.Result.Err(AudioError.InvalidFormat(string("sample rate mismatch")))
    }
    if(a.channels != b.channels) {
        return std.Result.Err(AudioError.InvalidFormat(string("channel count mismatch")))
    }

    var result = Audio.make()
    result.sample_rate = a.sample_rate
    result.channels = a.channels
    result.bits_per_sample = a.bits_per_sample
    result.num_samples = a.num_samples + b.num_samples

    var total_a = a.samples.size()
    var total_b = b.samples.size()
    result.samples.resize(total_a + total_b)

    var rptr = result.samples.data() as *mut i16
    var i : size_t = 0
    while(i < total_a) {
        rptr[i] = a.samples.get(i)
        i += 1
    }
    i = 0
    while(i < total_b) {
        rptr[total_a + i] = b.samples.get(i)
        i += 1
    }

    result.loaded = true
    return std.Result.Ok(result)
}

public func audio_resample(audio : *mut Audio, target_rate : u32) : Audio {
    if(audio.sample_rate == target_rate) {
        return audio_copy(audio)
    }

    var ratio = (target_rate as double) / (audio.sample_rate as double)
    var new_num_samples = (audio.num_samples as double * ratio) as size_t
    var ch = audio.channels as size_t

    var result = Audio.make()
    result.sample_rate = target_rate
    result.channels = audio.channels
    result.bits_per_sample = audio.bits_per_sample
    result.num_samples = new_num_samples
    result.samples.resize(new_num_samples * ch)

    var rptr = result.samples.data() as *mut i16
    var i : size_t = 0
    while(i < new_num_samples) {
        var src_pos = (i as double / ratio) as size_t
        if(src_pos >= audio.num_samples) { src_pos = audio.num_samples - 1 }
        var j : size_t = 0
        while(j < ch) {
            rptr[i * ch + j] = audio.samples.get(src_pos * ch + j)
            j += 1
        }
        i += 1
    }

    result.loaded = true
    return result
}

public func audio_copy(audio : *mut Audio) : Audio {
    var result = Audio.make()
    result.sample_rate = audio.sample_rate
    result.channels = audio.channels
    result.bits_per_sample = audio.bits_per_sample
    result.num_samples = audio.num_samples
    result.loaded = audio.loaded
    result.samples.resize(audio.samples.size())
    var rptr = result.samples.data() as *mut i16
    var i : size_t = 0
    while(i < audio.samples.size()) {
        rptr[i] = audio.samples.get(i)
        i += 1
    }
    return result
}

} // end namespace audio
