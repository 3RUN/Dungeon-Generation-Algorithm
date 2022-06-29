
#ifndef _DYNAMIC_ARRAY_H_
#define _DYNAMIC_ARRAY_H_

/**
 * \file    dynamic_array.h
 * \brief   Dynamically growing array implementation.
 *
 * \authors Originally created by Sidney Just aka JustSid
 *          Migrated to LiteC by Magomet Kocharov aka 3RUN
 * \date    16.06.2022
 * \version 1.0.0
 *
 * dynamic_array.h provides a generic dynamically growing array
 */

/**
 * If uncommented - enables 'error' messages when something goes wrong.
 */
#define DEBUG_ARRAY

/**
 * Initial capacity of the array. Array capacity set to this value automatically if <= 0 capacity was passed in array_create.
 */
#define ARRAY_INITIAL_CAPACITY 2

typedef long size_t;
typedef void ArrayData;
typedef int bool;

/**
 * A dynamically growing array that allows you to handle a collection of any data type.
 * Since it doesn't have a fixed size, you can just add or remove elements.
 */
typedef struct Array
{
    /**
     * Number of elements array has currently allocated space for.
     */
    size_t capacity;

    /**
     * Amount of stored elements in the array.
     */
    size_t size;

    /**
     * sizeof() the stored data type.
     */
    size_t type_size;

    /**
     * The data stored in the array.
     */
    ArrayData *data;
} Array;

/**
 * Creates a new array with the given parameters and returns it's pointer to the array_create macros.
 * \param   type_size       The sizeof() the element type that array is going to contain.
 * \return				    Pointer to a new array, or NULL if failed.
 */
Array *_array_create(size_t type_size);

/**
 * Destroys the array. Note that elements aren't freed.
 * \param   array           The array to be destroyed.
 */
void array_destroy(Array *array);

/**
 * Changes the capacity of the array to the given new_capacity. Can be used for decreasing the capacity as well.
 * \param   array           The array to change it's capacity.
 * \param   new_capacity    The new capacity to change array's capacity to.
 */
void array_change_capacity(Array *array, size_t new_capacity);

/**
 * Returns amount of stored elements in the given array.
 * \param   array           The array to return it's elements count.
 * \return				    Amount of stored elements in the array, or -1 if failed.
 */
size_t array_size(Array *array);

/**
 * Returns the number of elements that can be held in given array.
 * \param   array           The array to return it's elements count.
 * \return				    Number of elements that can be held, or -1 if failed.
 */
size_t array_capacity(Array *array);

/**
 * Returns ArrayData contained in the array. Gives direct access to the underlying array.
 * \param   array           The array to return it's data.
 * \return				    ArrayData contained in the array, or NULL if failed.
 */
ArrayData *array_data(Array *array);

/**
 * Checks whether given array is empty.
 * \param   array           The array to check.
 * \return				    true - if empty, otherwise - false.
 */
bool is_array_empty(Array *array);

/**
 * Checks if given index is within the array bounds.
 * \param   array           The array to get bounds from.
 * \param   index           The index to check.
 * \return				    true - if index is within the array bounds, otherwise - false.
 */
bool is_valid_index(Array *array, int index);

/**
 * Access specified element with bounds checking. Could be used for getting/changing elements.
 * \param   array           The array to access the it's element.
 * \param   index           Index of the element we need to access.
 * \return				    Element at the given index, or NULL if failed.
 */
ArrayData *_array_at(Array *array, int index);

/**
 * Inserts an element into the given position in the array. Size is increased by 1. Capacity automatically increased if needed.
 * \param   array           The array that we want to insert the element into.
 * \param   index           Position where we want to insert the element to.
 * \param   item            Element to insert into the array.
 */
void array_insert_at(Array *array, int index, ArrayData *item);

/**
 * Adds new element at the end of the given array.
 * \param   array           The array to add new element to.
 * \param   item            Element that is going to be added.
 */
void array_add(Array *array, ArrayData *item);

/**
 * Removes last element from the given array. Note that removed element isn't freed.
 * \param   array           The array to remove last element from.
 */
void array_remove_last(Array *array);

/**
 * Removes the element at the given position of the array. Note that removed element isn't freed.
 * \param   array           The array to remove last element from.
 */
void array_remove_at(Array *array, int index);

/**
 * Removes all elements from the given array. Note that elements aren't going to be freed.
 * \param   array           The array to clear all the elements from.
 */
void array_clear(Array *array);

/**
 * Find given element within the array and returns it's index.
 * \param   array           The array that needs to be searched.
 * \param   item            Item we are going to search for.
 * \return                  Index of the found element (first occurance), or -1 if failed.
 */
int array_find(Array *array, ArrayData *item);

/**
 * Swaps two elements within the array (if they are found).
 * \param   array           The array to swap elements in.
 * \param   a               Index of the element to swap.
 * \param   b               Index of the element to swap.
 */
void array_swap(Array *array, int index_a, int index_b);

/**
 * Creates new array and return it's pointer.
 * \param   t               A data type that array is going to contain (f.e. var, int, float, ENTITY*).
 * \return				    Pointer to the created array, or NULL if failed.
 */
#define array_create(t) _array_create(sizeof(t))

/**
 * Returns an element of the array at the given index. The element will be casted to the given data type.
 * \param   t               A data type to cast element to (f.e. var, int, float, ENTITY*).
 * \param   a               Pointer to the array we are getting element from.
 * \param   i               Index of the element in the array.
 * \return				    Element from given index position of the array, or NULL if failed.
 */
#define array_get_at(t, a, i) *((t *)_array_at(a, i))

/**
 * Changes an element of the array at the given index to a new element. NOTE that previous element isn't going to be freed.
 * \param   t               A data type to cast element to (f.e. var, int, float, ENTITY*).
 * \param   a               Pointer to the array we are changing element in.
 * \param   i               Index of the element in the array to change.
 * \param   v               New value to change element to.
 */
#define array_change_at(t, a, i, v) *((t *)_array_at(a, i)) = (v)

/**
 * Returns first element of the given array. The element will be casted to the given data type.
 * Calling on an empty array is undefined.
 * \param   t               A data type to cast element to (f.e. var, int, float, ENTITY*).
 * \param   a               Pointer to the array we are getting element from.
 * \return				    First element of the array, or NULL if failed.
 */
#define array_first(t, a) *((t *)_array_at(a, 0))

/**
 * Returns last element of the given array. The element will be casted to the given data type.
 * Calling on an empty array causes undefined behavior.
 * \param   t               A data type to cast element to (f.e. var, int, float, ENTITY*).
 * \param   a               Pointer to the array we are getting element from.
 * \return				    Last element of the array, or NULL if failed.
 */
#define array_last(t, a) *((t *)_array_at(a, a->size - 1))

/**
 * Begins enumerating through the array + 'v' is set to the next element of the array. Make sure to not use ';' at the end when using !
 * \param   t           A data type to cast element to (f.e. var, int, float, ENTITY*).
 * \param   a           Pointer to the array we are getting element from.
 * \param   v           Next element of the array + it's casted to the data type given above.
 */
#define array_enumerate_begin(t, a, v) do{ int i; for (i = 0; i < a->size; i++) { t v = array_get_at(t, a, i);

/**
 * Ends the enumerating through the array.
 * \param   a           Pointer to the array we are getting element from.
 */
#define array_enumerate_end(a) }}while (0)

#include "dynamic_array.c"
#endif