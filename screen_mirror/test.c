/* cc -Wall -Wextra screen_mirror.c test.c && ./a.out */
#include "screen_mirror.h"
#include <assert.h>
#include <stdio.h>
static uint8_t got[256]; static unsigned got_n;
static unsigned busy_after; /* >0: busy after that many messages */
static int send(const uint8_t *m, unsigned n)
{
    if (busy_after && got_n) return -2;
    for (unsigned i = 0; i < n && got_n < sizeof got; ++i) got[got_n++] = m[i];
    return 0;
}
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
    /* A busy output: what was sent leaves room at once (the buffer is compacted), not only once it's empty. */
    assert(sm_sysex(hi, 6));
    sm_poll(5000, send);
    sm_take_full_request();
    static uint16_t wide[240];
    for (unsigned i = 0; i < 240; ++i) wide[i] = (uint16_t)(i * 97u); /* every pixel different: many messages */
    unsigned rows = 0;
    while (sm_rows(wide, 0, rows, 240, 1)) ++rows; /* fill it */
    assert(rows > 0);
    busy_after = 1; got_n = 0;
    sm_poll(5100, send);                            /* one message goes, then busy */
    assert(sm_rows(wide, 0, 0, 1, 1));              /* room again for a small row */
    busy_after = 0;
    /* The sound's name: a name-only hello (02) sends it, but no screen; again after each hello, and when it changes. */
    sm_poll(9000, send);                            /* (the screen's viewer has gone: 3 s without a hello) */
    assert(!sm_on());
    const uint8_t name_hi[] = {0xf0, 0x7d, 'F', 'S', 2, 0xf7};
    assert(sm_sysex(name_hi, 6) && !sm_on());
    got_n = 0;
    sm_name("T1 FM6 \xB7ICE", 9100, send);
    const uint8_t name[] = {0xf0, 0x7d, 'F', 'S', 2, 'T', '1', ' ', 'F', 'M', '6', ' ', ' ', 'I', 'C', 'E', 0xf7};
    assert(got_n == sizeof name);
    for (unsigned i = 0; i < got_n; ++i) assert(got[i] == name[i]);
    got_n = 0;
    sm_name("T1 FM6 \xB7ICE", 9200, send);         /* the same: nothing */
    assert(got_n == 0);
    sm_name("T2 DRUM", 9300, send);                 /* a change: sent */
    assert(got_n == 13 && got[5] == 'T' && got[6] == '2');
    got_n = 0;
    sm_name("T3 X", 12200, send);                   /* 3 s since its hello: nobody's asking */
    assert(got_n == 0);
    puts("ok");
    return 0;
}
