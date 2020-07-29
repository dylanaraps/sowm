
/*
This is an implementation of a std::vector like growable array, but in plain
C89 code. The result is a type safe, easy to use, dynamic array that has a
familiar set of operations. Source: https://github.com/eteran/c-vector

The MIT License (MIT)

Copyright (c) 2020 Dylan Araps
Copyright (c) 2015 Evan Teran

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef VEC_H_
#define VEC_H_

#include <assert.h> /* for assert */
#include <stddef.h> /* for size_t */
#include <stdlib.h> /* for malloc/realloc/free */

/**
 * @brief vec_set_capacity - For internal use, set capacity variable.
 * @param vec - the vector
 * @param size - the new capacity to set
 * @return void
 */
#define vec_set_capacity(vec, size)         \
    do {                                    \
        if (vec) {                          \
            ((size_t *)(vec))[-1] = (size); \
        }                                   \
    } while (0)

/**
 * @brief vec_set_size - For internal use, sets the size variable of the vector
 * @param vec - the vector
 * @param size - the new capacity to set
 * @return void
 */
#define vec_set_size(vec, size)             \
    do {                                    \
        if (vec) {                          \
            ((size_t *)(vec))[-2] = (size); \
        }                                   \
    } while (0)

/**
 * @brief vec_capacity - gets the current capacity of the vector
 * @param vec - the vector
 * @return the capacity as a size_t
 */
#define vec_capacity(vec) ((vec) ? ((size_t *)(vec))[-1] : (size_t)0)

/**
 * @brief vec_size - gets the current size of the vector
 * @param vec - the vector
 * @return the size as a size_t
 */
#define vec_size(vec) ((vec) ? ((size_t *)(vec))[-2] : (size_t)0)

/**
 * @brief vec_empty - returns non-zero if the vector is empty
 * @param vec - the vector
 * @return non-zero if empty, zero if non-empty
 */
#define vec_empty(vec) (vec_size(vec) == 0)

/**
 * @brief vec_grow - For internal use, ensure that the vector is >= <count>.
 * @param vec - the vector
 * @param size - the new capacity to set
 * @return void
 */
#define vec_grow(vec, count)                                                    \
    do {                                                                        \
        const size_t vec__sz = (count) * sizeof(*(vec)) + (sizeof(size_t) * 2); \
        if (!(vec)) {                                                           \
            size_t *vec__p = malloc(vec__sz);                                   \
            assert(vec__p);                                                     \
            (vec) = (void *)(&vec__p[2]);                                       \
            vec_set_capacity((vec), (count));                                   \
            vec_set_size((vec), 0);                                             \
        } else {                                                                \
            size_t *vec__p1 = &((size_t *)(vec))[-2];                           \
            size_t *vec__p2 = realloc(vec__p1, (vec__sz));                      \
            assert(vec__p2);                                                    \
            (vec) = (void *)(&vec__p2[2]);                                      \
            vec_set_capacity((vec), (count));                                   \
        }                                                                       \
    } while (0)

/**
 * @brief vec_pop_back - removes the last element from the vector
 * @param vec - the vector
 * @return void
 */
#define vec_pop_back(vec)                    \
    do {                                     \
        vec_set_size((vec), vec_size(vec) - 1); \
    } while (0)

/**
 * @brief vec_erase - removes the element at index i from the vector
 * @param vec - the vector
 * @param i - index of element to remove
 * @return void
 */
#define vec_erase(vec, i)                                     \
    do {                                                      \
        if (vec) {                                            \
            const size_t vec__sz = vec_size(vec);             \
            if ((i) < vec__sz) {                              \
                vec_set_size((vec), vec__sz - 1);                \
                size_t vec__x;                                \
                for (vec__x = (i); vec__x < (vec__sz - 1); ++vec__x) { \
                    (vec)[vec__x] = (vec)[vec__x + 1];              \
                }                                             \
            }                                                 \
        }                                                     \
    } while (0)

/**
 * @brief vec_free - frees all memory associated with the vector
 * @param vec - the vector
 * @return void
 */
#define vec_free(vec)                            \
    do {                                         \
        if (vec) {                               \
            size_t *p1 = &((size_t *)(vec))[-2]; \
            free(p1);                            \
        }                                        \
    } while (0)

/**
 * @brief vec_end - returns an iterator to one past the last element
 * @param vec - the vector
 * @return a pointer to one past the last element (or NULL)
 */
#define vec_end(vec) ((vec) ? &((vec)[vec_size(vec)]) : NULL)

/**
 * @brief vec_push_back - adds an element to the end of the vector
 * @param vec - the vector
 * @param value - the value to add
 * @return void
 */
#define vec_push_back(vec, value)                            \
    do {                                                  \
        size_t vec__cap = vec_capacity(vec);                    \
        if (vec__cap <= vec_size(vec)) {                        \
            vec_grow((vec), !vec__cap ? vec__cap + 1 : vec__cap * 2); \
        }                                                 \
        vec[vec_size(vec)] = (value);                        \
        vec_set_size((vec), vec_size(vec) + 1);              \
    } while (0)

#endif /* VEC_H_ */
