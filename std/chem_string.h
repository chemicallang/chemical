// Copyright (c) Qinetik 2024.

#pragma once

#include <cstdlib>
#include <cstring>
#include <string>
#include "chem_string_view.h"

namespace chem {

    constexpr int STR_BUFF_SIZE = 16;

    /**
     * the string used in chemical
     */
    struct string {

        union {
            struct {
                const char* data;
                size_t length;
            } constant;
            struct {
                char* data;
                size_t length;
                size_t capacity;
            } heap;
            struct {
                char buffer[STR_BUFF_SIZE];
                unsigned char length;
            } sso;
        } storage;
        char state;

        static inline chem::string make_view(const std::string_view& view) {
            chem::string str;
            str.storage.constant.data = view.data();
            str.storage.constant.length = view.size();
            str.state = '0';
            return str;
        }

        constexpr explicit string(const char *Str) {
            storage.constant.data = Str;
            const size_t length = Str ?
                           // GCC 7 doesn't have constexpr char_traits. Fall back to __builtin_strlen.
                           #if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 8
                           __builtin_strlen(Str)
                           #else
                           std::char_traits<char>::length(Str)
                           #endif
                               : 0;
            storage.constant.length = length;
            state = '0';
        }

        constexpr string() {
            storage.constant.data = nullptr;
            storage.constant.length = 0;
            state = '0';
        }

        string(string& str) = delete;

        void _own(string& other) noexcept {
            state = other.state;
            switch(other.state) {
                case '0':
                    storage.constant.data = other.storage.constant.data;
                    storage.constant.length = other.storage.constant.length;
                    break;
                case '1':
                    memcpy(storage.sso.buffer, other.storage.sso.buffer, STR_BUFF_SIZE);
                    storage.sso.length = other.storage.sso.length;
                    break;
                case '2':
                    storage.heap.data = other.storage.heap.data;
                    storage.heap.length = other.storage.heap.length;
                    storage.heap.capacity = other.storage.heap.capacity;
                    other.state = '0';
                    other.storage.constant.data = storage.heap.data;
                    other.storage.constant.length = storage.heap.length;
                    break;
            }
        }

        inline string(string&& other) noexcept {
            _own(other);
        }

        string& operator=(string&& other) noexcept {
            if(state == '2') {
                free(storage.heap.data);
            }
            _own(other);
            return *this;
        }

        explicit string(const char* value, const size_t length) {
            storage.constant.data = value;
            storage.constant.length = length;
            state = '0';
            ensure_mut(length + 2);
        }

        explicit string(const chem::string& value) {
            const auto length = value.size();
            storage.constant.data = value.data();
            storage.constant.length = length;
            state = '0';
            if(value.state != '0') {
                ensure_mut(length + 2);
            }
        }

        explicit string(const std::string_view& view) : string(view.data(), view.size()) {

        }

        explicit string(const std::string& value) : string(value.data(), value.size()) {

        }

        explicit string(char* value) : string(value, strlen(value)){

        }

        [[nodiscard]]
        bool equals(const string& other) const {
            const size_t self_size = size();
            return ((self_size == other.size()) && (memcmp(data(), other.data(), self_size) == 0));
        }

        void move_const_to_buffer(){
            const char* data = storage.constant.data;
            size_t length = storage.constant.length;
            if(data) {
                for (int i = 0; (i < length); i += 1) {
                    storage.sso.buffer[i] = data[i];
                }
            }
            storage.sso.buffer[length] = '\0';
            storage.sso.length = length;
            state = '1';
        }

        void move_data_to_heap(const char* from_data, const size_t length, const size_t capacity){
            char* data = ((char*) malloc(capacity));
            int i = 0;
            while((i < length)) {
                data[i] = from_data[i];
                i += 1;
            }
            data[i] = '\0';
            storage.heap.data = data;
            storage.heap.length = length;
            storage.heap.capacity = capacity;
            state = '2';
        }

        void resize(const size_t new_capacity){
            char* data = ((char*) realloc(storage.heap.data, new_capacity));
            data[storage.heap.length] = '\0';
            storage.heap.data = data;
            storage.heap.capacity = new_capacity;
        }

        void ensure_mut(const size_t capacity){
            if((((state == '0') || (state == '1')) && (capacity < STR_BUFF_SIZE))){
                if(state == '0'){
                    move_const_to_buffer();
                }
            } else {
                if(state == '0'){
                    move_data_to_heap(storage.constant.data, storage.constant.length, capacity);
                }else if(state == '1'){
                    move_data_to_heap(&storage.sso.buffer[0], storage.sso.length, capacity);
                }else if(storage.heap.capacity <= capacity){
                    resize(capacity);
                }
            }
        }

        // will paste the given string at given index, requires capacity
        void set(const size_t index, const char* value, const size_t len) {
            auto d = mutable_data();
            size_t i = 0;
            while(i < len) {
                d[i + index] = value[i];
                i++;
            }
        }

        // will make string start at given index, requires that capacity exists
        void offset_data(char* const data, const unsigned int start) {
            const auto len = size();
            long i = (long) (start + len);
            const auto start_plus_one = start + 1;
            while(i >= 0) {
                data[i] = data[i - start_plus_one];
                i--;
            }
        }

        void prepend(const char* value, const size_t len) {
            const auto cur_len = size();
            const auto cap = capacity();
            if(cur_len + len < cap) {
                const auto d = mutable_data();
                offset_data(d,len);
                set(0, value, len);
            } else {
                // TODO
            }
        }

        void append(const char* value, size_t len){
            ensure_mut((size() + len + 10));
            size_t i = 0;
            while((i < len)) {
                append(value[i]);
                i += 1;
            }
        }

        inline void append(const char* value){
            append(value, strlen(value));
        }

        inline void append(const string* value){
            append(value->data(), value->size());
        }

        inline void append(const std::string_view& str) {
            append(str.data(), str.size());
        }

        inline void append(const std::string& str) {
            append(str.data(), str.size());
        }

        inline void append(const chem::string_view& str) {
            append(str.data(), str.size());
        }

        [[nodiscard]]
        string copy() const {
            return substring(0, size());
        }

        [[nodiscard]]
        string substring(const size_t start, const size_t end, const size_t new_cap) const {
            struct string s((const char*) nullptr);
            const size_t actual_len = (end - start);
            if((actual_len < STR_BUFF_SIZE)){
                s.state = '1';
                s.storage.sso.length = actual_len;
                const char* d = data();
                for(int i = 0;(i < actual_len);i += 1){
                    s.storage.sso.buffer[i] = d[(start + i)];
                }
                s.storage.sso.buffer[actual_len] = '\0';
            } else {
                s.state = '2';
                char* new_heap = ((char*) malloc(new_cap));
                const char* d = data();
                for(int i = 0;(i < actual_len);i += 1){
                    new_heap[i] = d[(start + i)];
                }
                s.storage.heap.data = new_heap;
                s.storage.heap.data[actual_len] = '\0';
                s.storage.heap.length = actual_len;
                s.storage.heap.capacity = new_cap;
            }
            return s;
        }

        [[nodiscard]]
        string substring(const size_t start, const size_t end) const {
            return substring(start, end, end - start + 5);
        }

        [[nodiscard]]
        string substring(const size_t start) const {
            return substring(start, size());
        }

        void append(const char value) {
            const size_t length = size();
            if((((state == '0') || (state == '1')) && (length < (STR_BUFF_SIZE - 1)))){
                if(state == '0'){
                    move_const_to_buffer();
                }
                storage.sso.buffer[length] = value;
                storage.sso.buffer[(length + 1)] = '\0';
                storage.sso.length = (length + 1);
            } else {
                if(state == '0'){
                    move_data_to_heap(storage.constant.data, length, (length * 2));
                }else if(state == '1'){
                    move_data_to_heap(&storage.sso.buffer[0], length, (length * 2));
                }else if((storage.heap.capacity <= (length + 2))){
                    resize((storage.heap.capacity * 2));
                }
                storage.heap.data[length] = value;
                storage.heap.data[(length + 1)] = '\0';
                storage.heap.length = (length + 1);
            }
        }

        // will cause heap allocation
        inline void reserve(std::size_t capacity) {
            ensure_mut(capacity);
        }

        [[nodiscard]]
        size_t size() const {
            switch(state) {
                case '0':
                    return storage.constant.length;
                case '1':
                    return storage.sso.length;
                case '2':
                    return storage.heap.length;
                default:
                    return 0;
            }
        }

        char get(const size_t index){
            switch(state) {
                case '0':
                    return storage.constant.data[index];
                case '1':
                    return storage.sso.buffer[index];
                case '2':
                    return storage.heap.data[index];
                default:
                    return '\0';
            }
        }

        void set(const size_t index, const char value){
            switch(state) {
                case '0':
                    move_const_to_buffer();
                    storage.sso.buffer[index] = value;
                    break;
                case '1':
                    storage.sso.buffer[index] = value;
                    break;
                case '2':
                    storage.heap.data[index] = value;
                    break;
            }
        }

        [[nodiscard]]
        bool empty() const {
            return size() == 0;
        }

        [[nodiscard]]
        size_t capacity() const {
            switch(state) {
                case '0':
                    return storage.constant.length;
                case '1':
                    return STR_BUFF_SIZE;
                case '2':
                    return storage.heap.capacity;
                default:
                    return 0;
            }
        }

        [[nodiscard]]
        const char* data() const {
            switch(state) {
                case '0':
                    return storage.constant.data;
                case '1':
                    return &storage.sso.buffer[0];
                case '2':
                    return storage.heap.data;
                default:
                    return nullptr;
            }
        }

        [[nodiscard]]
        char* mutable_data() {
            switch(state) {
                case '0':
                    move_const_to_buffer();
                    return &storage.sso.buffer[0];
                case '1':
                    return &storage.sso.buffer[0];
                case '2':
                    return storage.heap.data;
                default:
                    return nullptr;
            }
        }

        [[nodiscard]]
        std::string to_std_string() const {
            return std::string(to_view());
        }

        [[nodiscard]]
        std::string_view to_view() const {
            return { data(), size() };
        }

        [[nodiscard]]
        bool ends_with(const std::string& data) const {
            return to_std_string().ends_with(data);
        }

        [[nodiscard]]
        bool ends_with(const chem::string& str) const {
            return to_std_string().ends_with(str.data());
        }

        ~string(){
            if(state == '2'){
                free(storage.heap.data);
            }
        }

        bool operator ==(const chem::string& other) const{
            return equals(other);
        }

        char operator[](const size_t index){
            return get(index);
        }

        chem::string operator+(const chem::string& str) const {
            auto s = copy();
            s.append(str.data());
            return s;
        }

        chem::string operator+(const char* str) const {
            auto s = copy();
            s.append(str);
            return s;
        }

        // Friend function to overload the << operator
        friend std::ostream& operator<<(std::ostream& os, const chem::string& str);

    };


}

std::ostream& chem::operator<<(std::ostream& os, const chem::string& str);