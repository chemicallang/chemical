import "@cstd/common/integer_types.ch"

func hex_digit_to_int(c : char) : int {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    // Normalize to lowercase.
    c = tolower(c) as char;
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;  // Invalid hex digit.
}

/*
 * Parses a CSS hex color string into a 32-bit unsigned integer.
 * REQUIREMENT: str must not contain '#' symbol at the zero index
 * REQUIREMENT: str must of 3, 4, 6 or 8 in length
 * Acceptable formats: 3-digit (#RGB), 4-digit (#RGBA), 6-digit (#RRGGBB) or 8-digit (#RRGGBBAA).
 * The resulting 32-bit color is stored in 0xRRGGBBAA format, where the
 * alpha channel defaults to 0xFF if not provided.
 * Returns true on success, or false if the input is invalid.
 */
func parse_css_hex_color(str : *char, len : size_t, outColor : *mut uint32_t) : bool {

    var r : uint32_t = 0
    var g : uint32_t = 0
    var b : uint32_t = 0
    var a : uint32_t = 0xFF;  // Default alpha to opaque.

    if (len == 3 || len == 4) {
        // 3-digit (#RGB) or 4-digit (#RGBA) form.
        var r_digit : int = hex_digit_to_int(str[0]);
        var g_digit : int = hex_digit_to_int(str[1]);
        var b_digit : int = hex_digit_to_int(str[2]);
        if (r_digit < 0 || g_digit < 0 || b_digit < 0) {
            return false;
        }
        // Expand shorthand digit to full 8 bits (value * 17 equals (value << 4) | value)
        r = (r_digit * 17) as uint32_t;
        g = (g_digit * 17) as uint32_t;
        b = (b_digit * 17) as uint32_t;

        if (len == 4) {
            var a_digit : int = hex_digit_to_int(str[3]);
            if (a_digit < 0) {
                return false;
            }
            a = (a_digit * 17) as uint32_t;
        }
    } else {  // len == 6 or 8.
        var r_digit1 : int = hex_digit_to_int(str[0]);
        var r_digit2 : int = hex_digit_to_int(str[1]);
        var g_digit1 : int = hex_digit_to_int(str[2]);
        var g_digit2 : int = hex_digit_to_int(str[3]);
        var b_digit1 : int = hex_digit_to_int(str[4]);
        var b_digit2 : int = hex_digit_to_int(str[5]);
        if (r_digit1 < 0 || r_digit2 < 0 ||
            g_digit1 < 0 || g_digit2 < 0 ||
            b_digit1 < 0 || b_digit2 < 0) {
            return false;
        }
        r = ((r_digit1 << 4) | r_digit2) as uint32_t;
        g = ((g_digit1 << 4) | g_digit2) as uint32_t;
        b = ((b_digit1 << 4) | b_digit2) as uint32_t;

        if (len == 8) {
            var a_digit1 : int = hex_digit_to_int(str[6]);
            var a_digit2 : int = hex_digit_to_int(str[7]);
            if (a_digit1 < 0 || a_digit2 < 0) {
                return false;
            }
            a = ((a_digit1 << 4) | a_digit2) as uint32_t;
        }
    }

    // Pack the channels into a 32-bit value as 0xRRGGBBAA.
    *outColor = (r << 24) | (g << 16) | (b << 8) | a;
    return true;
}

/*
 * TOOO use snprintf to return a string instead
 * Prints the given color (stored as 0xRRGGBBAA) in the smallest valid CSS hex
 * representation.
 *
 * For opaque colors (alpha == 0xFF):
 *   - If each color channel can be expressed as a single hex digit (i.e. both
 *     nibbles are identical), prints in 3-digit shorthand (#RGB).
 *   - Otherwise, prints the full 6-digit form (#RRGGBB).
 *
 * For colors with transparency:
 *   - If each channel (including alpha) qualifies for shorthand, prints #RGBA.
 *   - Otherwise, prints the full 8-digit form (#RRGGBBAA).
 */
func print_color_hex(color : uint32_t) {
    var r : uint8_t = (color >> 24) & 0xFF;
    var g : uint8_t = (color >> 16) & 0xFF;
    var b : uint8_t = (color >> 8)  & 0xFF;
    var a : uint8_t = color & 0xFF;

    // Helper lambda (using C99 inline function style) could be used here, but we'll just
    // inline the condition: a channel qualifies for shorthand if its high nibble equals
    // its low nibble.
    var r_shorthand : bool = ((r >> 4) == (r & 0xF));
    var g_shorthand : bool = ((g >> 4) == (g & 0xF));
    var b_shorthand : bool = ((b >> 4) == (b & 0xF));

    if (a == 0xFF) {
        // Opaque color: try to use 3-digit shorthand if possible.
        if (r_shorthand && g_shorthand && b_shorthand) {
            // Each channel is representable in 1 digit.
            printf("#%X%X%X", r >> 4, g >> 4, b >> 4);
        } else {
            // Use full 6-digit form.
            printf("#%02X%02X%02X", r, g, b);
        }
    } else {
        // Transparent color: check for shorthand possibility for all channels.
        var a_shorthand : bool = ((a >> 4) == (a & 0xF));
        if (r_shorthand && g_shorthand && b_shorthand && a_shorthand) {
            printf("#%X%X%X%X", r >> 4, g >> 4, b >> 4, a >> 4);
        } else {
            printf("#%02X%02X%02X%02X", r, g, b, a);
        }
    }
}
