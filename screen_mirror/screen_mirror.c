/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Patrick F. Kellogg
 *
 * FM-1 screen mirror, firmware-neutral version of the "Screen mirror" section
 * in app/screen.c of the screen-out branch. Messages (see README.md):
 *   in:  F0 7D 'F' 'S' 00 F7  (01: and send the whole screen again)
 *   out: F0 7D 'F' 'S' 01 row_lo row_hi col_lo col_hi {count c0-6 c7-13 c14-15}... F7
 * Each message is at most SM_MESSAGE bytes, so it shares a small USB queue with notes.
 */
#include "screen_mirror.h"

#ifndef SM_BUFFER
#define SM_BUFFER 16384u /* queued messages, each with a length byte in front */
#endif
#define SM_MESSAGE 70u
#define SM_TIMEOUT 3000u

/* (sm_ prefixed: a firmware built as one compilation unit, like ChoralRoot, may have its own "buffer", "length"...) */
static uint8_t sm_buffer[SM_BUFFER];
static unsigned sm_length, sm_sent, sm_live, sm_hello, sm_full;
static uint32_t sm_heard;

int sm_sysex(const uint8_t *m, unsigned n)
{
    if (n != 6 || m[0] != 0xf0 || m[1] != 0x7d || m[2] != 'F' || m[3] != 'S' || m[5] != 0xf7)
        return 0;
    sm_hello = 1u + (m[4] == 1);
    return 1;
}

int sm_on(void) { return sm_live || sm_hello; }

int sm_take_full_request(void)
{
    unsigned f = sm_full;
    sm_full = 0;
    return (int)f;
}

int sm_rows(const uint16_t *px, unsigned left, unsigned top, unsigned width, unsigned rows)
{
    unsigned before = sm_length;
    for (unsigned row = 0; row < rows; ++row, px += width)
        for (unsigned i = 0; i < width;) {
            if (sm_length + 1u + SM_MESSAGE > SM_BUFFER) { /* room for a whole message, or wait */
                if (!before) return 1; /* bigger than the whole buffer: sent as far as it fits */
                sm_length = before;
                return 0;
            }
            uint8_t *out = sm_buffer + sm_length;
            unsigned y = top + row, x = left + i, k = 10;
            out[1] = 0xf0; out[2] = 0x7d; out[3] = 'F'; out[4] = 'S'; out[5] = 1;
            out[6] = (uint8_t)(y & 127u); out[7] = (uint8_t)(y >> 7);
            out[8] = (uint8_t)(x & 127u); out[9] = (uint8_t)(x >> 7);
            for (; i < width && k + 5u <= SM_MESSAGE; k += 4) {
                unsigned run = 1, c = px[i];
                while (i + run < width && run < 127 && px[i + run] == c) ++run;
                out[k] = (uint8_t)run;
                out[k + 1] = (uint8_t)(c & 127u);
                out[k + 2] = (uint8_t)((c >> 7) & 127u);
                out[k + 3] = (uint8_t)(c >> 14);
                i += run;
            }
            out[k] = 0xf7;
            out[0] = (uint8_t)k;
            sm_length += 1u + k;
        }
    return 1;
}

void sm_poll(uint32_t now, sm_send_fn send)
{
    if (sm_hello) {
        sm_heard = now;
        if (!sm_live || sm_hello > 1) sm_full = 1;
        sm_hello = 0;
        sm_live = 1;
    }
    if (now - sm_heard >= SM_TIMEOUT) sm_live = 0;
    while (sm_live && sm_sent < sm_length) {
        int r = send(sm_buffer + sm_sent + 1, sm_buffer[sm_sent]);
        if (r == -2) break; /* busy: the rest next time */
        if (r) sm_live = 0;
        sm_sent += 1u + sm_buffer[sm_sent];
    }
    if (!sm_live || sm_sent >= sm_length) {
        sm_length = sm_sent = 0;
    } else if (sm_sent) {
        /* Move what's left to the front, so the room it freed takes new rows now (with a small buffer, waiting for
         * all of it to go first held the screen up too long). Forward copy: the data only moves down. */
        unsigned i, n = sm_length - sm_sent;
        for (i = 0; i < n; i++)
            sm_buffer[i] = sm_buffer[sm_sent + i];
        sm_length = n;
        sm_sent = 0;
    }
}
