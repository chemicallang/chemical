// Copyright (c) Qinetik 2024.

#pragma once

#include <cstdlib>
#include <cstring>
#include <string>

namespace chem {

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
                char buffer[16];
                unsigned char length;
            } sso;
        } storage;
        char state;

        constexpr string(const char *Str) {
            storage.constant.data = Str;
            storage.constant.length = Str ?
                                     // GCC 7 doesn't have constexpr char_traits. Fall back to __builtin_strlen.
                                     #if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 8
                                     __builtin_strlen(Str)
                                     #else
                                     std::char_traits<char>::length(Str)
                                     #endif
                                         : 0;
            state = '0';
        }

        constexpr string(const char *Str, size_t length) {
            storage.constant.data = Str;
            storage.constant.length = length;
            state = '0';
        }

        string(string& str) = delete;

        string(string&& other) noexcept {
            state = other.state;
            switch(other.state) {
                case '0':
                    storage.constant.data = other.storage.constant.data;
                    storage.constant.length = other.storage.constant.length;
                    break;
                case '1':
                    memcpy(storage.sso.buffer, other.storage.sso.buffer, 16);
                    storage.constant.length = other.storage.constant.length;
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

        string(char* value, size_t length) {
            storage.constant.data = value;
            storage.constant.length = length;
            state = '0';
        }

        string(char* value) : string(value, strlen(value)){

        }

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

        [[nodiscard]]
        bool equals(const string& other) const {
            size_t self_size = size();
            return ((self_size == other.size()) && (memcmp(data(), other.data(), self_size) == 0));
        }

        bool operator ==(const string& other) const{
            return equals(other);
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

        void move_data_to_heap(const char* from_data, size_t length, size_t capacity){
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

        void resize(size_t new_capacity){
            char* data = ((char*) realloc(storage.heap.data, new_capacity));
            int i = 0;
            while((i < storage.heap.length)) {
                data[i] = storage.heap.data[i];
                i += 1;
            }
            data[i] = '\0';
            storage.heap.data = data;
            storage.heap.capacity = new_capacity;
        }

        void ensure_mut(size_t length){
            if((((state == '0') || (state == '1')) && (length < 16))){
                if(state == '0'){
                    move_const_to_buffer();
                }
            } else {
                if(state == '0'){
                    move_data_to_heap(storage.constant.data, storage.constant.length, length);
                }else if(state == '1'){
                    move_data_to_heap(&storage.sso.buffer[0], storage.sso.length, length);
                }else if(storage.heap.capacity <= length){
                    resize(length);
                }
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

        // will paste the given string at given index, requires capacity
        void set(size_t index, const char* value, const size_t len) {
            auto d = mutable_data();
            size_t i = 0;
            while(i < len) {
                d[i + index] = value[i];
                i++;
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

        char operator[](size_t index){
            return get(index);
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
            ensure_mut((size() + len));
            size_t i = 0;
            while((i < len)) {
                append(value[i]);
                i += 1;
            }
        }

        void append(const char* value){
            append(value, strlen(value));
        }

        void append(struct string* value){
            append(value->data(), value->size());
        }

        [[nodiscard]]
        string copy() const {
            string s((const char*) nullptr);
            s.state = state;
            switch(state) {
                case '0':
                    s.storage.constant.data = storage.constant.data;
                    s.storage.constant.length = storage.constant.length;
                    break;
                case '1':
                    s.storage.sso.length = storage.sso.length;
                    memcpy(s.storage.sso.buffer, storage.sso.buffer, storage.sso.length);
                    break;
                case '2':
                    char* new_heap = ((char*) malloc(storage.heap.capacity));
                    memcpy(new_heap, storage.heap.data, storage.heap.length);
                    s.storage.heap.data = new_heap;
                    s.storage.heap.length = storage.heap.length;
                    s.storage.heap.capacity = storage.heap.capacity;
                    break;
            }
            return s;
        }

        string substring(size_t start, size_t end) {
            struct string s((const char*) nullptr);
            size_t actual_len = (end - start);
            if((actual_len < 16)){
                s.state = '1';
                s.storage.sso.length = actual_len;
                const char* d = data();
                for(int i = 0;(i < actual_len);i += 1){
                    s.storage.sso.buffer[i] = d[(start + i)];
                }
                s.storage.sso.buffer[actual_len] = '\0';
            } else {
                s.state = '2';
                size_t new_cap = (actual_len * 2);
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

        void append(char value) {
            size_t length = size();;
            if((((state == '0') || (state == '1')) && (length < 16))){
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
                }else if((storage.heap.capacity <= (length + 1))){
                    resize((storage.heap.capacity * 2));
                }
                storage.heap.data[length] = value;
                storage.heap.data[(length + 1)] = '\0';
                storage.heap.length = (length + 1);
            }
        }

        [[nodiscard]]
        size_t capacity() const {
            switch(state) {
                case '0':
                    return storage.constant.length;
                case '1':
                    return 16;
                case '2':
                    return storage.heap.capacity;
                default:
                    return 0;
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

        std::string to_std_string () {
            const auto d = data();
            if(!d) return "";
            return { d };
        }

        ~string(){
            if(state == '2'){
                free(storage.heap.data);
            }
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

        friend chem::string operator+(const char* const left, chem::string& right) {
            chem::string s(left);
            s.append(right.data());
            return s;
        }

    };


}