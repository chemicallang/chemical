#pragma once

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <string>
#include <string_view>
#include <charconv>

/**
 * this exists to abstract away and keep separate everything we need
 * to write a single buffer to file, when doing c translation
 * everything in this class needs to be fast
 */
class BufferedWriter {
protected:

    char* buf_;
    size_t cap_;
    size_t pos_;

public:

    /**
     * this can be used to create a buffered writer from a stack allocated array
     */
    explicit BufferedWriter(char* buf, size_t cap) noexcept : buf_(buf), cap_(cap), pos_(0) {

    }

    // Default 4 MiB initial cap
    explicit BufferedWriter(size_t initial_capacity = (4 << 20)) noexcept
            : buf_(nullptr), cap_(0), pos_(0)
    {
        if (initial_capacity == 0) initial_capacity = (4 << 20);
        buf_ = static_cast<char*>(std::malloc(initial_capacity));
        if (buf_) cap_ = initial_capacity;
    }

    BufferedWriter(BufferedWriter&& o) noexcept
            : buf_(o.buf_), cap_(o.cap_), pos_(o.pos_)
    {
        o.buf_ = nullptr; o.cap_ = 0; o.pos_ = 0;
    }
    BufferedWriter& operator=(BufferedWriter&& o) noexcept {
        if (this != &o) {
            if (buf_) std::free(buf_);
            buf_ = o.buf_; cap_ = o.cap_; pos_ = o.pos_;
            o.buf_ = nullptr; o.cap_ = 0; o.pos_ = 0;
        }
        return *this;
    }

    BufferedWriter(const BufferedWriter&) = delete;
    BufferedWriter& operator=(const BufferedWriter&) = delete;

    ~BufferedWriter() noexcept {
        if (buf_) std::free(buf_);
    }

    // Reserve at least new_capacity (may allocate). Returns true on success.
    bool reserve_total(size_t new_capacity) noexcept {
        if (new_capacity <= cap_) return true;
        char* nb = static_cast<char*>(std::realloc(buf_, new_capacity));
        if (!nb) return false;
        buf_ = nb;
        cap_ = new_capacity;
        return true;
    }

    // Ensure there's space for extra bytes (pos_ + extra <= cap_).
    // If not, grows buffer using doubling strategy until it fits.
    // Returns true on success (might allocate), false on allocation failure.
    void ensure_total_capacity_for(size_t extra) noexcept {
        size_t need = pos_ + extra;
        if (need <= cap_) return;
        // start with either current cap*2 or default base
        size_t want = (cap_ ? cap_ * 2 : (4 << 20));
        while (want < need) {
            // prevent overflow
            if (want > (SIZE_MAX / 2)) { want = need; break; }
            want *= 2;
        }
        char* nb = static_cast<char*>(std::realloc(buf_, want));
        if (!nb) {
            printf("couldn't allocate memory %zu", extra);
            abort();
            return;
        }
        buf_ = nb;
        cap_ = want;
    }

    // --- Appends (grow if necessary). Return true on success ---
    void append(const char* s, size_t n) noexcept {
        ensure_total_capacity_for(n);
        std::memcpy(buf_ + pos_, s, n);
        pos_ += n;
    }

    void append_char(char c) noexcept {
        ensure_total_capacity_for(1);
        buf_[pos_++] = c;
    }

    // Fast digit count for u64 using branch ranges (very cheap)
    static inline size_t u64_digits(uint64_t v) noexcept {
        if (v < 10ULL) return 1;
        if (v < 100ULL) return 2;
        if (v < 1000ULL) return 3;
        if (v < 10000ULL) return 4;
        if (v < 100000ULL) return 5;
        if (v < 1000000ULL) return 6;
        if (v < 10000000ULL) return 7;
        if (v < 100000000ULL) return 8;
        if (v < 1000000000ULL) return 9;
        if (v < 10000000000ULL) return 10;
        if (v < 100000000000ULL) return 11;
        if (v < 1000000000000ULL) return 12;
        if (v < 10000000000000ULL) return 13;
        if (v < 100000000000000ULL) return 14;
        if (v < 1000000000000000ULL) return 15;
        if (v < 10000000000000000ULL) return 16;
        if (v < 100000000000000000ULL) return 17;
        if (v < 1000000000000000000ULL) return 18;
        if (v < 10000000000000000000ULL) return 19;
        return 20;
    }

    // Write unsigned integer quickly (no temporaries): compute digits, ensure space,
    // then write digits backward directly into buffer.
    void append_unsigned_u64(uint64_t v) noexcept {
        if (v == 0) return append_char('0');
        size_t digs = u64_digits(v);
        ensure_total_capacity_for(digs);
        size_t end = pos_ + digs;
        // fill backwards
        size_t i = end;
        while (v > 0) {
            uint32_t digit = static_cast<uint32_t>(v % 10ULL);
            buf_[--i] = char('0' + digit);
            v /= 10ULL;
        }
        pos_ = end;
    }

    void append_signed_i64(int64_t v) noexcept {
        if (v < 0) {
            // compute unsigned magnitude safely (handle INT64_MIN)
            uint64_t uv = static_cast<uint64_t>(-(v + 1)) + 1ULL;
            // 1 extra byte for '-'
            size_t digs = u64_digits(uv);
            ensure_total_capacity_for(digs + 1);
            buf_[pos_++] = '-';
            // write digits
            size_t end = pos_ + digs;
            size_t i = end;
            while (uv > 0) {
                uint32_t digit = static_cast<uint32_t>(uv % 10ULL);
                buf_[--i] = char('0' + digit);
                uv /= 10ULL;
            }
            pos_ = end;
        } else {
            return append_unsigned_u64(static_cast<uint64_t>(v));
        }
    }

    void append_float(float v) noexcept {
        // max length for float in decimal ~ 48 chars (scientific)
        ensure_total_capacity_for(48);
        char* start = buf_ + pos_;
        char* end   = buf_ + cap_;
        auto [ptr, ec] = std::to_chars(start, end, v, std::chars_format::general, 9);
        if (ec == std::errc()) {
            pos_ += (ptr - start);
        } else {
            // fallback: shouldn't happen unless buffer too small
            append("NaN", 3);
        }
    }

    void append_double(double v) noexcept {
        // max length for double in decimal ~ 64 chars (scientific)
        ensure_total_capacity_for(64);
        char* start = buf_ + pos_;
        char* end   = buf_ + cap_;
        auto [ptr, ec] = std::to_chars(start, end, v, std::chars_format::general, 17);
        if (ec == std::errc()) {
            pos_ += (ptr - start);
        } else {
            append("NaN", 3);
        }
    }

    bool append_file(const char* path) noexcept {
        if (!path) return false;
        FILE* f = std::fopen(path, "rb");
        if (!f) return false;
        constexpr size_t CHUNK = 16 * 1024; // when we need more space, grow by at least this much
        while (true) {
            size_t avail = (cap_ > pos_) ? (cap_ - pos_) : 0;
            if (avail == 0) {
                // ask for at least CHUNK bytes more; ensure_total_capacity_for will grow (or abort on OOM per your impl)
                ensure_total_capacity_for(CHUNK);
                avail = (cap_ > pos_) ? (cap_ - pos_) : 0;
                if (avail == 0) { std::fclose(f); return false; } // defensive: ensure_total_capacity_for failed silently
            }

            size_t got = std::fread(buf_ + pos_, 1, avail, f);
            if (got == 0) break; // EOF or error
            pos_ += got;
        }
        bool ok = !std::ferror(f);
        std::fclose(f);
        return ok;
    }

    // Thin wrappers mapping to requested names/types
    inline void append_int(int32_t v) noexcept { return append_signed_i64(static_cast<int64_t>(v)); }
    inline void append_long(long v) noexcept { return append_signed_i64(static_cast<int64_t>(v)); }
    inline void append_long_long(long long v) noexcept { return append_signed_i64(static_cast<int64_t>(v)); }

    inline void append_unsigned_int(unsigned int v) noexcept { return append_unsigned_u64(static_cast<uint64_t>(v)); }
    inline void append_unsigned_long(unsigned long v) noexcept { return append_unsigned_u64(static_cast<uint64_t>(v)); }
    inline void append_un_long_long(unsigned long long v) noexcept { return append_unsigned_u64(static_cast<uint64_t>(v)); }
    inline void append_unsigned_long_long(unsigned long long v) noexcept { return append_un_long_long(v); }

    // Expose raw pointer & sizes
    inline const char* data() const noexcept { return buf_; }
    inline size_t size() const noexcept { return pos_; }
    inline size_t capacity() const noexcept { return cap_; }

    inline void clear() noexcept { pos_ = 0; }

    /**
     * appends a single \0, which means it ends
     */
    inline void finalize() noexcept {
        append_char('\0');
    }

    inline const char* current_pos_data() const noexcept {
        return buf_ + pos_;
    }

    inline std::string_view finalized_std_view() noexcept {
        append_char('\0');
        return { data(), size() - 1 };
    }

    inline std::string_view to_std_view() noexcept {
        return { data(), size() };
    }

    inline chem::string_view to_chem_view() noexcept {
        return { data(), size() };
    }

    // Flush to file with one fwrite. Returns true on success.
    bool flush_to_file(const char* path) const noexcept {
        if (!buf_) return false;
        FILE* f = std::fopen(path, "wb");
        if (!f) return false;
        size_t written = std::fwrite(buf_, 1, pos_, f);
        std::fclose(f);
        return written == pos_;
    }

    template <std::size_t N>
    inline constexpr BufferedWriter& operator<<(const char (&str)[N]) noexcept {
        append(str, N - 1); // exclude null terminator
        return *this;
    }

    inline BufferedWriter& operator<<(const std::string_view& sv) noexcept {
        append(sv.data(), sv.size());
        return *this;
    }

    inline BufferedWriter& operator<<(const chem::string_view& sv) noexcept {
        append(sv.data(), sv.size());
        return *this;
    }

    inline BufferedWriter& operator<<(const std::string& sv) noexcept {
        append(sv.data(), sv.size());
        return *this;
    }

    inline BufferedWriter& operator<<(char c) noexcept {
        append_char(c);
        return *this;
    }

    // signed integrals (any width)
    template<typename T> requires (std::is_integral_v<T> && std::is_signed_v<T>)
    inline BufferedWriter& operator<<(T v) noexcept {
        append_signed_i64(static_cast<int64_t>(v));
        return *this;
    }

    // unsigned integrals (any width)
    template<typename T> requires (std::is_integral_v<T> && std::is_unsigned_v<T>)
    inline BufferedWriter& operator<<(T v) noexcept {
        append_unsigned_u64(static_cast<uint64_t>(v));
        return *this;
    }

    inline BufferedWriter& operator<<(float c) noexcept {
        append_float(c);
        return *this;
    }

    inline BufferedWriter& operator<<(double c) noexcept {
        append_double(c);
        return *this;
    }


};

template <size_t N>
class ScratchString : public BufferedWriter {
public:
    char my_buf_[N];
    inline ScratchString() noexcept : BufferedWriter(&my_buf_[0], N) {

    }
    inline operator chem::string_view() const noexcept {
        return chem::string_view(data(), size());
    }
    inline operator std::string_view() const noexcept {
        return std::string_view(data(), size());
    }
    inline ~ScratchString() {
        if(buf_ == &my_buf_[0]) {
            buf_ = nullptr;
        }
    }
};