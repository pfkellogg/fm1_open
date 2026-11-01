/* cc -Wall -Wextra screen_mirror.c test.c && ./a.out */
#include "screen_mirror.h"
#include <assert.h>
#include <stdio.h>
static uint8_t got[256]; static unsigned got_n;
static int send(const uint8_t *m, unsigned n) { for (unsigned i = 0; i < n; ++i) got[got_n++] = m[i]; return 0; }
int main(void)
{
    const uint8_t hi[] = {0xf0, 0x7d, 'F', 'S', 1, 0xf7};
    assert(sm_sysex(hi, 6) && sm_on());
    sm_poll(1000, send);
    assert(sm_take_full_request() && !sm_take_full_request());
    const uint16_t row[4] = {0xef5d, 0xef5d, 0xef5d, 0x07ff};
    assert(sm_rows(row, 30, 200, 4, 1));
    sm_poll(1100, send);
    const uint8_t want[] = {0xf0, 0x7d, 'F', 'S', 1, 200 & 127, 200 >> 7, 30, 0,
                            3, 0xef5d & 127, (0xef5d >> 7) & 127, 0xef5d >> 14,
                            1, 0x07ff & 127, (0x07ff >> 7) & 127, 0x07ff >> 14, 0xf7};
    assert(got_n == sizeof want);
    for (unsigned i = 0; i < got_n; ++i) assert(got[i] == want[i]);
    sm_poll(4200, send);
    assert(!sm_on()); /* stopped 3 s after the last hello */
    puts("ok");
    return 0;
}
