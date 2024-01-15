#include "test.h"

#include <inttypes.h>
#include <string.h>

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                  FNV1a Hash function
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
struct fnv1a_test_pair
{
    char *str;
    u64 hash;
};

/* Test some known hashes.
 *
 * Test values copied from http://www.isthe.com/chongo/src/fnv/test_fnv.c, which is in the public
 * domain.
 */
struct fnv1a_test_pair fnv1a_pairs[] =
{
    {"", UINT64_C(0xcbf29ce484222325)},
    {"a", UINT64_C(0xaf63dc4c8601ec8c)},
    {"b", UINT64_C(0xaf63df4c8601f1a5)},
    {"c", UINT64_C(0xaf63de4c8601eff2)},
    {"d", UINT64_C(0xaf63d94c8601e773)},
    {"e", UINT64_C(0xaf63d84c8601e5c0)},
    {"f", UINT64_C(0xaf63db4c8601ead9)},
    {"fo", UINT64_C(0x08985907b541d342)},
    {"foo", UINT64_C(0xdcb27518fed9d577)},
    {"foob", UINT64_C(0xdd120e790c2512af)},
    {"fooba", UINT64_C(0xcac165afa2fef40a)},
    {"foobar", UINT64_C(0x85944171f73967e8)},
    {"ch", UINT64_C(0x08a25607b54a22ae)},
    {"cho", UINT64_C(0xf5faf0190cf90df3)},
    {"chon", UINT64_C(0xf27397910b3221c7)},
    {"chong", UINT64_C(0x2c8c2b76062f22e0)},
    {"chongo", UINT64_C(0xe150688c8217b8fd)},
    {"chongo ", UINT64_C(0xf35a83c10e4f1f87)},
    {"chongo w", UINT64_C(0xd1edd10b507344d0)},
    {"chongo wa", UINT64_C(0x2a5ee739b3ddb8c3)},
    {"chongo was", UINT64_C(0xdcfb970ca1c0d310)},
    {"chongo was ", UINT64_C(0x4054da76daa6da90)},
    {"chongo was h", UINT64_C(0xf70a2ff589861368)},
    {"chongo was he", UINT64_C(0x4c628b38aed25f17)},
    {"chongo was her", UINT64_C(0x9dd1f6510f78189f)},
    {"chongo was here", UINT64_C(0xa3de85bd491270ce)},
    {"chongo was here!", UINT64_C(0x858e2fa32a55e61d)},
    {"chongo was here!\n", UINT64_C(0x46810940eff5f915)},
    {NULL, UINT64_C(0)}
};

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_fnv1a_tests(void)
{
    struct fnv1a_test_pair *next = &fnv1a_pairs[0];
    while (next->str) 
    {
        uint64_t calc_hash = elk_fnv1a_hash(strlen(next->str), next->str);
        Assert(calc_hash == next->hash);
        next++;
    }
}
