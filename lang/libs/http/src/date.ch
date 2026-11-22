// ===== HTTP dates =====
//
// RFC 9110 §6.6.1: a server SHOULD send a `Date` header on every response, in an
// IMF-fixdate (`Sun, 06 Nov 1994 08:49:37 GMT`, always UTC regardless of the local
// zone). Caches and clients use it for `Age`, freshness and clock checks, and a
// response without one looks unfinished to anyone reading `curl -v`.
//
// The formatter is calendar arithmetic over a Unix timestamp rather than
// `strftime`, so it is the same on every target (the C library's `%a`/`%b` are
// locale-dependent — a server whose header says "dim." is not RFC-conforming) and
// needs no timezone database.
public namespace http {

using std::string;
using std::string_view;

/// Three-letter English weekday (`0` is Sunday — 1970-01-01 was a Thursday).
func http_date_weekday(index : i64) : *char {
    switch(index) {
        0 => { return "Sun" }
        1 => { return "Mon" }
        2 => { return "Tue" }
        3 => { return "Wed" }
        4 => { return "Thu" }
        5 => { return "Fri" }
        default => { return "Sat" }
    }
}

func http_date_month(index : i64) : *char {
    switch(index) {
        1 => { return "Jan" }
        2 => { return "Feb" }
        3 => { return "Mar" }
        4 => { return "Apr" }
        5 => { return "May" }
        6 => { return "Jun" }
        7 => { return "Jul" }
        8 => { return "Aug" }
        9 => { return "Sep" }
        10 => { return "Oct" }
        11 => { return "Nov" }
        default => { return "Dec" }
    }
}

func http_date_two_digits(out : &mut string, value : i64) {
    if(value < 10) { out.append('0') }
    out.append_integer(value)
}

/// Days since 1970-01-01 for a civil date (Howard Hinnant's `days_from_civil`):
/// exact for every proleptic Gregorian date, no table, no leap-second special case.
func http_date_days_from_civil(year_in : i64, month : i64, day : i64) : i64 {
    var y = year_in
    if(month <= 2) { y = y - 1 }
    var era = y
    if(era < 0) { era = era - 399 }
    era = era / 400
    var yoe = y - era * 400
    var m = month
    var mp : i64 = 0
    if(m > 2) { mp = m - 3 } else { mp = m + 9 }
    var doy = (153 * mp + 2) / 5 + day - 1
    var doe = yoe * 365 + yoe / 4 - yoe / 100 + doy
    return era * 146097 + doe - 719468
}

/// Civil date for a day count since 1970-01-01 (`civil_from_days`).
func http_date_civil_from_days(days : i64, year : *mut i64, month : *mut i64, day : *mut i64) {
    var z = days + 719468
    var era = z
    if(era < 0) { era = era - 146096 }
    era = era / 146097
    var doe = z - era * 146097
    var yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365
    var y = yoe + era * 400
    var doy = doe - (365 * yoe + yoe / 4 - yoe / 100)
    var mp = (5 * doy + 2) / 153
    var d = doy - (153 * mp + 2) / 5 + 1
    var m : i64 = 0
    if(mp < 10) { m = mp + 3 } else { m = mp - 9 }
    if(m <= 2) { y = y + 1 }
    unsafe {
        *year = y
        *month = m
        *day = d
    }
}

/// `epoch_secs` as an IMF-fixdate: `Sun, 06 Nov 1994 08:49:37 GMT`.
public func format_http_date(epoch_secs : i64) : string {
    var days = epoch_secs
    if(days < 0) { days = days - 86399 }
    days = days / 86400
    var secs_of_day = epoch_secs - days * 86400

    var year : i64 = 1970
    var month : i64 = 1
    var day : i64 = 1
    http_date_civil_from_days(days, &raw mut year, &raw mut month, &raw mut day)

    var weekday = (days + 4) % 7  // 1970-01-01 was a Thursday (index 4, Sunday = 0)
    if(weekday < 0) { weekday = weekday + 7 }

    var out = string()
    out.reserve(29)
    out.append_char_ptr(http_date_weekday(weekday))
    out.append_view(", ")
    http_date_two_digits(&mut out, day)
    out.append(' ')
    out.append_char_ptr(http_date_month(month))
    out.append(' ')
    out.append_integer(year)
    out.append(' ')
    var hour = secs_of_day / 3600
    var minute = (secs_of_day % 3600) / 60
    var second = secs_of_day % 60
    http_date_two_digits(&mut out, hour)
    out.append(':')
    http_date_two_digits(&mut out, minute)
    out.append(':')
    http_date_two_digits(&mut out, second)
    out.append_view(" GMT")
    return out
}

/// The current time as an IMF-fixdate, for a `Date` response header.
public func now_http_date() : string {
    var now = std::chrono::SystemTime::now()
    return format_http_date(now.as_unix_epoch_secs())
}

/// True when `text` is a well-formed IMF-fixdate (used by tests and by a client
/// that wants to sanity-check a `Date` header before trusting `Age`).
public func is_imf_fixdate(text : std::string_view) : bool {
    // "Sun, 06 Nov 1994 08:49:37 GMT" — 29 bytes, fixed positions.
    if(text.size() != 29u) { return false }
    if(text.at(3) != ',' || text.at(4) != ' ') { return false }
    if(text.at(7) != ' ' || text.at(11) != ' ' || text.at(16) != ' ') { return false }
    if(text.at(19) != ':' || text.at(22) != ':') { return false }
    if(!text_equals(text.subview(26u, 29u), std::string_view("GMT"))) { return false }
    var digits_ok = true
    var digit_at : [8]size_t = [5u, 6u, 12u, 13u, 14u, 15u, 17u, 18u]
    var i : size_t = 0
    while(i < 8u) {
        var c = text.at(digit_at[i])
        if(c < '0' || c > '9') { digits_ok = false }
        i = i + 1u
    }
    if(!digits_ok) { return false }
    var time_ok = true
    var time_at : [6]size_t = [17, 18, 20, 21, 23, 24]
    i = 0
    while(i < 6u) {
        var c = text.at(time_at[i])
        if(c < '0' || c > '9') { time_ok = false }
        i = i + 1u
    }
    return time_ok
}

} // namespace http
