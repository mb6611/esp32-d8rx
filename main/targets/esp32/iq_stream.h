/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Continuous decimated IQ stream (IQS1 frames) for the classic ESP32.
 * Implemented by iq_stream.c (U6b); called from receiver.c (U6a).
 * Frame format: docs/iq-stream.md. Link budget: docs/devkitv1/iq-stream-uart.md. */
#pragma once
#include <stdbool.h>
#include <stdint.h>

/* End-of-stream record: the twelve IQSEND fields, in print order (same as S3
 * SPECEND, docs/spectrum.md):
 *   IQSEND status detail units pairs elapsed_us late_max work_max frames drops
 *          abandoned ffts stopped_by_host
 * Loss accounting: `abandoned` = input pairs lost to bank-switch seams and abandoned
 * work, counted exactly (also mirrored in `ffts`, as on S3); `pairs` = input pairs
 * consumed. U6b fills every field; unused ones are 0.
 */
typedef struct {
    uint32_t status;       /* 0 = normal stop */
    uint32_t detail;
    uint32_t units;
    uint64_t pairs;        /* input pairs consumed */
    uint64_t elapsed_us;
    uint32_t late_max;
    uint32_t work_max;     /* worst per-bank DSP cycles */
    uint32_t frames;       /* IQS1 frames emitted */
    uint32_t drops;        /* frames dropped on output overflow */
    uint32_t abandoned;    /* input pairs lost (exact) */
    uint32_t ffts;         /* S3 prints input pairs lost here too; keep equal to abandoned */
    uint8_t stopped_by_host;
} iq_stream_stats_t;

/* Validate nothing: receiver.c has already checked args against the budget table.
 * Configure the ring/DSP for the run and send nothing. Returns false on failure
 * (receiver.c then replies `ERR capture`). rate_code is 6 (16 MS/s) only. */
bool iq_stream_start(unsigned ms, unsigned dec, unsigned bits, unsigned rate_code, unsigned shift, unsigned mode);

/* Emit IQS1 frames until ms elapses (0 = until a host byte or output blocked for
 * 2 s), then fill `*out` and return. receiver.c prints IQSEND from it. */
void iq_stream_run_until_stop(iq_stream_stats_t *out);

/* Hop following (experiments/14-hop-follow/notes.md), core 0 during a run only. */
uint64_t iq_stream_pair_now(void);
/* A retune ran over pairs [ps, pe] to channel slot `slot`: tag the output from
 * ceil(ps / dec) on. False if the event queue is full. */
bool iq_stream_hop_mark(uint64_t ps, uint64_t pe, unsigned slot);
/* With a hook set, the run reads host lines: an empty line stops it, any other
 * line goes to `fn` (called from the run loop). NULL restores stop-on-any-byte. */
typedef void (*iq_stream_line_fn)(const char *line);
void iq_stream_set_line_hook(iq_stream_line_fn fn);
