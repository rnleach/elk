#include "elk.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <tgmath.h>

// Constant messages for errors.
static const char *err_out_of_mem = "out of memory";

/*-------------------------------------------------------------------------------------------------
 *                                        Date and Time Handling
 *-----------------------------------------------------------------------------------------------*/
// MT-unsafe
static time_t
tz_offset(time_t local)
{
    // gmtime is not thread safe, so this whole function is not thread safe.
    struct tm time = *gmtime(&local);
    time_t shifted = mktime(&time);
    return shifted - local;
}

time_t
elk_time_from_ymd_and_hms(int year, int month, int day, int hour, int minutes, int seconds)
{
    assert(month >= 1 && month <= 12);
    assert(day >= 1 && day <= 31);
    assert(hour >= 0 && hour <= 23);
    assert(minutes >= 0 && minutes <= 59);
    assert(seconds >= 0 && seconds <= 60); // 60 not 59 for leap seconds? Time is sooo hard.

    struct tm time = {0};
    time.tm_year = year - 1900;
    time.tm_mon = month - 1;
    time.tm_mday = day;
    time.tm_hour = hour;
    time.tm_min = minutes;
    time.tm_sec = seconds;

    // mktime is MT-safe (locale, env)
    time_t local = mktime(&time);

    // tz_offset is MT-unsafe, so this function is MT-unsafe
    return local - tz_offset(local);
}

time_t
elk_time_truncate_to_specific_hour(time_t time, int hour)
{
    assert(hour >= 0 && hour <= 23);

    struct tm tm_time = *gmtime(&time);

    tm_time.tm_hour = hour;
    tm_time.tm_sec = 0;
    tm_time.tm_min = 0;

    time_t adjusted = mktime(&tm_time);

    // tz_offset is MT-unsafe, so this function is MT-unsafe
    adjusted -= tz_offset(adjusted);

    while (adjusted > time) {
        adjusted -= ElkDay;
    }

    return adjusted;
}

time_t
elk_time_truncate_to_hour(time_t time)
{
    struct tm tm_time = *localtime(&time);

    tm_time.tm_sec = 0;
    tm_time.tm_min = 0;

    return mktime(&tm_time);
}

time_t
elk_time_add(time_t time, int change_in_time)
{
    return time + change_in_time;
}

/*-------------------------------------------------------------------------------------------------
 *                                           ElkArray
 *-----------------------------------------------------------------------------------------------*/
ElkArray
elk_array_new(size_t element_size)
{
    size_t const initial_num_elements = 0;
    return elk_array_new_with_capacity(initial_num_elements, element_size);
}

ElkArray
elk_array_new_with_capacity(size_t capacity, size_t element_size)
{
    unsigned char *data = NULL;
    if (capacity > 0) {
        data = calloc(capacity, element_size);
        PanicIf(!data, "%s", err_out_of_mem);
    }

    return (ElkArray){
        .element_size = element_size, .length = 0, .capacity = capacity, .data = data};
}

ElkCode
elk_array_clear(ElkArray *arr)
{
    assert(arr);
    if (arr->data) {
        assert(arr->capacity > 0);
        free(arr->data);
        arr->data = NULL;
        arr->capacity = 0;
        arr->length = 0;
    } else {
        assert(arr->capacity == 0);
        assert(arr->length == 0);
    }

    return ELK_CODE_SUCCESS;
}

ElkCode
elk_array_reset(ElkArray *arr)
{
    assert(arr);
    arr->length = 0;
    return ELK_CODE_SUCCESS;
}

static void
elk_array_expand(struct ElkArray *arr, size_t min_capacity)
{
    assert(arr);

    /* This function is only ever used in elk_array_push_back and this is checked for there.
    if (min_capacity <= arr->capacity) {
        return ;
    }
    */

    size_t new_capacity = arr->capacity * 3 / 2;
    new_capacity = new_capacity < 4 ? 4 : new_capacity;

    size_t allocation_size = arr->element_size * new_capacity;

    unsigned char *new = realloc(arr->data, allocation_size);
    PanicIf(!new, "%s", err_out_of_mem);

    arr->capacity = new_capacity;
    arr->data = new;

    return;
}

ElkCode
elk_array_push_back(ElkArray *arr, void *item)
{
    assert(arr);
    assert(item);

    if (arr->capacity == arr->length) {
        elk_array_expand(arr, arr->length + 1);
    }

    void *next = arr->data + arr->element_size * arr->length;
    memcpy(next, item, arr->element_size);
    arr->length++;

    return ELK_CODE_SUCCESS;
}

ElkCode
elk_array_pop_back(ElkArray *arr, void *item)
{
    assert(arr);

    if (arr->length == 0) {
        return ELK_CODE_EMPTY;
    }

    if (item) {
        memcpy(item, arr->data + arr->element_size * (arr->length - 1), arr->element_size);
    }

    arr->length--;

    return ELK_CODE_SUCCESS;
}

static void
elk_array_swap_remove_fast(ElkArray *const arr, size_t index, void *item)
{
    // A version for internal use with no range checks.
    void *index_ptr = arr->data + arr->element_size * index;
    void *last_ptr = arr->data + arr->element_size * (arr->length - 1);

    if (item) {
        memcpy(item, index_ptr, arr->element_size);
    }

    memcpy(index_ptr, last_ptr, arr->element_size);
    arr->length--;
}

ElkCode
elk_array_swap_remove(ElkArray *const arr, size_t index, void *item)
{
    assert(arr);

    if (arr->length == 0) {
        return ELK_CODE_EMPTY;
    } else if (index >= arr->length) {
        return ELK_CODE_INVALID_INDEX;
    }

    elk_array_swap_remove_fast(arr, index, item);

    return ELK_CODE_SUCCESS;
}

void *const
elk_array_alias_index(ElkArray *const arr, size_t index)
{
    assert(arr);
    return arr->data + index * arr->element_size;
}

ElkCode
elk_array_foreach(ElkArray *const arr, IterFunc ifunc, void *user_data)
{
    assert(arr);

    void *first = arr->data;
    void *last = arr->data + arr->element_size * (arr->length - 1);
    for (void *item = first; item <= last; item += arr->element_size) {
        bool keep_going = ifunc(item, user_data);
        if (!keep_going) {
            return ELK_CODE_EARLY_TERM;
        }
    }

    return ELK_CODE_SUCCESS;
}

ElkCode
elk_array_filter_out(ElkArray *const src, ElkArray *sink, FilterFunc filter, void *user_data)
{
    assert(src);
    if (sink) {
        assert(src->element_size == sink->element_size);
    }

    for (size_t i = 0; i < src->length; ++i) {
        void *item = elk_array_alias_index(src, i);
        bool to_remove = filter(item, user_data);

        if (to_remove) {
            if (sink) {
                elk_array_push_back(sink, item);
            }

            elk_array_swap_remove_fast(src, i, NULL);
            --i;
        }
    }

    return ELK_CODE_SUCCESS;
}

/*-------------------------------------------------------------------------------------------------
 *                                            Queue
 *-----------------------------------------------------------------------------------------------*/
struct ElkQueue {
    size_t element_size;
    size_t capacity;
    size_t head;
    size_t tail;
    unsigned char data[];
};

struct ElkQueue *
elk_queue_new(size_t element_size, size_t capacity)
{
    assert(capacity > 0);

    size_t size = capacity * element_size + sizeof(struct ElkQueue);
    struct ElkQueue *queue = malloc(size);
    assert(queue);
    memset(queue, 0, size); // Initialize all fields to zero!

    queue->element_size = element_size;
    queue->capacity = capacity;

    return queue;
}

void
elk_queue_free(struct ElkQueue *queue)
{
    free(queue);
    return;
}

bool
elk_queue_full(struct ElkQueue *queue)
{
    assert(queue && elk_queue_count(queue) <= queue->capacity);
    return elk_queue_count(queue) == queue->capacity;
}

bool
elk_queue_empty(ElkQueue *queue)
{
    assert(queue && elk_queue_count(queue) <= queue->capacity);
    return queue->head == queue->tail;
}

ElkCode
elk_queue_enqueue(struct ElkQueue *queue, void *item)
{
    assert(queue && elk_queue_count(queue) <= queue->capacity);
    if (elk_queue_full(queue)) {
        return ELK_CODE_FULL;
    }

    size_t byte_idx = (queue->tail % queue->capacity) * queue->element_size;
    memcpy(&queue->data[byte_idx], item, queue->element_size);

    queue->tail++;

    return ELK_CODE_SUCCESS;
}

ElkCode
elk_queue_dequeue(struct ElkQueue *queue, void *output)
{
    assert(queue);
    if (elk_queue_empty(queue)) {
        return ELK_CODE_EMPTY;
    }

    size_t byte_idx = (queue->head % queue->capacity) * queue->element_size;
    memcpy(output, &queue->data[byte_idx], queue->element_size);

    queue->head++;

    return ELK_CODE_SUCCESS;
}

void const *
elk_queue_peek_alias(struct ElkQueue *queue)
{
    assert(queue);
    if (elk_queue_empty(queue)) {
        return NULL;
    }

    size_t byte_idx = (queue->head % queue->capacity) * queue->element_size;
    return &queue->data[byte_idx];
}

size_t
elk_queue_count(struct ElkQueue *queue)
{
    assert(queue);
    return queue->tail - queue->head;
}

void
elk_queue_foreach(struct ElkQueue *queue, IterFunc ifunc, void *user_data)
{
    assert(queue);
    unsigned char *item = malloc(sizeof(queue->element_size));
    assert(item);

    while (elk_queue_dequeue(queue, item) == ELK_CODE_SUCCESS) {
        if (!ifunc(item, user_data))
            goto END;
    }

END:
    free(item);
    return;
}

/*-------------------------------------------------------------------------------------------------
 *                                             Dequeue
 *-----------------------------------------------------------------------------------------------*/
typedef struct ElkDequeue {
    size_t element_size;
    size_t capacity;
    size_t head;
    size_t length;
    unsigned char data[];
} ElkDequeue;

ElkDequeue *
elk_dequeue_new(size_t element_size, size_t capacity)
{
    assert(capacity > 0);
    assert(element_size > 0);

    size_t size = capacity * element_size + sizeof(struct ElkDequeue);
    struct ElkDequeue *dequeue = malloc(size);
    assert(dequeue);
    memset(dequeue, 0, size); // Initialize all fields to zero!

    dequeue->element_size = element_size;
    dequeue->capacity = capacity;

    return dequeue;
}

static size_t
elk_dequeue_tail(struct ElkDequeue *dequeue)
{
    assert(dequeue);
    return (dequeue->head + dequeue->length) % dequeue->capacity;
}

static void
elk_dequeue_increment_head(struct ElkDequeue *dequeue)
{
    assert(dequeue);
    assert(dequeue->length > 0);

    dequeue->head++;
    dequeue->head %= dequeue->capacity;
}

static void
elk_dequeue_decrement_head(struct ElkDequeue *dequeue)
{
    assert(dequeue);

    dequeue->head--;
    dequeue->head += dequeue->capacity;
    dequeue->head %= dequeue->capacity;
}

void
elk_dequeue_free(ElkDequeue *dequeue)
{
    free(dequeue);
}

bool
elk_dequeue_full(ElkDequeue *dequeue)
{
    assert(dequeue && elk_dequeue_count(dequeue) <= dequeue->capacity);
    return dequeue->length == dequeue->capacity;
}

bool
elk_dequeue_empty(ElkDequeue *dequeue)
{
    assert(dequeue && elk_dequeue_count(dequeue) <= dequeue->capacity);
    return dequeue->length == 0;
}

ElkCode
elk_dequeue_enqueue_back(ElkDequeue *dequeue, void *item)
{
    assert(dequeue && elk_dequeue_count(dequeue) <= dequeue->capacity);
    if (elk_dequeue_full(dequeue)) {
        return ELK_CODE_FULL;
    }

    size_t tail = elk_dequeue_tail(dequeue);
    size_t byte_idx = tail * dequeue->element_size;
    memcpy(&dequeue->data[byte_idx], item, dequeue->element_size);

    dequeue->length++;

    return ELK_CODE_SUCCESS;
}

ElkCode
elk_dequeue_enqueue_front(ElkDequeue *dequeue, void *item)
{
    assert(dequeue && elk_dequeue_count(dequeue) <= dequeue->capacity);

    if (elk_dequeue_full(dequeue)) {
        return ELK_CODE_FULL;
    }

    elk_dequeue_decrement_head(dequeue);
    dequeue->length++;

    size_t byte_idx = dequeue->head * dequeue->element_size;
    memcpy(&dequeue->data[byte_idx], item, dequeue->element_size);

    return ELK_CODE_SUCCESS;
}

ElkCode
elk_dequeue_dequeue_front(ElkDequeue *dequeue, void *output)
{
    assert(dequeue);

    if (elk_dequeue_empty(dequeue)) {
        return ELK_CODE_EMPTY;
    }

    size_t byte_idx = dequeue->head * dequeue->element_size;
    memcpy(output, &dequeue->data[byte_idx], dequeue->element_size);

    elk_dequeue_increment_head(dequeue);
    dequeue->length--;

    return ELK_CODE_SUCCESS;
}

ElkCode
elk_dequeue_dequeue_back(ElkDequeue *dequeue, void *output)
{
    assert(dequeue);

    if (elk_dequeue_empty(dequeue)) {
        return ELK_CODE_EMPTY;
    }

    dequeue->length--;
    size_t tail_idx = elk_dequeue_tail(dequeue);

    size_t byte_idx = tail_idx * dequeue->element_size;
    memcpy(output, &dequeue->data[byte_idx], dequeue->element_size);

    return ELK_CODE_SUCCESS;
}

void const *
elk_dequeue_peek_front_alias(ElkDequeue *dequeue)
{
    assert(dequeue);

    if (elk_dequeue_empty(dequeue)) {
        return NULL;
    }

    size_t byte_idx = dequeue->head * dequeue->element_size;
    return &dequeue->data[byte_idx];
}

void const *
elk_dequeue_peek_back_alias(ElkDequeue *dequeue)
{
    assert(dequeue);

    if (elk_dequeue_empty(dequeue)) {
        return NULL;
    }

    dequeue->length--;
    size_t tail_idx = elk_dequeue_tail(dequeue);
    dequeue->length++;

    size_t byte_idx = tail_idx * dequeue->element_size;
    return &dequeue->data[byte_idx];
}

size_t
elk_dequeue_count(ElkDequeue *dequeue)
{
    assert(dequeue);
    return dequeue->length;
}

void
elk_dequeue_foreach(ElkDequeue *dequeue, IterFunc ifunc, void *user_data)
{
    assert(dequeue);
    unsigned char *item = malloc(sizeof(dequeue->element_size));
    assert(item);

    while (elk_dequeue_dequeue_front(dequeue, item) == ELK_CODE_SUCCESS) {
        if (!ifunc(item, user_data))
            goto END;
    }

END:
    free(item);
    return;
}

/*-------------------------------------------------------------------------------------------------
 *                                    Heap or Priority Queue
 *-----------------------------------------------------------------------------------------------*/
struct ElkHeap {
    size_t element_size;
    size_t capacity;
    size_t length;
    size_t arity;
    Priority priority;
    _Alignas(max_align_t) unsigned char data[];
};

typedef struct ElkHeapTuple {
    int value;
    _Alignas(max_align_t) unsigned char payload[];
} ElkHeapTuple;

static size_t
elk_heap_tuple_size(size_t element_size)
{
    return sizeof(ElkHeapTuple) + element_size;
}

static ElkHeapTuple *
elk_heap_tuple_new(int value, void *payload, size_t element_size)
{
    size_t bytes = elk_heap_tuple_size(element_size);
    ElkHeapTuple *new = malloc(bytes);
    assert(new);

    new->value = value;
    memcpy(new->payload, payload, element_size);

    return new;
}

static size_t
elk_heap_size_bytes(size_t element_size, size_t capacity)
{
    assert(capacity > 0);
    return sizeof(ElkHeap) + capacity * elk_heap_tuple_size(element_size);
}

static ElkHeapTuple *
elk_heap_item_by_index(ElkHeap const *heap, size_t index)
{
    return (ElkHeapTuple *)(heap->data + index * elk_heap_tuple_size(heap->element_size));
}

static void
elk_heap_copy_tuple(ElkHeapTuple *dest, ElkHeapTuple const *src, size_t element_size)
{
    size_t nbytes = elk_heap_tuple_size(element_size);
    memcpy(dest, src, nbytes);
}

static size_t
elk_heap_parent_index(size_t index, size_t arity)
{
    assert(arity > 0);
    assert(index > 0); // The root node (top of the heap) has no parent. So don't try!

    return (index - 1) / arity;
}

static void
elk_heap_bubble_up(struct ElkHeap *heap, size_t item_index, ElkHeapTuple *current)
{
    assert(heap);
    assert(current);

    // Can not go farther up than the root!
    if (item_index == 0)
        return;

    // assume that current holds the same value as the one at the initial item_index
    ElkHeapTuple *item = elk_heap_item_by_index(heap, item_index);
    while (item_index > 0) {
        size_t parent_index = elk_heap_parent_index(item_index, heap->arity);

        ElkHeapTuple *parent = elk_heap_item_by_index(heap, parent_index);

        if (parent->value < current->value) {
            elk_heap_copy_tuple(item, parent, heap->element_size);
            item_index = parent_index;
            item = elk_heap_item_by_index(heap, item_index);
        } else {
            break;
        }
    }

    // Make the final copy.
    elk_heap_copy_tuple(item, current, heap->element_size);

    return;
}

static size_t
elk_heap_first_leaf_index(size_t heap_count, size_t arity)
{
    assert(heap_count > 0);

    if (heap_count == 1) {
        return 0;
    }

    return (heap_count - 2) / arity + 1;
}

static size_t
elk_heap_first_child_index(size_t index, size_t arity)
{
    assert(arity > 0);
    return arity * index + 1;
}

static void
elk_heap_highest_priority_child(struct ElkHeap *heap, size_t idx, ElkHeapTuple **res,
                                size_t *res_idx)
{
    size_t arity = heap->arity;
    size_t first_child_index = elk_heap_first_child_index(idx, arity);
    int max_priority = INT_MIN;
    for (size_t i = first_child_index; i < first_child_index + arity && i < heap->length; ++i) {
        ElkHeapTuple *item = elk_heap_item_by_index(heap, i);
        if (item->value >= max_priority) {
            max_priority = item->value;
            *res_idx = i;
            *res = item;
        }
    }

    assert(max_priority != INT_MIN);
    return;
}

static void
elk_heap_push_down(struct ElkHeap *heap, size_t item_index, ElkHeapTuple *current)
{
    assert(heap);
    assert(current);

    // assume that current holds the same value as the one at the initial item_index

    size_t first_leaf_idx = elk_heap_first_leaf_index(heap->length, heap->arity);
    ElkHeapTuple *item = elk_heap_item_by_index(heap, item_index);
    while (item_index < first_leaf_idx) {

        size_t child_idx = 0;
        ElkHeapTuple *child = 0;
        elk_heap_highest_priority_child(heap, item_index, &child, &child_idx);

        if (child->value > current->value) {
            elk_heap_copy_tuple(item, child, heap->element_size);
            item_index = child_idx;
            item = elk_heap_item_by_index(heap, item_index);
        } else {
            break;
        }
    }

    elk_heap_copy_tuple(item, current, heap->element_size);

    return;
}

struct ElkHeap *
elk_bounded_heap_new(size_t element_size, size_t capacity, Priority pri, size_t arity)
{
    assert(element_size > 0);
    assert(capacity > 0);
    assert(arity > 1);

    size_t heap_size = elk_heap_size_bytes(element_size, capacity);
    struct ElkHeap *new = malloc(heap_size);
    assert(new);
    new->element_size = element_size;
    new->capacity = capacity;
    new->length = 0;
    new->arity = arity;
    new->priority = pri;

    size_t data_size = elk_heap_tuple_size(element_size) * capacity;
    memset(new->data, 0, data_size);

    return new;
}

struct ElkHeap *
elk_heap_free(struct ElkHeap *heap)
{
    free(heap);
    return 0;
}

static ElkCode
elk_heap_check_resize(struct ElkHeap **heap)
{
    assert(heap && *heap);

    struct ElkHeap *h = *heap;
    assert(h->length <= h->capacity);

    if (h->length == h->capacity) {
        return ELK_CODE_FULL;
    }

    return ELK_CODE_SUCCESS;
}

struct ElkHeap *
elk_heap_insert(struct ElkHeap *heap, void *item, ElkCode *result)
{
    assert(heap);
    assert(item);

    struct ElkHeap *new_heap = heap;
    ElkCode resize_result = elk_heap_check_resize(&new_heap);
    if (resize_result == ELK_CODE_FULL) {
        if (result) {
            *result = ELK_CODE_FULL;
        }
        return new_heap;
    }

    int value = new_heap->priority(item);

    ElkHeapTuple *temp = elk_heap_tuple_new(value, item, new_heap->element_size);

    size_t target_index = new_heap->length++;

    ElkHeapTuple *target_item = elk_heap_item_by_index(new_heap, target_index);
    elk_heap_copy_tuple(target_item, temp, new_heap->element_size);

    elk_heap_bubble_up(new_heap, target_index, temp);

    if (result) {
        *result = ELK_CODE_SUCCESS;
    }

    free(temp);

    return new_heap;
}

struct ElkHeap *
elk_heap_top(struct ElkHeap *heap, void *item, ElkCode *result)
{
    assert(heap);

    // Nothing on the heap.
    if (heap->length == 0) {
        if (result) {
            *result = ELK_CODE_EMPTY;
        }
        memset(item, 0, heap->element_size);
        return heap;
    }

    --heap->length;
    ElkHeapTuple *temp = elk_heap_item_by_index(heap, heap->length);

    // Last item on the heap
    if (heap->length == 0) {
        if (result) {
            *result = ELK_CODE_SUCCESS;
        }
        memcpy(item, temp->payload, heap->element_size);
        return heap;
    }

    // Return top, copy in temp to the root, and push down
    ElkHeapTuple *top = elk_heap_item_by_index(heap, 0);

    if (result) {
        *result = ELK_CODE_SUCCESS;
    }
    memcpy(item, top->payload, heap->element_size);

    elk_heap_copy_tuple(top, temp, heap->element_size);

    elk_heap_push_down(heap, 0, temp);

    return heap;
}

size_t
elk_heap_count(struct ElkHeap const *const heap)
{
    assert(heap);
    return heap->length;
}

void const *
elk_heap_peek(struct ElkHeap const *const heap)
{
    assert(heap);

    // Return NULL if there is nothing to peek at.
    if (heap->length == 0) {
        return 0;
    }

    ElkHeapTuple *item = elk_heap_item_by_index(heap, 0);
    return item->payload;
}

/*-------------------------------------------------------------------------------------------------
 *                                   Hashes, Hash Table
 *-----------------------------------------------------------------------------------------------*/
struct ElkHashTable {
    ElkHashFunction hasher;
    int size_exp;
    void *values[];
};

ElkHashTable *
elk_hash_table_create(int size_exp, ElkHashFunction hasher)
{
    assert(false);
    return NULL;
}
void
elk_hash_table_destroy(ElkHashTable *table)
{
    assert(false);
    return;
}
bool
elk_hash_table_insert(ElkHashTable *table, size_t n, void *key, void *value)
{
    assert(false);
    return false;
}
void const *
elk_hash_table_retrieve(ElkHashTable *table, size_t n, void *key)
{
    assert(false);
    return NULL;
}
void
elk_hash_table_foreach(ElkHashTable *table, IterFunc func)
{
    assert(false);
    return;
}

/*-------------------------------------------------------------------------------------------------
 *                                       String Interner
 *-----------------------------------------------------------------------------------------------*/
struct ElkStringInterner {
    ElkHashTable *index;
    char buffer[];
};

ElkStringInterner *
elk_string_interner_create(int size_exp)
{
    assert(false);
    return NULL;
}
void
elk_string_interner_destroy(ElkStringInterner *interner)
{
    assert(false);
    return;
}
ElkInternedString
elk_string_interner_intern(ElkStringInterner *interner, char const *string)
{
    assert(false);
    return 0;
}
char const *
elk_string_interner_retrieve(ElkStringInterner *interner, ElkInternedString handle)
{
    assert(false);
    return NULL;
}
