public namespace audio {

using std::Result;
using std::string;
using std::vector;

func read_u16_le(data : *u8, offset : size_t) : u16 {
        unsafe {
        return (data[offset] as u16) | ((data[offset + 1] as u16) << 8)

        }
    }

func read_u32_le(data : *u8, offset : size_t) : u32 {
        unsafe {
        return (data[offset] as u32) | ((data[offset + 1] as u32) << 8) |
               ((data[offset + 2] as u32) << 16) | ((data[offset + 3] as u32) << 24)

        }
    }

func write_u16_le(out : *mut u8, offset : size_t, val : u16) {
        unsafe {
        out[offset] = (val & 0xFFu16) as u8
        out[offset + 1] = ((val >> 8) & 0xFFu16) as u8

        }
    }

func write_u32_le(out : *mut u8, offset : size_t, val : u32) {
        unsafe {
        out[offset] = (val & 0xFFu32) as u8
        out[offset + 1] = ((val >> 8) & 0xFFu32) as u8
        out[offset + 2] = ((val >> 16) & 0xFFu32) as u8
        out[offset + 3] = ((val >> 24) & 0xFFu32) as u8

        }
    }

func impl_load_wav(path : *char) : std::Result<Audio, AudioError> {
    var read_result = fs::raw::read_entire_file(path)
    if(read_result is Result.Err) {
        return std.Result.Err(AudioError.FileNotFound())
    }
    var Ok(data) = read_result else unsafe { unreachable }
    return impl_parse_wav(data.data(), data.size())
}

func impl_parse_wav(data : *u8, data_len : size_t) : std::Result<Audio, AudioError> {
        unsafe {
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

            if(chunk_id == 0x20746D66u32) {
                if(chunk_size < 16 || pos + 24 > data_len) {
                    return std.Result.Err(AudioError.InvalidFormat(string("fmt chunk too small")))
                }
                var audio_format = read_u16_le(data, pos + 8)
                audio.channel_count = read_u16_le(data, pos + 10)
                audio.sample_rate_hz = read_u32_le(data, pos + 12)
                var byte_rate = read_u32_le(data, pos + 16)
                var block_align = read_u16_le(data, pos + 20)
                audio.bit_depth = read_u16_le(data, pos + 22)

                if(audio_format != 1) {
                    return std.Result.Err(AudioError.UnsupportedFormat(string("only PCM WAV supported")))
                }

                pos += 8 + chunk_size
                if(chunk_size % 2 != 0) { pos += 1 }
            } else if(chunk_id == 0x61746164u32) {
                var data_size = chunk_size
                var bytes_per_sample = (audio.bit_depth / 8) as size_t
                var total_samples = data_size / bytes_per_sample

                audio.sample_data.resize(total_samples)
                audio.frame_total = total_samples / (audio.channel_count as size_t)

                var src_pos = pos + 8
                var sptr = audio.sample_data.data() as *mut i16
                var i : size_t = 0
                while(i < total_samples && src_pos + bytes_per_sample <= data_len) {
                    if(audio.bit_depth == 16) {
                        var lo = data[src_pos] as i16
                        var hi = (data[src_pos + 1] as i16) << 8
                        sptr[i] = lo | hi
                    } else if(audio.bit_depth == 8) {
                        sptr[i] = ((data[src_pos] as i16) - 128) << 8
                    }
                    src_pos += bytes_per_sample
                    i += 1
                }

                audio.loaded_flag = true
                break
            } else {
                pos += 8 + chunk_size
                if(chunk_size % 2 != 0) { pos += 1 }
            }
        }

        if(!audio.loaded_flag) {
            return std.Result.Err(AudioError.InvalidFormat(string("no data chunk found")))
        }

        return std.Result.Ok(audio)

        }
    }

// encode_wav — encode Audio to in-memory WAV bytes (no file I/O).
func impl_encode_wav(audio : *mut Audio) : std::Result<vector<u8>, AudioError> {
        unsafe {
        var bytes_per_sample = (audio.bit_depth / 8) as u32
        var data_size = (audio.sample_data.size() as u32) * bytes_per_sample
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
        write_u16_le(fptr, 22, audio.channel_count)
        write_u32_le(fptr, 24, audio.sample_rate_hz)
        write_u32_le(fptr, 28, audio.sample_rate_hz * (audio.channel_count as u32) * bytes_per_sample)
        write_u16_le(fptr, 32, audio.channel_count * (audio.bit_depth / 8))
        write_u16_le(fptr, 34, audio.bit_depth)

        fptr[36] = 'd' as u8
        fptr[37] = 'a' as u8
        fptr[38] = 't' as u8
        fptr[39] = 'a' as u8
        write_u32_le(fptr, 40, data_size)

        var i : size_t = 0
        while(i < audio.sample_data.size()) {
            var offset = 44 + i * (bytes_per_sample as size_t)
            if(offset < file_data.size()) {
                var sample = audio.sample_data.at(i)
                if(audio.bit_depth == 16) {
                    fptr[offset] = (sample & 0xFF) as u8
                    fptr[offset + 1] = ((sample >> 8) & 0xFF) as u8
                } else if(audio.bit_depth == 8) {
                    fptr[offset] = ((sample >> 8) + 128) as u8
                }
            }
            i += 1
        }

        return std.Result.Ok(file_data)

        }
    }

func impl_save_wav(audio : *mut Audio, path : *char) : std::Result<std::Unit, AudioError> {
        unsafe {
        var bytes_per_sample = (audio.bit_depth / 8) as u32
        var data_size = (audio.sample_data.size() as u32) * bytes_per_sample
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
        write_u16_le(fptr, 22, audio.channel_count)
        write_u32_le(fptr, 24, audio.sample_rate_hz)
        write_u32_le(fptr, 28, audio.sample_rate_hz * (audio.channel_count as u32) * bytes_per_sample)
        write_u16_le(fptr, 32, audio.channel_count * (audio.bit_depth / 8))
        write_u16_le(fptr, 34, audio.bit_depth)

        fptr[36] = 'd' as u8
        fptr[37] = 'a' as u8
        fptr[38] = 't' as u8
        fptr[39] = 'a' as u8
        write_u32_le(fptr, 40, data_size)

        var i : size_t = 0
        while(i < audio.sample_data.size()) {
            var offset = 44 + i * (bytes_per_sample as size_t)
            if(offset < file_data.size()) {
                var sample = audio.sample_data.at(i)
                if(audio.bit_depth == 16) {
                    fptr[offset] = (sample & 0xFF) as u8
                    fptr[offset + 1] = ((sample >> 8) & 0xFF) as u8
                } else if(audio.bit_depth == 8) {
                    fptr[offset] = ((sample >> 8) + 128) as u8
                }
            }
            i += 1
        }

        var write_result = fs::raw::write_text_file(path, file_data.data(), file_data.size())
        if(write_result is Result.Err) {
            return std.Result.Err(AudioError.IoError(string("failed to write WAV file")))
        }

        return std.Result.Ok(std::Unit{})

        }
    }

func impl_audio_duration_ms(audio : *mut Audio) : double {
        unsafe {
        if(audio.sample_rate_hz == 0) { return 0.0 }
        return (audio.frame_total as double) * 1000.0 / (audio.sample_rate_hz as double)

        }
    }

func impl_audio_trim(audio : *mut Audio, start_ms : double, end_ms : double) : Audio {
        unsafe {
        // Empty input (or a malformed channel count/rate) is an identity copy:
        // never underflow `frame_total - 1` and never divide by zero.
        if(audio.sample_rate_hz == 0 || audio.channel_count == 0 || audio.frame_total == 0) {
            return impl_audio_copy(audio)
        }

        var ch = audio.channel_count as size_t
        var available_frames = audio.sample_data.size() / ch
        var total_frames = audio.frame_total
        if(total_frames > available_frames) { total_frames = available_frames }
        if(total_frames == 0) { return impl_audio_copy(audio) }

        var clamped_start_ms = start_ms
        if(clamped_start_ms < 0.0) { clamped_start_ms = 0.0 }
        var clamped_end_ms = end_ms
        if(clamped_end_ms < clamped_start_ms) { clamped_end_ms = clamped_start_ms }

        var start_sample = ((clamped_start_ms * (audio.sample_rate_hz as double)) / 1000.0) as size_t
        var end_sample = ((clamped_end_ms * (audio.sample_rate_hz as double)) / 1000.0) as size_t

        if(start_sample > total_frames) { start_sample = total_frames }
        if(end_sample > total_frames) { end_sample = total_frames }
        if(end_sample < start_sample) { end_sample = start_sample }

        var result = Audio.make()
        result.sample_rate_hz = audio.sample_rate_hz
        result.channel_count = audio.channel_count
        result.bit_depth = audio.bit_depth
        result.frame_total = end_sample - start_sample

        var total = result.frame_total * ch
        result.sample_data.resize(total)

        var src_offset = start_sample * ch
        var sptr = result.sample_data.data() as *mut i16
        var i : size_t = 0
        while(i < total) {
            sptr[i] = audio.sample_data.at(src_offset + i)
            i += 1
        }

        result.loaded_flag = true
        return result

        }
    }

func impl_audio_volume(audio : *mut Audio, factor : float) {
        unsafe {
        var sptr = audio.sample_data.data() as *mut i16
        var i : size_t = 0
        while(i < audio.sample_data.size()) {
            var sample = (audio.sample_data.at(i) as float * factor) as i32
            if(sample > 32767) { sample = 32767 }
            if(sample < -32768) { sample = -32768 }
            sptr[i] = sample as i16
            i += 1
        }

        }
    }

func impl_audio_mix(a : *mut Audio, b : *mut Audio) : std::Result<Audio, AudioError> {
        unsafe {
        if(a.sample_rate_hz != b.sample_rate_hz) {
            return std.Result.Err(AudioError.InvalidFormat(string("sample rate mismatch")))
        }
        if(a.channel_count != b.channel_count) {
            return std.Result.Err(AudioError.InvalidFormat(string("channel count mismatch")))
        }

        var max_samples : size_t
        if(a.frame_total > b.frame_total) {
            max_samples = a.frame_total
        } else {
            max_samples = b.frame_total
        }
        var ch = a.channel_count as size_t

        var result = Audio.make()
        result.sample_rate_hz = a.sample_rate_hz
        result.channel_count = a.channel_count
        result.bit_depth = a.bit_depth
        result.frame_total = max_samples
        result.sample_data.resize(max_samples * ch)

        var rptr = result.sample_data.data() as *mut i16
        var i : size_t = 0
        while(i < max_samples * ch) {
            var sa : i32 = 0
            var sb : i32 = 0
            if(i < a.sample_data.size()) { sa = a.sample_data.at(i) as i32 }
            if(i < b.sample_data.size()) { sb = b.sample_data.at(i) as i32 }
            var mixed = (sa + sb) / 2
            if(mixed > 32767) { mixed = 32767 }
            if(mixed < -32768) { mixed = -32768 }
            rptr[i] = mixed as i16
            i += 1
        }

        result.loaded_flag = true
        return std.Result.Ok(result)

        }
    }

func impl_audio_append(a : *mut Audio, b : *mut Audio) : std::Result<Audio, AudioError> {
        unsafe {
        if(a.sample_rate_hz != b.sample_rate_hz) {
            return std.Result.Err(AudioError.InvalidFormat(string("sample rate mismatch")))
        }
        if(a.channel_count != b.channel_count) {
            return std.Result.Err(AudioError.InvalidFormat(string("channel count mismatch")))
        }

        var result = Audio.make()
        result.sample_rate_hz = a.sample_rate_hz
        result.channel_count = a.channel_count
        result.bit_depth = a.bit_depth
        result.frame_total = a.frame_total + b.frame_total

        var total_a = a.sample_data.size()
        var total_b = b.sample_data.size()
        result.sample_data.resize(total_a + total_b)

        var rptr = result.sample_data.data() as *mut i16
        var i : size_t = 0
        while(i < total_a) {
            rptr[i] = a.sample_data.at(i)
            i += 1
        }
        i = 0
        while(i < total_b) {
            rptr[total_a + i] = b.sample_data.at(i)
            i += 1
        }

        result.loaded_flag = true
        return std.Result.Ok(result)

        }
    }

func impl_audio_resample(audio : *mut Audio, target_rate : u32) : Audio {
        unsafe {
        if(audio.channel_count == 0) {
            return impl_audio_copy(audio)
        }

        var ch = audio.channel_count as size_t
        var available_frames = audio.sample_data.size() / ch
        var source_frames = audio.frame_total
        if(source_frames > available_frames) { source_frames = available_frames }

        // Empty source, zero/absent source rate, or an invalid target rate is
        // an empty result with a target rate when one was supplied. This keeps
        // the operation total instead of dividing by zero or reading
        // `frame_total - 1` on an empty buffer.
        if(source_frames == 0 || audio.sample_rate_hz == 0 || target_rate == 0) {
            var empty = Audio.make()
            empty.sample_rate_hz = target_rate
            if(target_rate == 0) { empty.sample_rate_hz = audio.sample_rate_hz }
            empty.channel_count = audio.channel_count
            empty.bit_depth = audio.bit_depth
            empty.frame_total = 0
            empty.loaded_flag = audio.loaded_flag
            return empty
        }

        if(audio.sample_rate_hz == target_rate) {
            return impl_audio_copy(audio)
        }

        var ratio = (target_rate as double) / (audio.sample_rate_hz as double)
        var new_num_samples = (source_frames as double * ratio) as size_t
        const max_size = (0 as size_t) - 1
        if(ch != 0 && new_num_samples > max_size / ch) {
            new_num_samples = max_size / ch
        }

        var result = Audio.make()
        result.sample_rate_hz = target_rate
        result.channel_count = audio.channel_count
        result.bit_depth = audio.bit_depth
        result.frame_total = new_num_samples
        result.sample_data.resize(new_num_samples * ch)

        var rptr = result.sample_data.data() as *mut i16
        var i : size_t = 0
        while(i < new_num_samples) {
            var src_pos = (i as double / ratio) as size_t
            if(src_pos >= source_frames) { src_pos = source_frames - 1 }
            var j : size_t = 0
            while(j < ch) {
                rptr[i * ch + j] = audio.sample_data.at(src_pos * ch + j)
                j += 1
            }
            i += 1
        }

        result.loaded_flag = true
        return result

        }
    }

func impl_audio_copy(audio : *mut Audio) : Audio {
        unsafe {
        var result = Audio.make()
        result.sample_rate_hz = audio.sample_rate_hz
        result.channel_count = audio.channel_count
        result.bit_depth = audio.bit_depth
        result.frame_total = audio.frame_total
        result.loaded_flag = audio.loaded_flag
        result.sample_data.resize(audio.sample_data.size())
        var rptr = result.sample_data.data() as *mut i16
        var i : size_t = 0
        while(i < audio.sample_data.size()) {
            rptr[i] = audio.sample_data.at(i)
            i += 1
        }
        return result

        }
    }

// Raw byte/pointer entry points. Safe code uses `Audio.load`/`Audio.from_*`
// and `Audio.save`; these take C strings and `*mut Audio` handles.
public namespace raw {
public func load_wav(path : *char) : std::Result<Audio, AudioError> {
    return impl_load_wav(path)
}

public func parse_wav(data : *u8, data_len : size_t) : std::Result<Audio, AudioError> {
    return impl_parse_wav(data, data_len)
}

public func encode_wav(audio : *mut Audio) : std::Result<vector<u8>, AudioError> {
    return impl_encode_wav(audio)
}

public func save_wav(audio : *mut Audio, path : *char) : std::Result<std::Unit, AudioError> {
    return impl_save_wav(audio, path)
}

public func audio_duration_ms(audio : *mut Audio) : double {
    return impl_audio_duration_ms(audio)
}

public func audio_trim(audio : *mut Audio, start_ms : double, end_ms : double) : Audio {
    return impl_audio_trim(audio, start_ms, end_ms)
}

public func audio_volume(audio : *mut Audio, factor : float) {
    impl_audio_volume(audio, factor)
}

public func audio_mix(a : *mut Audio, b : *mut Audio) : std::Result<Audio, AudioError> {
    return impl_audio_mix(a, b)
}

public func audio_append(a : *mut Audio, b : *mut Audio) : std::Result<Audio, AudioError> {
    return impl_audio_append(a, b)
}

public func audio_resample(audio : *mut Audio, target_rate : u32) : Audio {
    return impl_audio_resample(audio, target_rate)
}

public func audio_copy(audio : *mut Audio) : Audio {
    return impl_audio_copy(audio)
}
} // end namespace raw

} // end namespace audio
