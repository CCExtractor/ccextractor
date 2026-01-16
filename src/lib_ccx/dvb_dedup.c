#include "dvb_dedup.h"
#include <string.h>

void dvb_dedup_init(struct dvb_dedup_ring *ring)
{
	if (!ring)
		return;
	memset(ring, 0, sizeof(*ring));
	ring->head = 0;
}

int dvb_dedup_is_duplicate(struct dvb_dedup_ring *ring,
			    uint64_t pts, uint32_t pid,
			    uint16_t composition_id, uint16_t ancillary_id)
{
	if (!ring)
		return 0;

	for (int i = 0; i < DVB_DEDUP_RING_SIZE; i++)
	{
		const struct dvb_dedup_entry *e = &ring->entries[i];
		
		// Check if this entry matches
		if (e->pts == pts &&
		    e->pid == pid &&
		    e->composition_id == composition_id &&
		    e->ancillary_id == ancillary_id)
		{
			return 1; // Duplicate found
		}
	}
	
	return 0; // Not a duplicate
}

void dvb_dedup_add(struct dvb_dedup_ring *ring,
		   uint64_t pts, uint32_t pid,
		   uint16_t composition_id, uint16_t ancillary_id)
{
	if (!ring)
		return;

	// Add to ring buffer at head position
	ring->entries[ring->head].pts = pts;
	ring->entries[ring->head].pid = pid;
	ring->entries[ring->head].composition_id = composition_id;
	ring->entries[ring->head].ancillary_id = ancillary_id;

	// Advance head, wrapping around
	ring->head = (ring->head + 1) % DVB_DEDUP_RING_SIZE;
}
