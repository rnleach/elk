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
    elk_fnv1a_tests();
    elk_str_tests();
    elk_arena_tests();
    elk_pool_tests();
    elk_allocator_generic_api_tests();
    elk_string_interner_tests();
    elk_queue_ledger_tests();

    printf("\n\n*** Tests completed successfully. ***\n\n");
    return EXIT_SUCCESS;
}
