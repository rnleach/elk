#include "elk.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>
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
 *                                      Memory and Pointers
 *-----------------------------------------------------------------------------------------------*/
// "Assume" all this code works, and turn off memory debugging inside memory debugging. If memory
// debugging isn't turned off inside memory debugging then calls to malloc() will be replaced with
// elk_malloc(). So at runtime the function will recursively call itself infinitely!
#ifdef ELK_MEMORY_DEBUG
#    undef ELK_MEMORY_DEBUG
#    undef malloc
#    undef realloc
#    undef calloc
#    undef free

#    define MAGIC_NUM_1 251
#    define MAGIC_NUM_2 11

typedef struct allocation {
    void *ptr;
    unsigned char *magic;
    char const *file;
    unsigned line;
} Allocation;

int
elk_mem_allocation_ptr_compare(void const *a_void, void const *b_void)
{
    Allocation const *a = a_void;
    Allocation const *b = b_void;

    if (a->ptr == b->ptr)
        return 0;
    if (a->ptr < b->ptr)
        return -1;
    return 1;
}

// We could use an ElkList here, but that would then mean the memory debugging functions tracked
// themselves, which would be confusing when looking at output. So I re-implement an array backed
// list here.
static void *elk_mem_allocations_mutex = 0;
static void (*elk_mem_allocations_lock)(void *) = 0;
static void (*elk_mem_allocations_unlock)(void *) = 0;

static size_t elk_mem_allocations_capacity = 0;
static size_t elk_mem_allocations_len = 0;
static Allocation *elk_mem_allocations = 0;

static void
elk_mem_allocations_grow()
{
    size_t new_capacity = elk_mem_allocations_capacity * 2;
    Allocation *new_allocations = realloc(elk_mem_allocations, new_capacity * sizeof(Allocation));
    assert(new_allocations);

    elk_mem_allocations_capacity = new_capacity;
    elk_mem_allocations = new_allocations;
}

static Allocation *
elk_mem_find_allocation_record(void *ptr, char const *fname, unsigned line)
{
    qsort(elk_mem_allocations, elk_mem_allocations_len, sizeof *elk_mem_allocations,
          elk_mem_allocation_ptr_compare);

    Allocation bsearch_key = {.ptr = ptr};
    Allocation *alloc = bsearch(&bsearch_key, elk_mem_allocations, elk_mem_allocations_len,
                                sizeof *elk_mem_allocations, elk_mem_allocation_ptr_compare);

    PanicIf(!alloc,
            "Attempting to find a pointer not orignally allocated by us! (line: %4u - %s)\n", line,
            fname);

    return alloc;
}

static void
elk_mem_allocation_make_magic(unsigned char *magic)
{
    magic[0] = MAGIC_NUM_1;
    magic[1] = MAGIC_NUM_2;
}

static void
elk_mem_allocation_check_magic(Allocation *alloc, char const *fname, unsigned line)
{
    unsigned char *magic = alloc->magic;
    PanicIf(magic[0] != MAGIC_NUM_1 || magic[1] != MAGIC_NUM_2,
            "Buffer overrun detected, failed magic number check! (line: %u - %s)\n"
            "  Original allocation (address: %p, line: %u - %s\n",
            line, fname, alloc->ptr, alloc->line, alloc->file);
}

static void
elk_mem_allocations_push(void *ptr, unsigned char *magic, char const *fname, unsigned line)
{
    if (elk_mem_allocations_len == elk_mem_allocations_capacity)
        elk_mem_allocations_grow();

    Allocation *alloc = &elk_mem_allocations[elk_mem_allocations_len++];
    alloc->ptr = ptr;
    alloc->magic = magic;
    alloc->file = fname;
    alloc->line = line;
}

void
elk_init_memory_debug(void *mutex, void (*lock)(void *), void (*unlock)(void *))
{
    fprintf(stderr, "Initializing Elk memory debugger.\n");

    if (mutex) {
        PanicIf(!(lock && unlock), "Mutex supplied but not functions to lock and unlock it.");
        lock(mutex);
        if (!elk_mem_allocations_mutex) {
            elk_mem_allocations_mutex = mutex;
            elk_mem_allocations_lock = lock;
            elk_mem_allocations_unlock = unlock;
        }
    }

    if (!elk_mem_allocations) {
        elk_mem_allocations = realloc(elk_mem_allocations, 64 * sizeof(Allocation));
        assert(elk_mem_allocations);

        elk_mem_allocations_capacity = 64;
        elk_mem_allocations_len = 0;
    }

    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_unlock(elk_mem_allocations_mutex);
    }
}

void
elk_finalize_memory_debug()
{
    // Print out the memory data.
    elk_debug_mem();

    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_lock(elk_mem_allocations_mutex);
    }

    // Free resources
    free(elk_mem_allocations);

    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_unlock(elk_mem_allocations_mutex);
    }

    fprintf(stderr, "Finalized Elk memory debugger.\n");
}

void
elk_debug_mem()
{
    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_lock(elk_mem_allocations_mutex);
    }

    if (elk_mem_allocations_len == 0) {
        fprintf(stderr, "No memory leaks detected!\n");
    } else {

        fprintf(stderr, "Memory leaks detected!\n");
        fprintf(stderr, "%15s | %5s | %s\n", "Pointer", "Line", "File");
        for (unsigned i = 0; i < elk_mem_allocations_len; i++) {
            Allocation *alloc = &elk_mem_allocations[i];
            fprintf(stderr, "%15p | %5u | %s\n", alloc->ptr, alloc->line, alloc->file);
        }
    }

    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_unlock(elk_mem_allocations_mutex);
    }
}

void *
elk_malloc(size_t size, char const *fname, unsigned line)
{
    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_lock(elk_mem_allocations_mutex);
    }

    void *ptr = malloc(size + 2);
    assert(ptr);

    unsigned char *magic = (unsigned char *)ptr + size;
    elk_mem_allocation_make_magic(magic);

    elk_mem_allocations_push(ptr, magic, fname, line);

    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_unlock(elk_mem_allocations_mutex);
    }

    return ptr;
}

void *
elk_realloc(void *ptr, size_t size, char const *fname, unsigned line)
{
    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_lock(elk_mem_allocations_mutex);
    }

    Allocation *alloc = 0;
    if (ptr) {
        alloc = elk_mem_find_allocation_record(ptr, fname, line);
        elk_mem_allocation_check_magic(alloc, fname, line);
    }

    void *new_ptr = realloc(ptr, size + 2);
    assert(new_ptr);

    unsigned char *magic = (unsigned char *)new_ptr + size;
    elk_mem_allocation_make_magic(magic);

    if (new_ptr == ptr) {
        // Update the allocation record
        alloc->file = fname;
        alloc->line = line;
        alloc->magic = magic;
    } else {

        // If replacing an old pointer, remove its allocation record
        if (ptr) {

            // Swap remove
            size_t index = alloc - elk_mem_allocations;
            elk_mem_allocations[index] = elk_mem_allocations[--elk_mem_allocations_len];
        }

        elk_mem_allocations_push(new_ptr, magic, fname, line);
    }

    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_unlock(elk_mem_allocations_mutex);
    }

    return new_ptr;
}

void *
elk_calloc(size_t nmemb, size_t size, char const *fname, unsigned line)
{
    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_lock(elk_mem_allocations_mutex);
    }

    void *ptr = malloc(nmemb * size + 2);
    assert(ptr);
    memset(ptr, 0, nmemb * size + 2);

    unsigned char *magic = (unsigned char *)ptr + size * nmemb;
    elk_mem_allocation_make_magic(magic);

    elk_mem_allocations_push(ptr, magic, fname, line);

    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_unlock(elk_mem_allocations_mutex);
    }

    return ptr;
}

void
elk_free(void *ptr, char const *fname, unsigned line)
{
    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_lock(elk_mem_allocations_mutex);
    }

    if (ptr) {
        Allocation *alloc = elk_mem_find_allocation_record(ptr, fname, line);
        elk_mem_allocation_check_magic(alloc, fname, line);

        // Swap remove
        size_t index = alloc - elk_mem_allocations;
        elk_mem_allocations[index] = elk_mem_allocations[--elk_mem_allocations_len];

        free(ptr);
    } else {
        fprintf(stderr, "Attempted to free NULL pointer. (line: %4u - %s)\n", line, fname);
    }

    if (elk_mem_allocations_mutex) {
        elk_mem_allocations_unlock(elk_mem_allocations_mutex);
    }
}

#    undef MAGIC_NUM_2
#    undef MAGIC_NUM_1

// Turn memory debugging back on.
#    define ELK_MEMORY_DEBUG
#    define malloc(s) elk_malloc((s), __FILE__, __LINE__)
#    define realloc(p, s) elk_realloc((p), (s), __FILE__, __LINE__)
#    define calloc(n, s) elk_calloc((n), (s), __FILE__, __LINE__)
#    define free(p) elk_free((p), __FILE__, __LINE__)

#else
void
elk_init_memory_debug(void *mutex, void (*lock)(void *), void (*unlock)(void *))
{
}

void
elk_finalize_memory_debug()
{
}

void
elk_debug_mem()
{
}
#endif

/*-------------------------------------------------------------------------------------------------
 *                                             List
 *-----------------------------------------------------------------------------------------------*/
struct ElkList {
    size_t element_size;
    size_t len;
    size_t capacity;
    _Alignas(max_align_t) unsigned char data[];
};

ElkList *
elk_list_new(size_t element_size)
{
    size_t const initial_num_elements = 4;
    return elk_list_new_with_capacity(initial_num_elements, element_size);
}

ElkList *
elk_list_new_with_capacity(size_t capacity, size_t element_size)
{
    size_t allocation_size = sizeof(struct ElkList) + element_size * capacity;

    struct ElkList *new = malloc(allocation_size);
    PanicIf(!new, "%s", err_out_of_mem);

    new->element_size = element_size;
    new->len = 0;
    new->capacity = capacity;

    return new;
}

ElkList *
elk_list_free(ElkList *list)
{
    if (list) {
        free(list);
    }

    return NULL;
}

ElkList *
elk_list_clear(ElkList *list)
{
    assert(list);
    list->len = 0;
    return list;
}

static struct ElkList *
elk_list_expand(struct ElkList *list, size_t min_capacity)
{
    assert(list);

    if (min_capacity <= list->capacity) {
        return list;
    }

    size_t new_capacity = list->capacity * 3 / 2;
    new_capacity = new_capacity < 4 ? 4 : new_capacity;

    size_t allocation_size = sizeof(struct ElkList) + list->element_size * new_capacity;

    struct ElkList *new = realloc(list, allocation_size);
    PanicIf(!new, "%s", err_out_of_mem);

    new->capacity = new_capacity;

    return new;
}

ElkList *
elk_list_push_back(ElkList *list, void *item)
{
    assert(list);
    assert(item);

    if (list->capacity == list->len) {
        list = elk_list_expand(list, list->len + 1);
    }

    void *next = list->data + list->element_size * list->len;
    memcpy(next, item, list->element_size);
    list->len++;

    return list;
}

ElkList *
elk_list_pop_back(ElkList *list, void *item)
{
    assert(list);

    if (item) {
        memcpy(item, list->data + list->element_size * (list->len - 1), list->element_size);
    }

    list->len--;

    return list;
}

ElkList *
elk_list_swap_remove(ElkList *const list, size_t index, void *item)
{
    assert(list);

    void *index_ptr = list->data + list->element_size * index;
    void *last_ptr = list->data + list->element_size * (list->len - 1);

    if (item) {
        memcpy(item, index_ptr, list->element_size);
    }

    memcpy(index_ptr, last_ptr, list->element_size);
    list->len--;

    return list;
}

size_t
elk_list_count(ElkList const *const list)
{
    assert(list);
    return list->len;
}

ElkList *
elk_list_copy(ElkList const *const list)
{
    assert(list);

    struct ElkList *copy = elk_list_new_with_capacity(list->len, list->element_size);

    assert(copy->capacity <= list->capacity);

    size_t copy_size = sizeof(struct ElkList) + copy->element_size * copy->capacity;
    memcpy(copy, list, copy_size);
    copy->capacity = copy->len;

    return copy;
}

void *const
elk_list_get_alias_at_index(ElkList *const list, size_t index)
{
    assert(list);
    return list->data + index * list->element_size;
}

void
elk_list_foreach(ElkList *const list, IterFunc ifunc, void *user_data)
{
    assert(list);

    void *first = list->data;
    void *last = list->data + list->element_size * (list->len - 1);
    for (void *item = first; item <= last; item += list->element_size) {
        bool keep_going = ifunc(item, user_data);
        if (!keep_going) {
            return;
        }
    }

    return;
}

ElkList *
elk_list_filter_out(ElkList *const src, ElkList *sink, FilterFunc filter, void *user_data)
{
    assert(src);
    if (sink) {
        assert(src->element_size == sink->element_size);
    }

    for (size_t i = 0; i < src->len; ++i) {
        void *item = elk_list_get_alias_at_index(src, i);
        bool to_remove = filter(item, user_data);

        if (to_remove) {
            if (sink) {
                sink = elk_list_push_back(sink, item);
            }

            elk_list_swap_remove(src, i, NULL);
            --i;
        }
    }

    return sink;
}

/*-------------------------------------------------------------------------------------------------
 *                                            Queue
 *-----------------------------------------------------------------------------------------------*/
struct ElkQueue {
    size_t element_size;
    size_t capacity;
    size_t length;
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

    queue->element_size = element_size;
    queue->capacity = capacity;
    queue->length = 0;
    queue->head = 0;
    queue->tail = 0;
    memset(queue->data, 0, capacity * element_size);

    return queue;
}

struct ElkQueue *
elk_queue_free(struct ElkQueue *queue)
{
    free(queue);
    return 0;
}

bool
elk_queue_full(struct ElkQueue *queue)
{
    assert(queue);
    return queue->length >= queue->capacity;
}

bool
elk_queue_empty(ElkQueue *queue)
{
    assert(queue);
    return queue->length == 0;
}

bool
elk_queue_enqueue(struct ElkQueue *queue, void *item)
{
    assert(queue);
    if (elk_queue_full(queue)) {
        return false;
    }

    size_t byte_idx = queue->tail * queue->element_size;
    memcpy(&queue->data[byte_idx], item, queue->element_size);

    queue->length++;
    queue->tail = (queue->tail + 1) % queue->capacity;

    return true;
}

bool
elk_queue_dequeue(struct ElkQueue *queue, void *output)
{
    assert(queue);
    if (elk_queue_empty(queue)) {
        return false;
    }

    size_t byte_idx = queue->head * queue->element_size;
    memcpy(output, &queue->data[byte_idx], queue->element_size);

    queue->length--;
    queue->head = (queue->head + 1) % queue->capacity;

    return true;
}

void const *
elk_queue_peek_alias(struct ElkQueue *queue)
{
    assert(queue);
    if (elk_queue_empty(queue)) {
        return 0;
    }

    size_t byte_idx = queue->head * queue->element_size;
    return &queue->data[byte_idx];
}

size_t
elk_queue_count(struct ElkQueue *queue)
{
    assert(queue);
    return queue->length;
}

void
elk_queue_foreach(struct ElkQueue *queue, IterFunc ifunc, void *user_data)
{
    assert(queue);
    unsigned char *item = malloc(sizeof(queue->element_size));
    assert(item);

    while (elk_queue_dequeue(queue, item)) {
        ifunc(item, user_data);
    }

    free(item);
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
    bool bounded;
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
elk_heap_new(size_t element_size, Priority pri, size_t arity)
{
    size_t const INITIAL_SIZE = 4;

    struct ElkHeap *new = elk_bounded_heap_new(element_size, INITIAL_SIZE, pri, arity);
    assert(new);

    new->bounded = false;

    return new;
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
    new->bounded = true;

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
        if (h->bounded) {
            return ELK_CODE_FULL;
        } else {
            size_t new_capacity = (h->capacity * 2);

            struct ElkHeap *new = realloc(h, elk_heap_size_bytes(h->element_size, new_capacity));
            new->capacity = new_capacity;
            PanicIf(!new, "%s", err_out_of_mem);
            *heap = new;
        }
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
 *                                    Coordinates and Rectangles
 *-----------------------------------------------------------------------------------------------*/
static bool
elk_rect_overlaps(Elk2DRect const *left, Elk2DRect const *right)
{
    double left_min_x = left->ll.x;
    double left_min_y = left->ll.y;
    double right_max_x = right->ur.x;
    double right_max_y = right->ur.y;

    if (left_min_x > right_max_x || left_min_y > right_max_y) {
        return false;
    }

    double left_max_x = left->ur.x;
    double left_max_y = left->ur.y;
    double right_min_x = right->ll.x;
    double right_min_y = right->ll.y;

    if (left_max_x < right_min_x || left_max_y < right_min_y) {
        return false;
    }

    return true;
}

/*-------------------------------------------------------------------------------------------------
 *                                       Hilbert Curves
 *-----------------------------------------------------------------------------------------------*/

struct ElkHilbertCurve {
    /* The number of iterations to use for this curve.
     *
     * This can be a maximum of 31. If it is larger than 31, we won't have enough bits to do the
     * binary transformations correctly.
     */
    uint64_t iterations;

    /* This is the domain that the curve will cover. */
    Elk2DRect domain;

    /* This is needed for fast transformations from the "domain" space into "Hilbert" space. */
    uint32_t max_dim;
    double width;
    double height;
};

static uint32_t
elk_hilbert_max_dim(unsigned int iterations)
{
    return (UINT32_C(1) << iterations) - UINT32_C(1);
}

static struct ElkHilbertCurve
elk_hilbert_curve_initialize(unsigned int iterations, Elk2DRect domain)
{
    PanicIf(iterations < 1, "Require at least 1 iteration for Hilbert curve.");
    PanicIf(iterations > 31, "Maximum 31 iterations for Hilbert curve.");

    uint32_t max_dim = elk_hilbert_max_dim(iterations);
    double width = domain.ur.x - domain.ll.x;
    double height = domain.ur.y - domain.ll.y;

    PanicIf(width <= 0.0 || height <= 0.0, "Invalid rectangle, negative width or height.");

    return (struct ElkHilbertCurve){.iterations = iterations,
                                    .domain = domain,
                                    .max_dim = max_dim,
                                    .width = width,
                                    .height = height};
}

struct ElkHilbertCurve *
elk_hilbert_curve_new(unsigned int iterations, Elk2DRect domain)
{
    struct ElkHilbertCurve *new = malloc(sizeof(struct ElkHilbertCurve));
    PanicIf(!new, "%s", err_out_of_mem);

    *new = elk_hilbert_curve_initialize(iterations, domain);

    return new;
}

struct ElkHilbertCurve *
elk_hilbert_curve_free(struct ElkHilbertCurve *hc)
{
    if (hc) {
        free(hc);
    }

    return NULL;
}

#ifndef NDEBUG
static uint64_t
elk_hilbert_max_num(unsigned int iterations)
{
    return (UINT64_C(1) << (2 * iterations)) - UINT64_C(1);
}
#endif

struct HilbertCoord
elk_hilbert_integer_to_coords(struct ElkHilbertCurve const *hc, uint64_t hi)
{
    assert(hc);
    assert(hi <= elk_hilbert_max_num(hc->iterations));

    uint32_t x = 0;
    uint32_t y = 0;

    // This is the "transpose" operation.
    for (uint32_t b = 0; b < hc->iterations; ++b) {
        uint64_t x_mask = UINT64_C(1) << (2 * b + 1);
        uint64_t y_mask = UINT64_C(1) << (2 * b);

        uint32_t x_val = (hi & x_mask) >> (b + 1);
        uint32_t y_val = (hi & y_mask) >> (b);

        x |= x_val;
        y |= y_val;
    }

    // Gray decode
    uint32_t z = UINT32_C(2) << (hc->iterations - 1);
    uint32_t t = y >> 1;

    y ^= x;
    x ^= t;

    // Undo excess work
    uint32_t q = 2;
    while (q != z) {
        uint32_t p = q - 1;

        if (y & q) {
            x ^= p;
        } else {
            t = (x ^ y) & p;
            x ^= t;
            y ^= t;
        }

        if (x & q) {
            x ^= p;
        } else {
            t = (x ^ x) & p;
            x ^= t;
            x ^= t;
        }

        q <<= 1;
    }

    assert(x <= elk_hilbert_max_dim(hc->iterations));
    assert(y <= elk_hilbert_max_dim(hc->iterations));

    return (struct HilbertCoord){.x = x, .y = y};
}

uint64_t
elk_hilbert_coords_to_integer(struct ElkHilbertCurve const *hc, struct HilbertCoord coords)
{
    assert(hc);
    assert(coords.x <= elk_hilbert_max_dim(hc->iterations));
    assert(coords.y <= elk_hilbert_max_dim(hc->iterations));

    uint32_t x = coords.x;
    uint32_t y = coords.y;

    uint32_t m = UINT32_C(1) << (hc->iterations - 1);

    // Inverse undo excess work
    uint32_t q = m;
    while (q > 1) {
        uint32_t p = q - 1;

        if (x & q) {
            x ^= p;
        } else {
            uint32_t t = (x ^ x) & p;
            x ^= t;
            x ^= t;
        }

        if (y & q) {
            x ^= p;
        } else {
            uint32_t t = (x ^ y) & p;
            x ^= t;
            y ^= t;
        }

        q >>= 1;
    }

    // Gray encode
    y ^= x;
    uint32_t t = 0;
    q = m;
    while (q > 1) {
        if (y & q) {
            t ^= (q - 1);
        }

        q >>= 1;
    }

    x ^= t;
    y ^= t;

    // This is the transpose operation
    uint64_t hi = 0;
    for (uint32_t b = 0; b < hc->iterations; ++b) {

        uint64_t x_val = (((UINT64_C(1) << b) & x) >> b) << (2 * b + 1);
        uint64_t y_val = (((UINT64_C(1) << b) & y) >> b) << (2 * b);

        hi |= x_val;
        hi |= y_val;
    }

    assert(hi <= elk_hilbert_max_num(hc->iterations));
    return hi;
}

struct HilbertCoord
elk_hilbert_translate_to_curve_coords(struct ElkHilbertCurve *hc, Elk2DCoord coord)
{
    assert(hc);

    double hilbert_edge_len = hc->max_dim + 1;

    uint32_t x = (coord.x - hc->domain.ll.x) / hc->width * hilbert_edge_len;
    uint32_t y = (coord.y - hc->domain.ll.y) / hc->height * hilbert_edge_len;

    x = x > hc->max_dim ? hc->max_dim : x;
    y = y > hc->max_dim ? hc->max_dim : y;

    struct HilbertCoord hcoords = {.x = x, .y = y};
    return hcoords;
}

uint64_t
elk_hilbert_translate_to_curve_distance(struct ElkHilbertCurve *hc, Elk2DCoord coord)
{
    assert(hc);

    struct HilbertCoord hc_coords = elk_hilbert_translate_to_curve_coords(hc, coord);
    return elk_hilbert_coords_to_integer(hc, hc_coords);
}

/*-------------------------------------------------------------------------------------------------
 *                                          2D RTreeView
 *-----------------------------------------------------------------------------------------------*/

#define ELK_RTREE_CHILDREN_PER_NODE 4

enum NodeType { ELK_RTREE_LEAF_NODE, ELK_RTREE_GROUP };

struct ElkRTreeLeaf {
    void *item;
    uint64_t hilbert_num;
    Elk2DRect mbr;
};

struct RTreeNode {
    Elk2DRect mbr;
    union {
        struct ElkRTreeLeaf *item[ELK_RTREE_CHILDREN_PER_NODE];
        struct RTreeNode *child[ELK_RTREE_CHILDREN_PER_NODE];
    };
    enum NodeType child_type;
    unsigned char num_children;
};

struct Elk2DRTreeView {
    ElkList *leaves;
    struct RTreeNode nodes[];
};

struct BuildQSortDataArgs {
    Elk2DRect (*rect)(void *);
    Elk2DCoord (*centroid)(void *);
    ElkList *qsort_data;
    ElkHilbertCurve *hc;
};

static bool
build_qsort_data(void *item, void *user_data)
{
    struct BuildQSortDataArgs *args = user_data;

    Elk2DCoord centroid = args->centroid(item);
    Elk2DRect bbox = args->rect(item);
    uint64_t hilbert_num = elk_hilbert_translate_to_curve_distance(args->hc, centroid);

    struct ElkRTreeLeaf element = {.item = item, .hilbert_num = hilbert_num, .mbr = bbox};

    args->qsort_data = elk_list_push_back(args->qsort_data, &element);

    return true;
}

static int
compare_hilbert_nums(void const *left, void const *right)
{
    struct ElkRTreeLeaf const *a = left;
    struct ElkRTreeLeaf const *b = right;

    if (a->hilbert_num == b->hilbert_num)
        return 0;
    if (a->hilbert_num < b->hilbert_num)
        return -1;
    return 1;
}

struct MaxBoundingRectCallBackArgs {
    Elk2DRect mbr;
    Elk2DRect (*rect)(void *);
};

static bool
build_bounding_rectangle(void *item, void *user_data)
{
    struct MaxBoundingRectCallBackArgs *args = user_data;

    Elk2DRect item_rect = args->rect(item);

    args->mbr.ll.x = fmin(args->mbr.ll.x, item_rect.ll.x);
    args->mbr.ll.y = fmin(args->mbr.ll.y, item_rect.ll.y);

    args->mbr.ur.x = fmax(args->mbr.ur.x, item_rect.ur.x);
    args->mbr.ur.y = fmax(args->mbr.ur.y, item_rect.ur.y);

    return true;
}

static Elk2DRect
build_rtree_domain(ElkList *const list, Elk2DRect (*rect)(void *), Elk2DRect *pre_computed_domain)
{
    if (pre_computed_domain) {
        return *pre_computed_domain;
    } else {
        // We need to calculate the MBR (domain)
        struct MaxBoundingRectCallBackArgs args = {
            .rect = rect,
            .mbr = (Elk2DRect){.ll = (Elk2DCoord){.x = HUGE_VAL, .y = HUGE_VAL},
                               .ur = (Elk2DCoord){.x = -HUGE_VAL, .y = -HUGE_VAL}},
        };

        elk_list_foreach(list, build_bounding_rectangle, &args);

        return args.mbr;
    }
}

Elk2DRTreeView *
elk_2d_rtree_view_new(ElkList *const list, Elk2DCoord (*centroid)(void *),
                      Elk2DRect (*rect)(void *), Elk2DRect *pre_computed_domain)
{
    assert(list);
    assert(centroid);
    assert(rect);

    // Calculate the domain if a pre-computed domain was not supplied and use that to set up the
    // Hilbert curve.
    Elk2DRect data_domain = build_rtree_domain(list, rect, pre_computed_domain);
    ElkHilbertCurve hc = elk_hilbert_curve_initialize(16, data_domain);

    // Create the leaf nodes as a list of ElkRTreeLeaf, then process the data from our list to fill
    // those nodes.
    ElkList *q_data = elk_list_new_with_capacity(elk_list_count(list), sizeof(struct ElkRTreeLeaf));
    struct BuildQSortDataArgs args = {
        .rect = rect, .centroid = centroid, .qsort_data = q_data, .hc = &hc};
    elk_list_foreach(list, build_qsort_data, &args);

    // Sort the leaf nodes by Hilbert number. This is how we get locality for the parent nodes.
    qsort(q_data->data, q_data->len, q_data->element_size, compare_hilbert_nums);

    // Calculate the memory for the groups.
    size_t num_nodes_with_leaf_children = q_data->len / ELK_RTREE_CHILDREN_PER_NODE +
                                          (q_data->len % ELK_RTREE_CHILDREN_PER_NODE > 0 ? 1 : 0);
    size_t num_nodes = num_nodes_with_leaf_children;
    size_t level_nodes = num_nodes;
    while (level_nodes > 1) {
        level_nodes = level_nodes / ELK_RTREE_CHILDREN_PER_NODE +
                      (level_nodes % ELK_RTREE_CHILDREN_PER_NODE > 0 ? 1 : 0);
        num_nodes += level_nodes;
    }

    // Allocate the memory for the tree structure including the group nodes and associate the leaf
    // nodes with it.
    struct Elk2DRTreeView *rtree =
        malloc(sizeof(Elk2DRTreeView) + sizeof(struct RTreeNode) * num_nodes);
    PanicIf(!rtree, "%s", err_out_of_mem);
    rtree->leaves = q_data;

    //
    // Initialize the group nodes to point down the tree and have the correct minimum bounding
    // rectangles.
    //

    // Fill in the nodes whose children are leaves, call these level 1 nodes.
    // (Leaf nodes are level 0 nodes.)
    size_t first_level_1_node_index = num_nodes - num_nodes_with_leaf_children;
    size_t num_leaves_left_to_process = q_data->len;
    size_t next_leaf = 0;
    for (unsigned int i = first_level_1_node_index; i < num_nodes; ++i) {

        unsigned num_leaves_to_process = num_leaves_left_to_process < ELK_RTREE_CHILDREN_PER_NODE
                                             ? num_leaves_left_to_process
                                             : ELK_RTREE_CHILDREN_PER_NODE;

        assert(num_leaves_to_process > 0);

        // Initialize the minimum bounding rectangle to values that will certainly be overwritten.
        rtree->nodes[i].mbr = (Elk2DRect){.ll = (Elk2DCoord){.x = HUGE_VAL, .y = HUGE_VAL},
                                          .ur = (Elk2DCoord){.x = -HUGE_VAL, .y = -HUGE_VAL}};

        for (unsigned j = 0; j < num_leaves_to_process; ++j) {
            struct ElkRTreeLeaf *leaf =
                (struct ElkRTreeLeaf *)elk_list_get_alias_at_index(rtree->leaves, next_leaf);
            rtree->nodes[i].item[j] = leaf;

            // Update the mbr for the parent node
            rtree->nodes[i].mbr.ll.x = fmin(rtree->nodes[i].mbr.ll.x, leaf->mbr.ll.x);
            rtree->nodes[i].mbr.ll.y = fmin(rtree->nodes[i].mbr.ll.y, leaf->mbr.ll.y);
            rtree->nodes[i].mbr.ur.x = fmax(rtree->nodes[i].mbr.ur.x, leaf->mbr.ur.x);
            rtree->nodes[i].mbr.ur.y = fmax(rtree->nodes[i].mbr.ur.y, leaf->mbr.ur.y);

            ++next_leaf;
            --num_leaves_left_to_process;
        }

        rtree->nodes[i].child_type = ELK_RTREE_LEAF_NODE;
        rtree->nodes[i].num_children = num_leaves_to_process;
    }

    // Fill in the nodes whose children are not leaves.
    level_nodes = num_nodes_with_leaf_children;
    size_t num_filled_so_far = num_nodes_with_leaf_children;
    while (num_filled_so_far < num_nodes) {

        size_t num_children_at_level_below = level_nodes;
        size_t num_children_left_to_process = num_children_at_level_below;

        level_nodes = level_nodes / ELK_RTREE_CHILDREN_PER_NODE +
                      (level_nodes % ELK_RTREE_CHILDREN_PER_NODE > 0 ? 1 : 0);

        size_t first_node_at_level = num_nodes - num_filled_so_far - level_nodes;
        for (unsigned int i = first_node_at_level; i < num_nodes - num_filled_so_far; ++i) {

            unsigned num_to_fill = num_children_left_to_process < ELK_RTREE_CHILDREN_PER_NODE
                                       ? num_children_left_to_process
                                       : ELK_RTREE_CHILDREN_PER_NODE;

            assert(num_to_fill > 0);

            // Initialize the minimum bounding rectangle to values that will certainly be
            // overwritten.
            rtree->nodes[i].mbr = (Elk2DRect){.ll = (Elk2DCoord){.x = HUGE_VAL, .y = HUGE_VAL},
                                              .ur = (Elk2DCoord){.x = -HUGE_VAL, .y = -HUGE_VAL}};

            for (unsigned char j = 0; j < num_to_fill; ++j) {
                size_t child_index = first_node_at_level + level_nodes +
                                     (i - first_node_at_level) * ELK_RTREE_CHILDREN_PER_NODE + j;

                struct RTreeNode *node = &rtree->nodes[child_index];

                rtree->nodes[i].child[j] = node;

                // Update the mbr for the parent node
                rtree->nodes[i].mbr.ll.x = fmin(rtree->nodes[i].mbr.ll.x, node->mbr.ll.x);
                rtree->nodes[i].mbr.ll.y = fmin(rtree->nodes[i].mbr.ll.y, node->mbr.ll.y);
                rtree->nodes[i].mbr.ur.x = fmax(rtree->nodes[i].mbr.ur.x, node->mbr.ur.x);
                rtree->nodes[i].mbr.ur.y = fmax(rtree->nodes[i].mbr.ur.y, node->mbr.ur.y);

                --num_children_left_to_process;
            }

            rtree->nodes[i].child_type = ELK_RTREE_GROUP;
            rtree->nodes[i].num_children = num_to_fill;
        }

        num_filled_so_far += level_nodes;
    }

    return rtree;
}

Elk2DRTreeView *
elk_2d_rtree_view_free(Elk2DRTreeView *tv)
{
    if (tv) {
        elk_list_free(tv->leaves);
        free(tv);
    }

    return NULL;
}

static void
fprint_spaces(FILE *f, unsigned int num)
{
    for (unsigned int i = 0; i < num; ++i)
        fputc(' ', f);

    return;
}

static void
elk_2d_rtree_view_print_leaf(struct ElkRTreeLeaf *leaf, unsigned int level)
{
    assert(leaf);

    fprint_spaces(stderr, level * 2);
    fprintf(stderr, "Hilbert Num: %7" PRIu64 " LL = (%lf, %lf) UR= (%lf, %lf)\n", leaf->hilbert_num,
            leaf->mbr.ll.x, leaf->mbr.ll.y, leaf->mbr.ur.x, leaf->mbr.ur.y);
    return;
}

static void
elk_2d_rtree_view_inner_print(struct RTreeNode *node, unsigned int level)
{
    assert(node);

    fprint_spaces(stderr, level * 2);
    char *child_type = 0;
    if (node->child_type == ELK_RTREE_GROUP)
        child_type = "GROUP";
    else if (node->child_type == ELK_RTREE_LEAF_NODE)
        child_type = "LEAF";
    else
        Panic("unreachable");

    fprintf(stderr, "Num Children: %2u Child Type: %5s LL=(%lf, %lf) UR=(%lf, %lf)\n",
            node->num_children, child_type, node->mbr.ll.x, node->mbr.ll.y, node->mbr.ur.x,
            node->mbr.ur.y);

    if (node->child_type == ELK_RTREE_GROUP) {
        for (unsigned int i = 0; i < node->num_children; ++i) {
            elk_2d_rtree_view_inner_print(node->child[i], level + 1);
        }
    } else {
        for (unsigned int i = 0; i < node->num_children; ++i) {
            elk_2d_rtree_view_print_leaf(node->item[i], level + 1);
        }
    }

    return;
}

void
elk_2d_rtree_view_print(Elk2DRTreeView *rtree)
{
    assert(rtree);

    struct RTreeNode *root = &rtree->nodes[0];
    elk_2d_rtree_view_inner_print(root, 0);

    return;
}

static bool
elk_2d_rtree_view_node_foreach(struct RTreeNode *node, Elk2DRect region, IterFunc update,
                               void *user_data)
{
    if (elk_rect_overlaps(&node->mbr, &region)) {
        unsigned int num_children = node->num_children;

        if (node->child_type == ELK_RTREE_GROUP) {
            // Recurse
            for (unsigned int i = 0; i < num_children; ++i) {
                struct RTreeNode *next_node = node->child[i];
                assert(next_node != node);
                bool keep_going =
                    elk_2d_rtree_view_node_foreach(next_node, region, update, user_data);
                if (!keep_going) {
                    return false;
                }
            }
        } else if (node->child_type == ELK_RTREE_LEAF_NODE) {
            // Apply to all leaves
            for (unsigned int i = 0; i < num_children; ++i) {
                struct ElkRTreeLeaf *leaf = node->item[i];

                if (elk_rect_overlaps(&leaf->mbr, &region)) {
                    bool keep_going = update(leaf->item, user_data);
                    if (!keep_going) {
                        return false;
                    }
                }
            }
        } else {
            Panic("Invalid child_type - impossible enum value.");
        }
    }

    return true;
}

void
elk_2d_rtree_view_foreach(Elk2DRTreeView *tv, Elk2DRect region, IterFunc update, void *user_data)
{
    assert(tv);

    struct RTreeNode *root = &tv->nodes[0];
    elk_2d_rtree_view_node_foreach(root, region, update, user_data);

    return;
}

#undef ELK_RTREE_CHILDREN_PER_NODE
