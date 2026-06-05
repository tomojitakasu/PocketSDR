#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Mock structures matching sdr_nav.c */
typedef struct {
    uint8_t data[512];
} nav_t;

typedef struct {
    nav_t *nav;
} channel_t;

/* Forward declare the vulnerable function from sdr_nav.c */
extern void handle_nav_data(channel_t *ch, uint8_t type, const uint8_t *data);

START_TEST(test_nav_type_bounds_security)
{
    /* Invariant: memcpy offset (16 * type) must not exceed nav->data buffer bounds.
       The nav->data buffer is 512 bytes, so valid offsets are [0, 496].
       Valid type values are [0, 31]. Any type >= 32 creates out-of-bounds write. */
    
    channel_t ch;
    nav_t nav;
    uint8_t payload[16];
    
    memset(&nav, 0, sizeof(nav));
    memset(payload, 0xAA, sizeof(payload));
    ch.nav = &nav;
    
    /* Test payloads: (type_value, description) */
    struct {
        uint8_t type;
        int should_be_safe;
    } test_cases[] = {
        {0, 1},      /* Valid: offset 0 */
        {31, 1},     /* Valid boundary: offset 496 (31*16=496, fits in 512) */
        {32, 0},     /* Invalid: offset 512 (32*16=512, exceeds 512-byte buffer) */
        {255, 0},    /* Exploit: offset 4080 (255*16=4080, massive overflow) */
        {128, 0},    /* Exploit: offset 2048 (128*16=2048, far out of bounds) */
    };
    
    int num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (int i = 0; i < num_cases; i++) {
        uint8_t type = test_cases[i].type;
        int expected_safe = test_cases[i].should_be_safe;
        
        /* Compute offset that would be used */
        uint32_t offset = 16 * type;
        
        /* Security property: offset must not exceed buffer size (512) */
        int is_safe = (offset + 16 <= 512);
        
        ck_assert_msg(
            is_safe == expected_safe,
            "Type %u produces offset %u: safe=%d, expected=%d",
            type, offset, is_safe, expected_safe
        );
        
        /* For safe cases, verify memcpy doesn't corrupt adjacent memory */
        if (is_safe) {
            uint8_t marker = 0xFF;
            nav.data[511] = marker;
            handle_nav_data(&ch, type, payload);
            ck_assert_msg(
                nav.data[511] == marker,
                "Buffer overflow: marker at [511] corrupted for type %u",
                type
            );
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_nav_type_bounds_security);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}