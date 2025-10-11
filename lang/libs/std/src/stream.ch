public interface Stream {

    func writeStr(&self, value : *char, length : ubigint);

    func writeStrNoLen(&self, value : *char);

    func writeI8(&self, value : i8);

    func writeI16(&self, value : i16);

    func writeI32(&self, value : i32);

    func writeI64(&self, value : i64);

    func writeU8(&self, value : u8);

    func writeU16(&self, value : u16);

    func writeU32(&self, value : u32);

    func writeU64(&self, value : u64);

    func writeChar(&self, value : char)

    func writeUChar(&self, value : uchar)

    func writeShort(&self, value : short)

    func writeUShort(&self, value : ushort)

    func writeInt(&self, value : int);

    func writeUInt(&self, value : uint);

    func writeLong(&self, value : long);

    func writeULong(&self, value : ulong);

    func writeLongLong(&self, value : longlong);

    func writeULongLong(&self, value : ulonglong);

    func writeFloat(&self, value : float);

    func writeDouble(&self, value : double);

}

public struct CommandLineStream : Stream {

    // --- helpers --------------------------------------------------------

    // convert unsigned 64-bit to ASCII in out_buf, return length written
    func u64_to_chars(&self, out_buf : *mut char, value : ubigint) : size_t {
        // fast path
        if(value == 0) {
            out_buf[0] = '0';
            return 1;
        }

        // temporary reversed digits (max 20 digits for 64-bit)
        var rev : [20]char;
        var ri : int = 0;
        while(value != 0) {
            const d = (value % 10) as uint;
            rev[ri] = ('0' as int + d as int) as char;
            ri = ri + 1;
            value = value / 10;
        }

        // reverse into out_buf
        var i : size_t = 0;
        while(i < (ri as size_t)) {
            out_buf[i] = rev[(ri - 1 - (i as int))];
            i = i + 1;
        }
        return ri as size_t;
    }

    // convert signed 64-bit to ASCII in out_buf, return length written
    // handles INT64_MIN safely using the -(value+1) trick
    func i64_to_chars(&self, out_buf : *mut char, value : bigint) : size_t {
        var pos : size_t = 0;
        if(value < 0) {
            out_buf[0] = '-';
            pos = 1;
            // handle INT64_MIN safely
            var tmp = value + 1; // still negative or zero
            var uv : ubigint = 0;
            if(tmp < 0) {
                uv = (-(tmp)) as ubigint;
                uv = uv + 1;
            } else {
                // value was -1 -> tmp == 0 -> uv = 1
                uv = 1;
            }
            // write uv after the '-'
            var written = self.u64_to_chars(out_buf + 1, uv);
            return pos + written;
        } else {
            // positive
            var w = self.u64_to_chars(out_buf, (value as ubigint));
            return w;
        }
    }

    // convert double to ASCII into out_buf, return length written
    // not full IEEE formatting, but fast and uses integer rounding of fractional part
    func double_to_chars(&self, out_buf : *mut char, value : double, precision : int) : size_t {
        // clamp precision
        if(precision < 0) {
            precision = 6;
        } else if(precision > 18) {
            precision = 18;
        }

        // NaN
        if(std::dbl_is_nan(value)) {
            out_buf[0] = 'n'; out_buf[1] = 'a'; out_buf[2] = 'n';
            return 3;
        }

        // infinities (use bit-pattern comparisons like your example)
        if(std::dbl_is_inf(value)) {
            if(std::dbl_is_neg(value)) {
                out_buf[0] = '-'; out_buf[1] = 'i'; out_buf[2] = 'n'; out_buf[3] = 'f';
                return 4;
            } else {
                out_buf[0] = 'i'; out_buf[1] = 'n'; out_buf[2] = 'f';
                return 3;
            }
        }

        var dst_i : size_t = 0;
        var v = value;
        if(v < 0.0) {
            out_buf[dst_i] = '-';
            dst_i = dst_i + 1;
            v = -v;
        }

        // integer part
        var int_part = (v as bigint);

        // write integer part into tmp then copy to out_buf
        var tmp_int : [32]char;
        var int_len = self.i64_to_chars(&mut tmp_int[0], int_part);
        // copy integer chars
        var ci : size_t = 0;
        while(ci < int_len) {
            out_buf[dst_i + ci] = tmp_int[ci];
            ci = ci + 1;
        }
        dst_i = dst_i + int_len;

        if(precision == 0) {
            return dst_i;
        }

        // append decimal point
        out_buf[dst_i] = '.';
        dst_i = dst_i + 1;

        // fractional scaling
        var frac = v - (int_part as double);
        var pow10_d : double = 1.0;
        var pow10_u : ubigint = 1;
        var pi : int = 0;
        while(pi < precision) {
            pow10_d = pow10_d * 10.0;
            pow10_u = pow10_u * 10;
            pi = pi + 1;
        }

        // scaled fractional with rounding
        var scaled = (frac * pow10_d + 0.5) as ubigint;

        // if rounding carried into integer part
        if(scaled >= pow10_u) {
            // compute new integer = int_part + 1
            var new_int = int_part + 1;

            // convert new_int into tmp_int
            var new_int_len = self.i64_to_chars(&mut tmp_int[0], new_int);

            // overwrite the integer part at the start of out_buf (handle sign)
            var offset_sign = 0;
            if(value < 0.0) { offset_sign = 1; } // sign already in out_buf[0]
            // shift: replace characters from offset_sign..(offset_sign+int_len-1)
            var k : size_t = 0;
            while(k < new_int_len) {
                out_buf[offset_sign + k] = tmp_int[k];
                k = k + 1;
            }
            // adjust dst_i to after the new integer
            dst_i = offset_sign + new_int_len;
            // append '.' again
            out_buf[dst_i] = '.';
            dst_i = dst_i + 1;
            // fractional became zero after carry
            scaled = 0;
            // make sure leading zeros logic below will print zeros
            int_len = new_int_len;
        }

        // write fractional digits with zero-padding
        if(scaled == 0) {
            var z = 0;
            while(z < precision) {
                out_buf[dst_i] = '0';
                dst_i = dst_i + 1;
                z = z + 1;
            }
            return dst_i;
        }

        // convert scaled to digits reversed
        var revf : [20]char;
        var rfi : int = 0;
        var tmpf = scaled;
        while(tmpf != 0) {
            const d = (tmpf % 10) as uint;
            revf[rfi] = ('0' as int + d as int) as char;
            rfi = rfi + 1;
            tmpf = tmpf / 10;
        }

        // leading zeros needed
        var leading = precision - rfi;
        var zz = 0;
        while(zz < leading) {
            out_buf[dst_i] = '0';
            dst_i = dst_i + 1;
            zz = zz + 1;
        }

        // append reversed digits in proper order
        var fj : int = 0;
        while(fj < rfi) {
            out_buf[dst_i] = revf[rfi - 1 - fj];
            dst_i = dst_i + 1;
            fj = fj + 1;
        }

        return dst_i;
    }

    func float_to_chars(&self, out_buf : *mut char, value : float, precision : int) : size_t {
        return self.double_to_chars(out_buf, (value as double), precision);
    }

    // --- Stream methods -----------------------------------------------

    @override
    func writeI8(&self, value : i8) {
        fwrite(&value, 1, 1, get_stdout());
    }

    @override
    func writeI16(&self, value : i16) {
        var buf : [32]char;
        var len = self.i64_to_chars(&mut buf[0], (value as bigint));
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeI32(&self, value : i32) {
        var buf : [32]char;
        var len = self.i64_to_chars(&mut buf[0], (value as bigint));
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeI64(&self, value : i64) {
        var buf : char[64];
        var len = self.i64_to_chars(&mut buf[0], value);
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeU8(&self, value : u8) {
        fwrite(&value, 1, 1, get_stdout());
    }

    @override
    func writeU16(&self, value : u16) {
        var buf : [32]char;
        var len = self.u64_to_chars(&mut buf[0], (value as ubigint));
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeU32(&self, value : u32) {
        var buf : [32]char;
        var len = self.u64_to_chars(&mut buf[0], (value as ubigint));
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeU64(&self, value : u64) {
        var buf : char[64];
        var len = self.u64_to_chars(&mut buf[0], value);
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeStr(&self, value : *char, length : ubigint) {
        fwrite(value, 1, length, get_stdout());
    }

    @override
    func writeStrNoLen(&self, value : *char) {
        fwrite(value, 1, strlen(value), get_stdout());
    }

    @override
    func writeChar(&self, value : char) {
        fwrite(&value, 1, 1, get_stdout());
    }

    @override
    func writeUChar(&self, value : uchar) {
        fwrite(&value, 1, 1, get_stdout());
    }

    @override
    func writeShort(&self, value : short) {
        var buf : [32]char;
        var len = self.i64_to_chars(&mut buf[0], (value as bigint));
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeUShort(&self, value : ushort) {
        var buf : [32]char;
        var len = self.u64_to_chars(&mut buf[0], (value as ubigint));
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeInt(&self, value : int) {
        var buf : [32]char;
        var len = self.i64_to_chars(&mut buf[0], (value as bigint));
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeUInt(&self, value : uint) {
        var buf : [32]char;
        var len = self.u64_to_chars(&mut buf[0], (value as ubigint));
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeLong(&self, value : long) {
        var buf : [48]char;
        var len = self.i64_to_chars(&mut buf[0], (value as bigint));
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeULong(&self, value : ulong) {
        var buf : [48]char;
        var len = self.u64_to_chars(&mut buf[0], (value as ubigint));
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeLongLong(&self, value : longlong) {
        var buf : [64]char;
        var len = self.i64_to_chars(&mut buf[0], value);
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeULongLong(&self, value : ulonglong) {
        var buf : [64]char;
        var len = self.u64_to_chars(&mut buf[0], value);
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeFloat(&self, value : float) {
        // default precision 6
        var buf : [128]char;
        var len = self.float_to_chars(&mut buf[0], value, 6);
        fwrite(&buf[0], 1, len, get_stdout());
    }

    @override
    func writeDouble(&self, value : double) {
        // default precision 6 (matches printf default)
        var buf : char[256];
        var len = self.double_to_chars(&mut buf[0], value, 6);
        fwrite(&buf[0], 1, len, get_stdout());
    }

}

public comptime func print(expr : %expressive_string) : any {
    return intrinsics::wrap(intrinsics::expr_str_block_value(CommandLineStream{}, expr))
}

public comptime func println(expr : %expressive_string) : any {
    return intrinsics::wrap(intrinsics::expr_str_block_value(CommandLineStream{}, expr, '\n'))
}