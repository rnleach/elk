#include "elk.h"

#include <assert.h>
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

typedef struct allocation {
    void *ptr;
    char const *file;
    unsigned line;
} Allocation;

int
allocation_compare(void const *a_void, void const *b_void)
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
static size_t elk_mem_debug_capacity = 0;
static size_t elk_mem_debug_len = 0;
static Allocation *allocations = 0;

static void
elk_mem_allocations_grow()
{
    size_t new_capacity = elk_mem_debug_capacity * 2;
    Allocation *new_allocations = realloc(allocations, new_capacity * sizeof(Allocation));
    assert(new_allocations);

    elk_mem_debug_capacity = new_capacity;
    allocations = new_allocations;
}

static Allocation *
elk_mem_find_allocation_record(void *ptr, char const *fname, unsigned line)
{
    qsort(allocations, elk_mem_debug_len, sizeof *allocations, allocation_compare);

    Allocation bsearch_key = {.ptr = ptr};
    Allocation *alloc = bsearch(&bsearch_key, allocations, elk_mem_debug_len, sizeof *allocations,
                                allocation_compare);

    PanicIf(!alloc,
            "Attempting to find a pointer not orignally allocated by us! (line: %4u - %s)\n", line,
            fname);

    return alloc;
}

static void
elk_mem_allocations_push(void *ptr, char const *fname, unsigned line)
{
    if (elk_mem_debug_len == elk_mem_debug_capacity)
        elk_mem_allocations_grow();

    Allocation *alloc = &allocations[elk_mem_debug_len++];
    alloc->ptr = ptr;
    alloc->file = fname;
    alloc->line = line;
}

void
elk_init_memory_debug()
{
    printf("Initializing Elk memory debugger.\n");
    allocations = realloc(allocations, 64 * sizeof(Allocation));
    assert(allocations);

    elk_mem_debug_capacity = 64;
    elk_mem_debug_len = 0;
}

void
elk_finalize_memory_debug()
{
    // Print out the memory data.
    elk_debug_mem();

    // Free resources
    free(allocations);
    printf("Finalized Elk memory debugger.\n");
}

void
elk_debug_mem()
{
    // TODO: implement
    assert(false);
}

void *
elk_malloc(size_t size, char const *fname, unsigned line)
{
    void *ptr = malloc(size);
    assert(ptr);

    elk_mem_allocations_push(ptr, fname, line);

    return ptr;
}

void *
elk_realloc(void *ptr, size_t size, char const *fname, unsigned line)
{
    Allocation *alloc = 0;
    if (ptr) {
        alloc = elk_mem_find_allocation_record(ptr, fname, line);
    }

    void *new_ptr = realloc(ptr, size);
    assert(new_ptr);

    if (new_ptr == ptr) {
        // Update the allocation record
        alloc->file = fname;
        alloc->line = line;
    } else {

        // If replacing an old pointer, remove its allocation record
        if (ptr) {

            // Swap remove
            size_t index = alloc - allocations;
            allocations[index] = allocations[--elk_mem_debug_len];
        }

        elk_mem_allocations_push(new_ptr, fname, line);
    }

    return new_ptr;
}

void *
elk_calloc(size_t nmemb, size_t size, char const *fname, unsigned line)
{
    void *ptr = calloc(nmemb, size);
    assert(ptr);

    elk_mem_allocations_push(ptr, fname, line);

    return ptr;
}

void
elk_free(void *ptr, char const *fname, unsigned line)
{
    if (ptr) {
        Allocation *alloc = elk_mem_find_allocation_record(ptr, fname, line);

        // Swap remove
        size_t index = alloc - allocations;
        allocations[index] = allocations[--elk_mem_debug_len];

        free(ptr);
    } else {
        printf("Attempted to free NULL pointer. (line: %4u - %s)\n", line, fname);
    }
}

// Turn memory debugging back on.
#    define ELK_MEMORY_DEBUG
#    define malloc(s) elk_malloc((s), __FILE__, __LINE__)
#    define realloc(p, s) elk_realloc((p), (s), __FILE__, __LINE__)
#    define calloc(n, s) elk_calloc((n), (s), __FILE__, __LINE__)
#    define free(p) elk_free((p), __FILE__, __LINE__)

#else
void
elk_init_memory_debug()
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
