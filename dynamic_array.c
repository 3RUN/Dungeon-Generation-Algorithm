

Array *_array_create(size_t type_size)
{
    Array *array = sys_malloc(sizeof(Array));
    if (!array)
        return NULL;

    array->capacity = ARRAY_INITIAL_CAPACITY;
    array->size = 0;
    array->type_size = type_size;
    array->data = sys_malloc(ARRAY_INITIAL_CAPACITY * type_size);
    if (!array->data)
        return NULL;

    return array;
}

void array_destroy(Array *array)
{
    if (!array)
        return;

    sys_free(array->data);
    sys_free(array);
}

void array_change_capacity(Array *array, size_t new_capacity)
{
    if (!array)
        return;

    if (new_capacity <= 0)
        new_capacity = array->capacity * 2;

    ArrayData *new_data = sys_malloc(new_capacity * array->type_size);
    memcpy(new_data, array->data, array->size * array->type_size);
    sys_free(array->data);

    array->data = new_data;
    array->capacity = new_capacity;
}

size_t array_size(Array *array)
{
    if (!array)
        return -1;

    return array->size;
}

size_t array_capacity(Array *array)
{
    if (!array)
        return -1;

    return array->capacity;
}

ArrayData *array_data(Array *array)
{
    if (!array)
        return NULL;

    return array->data;
}

bool is_array_empty(Array *array)
{
    if (!array)
        return true;

    return array_size(array) <= 0;
}

bool is_valid_index(Array *array, int index)
{
    if (!array)
        return false;

    if (is_array_empty(array))
        return false;

    return index > 0 && index < array->size;
}

ArrayData *_array_at(Array *array, int index)
{
    if (!array)
        return NULL;

    if (index < 0 || index > array->size)
    {
#ifdef DEBUG_ARRAY
        error("Can't get item. Index out of range!");
#endif
        return NULL;
    }

    return (unsigned char *)(array->data) + index * array->type_size;
}

void array_insert_at(Array *array, int index, ArrayData *item)
{
    if (!array)
        return;

    if (index < 0 || index > array->size)
    {
#ifdef DEBUG_ARRAY
        error("Can't insert item. Index out of range!");
#endif
        return NULL;
    }

    array->size++;
    if (array->size >= array->capacity)
        array_change_capacity(array, array->capacity * 2);

    int i = 0;
    for (i = array_size(array); i > index; i--)
        array_change_at(ArrayData *, array, i, array_get_at(ArrayData *, array, i - 1));

    array_change_at(ArrayData *, array, index, item);
}

void array_add(Array *array, ArrayData *item)
{
    if (!array)
        return;

    if (array->size >= array->capacity)
        array_change_capacity(array, array->capacity * 2);

    array_change_at(ArrayData *, array, array->size, item);
    array->size++;
}

void array_remove_last(Array *array)
{
    if (!array)
        return;

    if (is_array_empty(array))
        return;

    array_last(ArrayData *, array) = NULL;

    array->size--;
    if (array->size < (array->capacity * 0.5))
        array_change_capacity(array, array->capacity * 0.5);
}

void array_remove_at(Array *array, int index)
{
    if (!array)
        return;

    if (is_array_empty(array))
        return;

    if (index < 0 || index > array->size)
    {
#ifdef DEBUG_ARRAY
        error("Can't remove item. Index out of range!");
#endif
        return;
    }

    if (index == array->size)
        return;

    int i = 0;
    for (i = index; i < array_size(array) - 1; i++)
        array_change_at(ArrayData *, array, i, array_get_at(ArrayData *, array, i + 1));

    array_remove_last(array);
}

void array_clear(Array *array)
{
    if (!array)
        return;

    if (is_array_empty(array))
        return;

    int i = 0, size = array_size(array);
    for (i = 0; i < size; i++)
        array_remove_last(array);
}

int array_find(Array *array, ArrayData *item)
{
    if (!array)
        return -1;

    if (is_array_empty(array))
        return -1;

    int i = 0;
    for (i = 0; i < array_size(array); i++)
    {
        if (array_get_at(ArrayData *, array, i) != item)
            continue;

        return i;
    }

    return -1;
}

void array_swap(Array *array, int index_a, int index_b)
{
    if (!array)
        return;

    if (is_array_empty(array))
        return;

    if (index_a < 0 || index_b < 0 || index_a >= array->size || index_b >= array->size)
        return;

    ArrayData *temp_data = array_get_at(ArrayData *, array, index_a);
    array_change_at(ArrayData *, array, index_a, array_get_at(ArrayData *, array, index_b));
    array_change_at(ArrayData *, array, index_b, temp_data);
}