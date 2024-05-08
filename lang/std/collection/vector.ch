// Copyright (c) Qinetik 2024.
import "../std.ch"

// Define the vector structure
struct Vector {
    var data : void**;    // Pointer to an array of void pointers
    var size : size_t;    // Current number of elements in the vector
    var capacity : size_t; // Total capacity of the vector
};

// Function to initialize a vector
func vector_init(size_t initial_capacity) : Vector* {
    var vec = malloc(sizeof(Vector)) as Vector*;
    if (vec == null) {
        perror("Memory allocation error");
        exit(1);
    }
    vec.data = malloc(initial_capacity * sizeof(void *)) as void**;
    if (vec.data == null) {
        perror("Memory allocation error");
        exit(1);
    }
    vec.size = 0;
    vec.capacity = initial_capacity;
    return vec;
}

// Function to add an element to the vector
func vector_add(vec : Vector*, element : void*) {
    if (vec.size == vec.capacity) {
        // Double the capacity if the vector is full
        vec.capacity *= 2;
        vec.data = realloc(vec.data, vec.capacity * sizeof(void *)) as void**;
        if (vec->data == null) {
            perror("Memory allocation error");
            exit(EXIT_FAILURE);
        }
    }
    vec.data[vec.size++] = element;
}

// Function to get an element from the vector at a specific index
func vector_get(vec : Vector*, index : size_t) : void* {
    if (index < vec.size) {
        return vec.data[index];
    } else {
        return null;
    }
}

// Function to free the memory allocated for the vector
func vector_free(vec : Vector*) {
    free(vec.data);
    free(vec);
}