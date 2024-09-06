#include "test.h"

#pragma warning(push)

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                       Radix Sort
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

typedef struct 
{
    u64 a;
    i64 b;
    f64 c;
    u32 d;
    i32 e;
    f32 f;
    u16 g;
    i16 h;
    u8 i;
    i8 j;
} TestStruct;

static inline void
print_test_structs(TestStruct *list, size num)
{
    for(size i = 0; i < num; ++i)
    {
        printf("%21"PRIu64" %21"PRIi64" %8g "
               "%11"PRIu32" %12"PRIi32" %8g "
               "%6u %7d %3u %4d\n", 
                list[i].a, list[i].b, list[i].c,
                list[i].d, list[i].e, list[i].f,
                list[i].g, list[i].h,
                list[i].i, list[i].j);
    }

}

static inline void
test_ordering(TestStruct *list, size num, size offset, ElkRadixSortByType cmp_type, ElkSortOrder order)
{
    byte *base = (byte *)list + offset;

    size start_idx = order == ELK_SORT_ASCENDING ? 0 : num - 1;
    size end_idx = order == ELK_SORT_ASCENDING ? num - 1 : 0;
    size delta = order == ELK_SORT_ASCENDING ? 1 : -1;

    u64 u64_0;
    i64 i64_0;
    f64 f64_0;
    u32 u32_0;
    i32 i32_0;
    f32 f32_0;
    u16 u16_0;
    i16 i16_0;
    u8 u8_0;
    i8 i8_0;

    byte *ptr = base + start_idx * sizeof(*list);
    switch(cmp_type)
    {
        case ELK_RADIX_SORT_UINT64: u64_0 = *(u64 *)ptr; break;
        case ELK_RADIX_SORT_INT64:  i64_0 = *(i64 *)ptr; break;
        case ELK_RADIX_SORT_F64:    f64_0 = *(f64 *)ptr; break;
        case ELK_RADIX_SORT_UINT32: u32_0 = *(u32 *)ptr; break;
        case ELK_RADIX_SORT_INT32:  i32_0 = *(i32 *)ptr; break;
        case ELK_RADIX_SORT_F32:    f32_0 = *(f32 *)ptr; break;
        case ELK_RADIX_SORT_UINT16: u16_0 = *(u16 *)ptr; break;
        case ELK_RADIX_SORT_INT16:  i16_0 = *(i16 *)ptr; break;
        case ELK_RADIX_SORT_UINT8:   u8_0 = *(u8 *) ptr; break;
        case ELK_RADIX_SORT_INT8:    i8_0 = *(i8 *) ptr; break;
    }

    start_idx += delta;

    for(size i = start_idx; i != end_idx; i += delta)
    {
        ptr = base + i * sizeof(*list);
        switch(cmp_type)
        {
            case ELK_RADIX_SORT_UINT64:
            {
                u64 u64_1 = *(u64 *)ptr;
                Assert(u64_0 <= u64_1);
                u64_0 = u64_1;
            } break;

            case ELK_RADIX_SORT_INT64:
            {
                i64 i64_1 = *(i64 *)ptr;
                Assert(i64_0 <= i64_1);
                i64_0 = i64_1;
            } break;

            case ELK_RADIX_SORT_F64:
            {
                f64 f64_1 = *(f64 *)ptr;
                if(!(isnan(f64_0) || isnan(f64_1)))
                {
                    Assert(f64_0 <= f64_1);
                }
                f64_0 = f64_1;
            } break;

            case ELK_RADIX_SORT_UINT32:
            {
                u32 u32_1 = *(u32 *)ptr;
                Assert(u32_0 <= u32_1);
                u32_0 = u32_1;
            } break;

            case ELK_RADIX_SORT_INT32:
            {
                i32 i32_1 = *(i32 *)ptr;
                Assert(i32_0 <= i32_1);
                i32_0 = i32_1;
            } break;

            case ELK_RADIX_SORT_F32:
            {
                f32 f32_1 = *(f32 *)ptr;
                if(!(isnan(f32_0) || isnan(f32_1)))
                {
                    Assert(f32_0 <= f32_1);
                }
                f32_0 = f32_1;
            } break;

            case ELK_RADIX_SORT_UINT16:
            {
                u16 u16_1 = *(u16 *)ptr;
                Assert(u16_0 <= u16_1);
                u16_0 = u16_1;
            } break;

            case ELK_RADIX_SORT_INT16:
            {
                i16 i16_1 = *(i16 *)ptr;
                Assert(i16_0 <= i16_1);
                i16_0 = i16_1;
            } break;

            case ELK_RADIX_SORT_UINT8:
            {
                u8 u8_1 = *(u8 *)ptr;
                Assert(u8_0 <= u8_1);
                u8_0 = u8_1;
            } break;

            case ELK_RADIX_SORT_INT8:
            {
                i8 i8_1 = *(i8 *)ptr;
                Assert(i8_0 <= i8_1);
                i8_0 = i8_1;
            } break;
        }
    }
}

#define ARRAY_LEN(X) (sizeof(X) / sizeof(X[0]))

#define TEST(member, mode, order, desc)                                                                                     \
    elk_radix_sort(test_data, ARRAY_LEN(test_data), offsetof(TestStruct, member), stride, scratch, mode, order);                \
    /*                                                                                                                      \
    printf("\nSort "#order" by '"#member"', "desc" member.\n");                                                             \
    print_test_structs(test_data, ARRAY_LEN(test_data));                                                                    \
    */                                                                                                                      \
    test_ordering(test_data, ARRAY_LEN(test_data), offsetof(TestStruct, member), mode, order);

static inline void
elk_radix_sort_test(void)
{
    TestStruct test_data[] = 
    {
        { .a =    1234567, .b =         5, .c =                    INFINITY, .d =    2345678, .e =         5, .f =   INFINITY, .g =          0, .h =        -1, .i =         0, .j =       -2 },
        { .a =     234567, .b =       -49, .c =                   -INFINITY, .d =     345672, .e =       -48, .f =  -INFINITY, .g =         21, .h =        26, .i =         2, .j =        2 },
        { .a =      34567, .b =        48, .c =     0.0                    , .d =      45673, .e =        49, .f =    0.0    , .g =        305, .h =      -475, .i =         8, .j =      120 },
        { .a =       4567, .b =      -470, .c =     1.23e53                , .d =       5674, .e =      -469, .f =   1.23e23f, .g =       4007, .h =       173, .i =        16, .j =       22 },
        { .a =       4567, .b =      -470, .c =     1.23e-53               , .d =       5674, .e =      -469, .f =   1.23e23f, .g =       4007, .h =       173, .i =        16, .j =       22 },
        { .a =       4567, .b =      -470, .c =     4.9406564584124654e-324, .d =       5674, .e =      -469, .f =   1.23e23f, .g =       4007, .h =       173, .i =        16, .j =       22 },
        { .a =        567, .b =       468, .c =  -500.2                    , .d =        675, .e =       462, .f = -500.321f , .g =      50062, .h =    -31056, .i =        28, .j =      -22 },
        { .a =        527, .b =      -468, .c =                         NAN, .d =        527, .e =      -465, .f =        NAN, .g =      60006, .h =     10567, .i =       212, .j =     -120 },
        { .a =          0, .b = INT64_MAX, .c =                         NAN, .d =          0, .e = INT32_MAX, .f =        NAN, .g =       7010, .h = INT16_MAX, .i =       200, .j = INT8_MAX },
        { .a = UINT64_MAX, .b = INT64_MIN, .c =                         NAN, .d = UINT32_MAX, .e = INT32_MIN, .f =        NAN, .g = UINT16_MAX, .h = INT16_MIN, .i = UINT8_MAX, .j = INT8_MIN },
    };

    TestStruct scratch[ARRAY_LEN(test_data)] = {0};
    size const stride = sizeof(TestStruct);

#if 0
    printf("Original\n");
    print_test_structs(test_data, ARRAY_LEN(test_data));
#endif

    TEST(a, ELK_RADIX_SORT_UINT64, ELK_SORT_ASCENDING,  "first");
    TEST(a, ELK_RADIX_SORT_UINT64, ELK_SORT_DESCENDING, "first");

    TEST(b, ELK_RADIX_SORT_INT64,  ELK_SORT_ASCENDING,  "second");
    TEST(b, ELK_RADIX_SORT_INT64,  ELK_SORT_DESCENDING, "second");

    TEST(c, ELK_RADIX_SORT_F64,    ELK_SORT_ASCENDING,  "third");
    TEST(c, ELK_RADIX_SORT_F64,    ELK_SORT_DESCENDING, "third");

    TEST(d, ELK_RADIX_SORT_UINT32, ELK_SORT_ASCENDING,  "fourth");
    TEST(d, ELK_RADIX_SORT_UINT32, ELK_SORT_DESCENDING, "fourth");

    TEST(e, ELK_RADIX_SORT_INT32,  ELK_SORT_ASCENDING,  "fifth");
    TEST(e, ELK_RADIX_SORT_INT32,  ELK_SORT_DESCENDING, "fifth");

    TEST(f, ELK_RADIX_SORT_F32,    ELK_SORT_ASCENDING,  "sixth");
    TEST(f, ELK_RADIX_SORT_F32,    ELK_SORT_DESCENDING, "sixth");

    TEST(g, ELK_RADIX_SORT_UINT16, ELK_SORT_ASCENDING,  "seventh");
    TEST(g, ELK_RADIX_SORT_UINT16, ELK_SORT_DESCENDING, "seventh");

    TEST(h, ELK_RADIX_SORT_INT16,  ELK_SORT_ASCENDING,  "eighth");
    TEST(h, ELK_RADIX_SORT_INT16,  ELK_SORT_DESCENDING, "eighth");

    TEST(i, ELK_RADIX_SORT_UINT8,  ELK_SORT_ASCENDING,  "ninth");
    TEST(i, ELK_RADIX_SORT_UINT8,  ELK_SORT_DESCENDING, "ninth");

    TEST(j, ELK_RADIX_SORT_INT8,   ELK_SORT_ASCENDING,  "tenth");
    TEST(j, ELK_RADIX_SORT_INT8,   ELK_SORT_DESCENDING, "tenth");
}

#define NUM_ROWS 1000
#define NUM_COLS 7
static f64 test_array[NUM_ROWS * NUM_COLS] = {0};
static f64 test_array_scratch[NUM_ROWS * NUM_COLS] = {0};

static inline void
elk_radix_sort_2darray_test(void)
{
    ElkRandomState state = elk_random_state_create(123456);

    /* Fill the array with test data. */
    for(size r = 0; r < NUM_ROWS; ++r)
    {
        size offset = r * NUM_COLS;
        for(size c = 0; c < NUM_COLS; ++c)
        {
            test_array[offset + c] = elk_random_state_uniform_f64(&state);
            /* Make  4th column really small! */
            if(c == 4)
            {
                test_array[offset + c] *= 4.9406564584124654e-324;
            }
        }
    }


    size stride = NUM_COLS * sizeof(test_array[0]);
    ElkRadixSortByType type = ELK_RADIX_SORT_F64;
    ElkSortOrder order = ELK_SORT_ASCENDING;

    for(size c = 0; c < NUM_COLS; ++c)
    {
        size offset = c * sizeof(test_array[0]);
        elk_radix_sort(test_array, NUM_ROWS, offset, stride, test_array_scratch, type, order);

        for(size r = 1; r < NUM_ROWS; ++r)
        {
            Assert(test_array[r * NUM_COLS + c] >= test_array[(r - 1) * NUM_COLS + c]);
        }
    }
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_sort_tests(void)
{
    elk_radix_sort_test();
    elk_radix_sort_2darray_test();
}

#pragma warning(pop)
