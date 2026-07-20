public namespace archive {

using std::Result;
using std::string;
using std::vector;

struct BitReader {
    var data : *u8
    var data_len : size_t
    var byte_pos : size_t
    var bit_buf : u32
    var bits_in_buf : int

    func init(d : *u8, d_len : size_t) : BitReader {
        return BitReader {
            data : d,
            data_len : d_len,
            byte_pos : 0,
            bit_buf : 0,
            bits_in_buf : 0
        }
    }

    func read_bits(&mut self, n : int) : u32 {
        while(self.bits_in_buf < n) {
            if(self.byte_pos < self.data_len) {
                self.bit_buf = self.bit_buf | ((self.data[self.byte_pos] as u32) << (self.bits_in_buf as u32))
                self.byte_pos += 1
                self.bits_in_buf += 8
            } else {
                break
            }
        }
        var val = self.bit_buf & ((1u32 << (n as u32)) - 1u32)
        self.bit_buf = self.bit_buf >> (n as u32)
        self.bits_in_buf -= n
        return val
    }

    func read_bits_reversed(&mut self, n : int) : u32 {
        var val = self.read_bits(n)
        var result : u32 = 0
        var i : int = 0
        while(i < n) {
            result = (result << 1) | (val & 1u32)
            val = val >> 1
            i += 1
        }
        return result
    }

    func align_to_byte(&mut self) {
        var extra = self.bits_in_buf % 8
        if(extra > 0) {
            self.bit_buf = self.bit_buf >> (extra as u32)
            self.bits_in_buf -= extra
        }
    }
}

const MAX_HUFFMAN_CODES = 288
const MAX_HUFFMAN_BITS = 15

struct HuffEntry {
    var code : u16
    var bits : u8
    var value : u16
}

@direct_init
struct HuffTable {
    var entries : vector<HuffEntry>
    var min_bits : int
    var max_bits : int
}

func build_huffman_table(lengths : *u8, num_codes : int, output : *mut HuffTable) : std::Result<std::Unit, ArchiveError> {
    var bl_count : [MAX_HUFFMAN_BITS + 1]u32;
    var i : int = 0
    while(i <= MAX_HUFFMAN_BITS) {
        bl_count[i] = 0
        i += 1
    }

    i = 0
    while(i < num_codes) {
        var len = lengths[i] as int
        if(len > 0 && len <= MAX_HUFFMAN_BITS) {
            bl_count[len] += 1
        }
        i += 1
    }

    var max_bits : int = 0
    i = 1
    while(i <= MAX_HUFFMAN_BITS) {
        if(bl_count[i] > 0) { max_bits = i }
        i += 1
    }

    var next_code : [MAX_HUFFMAN_BITS + 1]u32
    var code : u32 = 0
    i = 1
    while(i <= MAX_HUFFMAN_BITS) {
        code = (code + bl_count[i - 1]) << 1
        next_code[i] = code
        i += 1
    }

    var entries = vector<HuffEntry>()
    var min_bits = MAX_HUFFMAN_BITS + 1
    if(max_bits > 0) { min_bits = max_bits }

    i = 0
    while(i < num_codes) {
        var len = lengths[i] as int
        if(len > 0) {
            var entry : HuffEntry
            entry.code = next_code[len] as u16
            entry.bits = len as u8
            entry.value = i as u16
            entries.push(entry)
            if(len < min_bits) { min_bits = len }
        }
        i += 1
    }

    output.entries = entries
    output.min_bits = min_bits
    output.max_bits = max_bits
    return std.Result.Ok(std::Unit{})
}

func decode_huffman(reader : *mut BitReader, table : *mut HuffTable) : u16 {
    var code : u32 = 0
    var found_bits : int = 0
    var i : int = 0
    while(i < table.max_bits) {
        code = (code << 1) | reader.read_bits(1)
        found_bits += 1
        var j : size_t = 0
        while(j < table.entries.size()) {
            var e = table.entries.get_ptr(j)
            if(e.bits as int == found_bits && e.code as u32 == code) {
                return e.value
            }
            j += 1
        }
    }
    return 0
}

public func deflate_decompress(input : *u8, input_len : size_t, output : *mut u8, output_capacity : size_t) : std::Result<size_t, ArchiveError> {
    var reader = BitReader.init(input, input_len)
    var out_pos : size_t = 0
    var last_block = false

    while(!last_block) {
        var bfinal = reader.read_bits(1)
        var btype = reader.read_bits(2)

        if(bfinal == 1) { last_block = true }

        if(btype == 0) {
            reader.align_to_byte()
            if(reader.byte_pos + 4 > input_len) {
                return std.Result.Err(ArchiveError.DecompressionFailed(string("truncated stored block header")))
            }
            var len = (reader.data[reader.byte_pos] as u16) | ((reader.data[reader.byte_pos + 1] as u16) << 8)
            reader.byte_pos += 4
            var i : u16 = 0
            while(i < len) {
                if(out_pos >= output_capacity || reader.byte_pos >= input_len) {
                    return std.Result.Err(ArchiveError.DecompressionFailed(string("truncated stored block data")))
                }
                output[out_pos] = reader.data[reader.byte_pos]
                out_pos += 1
                reader.byte_pos += 1
                i += 1
            }
        } else if(btype == 1 || btype == 2) {
            var lit_lens : [MAX_HUFFMAN_CODES]u8
            var dist_lens : [32]u8
            var num_lit_codes : int = 288
            var num_dist_codes : int = 0

            if(btype == 1) {
                var i : int = 0
                while(i < 144) { lit_lens[i] = 8; i += 1 }
                while(i < 256) { lit_lens[i] = 9; i += 1 }
                while(i < 280) { lit_lens[i] = 7; i += 1 }
                while(i < 288) { lit_lens[i] = 8; i += 1 }
                i = 0
                while(i < 32) { dist_lens[i] = 5; i += 1 }
                num_dist_codes = 32
            } else {
                var hlit = reader.read_bits(5) as int + 257
                var hdist = reader.read_bits(5) as int + 1
                var hclen = reader.read_bits(4) as int + 4

                num_lit_codes = hlit
                num_dist_codes = hdist

                var cl_order : [19]int = [16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15]
                var cl_lens : [19]u8
                var k : int = 0
                while(k < 19) { cl_lens[k] = 0; k += 1 }
                k = 0
                while(k < hclen) {
                    cl_lens[cl_order[k]] = reader.read_bits(3) as u8
                    k += 1
                }

                var cl_table : HuffTable
                var cl_result = build_huffman_table(&raw cl_lens[0], 19, &raw mut cl_table)
                if(cl_result is Result.Err) {
                    return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid code length Huffman table")))
                }

                var total = hlit + hdist
                var idx = 0
                while(idx < total) {
                    var sym = decode_huffman(&raw mut reader, &raw mut cl_table) as int
                    if(sym <= 15) {
                        if(idx < hlit) { lit_lens[idx] = sym as u8 }
                        else { dist_lens[idx - hlit] = sym as u8 }
                        idx += 1
                    } else if(sym == 16) {
                        var repeat = reader.read_bits(2) as int + 3
                        var prev_len : u8 = 0
                        if(idx > 0) {
                            if(idx - 1 < hlit) { prev_len = lit_lens[idx - 1] }
                            else { prev_len = dist_lens[idx - 1 - hlit] }
                        }
                        var r : int = 0
                        while(r < repeat && idx < total) {
                            if(idx < hlit) { lit_lens[idx] = prev_len }
                            else { dist_lens[idx - hlit] = prev_len }
                            idx += 1
                            r += 1
                        }
                    } else if(sym == 17) {
                        var repeat = reader.read_bits(3) as int + 3
                        var r : int = 0
                        while(r < repeat && idx < total) {
                            if(idx < hlit) { lit_lens[idx] = 0 }
                            else { dist_lens[idx - hlit] = 0 }
                            idx += 1
                            r += 1
                        }
                    } else if(sym == 18) {
                        var repeat = reader.read_bits(7) as int + 11
                        var r : int = 0
                        while(r < repeat && idx < total) {
                            if(idx < hlit) { lit_lens[idx] = 0 }
                            else { dist_lens[idx - hlit] = 0 }
                            idx += 1
                            r += 1
                        }
                    }
                }
            }

            var lit_table : HuffTable
            var lit_result = build_huffman_table(&raw lit_lens[0], num_lit_codes, &raw mut lit_table)
            if(lit_result is Result.Err) {
                return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid literal/length Huffman table")))
            }

            var dist_table : HuffTable
            var dist_result = build_huffman_table(&raw dist_lens[0], num_dist_codes, &raw mut dist_table)
            if(dist_result is Result.Err) {
                return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid distance Huffman table")))
            }

            loop {
                var sym = decode_huffman(&raw mut reader, &raw mut lit_table)
                if(sym < 256) {
                    if(out_pos >= output_capacity) {
                        return std.Result.Err(ArchiveError.DecompressionFailed(string("output buffer full")))
                    }
                    output[out_pos] = sym as u8
                    out_pos += 1
                } else if(sym == 256) {
                    break
                } else {
                    var len_extra_bits : [29]int = [0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0]
                    var len_base : [29]int = [3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258]
                    var len_idx = (sym as int) - 257
                    if(len_idx < 0 || len_idx >= 29) {
                        return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid length code")))
                    }
                    var length = len_base[len_idx] + reader.read_bits(len_extra_bits[len_idx]) as int

                    var dist_sym = decode_huffman(&raw mut reader, &raw mut dist_table)
                    var dist_extra_bits : [30]int = [0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13]
                    var dist_base : [30]int = [1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577]
                    var dist_idx = dist_sym as int
                    if(dist_idx < 0 || dist_idx >= 30) {
                        return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid distance code")))
                    }
                    var distance = dist_base[dist_idx] + reader.read_bits(dist_extra_bits[dist_idx]) as int

                    if(distance <= 0 || distance as size_t > out_pos) {
                        return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid distance")))
                    }

                    var src_pos = out_pos - (distance as size_t)
                    var i : int = 0
                    while(i < length) {
                        if(out_pos >= output_capacity) {
                            return std.Result.Err(ArchiveError.DecompressionFailed(string("output buffer full during backreference")))
                        }
                        output[out_pos] = output[src_pos]
                        out_pos += 1
                        src_pos += 1
                        i += 1
                    }
                }
            }
        } else {
            return std.Result.Err(ArchiveError.DecompressionFailed(string("reserved block type")))
        }
    }

    return std.Result.Ok(out_pos)
}

} // end namespace archive
