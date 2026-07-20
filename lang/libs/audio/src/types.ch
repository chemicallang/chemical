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
}

} // end namespace audio
