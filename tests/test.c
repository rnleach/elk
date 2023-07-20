#include "test.h"
/*-------------------------------------------------------------------------------------------------
 *
 *                                       Main - Run the tests
 *
 *-----------------------------------------------------------------------------------------------*/
int
main(void)
{
    printf("\n\n***        Starting Tests.        ***\n\n");

    elk_time_tests();
    elk_array_tests();
    elk_queue_tests();
    elk_dequeue_tests();
    elk_heap_tests();
    elk_fnv1a_tests();
    elk_hash_table_tests();
    elk_string_interner_tests();

    printf("\n\n*** Tests completed successfully. ***\n\n");
    return EXIT_SUCCESS;
}
