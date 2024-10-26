// Copyright (c) Qinetik 2024.

#include "LocationManager.h"
#include <stdexcept>

unsigned int LocationManager::encodeFile(const std::string& filePath) {
    auto itr = file_paths.find(filePath);
    if(itr == file_paths.end()) {
        const auto s = file_paths.size();
        file_paths[filePath] = true;
        return s;
    } else {
        return itr - file_paths.begin();
    }
}

std::string_view LocationManager::getPathForFileId(unsigned int fileId) {
    return (file_paths.begin() + fileId)->first;
}

uint64_t LocationManager::addLocation(uint32_t fileId, uint32_t lineStart, uint32_t charStart, uint32_t lineEnd, uint32_t charEnd) {
    // Validate values against bit constraints
    if (
            fileId <= MAX_FILE_ID &&
            lineStart <= MAX_LINE_START &&
            charStart <= MAX_CHAR_START &&
            (lineEnd - lineEnd) <= MAX_LINE_END_OFFSET &&
            charEnd <= MAX_CHAR_END
    ) {
#ifdef DEBUG
        if(lineEnd < lineStart || (lineEnd - lineStart) >= MAX_LINE_END_OFFSET) {
            throw std::runtime_error("invalid line end provided to the addLocation");
        }
#endif
        uint64_t location = 0;
        location |= static_cast<uint64_t>(fileId) << (FILE_ID_SHIFT_BITS);
        location |= static_cast<uint64_t>(lineStart) << (LINE_START_SHIFT_BITS);
        location |= static_cast<uint64_t>(charStart) << (CHAR_START_SHIFT_BITS);
        location |= static_cast<uint64_t>(lineEnd - lineStart) << CHAR_END_BITS;
        location |= static_cast<uint64_t>(charEnd);
        return location;
    } else {
        // Store in vector if it doesn't fit
        uint64_t index = locations.size();
        locations.emplace_back(fileId, lineStart, charStart, lineEnd, charEnd);
        return INDICATOR_BIT_MASK | index; // Mark as an index with the indicator bit
    }
}

uint32_t LocationManager::getLineStartFast(SourceLocation loc) {
    const auto data = loc.encoded;
    if (data & INDICATOR_BIT_MASK) { // Indicator bit check
        uint64_t index = data & NOT_INDICATOR_BIT_MASK;
#ifdef DEBUG
        if (index >= locations.size()) {
            throw std::out_of_range("Location index out of range.");
        }
#endif
        return locations[index].lineStart;
    } else {
        return (data >> (LINE_START_SHIFT_BITS)) & MAX_LINE_START;
    }
}

LocationManager::LocationData LocationManager::getLocation(uint64_t data) const {
    if (data & INDICATOR_BIT_MASK) { // Indicator bit check
        uint64_t index = data & NOT_INDICATOR_BIT_MASK;
#ifdef DEBUG
        if (index >= locations.size()) {
            throw std::out_of_range("Location index out of range.");
        }
#endif
        return locations[index];
    } else {
        const auto lineStart = (data >> (LINE_START_SHIFT_BITS)) & MAX_LINE_START;
        return LocationManager::LocationData {
                .fileId = static_cast<uint32_t>((data >> (FILE_ID_SHIFT_BITS)) & MAX_FILE_ID),
                .lineStart = static_cast<uint32_t>(lineStart),
                .charStart = static_cast<uint32_t>((data >> (CHAR_START_SHIFT_BITS)) & MAX_CHAR_START),
                .lineEnd = static_cast<uint32_t>(lineStart + ((data >> CHAR_END_BITS) & MAX_LINE_END_OFFSET)),
                .charEnd =static_cast<uint32_t>(data & MAX_CHAR_END)
        };
    }
}