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

    elk_init_memory_debug(NULL, NULL, NULL);

    elk_time_tests();
    elk_list_tests();
    elk_queue_tests();
    elk_hilbert_tests();
    elk_rtree_view_tests();

    elk_finalize_memory_debug();

    printf("\n\n*** Tests completed successfully. ***\n\n");
    return EXIT_SUCCESS;
}
