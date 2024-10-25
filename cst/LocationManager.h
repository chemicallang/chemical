// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>

class LocationManager {
public:

    struct LocationData {
        uint32_t fileId;
        uint32_t lineStart;
        uint32_t charStart;
        uint32_t lineEnd;
        uint32_t charEnd;
    };

private:
    std::vector<LocationData> locations;
public:

    // Bit allocations and maximum values for validation
    static constexpr uint32_t FILE_ID_BITS = 10;
    static constexpr uint32_t LINE_START_BITS = 18;
    static constexpr uint32_t CHAR_START_BITS = 12;
    static constexpr uint32_t LINE_END_OFFSET_BITS = 11;
    static constexpr uint32_t CHAR_END_BITS = 12;

    static constexpr uint32_t MAX_FILE_ID = (1U << FILE_ID_BITS) - 1;
    static constexpr uint32_t MAX_LINE_START = (1U << LINE_START_BITS) - 1;
    static constexpr uint32_t MAX_CHAR_START = (1U << CHAR_START_BITS) - 1;
    static constexpr uint32_t MAX_LINE_END_OFFSET = (1U << LINE_END_OFFSET_BITS) - 1;
    // even though offset from line start is stored, we add this offset to get the maximum
    // line end number that would be possible after decoding, this avoids us calculating the maximum lineEnd value
    static constexpr uint32_t MAX_LINE_END = MAX_LINE_START + MAX_LINE_END_OFFSET;
    static constexpr uint32_t MAX_CHAR_END = (1U << CHAR_END_BITS) - 1;

    static constexpr uint64_t INDICATOR_BIT_MASK = 1ULL << 63;
    static constexpr uint64_t NOT_INDICATOR_BIT_MASK = ~INDICATOR_BIT_MASK;

    static constexpr uint64_t FILE_ID_SHIFT_BITS = LINE_START_BITS + CHAR_START_BITS + LINE_END_OFFSET_BITS + CHAR_END_BITS;
    static constexpr uint64_t LINE_START_SHIFT_BITS = CHAR_START_BITS + LINE_END_OFFSET_BITS + CHAR_END_BITS;
    static constexpr uint64_t CHAR_START_SHIFT_BITS = LINE_END_OFFSET_BITS + CHAR_END_BITS;

    uint64_t addLocation(
            uint32_t fileId,
            uint32_t lineStart,
            uint32_t charStart,
            uint32_t lineEnd,
            uint32_t charEnd
    );

    [[nodiscard]]
    LocationData getLocation(uint64_t data) const;

};