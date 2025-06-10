// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <unordered_map>
#include <list>
#include <utility>

// A limited-size unordered map with LRU eviction policy.
// When the capacity is exceeded, the least recently accessed item is removed.
// Accessing an item (get or operator[]) promotes it to most recently used.

template <
        typename Key,
        typename T,
        typename Hash = std::hash<Key>,
        typename KeyEqual = std::equal_to<Key>
>
class LRUCache {
public:
    using value_type = std::pair<Key, T>;

    explicit LRUCache(size_t capacity) : _capacity(capacity) {}

    // Insert or update a value. Promotes key to most recently used.
    void put(const Key& key, const T& value) {
        auto it = _map.find(key);
        if (it != _map.end()) {
            // Update existing
            it->second.first = value;
            touch(it);
        } else {
            // Insert new
            if (_map.size() == _capacity) {
                // Evict least recently used (tail of list)
                const Key& lru = _lru_list.back();
                _map.erase(lru);
                _lru_list.pop_back();
            }
            _lru_list.push_front(key);
            _map.emplace(key, std::make_pair(value, _lru_list.begin()));
        }
    }

    // Try to get a value. Returns nullptr if not found.
    // Promotes key to most recently used on success.
    T* get(const Key& key) {
        auto it = _map.find(key);
        if (it == _map.end())
            return nullptr;
        touch(it);
        return &it->second.first;
    }

    // Check existence without promoting.
    bool contains(const Key& key) const {
        return _map.find(key) != _map.end();
    }

    // Current size
    size_t size() const noexcept {
        return _map.size();
    }

    // Capacity
    size_t capacity() const noexcept {
        return _capacity;
    }

    // Clear all contents
    void clear() {
        _map.clear();
        _lru_list.clear();
    }

private:
    // Move the accessed key to front of the LRU list
    void touch(typename std::unordered_map<Key, std::pair<T, typename std::list<Key>::iterator>, Hash, KeyEqual>::iterator it) {
        _lru_list.splice(_lru_list.begin(), _lru_list, it->second.second);
        it->second.second = _lru_list.begin();
    }

    size_t _capacity;
    std::list<Key> _lru_list; // MRU at front, LRU at back
    std::unordered_map<
            Key,
            std::pair<T, typename std::list<Key>::iterator>,
            Hash,
            KeyEqual
    > _map;
};
