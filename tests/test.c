#include <stdlib.h>
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
    elk_parse_tests();
    elk_arena_tests();
    elk_pool_tests();
    elk_string_interner_tests();
    elk_queue_ledger_tests();
    elk_array_ledger_tests();
    elk_hash_table_tests();
    elk_hash_set_tests();
    elk_csv_tests();

    printf("\n\n*** Tests completed successfully. ***\n\n");
    return EXIT_SUCCESS;
}
