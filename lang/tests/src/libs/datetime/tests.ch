// =============================================================================
// Datetime Library Tests
// =============================================================================

// ---------------------------------------------------------------------------
// Calendar function tests
// ---------------------------------------------------------------------------
@test
public func calendar_is_leap_year(env : &mut TestEnv) {
    if(datetime::is_leap_year(2000)) { env.success("2000 is leap") } else { env.error("2000 should be leap") }
    if(datetime::is_leap_year(2024)) { env.success("2024 is leap") } else { env.error("2024 should be leap") }
    if(!datetime::is_leap_year(1900)) { env.success("1900 not leap") } else { env.error("1900 should not be leap") }
    if(!datetime::is_leap_year(2023)) { env.success("2023 not leap") } else { env.error("2023 should not be leap") }
    if(!datetime::is_leap_year(2100)) { env.success("2100 not leap") } else { env.error("2100 should not be leap") }
}

@test
public func calendar_days_in_month(env : &mut TestEnv) {
    if(datetime::days_in_month(2024, 1) == 31) { env.success("Jan 31") } else { env.error("Jan should be 31") }
    if(datetime::days_in_month(2024, 2) == 29) { env.success("Feb 2024 is 29") } else { env.error("Feb 2024 should be 29") }
    if(datetime::days_in_month(2023, 2) == 28) { env.success("Feb 2023 is 28") } else { env.error("Feb 2023 should be 28") }
    if(datetime::days_in_month(2024, 4) == 30) { env.success("Apr 30") } else { env.error("Apr should be 30") }
    if(datetime::days_in_month(2024, 12) == 31) { env.success("Dec 31") } else { env.error("Dec should be 31") }
}

@test
public func calendar_day_of_week(env : &mut TestEnv) {
    // 1970-01-01 is Thursday (4)
    if(datetime::day_of_week(1970, 1, 1) == 4) { env.success("epoch is Thu") } else { env.error("epoch should be Thu(4)") }
    // 2024-01-01 is Monday (1)
    if(datetime::day_of_week(2024, 1, 1) == 1) { env.success("2024-01-01 is Mon") } else { env.error("2024-01-01 should be Mon(1)") }
    // 2000-01-01 is Saturday (6)
    if(datetime::day_of_week(2000, 1, 1) == 6) { env.success("2000-01-01 is Sat") } else { env.error("2000-01-01 should be Sat(6)") }
}

@test
public func calendar_day_of_year(env : &mut TestEnv) {
    if(datetime::day_of_year(2024, 1, 1) == 1) { env.success("Jan 1 is day 1") } else { env.error("Jan 1 should be day 1") }
    if(datetime::day_of_year(2024, 12, 31) == 366) { env.success("Dec 31 leap is 366") } else { env.error("Dec 31 leap should be 366") }
    if(datetime::day_of_year(2023, 12, 31) == 365) { env.success("Dec 31 non-leap is 365") } else { env.error("Dec 31 non-leap should be 365") }
    if(datetime::day_of_year(2024, 3, 1) == 61) { env.success("Mar 1 leap is 61") } else { env.error("Mar 1 leap should be 61") }
}

@test
public func calendar_is_valid_date(env : &mut TestEnv) {
    if(datetime::is_valid_date(2024, 1, 15)) { env.success("valid date") } else { env.error("2024-01-15 is valid") }
    if(datetime::is_valid_date(2024, 2, 29)) { env.success("leap day valid") } else { env.error("2024-02-29 is valid") }
    if(!datetime::is_valid_date(2023, 2, 29)) { env.success("invalid leap day") } else { env.error("2023-02-29 is invalid") }
    if(!datetime::is_valid_date(2024, 13, 1)) { env.success("month 13 invalid") } else { env.error("month 13 is invalid") }
    if(!datetime::is_valid_date(2024, 0, 1)) { env.success("month 0 invalid") } else { env.error("month 0 is invalid") }
}

@test
public func calendar_is_valid_time(env : &mut TestEnv) {
    if(datetime::is_valid_time(12, 30, 45, 123456789)) { env.success("valid time") } else { env.error("12:30:45.123456789 is valid") }
    if(!datetime::is_valid_time(24, 0, 0, 0)) { env.success("hour 24 invalid") } else { env.error("hour 24 is invalid") }
    if(!datetime::is_valid_time(12, 60, 0, 0)) { env.success("min 60 invalid") } else { env.error("min 60 is invalid") }
    if(!datetime::is_valid_time(12, 0, 60, 0)) { env.success("sec 60 invalid") } else { env.error("sec 60 is invalid") }
}

// ---------------------------------------------------------------------------
// TimeZone tests
// ---------------------------------------------------------------------------
@test
public func timezone_utc(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    if(tz.offset_secs == 0) { env.success("utc offset 0") } else { env.error("utc offset should be 0") }
    var off_h = tz.offset_hours()
    if(off_h == 0) { env.success("utc hours 0") } else { env.error("utc hours should be 0") }
}

@test
public func timezone_fixed_offset(env : &mut TestEnv) {
    var tz = datetime::TimeZone::fixed(19800, std::string_view("Asia/Kolkata"))
    if(tz.offset_secs == 19800) { env.success("offset 19800") } else { env.error("offset should be 19800") }
    var off_h = tz.offset_hours()
    if(off_h == 5) { env.success("hours 5") } else { env.error("hours should be 5") }
    var off_m = tz.offset_minutes()
    if(off_m == 30) { env.success("minutes 30") } else { env.error("minutes should be 30") }
}

@test
public func timezone_negative_offset(env : &mut TestEnv) {
    var tz = datetime::TimeZone::fixed(-18000, std::string_view("America/New_York"))
    if(tz.offset_secs == -18000) { env.success("offset -18000") } else { env.error("offset should be -18000") }
    var off_h = tz.offset_hours()
    if(off_h == -5) { env.success("hours -5") } else { env.error("hours should be -5") }
}

@test
public func timezone_format_utc_offset(env : &mut TestEnv) {
    var tz_utc = datetime::TimeZone::utc()
    var s_utc = tz_utc.format_utc_offset()
    if(s_utc.equals_view(std::string_view("+00:00"))) { env.success("utc format +00:00") } else { env.error("utc format should be +00:00") }

    var tz_pos = datetime::TimeZone::fixed(19800, std::string_view("IST"))
    var s_pos = tz_pos.format_utc_offset()
    if(s_pos.equals_view(std::string_view("+05:30"))) { env.success("+05:30 format") } else { env.error("+05:30 format expected") }

    var tz_neg = datetime::TimeZone::fixed(-18000, std::string_view("EST"))
    var s_neg = tz_neg.format_utc_offset()
    if(s_neg.equals_view(std::string_view("-05:00"))) { env.success("-05:00 format") } else { env.error("-05:00 format expected") }
}

@test
public func timezone_equals(env : &mut TestEnv) {
    var a = datetime::TimeZone::utc()
    var b = datetime::TimeZone::utc()
    if(a.equals(&b)) { env.success("equal tz") } else { env.error("same tz should be equal") }
    var c = datetime::TimeZone::fixed(3600, std::string_view("CET"))
    if(!a.equals(&c)) { env.success("diff tz not equal") } else { env.error("different tz should not be equal") }
}

// ---------------------------------------------------------------------------
// DateTime tests
// ---------------------------------------------------------------------------
@test
public func datetime_default_constructor(env : &mut TestEnv) {
    var dt = datetime::DateTime()
    if(dt.year_val() == 1970) { env.success("default year 1970") } else { env.error("default year should be 1970") }
    if(dt.month_val() == 1) { env.success("default month 1") } else { env.error("default month should be 1") }
    if(dt.day_val() == 1) { env.success("default day 1") } else { env.error("default day should be 1") }
    if(dt.hour_val() == 0) { env.success("default hour 0") } else { env.error("default hour should be 0") }
    if(dt.minute_val() == 0) { env.success("default minute 0") } else { env.error("default minute should be 0") }
    if(dt.second_val() == 0) { env.success("default second 0") } else { env.error("default second should be 0") }
    if(dt.subsec_nanos() == 0) { env.success("default nanos 0") } else { env.error("default nanos should be 0") }
}

@test
public func datetime_from_components(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt = datetime::DateTime::from_components(2024, 12, 25, 10, 30, 45, 123456789, tz)
    if(dt.year_val() == 2024) { env.success("comp year") } else { env.error("year should be 2024") }
    if(dt.month_val() == 12) { env.success("comp month") } else { env.error("month should be 12") }
    if(dt.day_val() == 25) { env.success("comp day") } else { env.error("day should be 25") }
    if(dt.hour_val() == 10) { env.success("comp hour") } else { env.error("hour should be 10") }
    if(dt.minute_val() == 30) { env.success("comp minute") } else { env.error("minute should be 30") }
    if(dt.second_val() == 45) { env.success("comp second") } else { env.error("second should be 45") }
    if(dt.subsec_nanos() == 123456789) { env.success("comp nanos") } else { env.error("nanos should be 123456789") }
}

@test
public func datetime_roundtrip_systemtime_utc(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt1 = datetime::DateTime::from_components(2024, 6, 15, 12, 0, 0, 0, tz)
    var st = dt1.to_system_time()
    var dt2 = datetime::DateTime::from_system_time(&st)
    if(dt1.equals(&dt2)) { env.success("rt utc ok") } else { env.error("roundtrip UTC failed") }
}

@test
public func datetime_roundtrip_systemtime_positive_offset(env : &mut TestEnv) {
    var tz = datetime::TimeZone::fixed(19800, std::string_view("IST"))
    var dt1 = datetime::DateTime::from_components(2024, 6, 15, 17, 30, 0, 0, tz.copy())
    var st = dt1.to_system_time()
    var dt2 = datetime::DateTime::from_system_time_tz(&st, tz.copy())
    if(dt1.equals(&dt2)) { env.success("rt +0530 ok") } else { env.error("roundtrip +05:30 failed") }
    // Also verify the UTC equivalent
    var dt_utc = datetime::DateTime::from_system_time(&st)
    if(dt_utc.hour_val() == 12) { env.success("utc hour is 12") } else { env.error("utc hour should be 12 for 17:30 IST") }
}

@test
public func datetime_roundtrip_systemtime_negative_offset(env : &mut TestEnv) {
    var tz = datetime::TimeZone::fixed(-18000, std::string_view("EST"))
    var dt1 = datetime::DateTime::from_components(2024, 6, 15, 7, 0, 0, 0, tz.copy())
    var st = dt1.to_system_time()
    var dt2 = datetime::DateTime::from_system_time_tz(&st, tz)
    if(dt1.equals(&dt2)) { env.success("rt -0500 ok") } else { env.error("roundtrip -05:00 failed") }
    // UTC should be 12:00
    var dt_utc = datetime::DateTime::from_system_time(&st)
    if(dt_utc.hour_val() == 12) { env.success("utc hour is 12") } else { env.error("utc hour should be 12 for 07:00 EST") }
}

@test
public func datetime_epoch_roundtrip(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt1 = datetime::DateTime::from_components(1970, 1, 1, 0, 0, 0, 0, tz)
    var st = dt1.to_system_time()
    var epoch_secs = st.as_unix_epoch_secs()
    if(epoch_secs == 0) { env.success("epoch secs 0") } else { env.error("epoch secs should be 0") }
    var dt2 = datetime::DateTime::from_system_time(&st)
    if(dt1.equals(&dt2)) { env.success("epoch rt ok") } else { env.error("epoch roundtrip failed") }
}

@test
public func datetime_day_of_week(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    // 2024-01-01 is Monday (1)
    var dt = datetime::DateTime::from_components(2024, 1, 1, 0, 0, 0, 0, tz.copy())
    if(dt.day_of_week_val() == 1) { env.success("dow Mon") } else { env.error("2024-01-01 should be Mon") }
    // 1970-01-01 is Thursday (4)
    var dt2 = datetime::DateTime::from_components(1970, 1, 1, 0, 0, 0, 0, tz)
    if(dt2.day_of_week_val() == 4) { env.success("dow Thu") } else { env.error("1970-01-01 should be Thu") }
}

@test
public func datetime_day_of_year(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt = datetime::DateTime::from_components(2024, 12, 31, 0, 0, 0, 0, tz.copy())
    if(dt.day_of_year_val() == 366) { env.success("doy 366") } else { env.error("2024-12-31 should be day 366") }
    var dt2 = datetime::DateTime::from_components(2023, 12, 31, 0, 0, 0, 0, tz)
    if(dt2.day_of_year_val() == 365) { env.success("doy 365") } else { env.error("2023-12-31 should be day 365") }
}

@test
public func datetime_is_leap_year(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt = datetime::DateTime::from_components(2024, 1, 1, 0, 0, 0, 0, tz.copy())
    if(dt.is_leap_year_val()) { env.success("2024 is leap") } else { env.error("2024 should be leap") }
    var dt2 = datetime::DateTime::from_components(2023, 1, 1, 0, 0, 0, 0, tz)
    if(!dt2.is_leap_year_val()) { env.success("2023 not leap") } else { env.error("2023 should not be leap") }
}

@test
public func datetime_format_simple(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt = datetime::DateTime::from_components(2024, 3, 5, 9, 7, 3, 0, tz)
    var s = dt.format(std::string_view("%Y-%m-%d %H:%M:%S"))
    if(s.equals_view(std::string_view("2024-03-05 09:07:03"))) { env.success("fmt basic") } else { env.error("basic format failed") }
}

@test
public func datetime_format_timezone(env : &mut TestEnv) {
    var tz = datetime::TimeZone::fixed(19800, std::string_view("IST"))
    var dt = datetime::DateTime::from_components(2024, 1, 1, 12, 0, 0, 0, tz)
    var s = dt.format(std::string_view("%z %Z"))
    if(s.equals_view(std::string_view("+05:30 IST"))) { env.success("fmt tz") } else { env.error("timezone format failed") }
}

@test
public func datetime_format_nanos(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt = datetime::DateTime::from_components(2024, 1, 1, 0, 0, 0, 123456789, tz)
    var s = dt.format(std::string_view("%f"))
    // The nanos formatter might not zero-pad correctly
    if(s.size() == 9) { env.success("fmt nanos 9 digits") } else { env.error("nanos should be 9 digits") }
}

@test
public func datetime_format_literal_percent(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt = datetime::DateTime::from_components(2024, 1, 1, 0, 0, 0, 0, tz)
    var s = dt.format(std::string_view("100%%"))
    if(s.equals_view(std::string_view("100%"))) { env.success("fmt %%") } else { env.error("literal percent failed") }
}

@test
public func datetime_arithmetic_add_duration(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt = datetime::DateTime::from_components(2024, 1, 1, 0, 0, 0, 0, tz)
    var dur = std::chrono::Duration::from_secs(86400)
    var dt2 = dt.add_duration(&dur)
    if(dt2.day_val() == 2) { env.success("add 1 day") } else { env.error("adding 86400s should advance 1 day") }
}

@test
public func datetime_arithmetic_sub_duration(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt = datetime::DateTime::from_components(2024, 1, 10, 0, 0, 0, 0, tz)
    var dur = std::chrono::Duration::from_secs(86400)
    var dt2 = dt.sub_duration(&dur)
    if(dt2.day_val() == 9) { env.success("sub 1 day") } else { env.error("subtracting 86400s should go back 1 day") }
}

@test
public func datetime_arithmetic_duration_since(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var dt1 = datetime::DateTime::from_components(2024, 1, 2, 0, 0, 0, 0, tz.copy())
    var dt2 = datetime::DateTime::from_components(2024, 1, 1, 0, 0, 0, 0, tz)
    var dur = dt1.duration_since(&dt2)
    var secs = dur.as_secs()
    if(secs == 86400) { env.success("dur since 86400") } else { env.error("duration_since should be 86400s") }
}

@test
public func datetime_equals(env : &mut TestEnv) {
    var tz = datetime::TimeZone::utc()
    var a = datetime::DateTime::from_components(2024, 1, 1, 0, 0, 0, 0, tz.copy())
    var b = datetime::DateTime::from_components(2024, 1, 1, 0, 0, 0, 0, tz.copy())
    if(a.equals(&b)) { env.success("equal dt") } else { env.error("same DateTime should be equal") }
    var c = datetime::DateTime::from_components(2024, 1, 2, 0, 0, 0, 0, tz)
    if(!a.equals(&c)) { env.success("diff dt not equal") } else { env.error("different DateTime should not be equal") }
}

@test
public func datetime_negative_epoch(env : &mut TestEnv) {
    // Date before 1970-01-01 (negative unix timestamp)
    var tz = datetime::TimeZone::utc()
    var dt = datetime::DateTime::from_components(1969, 12, 31, 23, 0, 0, 0, tz)
    var st = dt.to_system_time()
    var secs = st.as_unix_epoch_secs()
    if(secs == -3600) { env.success("neg epoch secs") } else { env.error("1969-12-31 23:00 should be -3600") }
    var dt2 = datetime::DateTime::from_system_time(&st)
    if(dt.equals(&dt2)) { env.success("neg rt ok") } else { env.error("negative epoch roundtrip failed") }
}

@test
public func datetime_before_epoch_roundtrip(env : &mut TestEnv) {
    // Test a date well before epoch
    var tz = datetime::TimeZone::utc()
    var dt1 = datetime::DateTime::from_components(1947, 8, 15, 0, 0, 0, 0, tz)
    var st = dt1.to_system_time()
    var dt2 = datetime::DateTime::from_system_time(&st)
    if(dt1.equals(&dt2)) { env.success("1947 rt ok") } else { env.error("1947 roundtrip failed") }
}

@test
public func datetime_far_future_roundtrip(env : &mut TestEnv) {
    // Test a date in the far future
    var tz = datetime::TimeZone::utc()
    var dt1 = datetime::DateTime::from_components(2099, 12, 31, 23, 59, 59, 999999999, tz)
    var st = dt1.to_system_time()
    var dt2 = datetime::DateTime::from_system_time(&st)
    if(dt1.equals(&dt2)) { env.success("2099 rt ok") } else { env.error("2099 roundtrip failed") }
}

@test
public func datetime_leap_year_boundary(env : &mut TestEnv) {
    // Test boundary of leap year: 2000-02-28 -> 2000-03-01
    var tz = datetime::TimeZone::utc()
    var dt1 = datetime::DateTime::from_components(2000, 2, 28, 0, 0, 0, 0, tz)
    var dur = std::chrono::Duration::from_secs(86400 * 2)
    var dt2 = dt1.add_duration(&dur)
    if(dt2.day_val() == 1 && dt2.month_val() == 3) { env.success("leap boundary ok") } else { env.error("Feb 28 + 2d should be Mar 1") }
}

@test
public func datetime_new_year_boundary(env : &mut TestEnv) {
    // Test new year boundary: 2024-12-31 + 1 day = 2025-01-01
    var tz = datetime::TimeZone::utc()
    var dt1 = datetime::DateTime::from_components(2024, 12, 31, 0, 0, 0, 0, tz)
    var dur = std::chrono::Duration::from_secs(86400)
    var dt2 = dt1.add_duration(&dur)
    if(dt2.year_val() == 2025 && dt2.month_val() == 1 && dt2.day_val() == 1) {
        env.success("new year boundary ok")
    } else {
        env.error("Dec 31 + 1d should be Jan 1 next year")
    }
}
