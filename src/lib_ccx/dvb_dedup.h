/*
 * DVB subtitle deduplication ring buffer
 * Prevents repeated subtitles from propagating to output files
 */

#ifndef DVBSUB_DEDUP_H
#define DVBSUB_DEDUP_H

#include <stdint.h>
#include <stdlib.h>

#define DVB_DEDUP_RING_SIZE 8

struct dvb_dedup_entry
{
	uint64_t pts;		 /* Presentation timestamp (ms) */
	uint32_t pid;		 /* Stream PID */
	uint16_t composition_id; /* DVB composition ID */
	uint16_t ancillary_id;	 /* DVB ancillary ID */
	uint8_t valid;		 /* Entry is valid */
};

struct dvb_dedup_ring
{
	struct dvb_dedup_entry entries[DVB_DEDUP_RING_SIZE];
	uint8_t head;  /* Next position to write */
	uint8_t count; /* Number of valid entries */
};

void dvb_dedup_init(struct dvb_dedup_ring *ring);
int dvb_dedup_is_duplicate(struct dvb_dedup_ring *ring, uint64_t pts, uint32_t pid, uint16_t composition_id, uint16_t ancillary_id);
void dvb_dedup_add(struct dvb_dedup_ring *ring, uint64_t pts, uint32_t pid, uint16_t composition_id, uint16_t ancillary_id);

#endif /* DVBSUB_DEDUP_H */
