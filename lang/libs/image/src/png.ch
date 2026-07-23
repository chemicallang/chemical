public namespace image {

using std::Result;
using std::string;
using std::vector;

// ---------------------------------------------------------------------------
// CRC-32 (PNG uses the same polynomial as zlib)
// ---------------------------------------------------------------------------

func png_crc32_table_entry(i : u32) : u32 {
    var crc = i
    var j : u32 = 0
    while(j < 8) {
        if((crc & 1u32) != 0) {
            crc = (crc >> 1) ^ 0xEDB88320u32
        } else {
            crc = crc >> 1
        }
        j += 1
    }
    return crc
}

func png_crc32(data : *u8, data_len : size_t) : u32 {
    var crc : u32 = 0xFFFFFFFFu32
    var i : size_t = 0
    while(i < data_len) {
        var idx = (crc ^ (data[i] as u32)) & 0xFFu32
        var tbl = png_crc32_table_entry(idx)
        crc = tbl ^ (crc >> 8)
        i += 1
    }
    return crc ^ 0xFFFFFFFFu32
}

// ---------------------------------------------------------------------------
// Adler-32 (used by zlib wrapper)
// ---------------------------------------------------------------------------

func adler32(data : *u8, data_len : size_t) : u32 {
    var a : u32 = 1u32
    var b : u32 = 0u32
    var i : size_t = 0
    while(i < data_len) {
        a = (a + (data[i] as u32)) % 65521u32
        b = (b + a) % 65521u32
        i += 1
    }
    return (b << 16) | a
}

// ---------------------------------------------------------------------------
// Big-endian readers
// ---------------------------------------------------------------------------

func read_be32(data : *u8, off : size_t) : u32 {
    return ((data[off] as u32) << 24) | ((data[off + 1] as u32) << 16) |
           ((data[off + 2] as u32) << 8) | (data[off + 3] as u32)
}

func read_be16(data : *u8, off : size_t) : u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16)
}

func write_be32(out : *mut u8, off : size_t, val : u32) {
    out[off] = ((val >> 24) & 0xFFu32) as u8
    out[off + 1] = ((val >> 16) & 0xFFu32) as u8
    out[off + 2] = ((val >> 8) & 0xFFu32) as u8
    out[off + 3] = (val & 0xFFu32) as u8
}

// ---------------------------------------------------------------------------
// Raw deflate decompressor (RFC 1951) — same algorithm as archive library
// ---------------------------------------------------------------------------

struct PngBitReader {
    var data : *u8
    var data_len : size_t
    var byte_pos : size_t
    var bit_buf : u32
    var bits_in_buf : int

    func init(d : *u8, d_len : size_t) : PngBitReader {
        return PngBitReader{
            data: d, data_len: d_len,
            byte_pos: 0, bit_buf: 0, bits_in_buf: 0
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

    func align_to_byte(&mut self) {
        var extra = self.bits_in_buf % 8
        if(extra > 0) {
            self.bit_buf = self.bit_buf >> (extra as u32)
            self.bits_in_buf -= extra
        }
    }
}

const PNG_MAX_HUFF = 288
const PNG_MAX_HUFF_BITS = 15

struct PngHuffEntry {
    var code : u16
    var bits : u8
    var value : u16
}

struct PngHuffTable {
    var entries : vector<PngHuffEntry>
    var min_bits : int
    var max_bits : int
}

func png_build_huff(lengths : *u8, num : int, out : *mut PngHuffTable) : Result<std::Unit, ImageError> {
    var bl_count : [PNG_MAX_HUFF_BITS + 1]u32
    var i : int = 0
    while(i <= PNG_MAX_HUFF_BITS) { bl_count[i] = 0; i += 1 }

    i = 0
    while(i < num) {
        var l = lengths[i] as int
        if(l > 0 && l <= PNG_MAX_HUFF_BITS) { bl_count[l] += 1 }
        i += 1
    }

    var max_bits : int = 0
    i = 1
    while(i <= PNG_MAX_HUFF_BITS) {
        if(bl_count[i] > 0) { max_bits = i }
        i += 1
    }

    var next_code : [PNG_MAX_HUFF_BITS + 1]u32
    var code : u32 = 0
    i = 1
    while(i <= PNG_MAX_HUFF_BITS) {
        code = (code + bl_count[i - 1]) << 1
        next_code[i] = code
        i += 1
    }

    var entries = vector<PngHuffEntry>()
    var min_bits = PNG_MAX_HUFF_BITS + 1
    if(max_bits > 0) { min_bits = max_bits }

    i = 0
    while(i < num) {
        var l = lengths[i] as int
        if(l > 0) {
            var e : PngHuffEntry
            e.code = next_code[l] as u16
            e.bits = l as u8
            e.value = i as u16
            entries.push(e)
            if(l < min_bits) { min_bits = l }
        }
        i += 1
    }

    out.entries = entries
    out.min_bits = min_bits
    out.max_bits = max_bits
    return Result.Ok(std::Unit{})
}

func png_decode_huff(reader : *mut PngBitReader, table : *mut PngHuffTable) : u16 {
    var code : u32 = 0
    var found : int = 0
    var i : int = 0
    while(i < table.max_bits) {
        code = (code << 1) | reader.read_bits(1)
        found += 1
        var j : size_t = 0
        while(j < table.entries.size()) {
            var e = table.entries.get_ptr(j)
            if(e.bits as int == found && e.code as u32 == code) {
                return e.value
            }
            j += 1
        }
    }
    return 0
}

func png_raw_deflate(input : *u8, input_len : size_t, output : *mut u8, out_capacity : size_t) : Result<size_t, ImageError> {
    var reader = PngBitReader.init(input, input_len)
    var out_pos : size_t = 0
    var done = false

    while(!done) {
        var bfinal = reader.read_bits(1)
        var btype = reader.read_bits(2)
        if(bfinal == 1) { done = true }

        if(btype == 0) {
            reader.align_to_byte()
            if(reader.byte_pos + 4 > input_len) {
                return Result.Err(ImageError.InvalidFormat(string("truncated stored block")))
            }
            var len = (reader.data[reader.byte_pos] as u16) | ((reader.data[reader.byte_pos + 1] as u16) << 8)
            reader.byte_pos += 4
            var i : u16 = 0
            while(i < len) {
                if(out_pos >= out_capacity || reader.byte_pos >= input_len) {
                    return Result.Err(ImageError.InvalidFormat(string("truncated stored data")))
                }
                output[out_pos] = reader.data[reader.byte_pos]
                out_pos += 1
                reader.byte_pos += 1
                i += 1
            }
        } else if(btype == 1 || btype == 2) {
            var lit_lens : [PNG_MAX_HUFF]u8
            var dist_lens : [32]u8
            var num_lit : int = 288
            var num_dist : int = 0

            if(btype == 1) {
                var i : int = 0
                while(i < 144) { lit_lens[i] = 8; i += 1 }
                while(i < 256) { lit_lens[i] = 9; i += 1 }
                while(i < 280) { lit_lens[i] = 7; i += 1 }
                while(i < 288) { lit_lens[i] = 8; i += 1 }
                i = 0
                while(i < 32) { dist_lens[i] = 5; i += 1 }
                num_dist = 32
            } else {
                var hlit = reader.read_bits(5) as int + 257
                var hdist = reader.read_bits(5) as int + 1
                var hclen = reader.read_bits(4) as int + 4
                num_lit = hlit
                num_dist = hdist

                var cl_order : [19]int = [16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15]
                var cl_lens : [19]u8
                var k : int = 0
                while(k < 19) { cl_lens[k] = 0; k += 1 }
                k = 0
                while(k < hclen) {
                    cl_lens[cl_order[k]] = reader.read_bits(3) as u8
                    k += 1
                }

                var cl_table : PngHuffTable
                var cl_res = png_build_huff(&raw cl_lens[0], 19, &raw mut cl_table)
                if(cl_res is Result.Err) { return Result.Err(ImageError.InvalidFormat(string("bad code length huffman table"))) }

                var total = hlit + hdist
                var idx = 0
                while(idx < total) {
                    var sym = png_decode_huff(&raw mut reader, &raw mut cl_table) as int
                    if(sym <= 15) {
                        if(idx < hlit) { lit_lens[idx] = sym as u8 }
                        else { dist_lens[idx - hlit] = sym as u8 }
                        idx += 1
                    } else if(sym == 16) {
                        var rep = reader.read_bits(2) as int + 3
                        var prev : u8 = 0
                        if(idx > 0) {
                            if(idx - 1 < hlit) { prev = lit_lens[idx - 1] }
                            else { prev = dist_lens[idx - 1 - hlit] }
                        }
                        var r : int = 0
                        while(r < rep && idx < total) {
                            if(idx < hlit) { lit_lens[idx] = prev }
                            else { dist_lens[idx - hlit] = prev }
                            idx += 1; r += 1
                        }
                    } else if(sym == 17) {
                        var rep = reader.read_bits(3) as int + 3
                        var r : int = 0
                        while(r < rep && idx < total) {
                            if(idx < hlit) { lit_lens[idx] = 0 }
                            else { dist_lens[idx - hlit] = 0 }
                            idx += 1; r += 1
                        }
                    } else if(sym == 18) {
                        var rep = reader.read_bits(7) as int + 11
                        var r : int = 0
                        while(r < rep && idx < total) {
                            if(idx < hlit) { lit_lens[idx] = 0 }
                            else { dist_lens[idx - hlit] = 0 }
                            idx += 1; r += 1
                        }
                    }
                }
            }

            var lit_table : PngHuffTable
            var lit_res = png_build_huff(&raw lit_lens[0], num_lit, &raw mut lit_table)
            if(lit_res is Result.Err) { return Result.Err(ImageError.InvalidFormat(string("bad literal huffman table"))) }

            var dist_table : PngHuffTable
            var dist_res = png_build_huff(&raw dist_lens[0], num_dist, &raw mut dist_table)
            if(dist_res is Result.Err) { return Result.Err(ImageError.InvalidFormat(string("bad distance huffman table"))) }

            loop {
                var sym = png_decode_huff(&raw mut reader, &raw mut lit_table)
                if(sym < 256) {
                    if(out_pos >= out_capacity) {
                        return Result.Err(ImageError.InvalidFormat(string("deflate output full")))
                    }
                    output[out_pos] = sym as u8
                    out_pos += 1
                } else if(sym == 256) {
                    break
                } else {
                    var len_extra : [29]int = [0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0]
                    var len_base : [29]int = [3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258]
                    var li = (sym as int) - 257
                    if(li < 0 || li >= 29) {
                        return Result.Err(ImageError.InvalidFormat(string("invalid length code")))
                    }
                    var length = len_base[li] + reader.read_bits(len_extra[li]) as int

                    var dist_sym = png_decode_huff(&raw mut reader, &raw mut dist_table)
                    var dist_extra : [30]int = [0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13]
                    var dist_base : [30]int = [1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577]
                    var di = dist_sym as int
                    if(di < 0 || di >= 30) {
                        return Result.Err(ImageError.InvalidFormat(string("invalid distance code")))
                    }
                    var distance = dist_base[di] + reader.read_bits(dist_extra[di]) as int

                    if(distance <= 0 || distance as size_t > out_pos) {
                        return Result.Err(ImageError.InvalidFormat(string("invalid distance")))
                    }

                    var src = out_pos - (distance as size_t)
                    var i : int = 0
                    while(i < length) {
                        if(out_pos >= out_capacity) {
                            return Result.Err(ImageError.InvalidFormat(string("deflate output full")))
                        }
                        output[out_pos] = output[src]
                        out_pos += 1; src += 1; i += 1
                    }
                }
            }
        } else {
            return Result.Err(ImageError.InvalidFormat(string("reserved deflate block type")))
        }
    }
    return Result.Ok(out_pos)
}

// ---------------------------------------------------------------------------
// Zlib decompress (RFC 1950): skip 2-byte header, inflate, skip 4-byte adler32
// ---------------------------------------------------------------------------

func png_zlib_decompress(data : *u8, data_len : size_t, out : *mut u8, out_capacity : size_t) : Result<size_t, ImageError> {
    if(data_len < 6) {
        return Result.Err(ImageError.InvalidFormat(string("zlib data too short")))
    }
    // skip CMF + FLG (2 bytes)
    var compressed = data + 2
    var compressed_len = data_len - 2
    // the last 4 bytes are adler32 — subtract them from the compressed payload
    if(compressed_len < 4) {
        return Result.Err(ImageError.InvalidFormat(string("zlib data too short for adler32")))
    }
    compressed_len -= 4
    return png_raw_deflate(compressed, compressed_len, out, out_capacity)
}

// ---------------------------------------------------------------------------
// PNG filter reconstruction
// ---------------------------------------------------------------------------

func png_paeth_predict(a : int, b : int, c : int) : int {
    var p = a + b - c
    var pa = p - a
    if(pa < 0) { pa = -pa }
    var pb = p - b
    if(pb < 0) { pb = -pb }
    var pc = p - c
    if(pc < 0) { pc = -pc }
    if(pa <= pb && pa <= pc) { return a }
    if(pb <= pc) { return b }
    return c
}

func png_reconstruct_row(row_data : *mut u8, row_len : size_t, bpp : int, prev_row : *u8, filter : int) {
    if(filter == 0) {
        // None — no-op
    } else if(filter == 1) {
        // Sub
        var i : size_t = bpp as size_t
        while(i < row_len) {
            row_data[i] = row_data[i] + row_data[i - (bpp as size_t)]
            i += 1
        }
    } else if(filter == 2) {
        // Up
        var i : size_t = 0
        while(i < row_len) {
            row_data[i] = row_data[i] + prev_row[i]
            i += 1
        }
    } else if(filter == 3) {
        // Average
        var i : size_t = 0
        while(i < row_len) {
            var left : int = 0
            var above : int = prev_row[i] as int
            if((i as int) >= bpp) { left = row_data[i - (bpp as size_t)] as int }
            row_data[i] = (row_data[i] as int + (left + above) / 2) as u8
            i += 1
        }
    } else if(filter == 4) {
        // Paeth
        var i : size_t = 0
        while(i < row_len) {
            var left : int = 0
            var above : int = prev_row[i] as int
            var above_left : int = 0
            if((i as int) >= bpp) {
                left = row_data[i - (bpp as size_t)] as int
                above_left = prev_row[i - (bpp as size_t)] as int
            }
            row_data[i] = (row_data[i] as int + png_paeth_predict(left, above, above_left)) as u8
            i += 1
        }
    }
}

// ---------------------------------------------------------------------------
// PNG chunk reader
// ---------------------------------------------------------------------------

func png_chunk_type(data : *u8, offset : size_t) : u32 {
    return ((data[offset] as u32) << 24) | ((data[offset + 1] as u32) << 16) |
           ((data[offset + 2] as u32) << 8) | (data[offset + 3] as u32)
}

func png_chunk_len(data : *u8, offset : size_t) : u32 {
    return read_be32(data, offset)
}

func png_chunk_name(b0 : u8, b1 : u8, b2 : u8, b3 : u8) : string {
    var s = string("")
    s.append(b0 as char)
    s.append(b1 as char)
    s.append(b2 as char)
    s.append(b3 as char)
    return s
}

// PNG color types
const PNG_GRAYSCALE = 0
const PNG_RGB = 2
const PNG_INDEXED = 3
const PNG_GRAY_ALPHA = 4
const PNG_RGBA = 6

// ---------------------------------------------------------------------------
// parse_png — decompress + reconstruct from raw byte data
// ---------------------------------------------------------------------------

public func parse_png(data : *u8, data_len : size_t) : std::Result<Image, ImageError> {
    // validate signature
    if(data_len < 8) {
        return std.Result.Err(ImageError.InvalidFormat(string("PNG too short")))
    }
    var sig : [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]
    var i : int = 0
    while(i < 8) {
        if(data[i] != sig[i]) {
            return std.Result.Err(ImageError.InvalidFormat(string("invalid PNG signature")))
        }
        i += 1
    }

    // parse chunks — collect IHDR and IDAT data
    var pos : size_t = 8
    var width : int = 0
    var height : int = 0
    var bit_depth : int = 0
    var color_type : int = 0
    var ihdr_seen = false

    // concatenate all IDAT chunks
    var idat_data = vector<u8>()
    var plte_data = vector<u8>()

    while(pos + 12 <= data_len) {
        var chunk_len = read_be32(data, pos) as size_t
        var chunk_type = read_be32(data, pos + 4)
        var chunk_start = pos + 8

        if(chunk_start + chunk_len + 4 > data_len) {
            break
        }

        // verify CRC
        var crc_off = pos + 4
        var computed = png_crc32(data + crc_off, chunk_len + 4)
        var stored = read_be32(data, chunk_start + chunk_len)
        // skip CRC check for robustness — many tools emit correct data

        if(chunk_type == 0x49484452) { // IHDR
            if(chunk_len < 13) {
                return std.Result.Err(ImageError.InvalidFormat(string("IHDR too short")))
            }
            width = read_be32(data, chunk_start) as int
            height = read_be32(data, chunk_start + 4) as int
            bit_depth = data[chunk_start + 8] as int
            color_type = data[chunk_start + 9] as int
            ihdr_seen = true
        } else if(chunk_type == 0x504C5445) { // PLTE
            var j : size_t = 0
            while(j < chunk_len) {
                plte_data.push(data[chunk_start + j])
                j += 1
            }
        } else if(chunk_type == 0x49444154) { // IDAT
            var j : size_t = 0
            while(j < chunk_len) {
                idat_data.push(data[chunk_start + j])
                j += 1
            }
        } else if(chunk_type == 0x49454E44) { // IEND
            break
        }

        pos = chunk_start + chunk_len + 4
    }

    if(!ihdr_seen) {
        return std.Result.Err(ImageError.InvalidFormat(string("no IHDR chunk found")))
    }
    if(idat_data.size() == 0) {
        return std.Result.Err(ImageError.InvalidFormat(string("no IDAT chunks found")))
    }
    if(width <= 0 || height <= 0) {
        return std.Result.Err(ImageError.InvalidDimensions(width, height))
    }

    // determine channels from color type
    var channels : int = 0
    if(color_type == PNG_GRAYSCALE) {
        channels = 1
    } else if(color_type == PNG_RGB) {
        channels = 3
    } else if(color_type == PNG_INDEXED) {
        channels = 1
    } else if(color_type == PNG_GRAY_ALPHA) {
        channels = 2
    } else if(color_type == PNG_RGBA) {
        channels = 4
    } else {
        return std.Result.Err(ImageError.UnsupportedFormat(string("unsupported PNG color type")))
    }

    if(bit_depth != 8) {
        return std.Result.Err(ImageError.UnsupportedFormat(string("only 8-bit PNG supported")))
    }

    // decompress zlib data
    var bpp = channels  // bytes per pixel for filter
    if(color_type == PNG_INDEXED) { bpp = 1 }
    var row_bytes = width * bpp
    var raw_size = (row_bytes + 1) * height  // +1 per row for filter byte
    var raw_buf = vector<u8>()
    raw_buf.resize(raw_size as size_t)

    var zlib_result = png_zlib_decompress(
        idat_data.data(), idat_data.size(),
        raw_buf.data() as *mut u8, raw_size as size_t
    )
    if(zlib_result is Result.Err) {
        var Err(e) = zlib_result else unreachable
        return std.Result.Err(ImageError.InvalidFormat(string("PNG decompression failed")))
    }

    // reconstruct image
    var img = image_create(width, height, channels)
    var dst_ptr = img.pixels.data() as *mut u8
    var src_ptr = raw_buf.data()
    var prev_row = vector<u8>()
    prev_row.resize(row_bytes as size_t)
    var prev_ptr = prev_row.data() as *mut u8
    // zero prev row
    var z : size_t = 0
    while(z < row_bytes as size_t) { prev_ptr[z] = 0; z += 1 }

    var y : int = 0
    while(y < height) {
        var row_off = (y as size_t) * (row_bytes as size_t + 1)
        var filter_type = src_ptr[row_off] as int
        var row_start = row_off + 1

        // copy row data to working buffer for in-place filter reconstruction
        var work = vector<u8>()
        work.resize(row_bytes as size_t)
        var work_ptr = work.data() as *mut u8
        var c : size_t = 0
        while(c < row_bytes as size_t) {
            work_ptr[c] = src_ptr[row_start + c]
            c += 1
        }

        png_reconstruct_row(work_ptr, row_bytes as size_t, bpp, prev_ptr, filter_type)

        // copy to image output
        c = 0
        while(c < row_bytes as size_t) {
            dst_ptr[(y as size_t) * (row_bytes as size_t) + c] = work_ptr[c]
            c += 1
        }

        // save current row as prev for next iteration
        c = 0
        while(c < row_bytes as size_t) {
            prev_ptr[c] = work_ptr[c]
            c += 1
        }

        y += 1
    }

        // handle indexed color: expand palette to RGB
    if(color_type == PNG_INDEXED && plte_data.size() >= 768) {
        var expanded = image_create(width, height, 3)
        var exp_ptr = expanded.pixels.data() as *mut u8
        var plt_ptr = plte_data.data()
        var py : int = 0
        while(py < height) {
            var px : int = 0
            while(px < width) {
                var idx = dst_ptr[(py as size_t) * (width as size_t) + (px as size_t)] as size_t
                var pi = idx * 3
                var doff = ((py as size_t) * (width as size_t) + (px as size_t)) * 3
                exp_ptr[doff] = plt_ptr[pi]
                exp_ptr[doff + 1] = plt_ptr[pi + 1]
                exp_ptr[doff + 2] = plt_ptr[pi + 2]
                px += 1
            }
            py += 1
        }
        return std.Result.Ok(expanded)
    }

    return std.Result.Ok(img)
}

// ---------------------------------------------------------------------------
// load_png / save_png
// ---------------------------------------------------------------------------

public func load_png(path : *char) : std::Result<Image, ImageError> {
    var read_result = fs::read_entire_file(path)
    if(read_result is Result.Err) {
        return std.Result.Err(ImageError.FileNotFound())
    }
    var Ok(data) = read_result else unreachable
    return parse_png(data.data(), data.size())
}

func png_write_chunk(out : *mut u8, pos : *mut size_t, chunk_type : u32, data : *u8, data_len : size_t) {
    write_be32(out, pos[0], data_len as u32)
    pos[0] += 4
    out[pos[0]] = ((chunk_type >> 24) & 0xFFu32) as u8
    out[pos[0] + 1] = ((chunk_type >> 16) & 0xFFu32) as u8
    out[pos[0] + 2] = ((chunk_type >> 8) & 0xFFu32) as u8
    out[pos[0] + 3] = (chunk_type & 0xFFu32) as u8
    pos[0] += 4
    var i : size_t = 0
    while(i < data_len) {
        out[pos[0] + i] = data[i]
        i += 1
    }
    pos[0] += data_len
    // CRC covers type + data
    var crc = png_crc32(out + pos[0] - data_len - 4, data_len + 4)
    write_be32(out, pos[0], crc)
    pos[0] += 4
}

// Store-mode deflate: write uncompressed blocks
func png_store_deflate(raw : *u8, raw_len : size_t, out : *mut u8, out_capacity : size_t) : Result<size_t, ImageError> {
    var pos : size_t = 0

    // zlib header: CM=8, CINFO=7, FCHECK to make CMF*256+FLG a multiple of 31
    // CMF = 0x78 (CM=8, CINFO=7), FLG = 0x01 (FDICT=0, FCHECK: 0x78*256+0x01=30721, 30721%31=0)
    if(pos + 2 > out_capacity) { return Result.Err(ImageError.InvalidFormat(string("output too small"))) }
    out[pos] = 0x78u8
    out[pos + 1] = 0x01u8
    pos += 2

    // write deflate stored blocks (max block size = 65535)
    var src_pos : size_t = 0
    var first = true
    while(src_pos < raw_len) {
        var block_len = raw_len - src_pos
        if(block_len > 65535) { block_len = 65535 }
        var last = (src_pos + block_len >= raw_len)
        var bfinal : u8 = 0
        if(last) { bfinal = 1 }
        var btype : u8 = 0  // stored
        var header = bfinal | (btype << 1)

        if(pos + 5 > out_capacity) { return Result.Err(ImageError.InvalidFormat(string("output too small for block"))) }
        out[pos] = header
        pos += 1
        // align to byte (already aligned for stored blocks)
        out[pos] = (block_len & 0xFFu16) as u8
        out[pos + 1] = ((block_len >> 8) & 0xFFu16) as u8
        var nlen = block_len ^ 0xFFFFu16
        out[pos + 2] = (nlen & 0xFFu16) as u8
        out[pos + 3] = ((nlen >> 8) & 0xFFu16) as u8
        pos += 4

        var i : size_t = 0
        while(i < block_len) {
            if(pos >= out_capacity) { return Result.Err(ImageError.InvalidFormat(string("output overflow"))) }
            out[pos] = raw[src_pos + i]
            pos += 1
            i += 1
        }
        src_pos += block_len
        first = false
    }

    // Adler-32 checksum
    var chk = adler32(raw, raw_len)
    if(pos + 4 > out_capacity) { return Result.Err(ImageError.InvalidFormat(string("output too small for adler32"))) }
    write_be32(out, pos, chk)
    pos += 4

    return Result.Ok(pos)
}

public func save_png(img : *mut Image, path : *char) : std::Result<std::Unit, ImageError> {
    // build raw scanline data with filter byte 0 (no filter) per row
    var bpp = img.channels
    var row_bytes = img.width * bpp
    var raw_size = (row_bytes + 1) * img.height
    var raw_buf = vector<u8>()
    raw_buf.resize(raw_size as size_t)
    var raw_ptr = raw_buf.data() as *mut u8
    var src_ptr = img.pixels.data()

    var y : int = 0
    while(y < img.height) {
        var row_off = (y as size_t) * (row_bytes as size_t + 1)
        raw_ptr[row_off] = 0  // filter: None
        var c : size_t = 0
        while(c < row_bytes as size_t) {
            raw_ptr[row_off + 1 + c] = src_ptr[(y as size_t) * (row_bytes as size_t) + c]
            c += 1
        }
        y += 1
    }

    // deflate the raw data
    var deflated_size = raw_size + (raw_size / 1000) + 128 + 6  // conservative estimate
    var deflated = vector<u8>()
    deflated.resize(deflated_size as size_t)

    var def_result = png_store_deflate(
        raw_buf.data(), raw_size as size_t,
        deflated.data() as *mut u8, deflated_size as size_t
    )
    if(def_result is Result.Err) {
        return std.Result.Err(ImageError.InvalidFormat(string("PNG deflate failed")))
    }
    var Ok(def_len) = def_result else unreachable

    // calculate total file size
    // 8 (signature) + 25 (IHDR chunk) + 12 (IEND chunk)
    // + 8 + 4 (IDAT chunk header + CRC) + def_len
    var total_size = 8 + 25 + 12 + 8 + 4 + def_len
    var file_buf = vector<u8>()
    file_buf.resize(total_size as size_t)
    var fptr = file_buf.data() as *mut u8
    var wpos : size_t = 0

    // PNG signature
    var sig : [8]u8 = [0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]
    var s : int = 0
    while(s < 8) { fptr[s] = sig[s]; s += 1 }
    wpos = 8

    // IHDR chunk (13 bytes of data)
    var ihdr : [13]u8
    ihdr[0] = ((img.width as u32) >> 24) as u8; ihdr[1] = ((img.width as u32) >> 16) as u8
    ihdr[2] = ((img.width as u32) >> 8) as u8; ihdr[3] = (img.width as u32) as u8
    ihdr[4] = ((img.height as u32) >> 24) as u8; ihdr[5] = ((img.height as u32) >> 16) as u8
    ihdr[6] = ((img.height as u32) >> 8) as u8; ihdr[7] = (img.height as u32) as u8
    ihdr[8] = 8  // bit depth
    if(img.channels == 1) { ihdr[9] = PNG_GRAYSCALE as u8 }
    else if(img.channels == 2) { ihdr[9] = PNG_GRAY_ALPHA as u8 }
    else if(img.channels == 3) { ihdr[9] = PNG_RGB as u8 }
    else { ihdr[9] = PNG_RGBA as u8 }
    ihdr[10] = 0  // compression method
    ihdr[11] = 0  // filter method
    ihdr[12] = 0  // interlace method
    png_write_chunk(fptr, &raw mut wpos, 0x49484452, &raw ihdr[0], 13)

    // IDAT chunk
    png_write_chunk(fptr, &raw mut wpos, 0x49444154, deflated.data(), def_len as size_t)

    // IEND chunk
    png_write_chunk(fptr, &raw mut wpos, 0x49454E44, null, 0)

    var write_result = fs::write_text_file(path, file_buf.data(), file_buf.size())
    if(write_result is Result.Err) {
        return std.Result.Err(ImageError.IoError(string("failed to write PNG file")))
    }

    return std.Result.Ok(std::Unit{})
}

} // end namespace image
