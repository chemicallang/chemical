// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "SourceLocation.h"
#include "integration/common/Position.h"
#include "ordered_map.h"
#include <mutex>

class LocationManager {
public:

    struct LocationData {
        uint32_t fileId;
        uint32_t lineStart;
        uint32_t charStart;
        uint32_t lineEnd;
        uint32_t charEnd;
    };

    struct LocationPosData {
        uint32_t fileId;
        Position start;
        Position end;
    };

private:

    /**
     * locations that are too large and cannot be stored in 63 bits
     * we store those locations on this vector and an index is of 63 bits
     * is provided to the user where the most significant bit is set to 1 to indicate
     * it's an index
     */
    std::vector<LocationData> locations;

    /**
     * the file paths are stored on this ordered map, to only store a single instance of the
     * path
     */
    tsl::ordered_map<std::string, bool> file_paths;

    /**
     * the mutex is used when encoding file paths
     * to ensure multiple invocations are synchronized
     */
    std::mutex file_mutex;

    /**
     * the mutex is used when adding locations because
     * locations can be added concurrently, only large locations use the mutex
     * other are encoded concurrently
     */
    std::mutex location_mutex;

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

    /**
     * encode the given file to get a file id
     */
    unsigned int encodeFile(const std::string& filePath);

    /**
     * the file id can be used to get absolute path to the file
     */
    std::string_view getPathForFileId(unsigned int fileId);

    /**
     * get encoded location data from given location
     */
    uint64_t addLocation(
            uint32_t fileId,
            uint32_t lineStart,
            uint32_t charStart,
            uint32_t lineEnd,
            uint32_t charEnd
    );

    /**
     * get location data for the given encoded location
     */
    [[nodiscard]]
    LocationData getLocation(uint64_t data) const;

    /**
     * get location using the source location object
     */
    [[nodiscard]]
    inline LocationData getLocation(SourceLocation loc) const {
        return getLocation(loc.encoded);
    }

    /**
     * get location position data for the given encoded location
     */
    [[nodiscard]]
    LocationPosData getLocationPos(SourceLocation loc) const {
        const auto data = getLocation(loc.encoded);
        return LocationPosData { data.fileId, data.lineStart, data.charStart, data.lineEnd, data.charEnd };
    }

    /**
     * method only should be used if not using any other method for a single location
     */
    uint32_t getLineStartFast(SourceLocation data);

};