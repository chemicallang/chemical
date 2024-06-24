// Copyright (c) Qinetik 2024.

#pragma once

#include <cstdlib>
#include <cstring>

namespace chemical {

    /**
     * the string used in chemical
     */
    struct String {

        char *data;
        size_t length;
        size_t capacity;

        /**
         * initializes an empty string with initial capacity
         */
        explicit String(unsigned long initial_capacity) {
            data = (char *) malloc(initial_capacity);
            data[0] = '\0';
            length = 0;
            capacity = initial_capacity;
        }

        /**
         * resize the string to a new capacity
         */
        void resize(unsigned long new_capacity) {
            auto new_data = (char *) realloc(data, new_capacity);
            if (new_data) {
                data = new_data;
                capacity = new_capacity;
            }
        }

        /**
         * append a character to a string
         */
        void append_char(char c) {
            if(length + 1 >= capacity) {
                resize(length + 1 + 10);
            }
            data[length] = c;
            length++;
            data[length] = '\0';
        }

        /**
         * append a string to this string, with given length
         */
        void append_str(const char* s, size_t len) {
            if(length + len >= capacity) {
                resize(length + len + 3);
            }
            memcpy(data + length, s, len);
            length += len;
            data[length] = '\0';
        }

        /**
         * append another string to this string
         */
        void append_str(const char *s) {
            size_t len = strlen(s);
            append_str(s, len);
        }

        /**
         * frees the string
         */
        ~String() {
            free(data);
        }

    };


}