using namespace std;
using namespace uuid;

@test
func test_uuid_constructors(env : &mut TestEnv) {
    var u1 = UUID();
    var b1 = u1.to_bytes();
    for (var i = 0u; i < 16u; i++) {
        expect_uint_eq(env, b1.get(i) as uint, 0u, "Default UUID must be zeroed");
    }

    var custom_bytes : [16]u8 = [
        0x01u8, 0x23u8, 0x45u8, 0x67u8, 0x89u8, 0xABu8, 0xCDu8, 0xEFu8,
        0xfeu8, 0xdcu8, 0xbau8, 0x98u8, 0x76u8, 0x54u8, 0x32u8, 0x10u8
    ];
    var u2 = UUID.from_bytes(custom_bytes);
    var b2 = u2.to_bytes();
    for (var i = 0u; i < 16u; i++) {
        expect_uint_eq(env, b2.get(i) as uint, custom_bytes[i] as uint, "from_bytes must retain the input bytes");
    }
}

@test
func test_uuid_equality_comparison(env : &mut TestEnv) {
    var b1 : [16]u8 = [0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 1u8];
    var b2 : [16]u8 = [0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 2u8];

    var u1 = UUID.from_bytes(b1);
    var u1_dup = UUID.from_bytes(b1);
    var u2 = UUID.from_bytes(b2);

    expect_true(env, u1.equals(u1_dup), "UUIDs with same bytes must be equal");
    expect_false(env, u1.equals(u2), "UUIDs with different bytes must not be equal");

    expect_uint_eq(env, u1.compare(u1_dup) as uint, 0u, "compare identical UUIDs should return 0");
    expect_true(env, u1.compare(u2) < 0, "u1 < u2 comparison should be negative");
    expect_true(env, u2.compare(u1) > 0, "u2 > u1 comparison should be positive");
}

@test
func test_uuid_formatting(env : &mut TestEnv) {
    var b : [16]u8 = [
        0x01u8, 0x23u8, 0x45u8, 0x67u8, 0x89u8, 0xABu8, 0xCDu8, 0xEFu8,
        0x01u8, 0x23u8, 0x45u8, 0x67u8, 0x89u8, 0xABu8, 0xCDu8, 0xEFu8
    ];
    var u = UUID.from_bytes(b);
    var expected = "01234567-89ab-cdef-0123-456789abcdef";

    var s = u.to_string();
    expect_true(env, s.to_view().equals(std::string_view(expected)), "to_string() formatted representation does not match expected");

    var reused = std::string();
    u.format_to(reused);
    expect_true(env, reused.to_view().equals(std::string_view(expected)), "format_to() formatted representation does not match expected");
}

@test
func test_uuid_parsing_valid(env : &mut TestEnv) {
    var input_str = "01234567-89ab-cdef-0123-456789abcdef";
    var result = uuid::parse(std::string_view(input_str));

    switch (result) {
        Ok(u) => {
            var s = u.to_string();
            expect_true(env, s.to_view().equals(std::string_view(input_str)), "Parsed UUID formatted back does not match original input");
        }
        Err(err) => {
            env.error("Failed to parse valid UUID");
        }
    }
}

@test
func test_uuid_parsing_invalid(env : &mut TestEnv) {
    // Test invalid length
    var res1 = uuid::parse(std::string_view("01234567-89ab-cdef-0123-456789abcde"));
    expect_true(env, res1 is std::Result.Err, "Parsing should fail for invalid length");

    // Test missing hyphens
    var res2 = uuid::parse(std::string_view("01234567889ab-cdef-0123-456789abcdef"));
    expect_true(env, res2 is std::Result.Err, "Parsing should fail for missing hyphen");

    // Test invalid hex characters and error reporting
    var res3 = uuid::parse(std::string_view("01234567-89ab-cdef-0123-456789abcdeg"));
    switch (res3) {
        Ok(u) => {
            env.error("Parsing should have failed for invalid hex char 'g'");
        }
        Err(err) => {
            expect_true(env, err.contains(std::string_view("Invalid hex character")), "Error message should mention invalid hex character");
            expect_true(env, err.contains(std::string_view("35")), "Error message should point to index 35");
        }
    }
}

@test
func test_uuid_v4(env : &mut TestEnv) {
    var u1 = uuid::v4();
    var b1 = u1.to_bytes();

    // Check version is 4 (byte 6 high nibble == 4)
    expect_uint_eq(env, (b1.get(6) >> 4u) as uint, 4u, "UUIDv4 version bits must be 4");

    // Check variant is RFC 4122 (byte 8 high bits == 10xx, i.e. 8, 9, a, or b)
    expect_uint_eq(env, (b1.get(8) >> 6u) as uint, 2u, "UUIDv4 variant bits must be 2 (0b10)");

    // Uniqueness test
    var u2 = uuid::v4();
    expect_false(env, u1.equals(u2), "Consecutive v4 calls must generate unique UUIDs");
}

@test
func test_uuid_v7(env : &mut TestEnv) {
    var u1 = uuid::v7();
    var b1 = u1.to_bytes();

    // Check version is 7 (byte 6 high nibble == 7)
    expect_uint_eq(env, (b1.get(6) >> 4u) as uint, 7u, "UUIDv7 version bits must be 7");

    // Check variant is RFC 9562 (byte 8 high bits == 10xx, i.e. 2 when shifted by 6)
    expect_uint_eq(env, (b1.get(8) >> 6u) as uint, 2u, "UUIDv7 variant bits must be 2 (0b10)");

    // Verify timestamp makes sense (must be non-zero and roughly current)
    var ts : u64 = 0;
    ts |= (b1.get(0) as u64) << 40u64;
    ts |= (b1.get(1) as u64) << 32u64;
    ts |= (b1.get(2) as u64) << 24u64;
    ts |= (b1.get(3) as u64) << 16u64;
    ts |= (b1.get(4) as u64) << 8u64;
    ts |= (b1.get(5) as u64);
    expect_true(env, ts > 0u64, "UUIDv7 timestamp must be non-zero");

    // Monotonicity / Uniqueness test under rapid generation
    var u2 = uuid::v7();
    expect_false(env, u1.equals(u2), "Consecutive v7 calls must generate unique UUIDs");
    expect_true(env, u1.compare(u2) < 0, "UUIDv7 generated later must compare greater lexicographically");
}
