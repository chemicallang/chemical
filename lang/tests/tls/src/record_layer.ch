// ============================================================================
// Record Layer Buffer Consumption Tests
// ============================================================================
// These tests verify that ssl_consume_record properly advances the input buffer
// after reading a TLS record. Without proper consumption, repeated read calls
// would return the SAME record instead of advancing to the next one.
// ============================================================================

using namespace tls

// ─── Test 1: Single record consumption ─────────────────────────────────────
// After consuming a single record, in_left should be 0 and buffer should be empty.

@test
public func UNIT_record_consume_single(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(unsafe(&raw mut ctx))

    // Create a simulated TLS record: content_type(1) + version(2) + length(2) + payload
    // ServerHello: content_type=22 (0x16), version=0x0303, length=3, payload=[0x02, 0x00, 0x01, 0x00]
    var record_data : [9]u8 = [
        0x16 as u8,  // content_type: Handshake (22)
        0x03 as u8,  // version major
        0x03 as u8,  // version minor
        0x00 as u8,  // length high
        0x04 as u8,  // length low (4 bytes payload)
        // payload (handshake message)
        0x02 as u8,  // ServerHello (2)
        0x00 as u8,  // body length high
        0x00 as u8,  // body length
        0x01 as u8   // body length low (1 byte body)
    ]

    // Set the context's input buffer with this single record
    ctx.in_buf[0] = record_data[0]
    ctx.in_buf[1] = record_data[1]
    ctx.in_buf[2] = record_data[2]
    ctx.in_buf[3] = record_data[3]
    ctx.in_buf[4] = record_data[4]
    ctx.in_buf[5] = record_data[5]
    ctx.in_buf[6] = record_data[6]
    ctx.in_buf[7] = record_data[7]
    ctx.in_buf[8] = record_data[8]
    unsafe { ctx.in_left = 9 }
    unsafe { ctx.in_msglen = 4 }  // 4 bytes payload

    // Consume the record
    ssl_consume_record(unsafe(&raw mut ctx))

    // After consuming: in_left should be 0 (no more data)
    if(unsafe(ctx.in_left) != 0) {
        env.error("UNIT_record_consume_single: in_left should be 0 after consuming single record")
    }
    if(unsafe(ctx.in_msglen) != 0) {
        env.error("UNIT_record_consume_single: in_msglen should be 0 after consuming")
    }
}

// ─── Test 2: Two coalesced records ─────────────────────────────────────────
// When two records arrive in the same TCP segment, consuming the first should
// make the second accessible at position 0 of the buffer.

@test
public func UNIT_record_consume_two_records(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(unsafe(&raw mut ctx))

    // First record: ServerHello (content_type=22), length=4
    // Bytes: 16 03 03 00 04 [payload: 02 00 00 01]
    var rec1 : [9]u8 = [
        0x16 as u8, 0x03 as u8, 0x03 as u8, 0x00 as u8, 0x04 as u8,
        0x02 as u8, 0x00 as u8, 0x00 as u8, 0x01 as u8
    ]

    // Second record: Certificate (content_type=22), length=3  
    // Bytes: 16 03 03 00 03 [payload: 0B 00 00 00]
    var rec2 : [8]u8 = [
        0x16 as u8, 0x03 as u8, 0x03 as u8, 0x00 as u8, 0x03 as u8,
        0x0B as u8, 0x00 as u8, 0x00 as u8
    ]

    // Fill buffer with both records concatenated
    var total_len : size_t = 17  // 9 + 8
    var pos : size_t = 0
    while(pos < 9) { ctx.in_buf[pos] = rec1[pos]; pos += 1 }
    while(pos < 17) { ctx.in_buf[pos] = rec2[pos - 9]; pos += 1 }
    unsafe { ctx.in_left = 17 }
    unsafe { ctx.in_msglen = 4 }  // first record payload length = 4

    // Capture header for first record
    ctx.in_hdr[0] = rec1[0]
    ctx.in_hdr[1] = rec1[1]
    ctx.in_hdr[2] = rec1[2]
    ctx.in_hdr[3] = rec1[3]
    ctx.in_hdr[4] = rec1[4]

    // Consume first record (5 header + 4 payload = 9 bytes)
    ssl_consume_record(unsafe(&raw mut ctx))

    // After consuming: in_left should be 8 (the second record)
    if(unsafe(ctx.in_left) != 8) {
        env.error("UNIT_record_consume_two_records: in_left should be 8 after consuming first record")
        return
    }

    // The second record should now be at the start of the buffer
    var i : size_t = 0
    while(i < 5) {
        if(unsafe(ctx.in_buf[i]) != rec2[i]) {
            env.error("UNIT_record_consume_two_records: second record header mismatch")
            return
        }
        i += 1
    }

    // Verify the second record's content_type is HANDSHAKE (22 = 0x16)
    if(unsafe(ctx.in_buf[0]) != 0x16 as u8) {
        env.error("UNIT_record_consume_two_records: second record content_type should be 0x16")
    }
}

// ─── Test 3: CCS followed by handshake record ──────────────────────────────
// ChangeCipherSpec (content_type=20) followed by a handshake should be correctly
// consumed. After CCS consumption, the handshake should be at position 0.

@test
public func UNIT_record_consume_ccs_then_handshake(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(unsafe(&raw mut ctx))

    // CCS record: content_type=20 (0x14), version=0x0303, length=1, payload=[0x01]
    var ccs_rec : [6]u8 = [
        0x14 as u8, 0x03 as u8, 0x03 as u8, 0x00 as u8, 0x01 as u8,
        0x01 as u8
    ]

    // Handshake record: content_type=22 (0x16), version=0x0303, length=4
    var hs_rec : [9]u8 = [
        0x16 as u8, 0x03 as u8, 0x03 as u8, 0x00 as u8, 0x04 as u8,
        0x02 as u8, 0x00 as u8, 0x00 as u8, 0x01 as u8
    ]

    // Fill buffer: CCS + Handshake
    var pos : size_t = 0
    while(pos < 6) { ctx.in_buf[pos] = ccs_rec[pos]; pos += 1 }
    while(pos < 15) { ctx.in_buf[pos] = hs_rec[pos - 6]; pos += 1 }
    unsafe { ctx.in_left = 15 }
    unsafe { ctx.in_msglen = 1 }  // CCS payload = 1 byte
    ctx.in_hdr[0] = ccs_rec[0]
    ctx.in_hdr[1] = ccs_rec[1]
    ctx.in_hdr[2] = ccs_rec[2]
    ctx.in_hdr[3] = ccs_rec[3]
    ctx.in_hdr[4] = ccs_rec[4]

    // Consume CCS (5 header + 1 payload = 6 bytes)
    ssl_consume_record(unsafe(&raw mut ctx))

    // After consuming CCS: in_left should be 9 (the handshake record)
    if(unsafe(ctx.in_left) != 9) {
        env.error("UNIT_record_consume_ccs_then_handshake: in_left should be 9 after consuming CCS")
        return
    }

    // Handshake should now be at position 0
    var i : size_t = 0
    while(i < 5) {
        if(unsafe(ctx.in_buf[i]) != hs_rec[i]) {
            env.error("UNIT_record_consume_ccs_then_handshake: handshake record header mismatch")
            return
        }
        i += 1
    }

    // Verify content_type is HANDSHAKE
    if(unsafe(ctx.in_buf[0]) != 0x16 as u8) {
        env.error("UNIT_record_consume_ccs_then_handshake: content_type should be 0x16")
    }
}

// ─── Test 4: Consume with exact boundary ───────────────────────────────────
// When in_left equals consumed amount, no shift should be needed.

@test
public func UNIT_record_consume_exact_boundary(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(unsafe(&raw mut ctx))

    // Single record: length = 0 (empty payload)
    var record : [5]u8 = [
        0x15 as u8,  // Alert (21)
        0x03 as u8, 0x03 as u8,
        0x00 as u8, 0x00 as u8  // length = 0
    ]

    var i : size_t = 0
    while(i < 5) { ctx.in_buf[i] = record[i]; i += 1 }
    unsafe { ctx.in_left = 5 }
    unsafe { ctx.in_msglen = 0 }  // 0 bytes payload
    ctx.in_hdr[0] = record[0]
    ctx.in_hdr[1] = record[1]
    ctx.in_hdr[2] = record[2]
    ctx.in_hdr[3] = record[3]
    ctx.in_hdr[4] = record[4]

    // Consume (5 header + 0 payload = 5 bytes)
    ssl_consume_record(unsafe(&raw mut ctx))

    // in_left should be 0
    if(unsafe(ctx.in_left) != 0) {
        env.error("UNIT_record_consume_exact_boundary: in_left should be 0")
    }
}

// ─── Test 5: Consume with empty buffer ─────────────────────────────────────
// Consuming when in_left is 0 should be a no-op.

@test
public func UNIT_record_consume_empty(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(unsafe(&raw mut ctx))

    unsafe { ctx.in_left = 0 }
    unsafe { ctx.in_msglen = 0 }

    ssl_consume_record(unsafe(&raw mut ctx))

    // Should still be 0
    if(unsafe(ctx.in_left) != 0) {
        env.error("UNIT_record_consume_empty: in_left should still be 0")
    }
}

// ─── Test 6: Consume with msglen larger than in_left ───────────────────────
// ssl_consume_record should cap consumed to in_left.

@test
public func UNIT_record_consume_capped(env : &mut TestEnv) {
    var ctx : SSLContext; ssl_init(unsafe(&raw mut ctx))

    // Set in_left to 7 (less than 5 header + 5 payload = 10)
    unsafe { ctx.in_left = 7 }
    unsafe { ctx.in_msglen = 5 }  // claims 5 bytes payload but only 2 available

    ssl_consume_record(unsafe(&raw mut ctx))

    // Should consume all 7 bytes (capped)
    if(unsafe(ctx.in_left) != 0) {
        env.error("UNIT_record_consume_capped: in_left should be 0 (capped consumption)")
    }
}
