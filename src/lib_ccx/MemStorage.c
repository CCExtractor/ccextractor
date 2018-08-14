#include "types.h"

void init_MemStorage(MemStorage* storage, int block_size)
{
    if(block_size <= 0)
        block_size = (1<<16) - 128;

    block_size = Align(block_size, (int)sizeof(double));
    assert(sizeof(MemBlock)%(int)sizeof(double) == 0);

    memset(storage, 0, sizeof(*storage));
    storage->signature = 0x42890000;
    storage->block_size = block_size;
}

/** Remember a storage "free memory" position */
void SaveMemStoragePos(const MemStorage* storage, MemStoragePos* pos);\
{
    pos->top = storage->top;
    pos->free_space = storage->free_space;
}

/** Restore a storage "free memory" position */
void RestoreMemStoragePos(MemStorage* storage, MemStoragePos* pos )
{
    if(!storage || !pos)
        fatal("NULL Pointer");
    if(pos->free_space > storage->block_size)
        fatal("Bad size error");

    storage->top = pos->top;
    storage->free_space = pos->free_space;

    if(!storage->top)
    {
        storage->top = storage->bottom;
        storage->free_space = storage->top ? storage->block_size - sizeof(MemBlock) : 0;
    }
}

/* Moves stack pointer to next block.
   If no blocks, allocate new one and link it to the storage: */
static void GoNextMemBlock(MemStorage* storage)
{
    if(!storage)
        fatal("NULL Pointer");

    if(!storage->top || !storage->top->next)
    {
        MemBlock *block;

        if(!(storage->parent))
        {
            block = malloc(storage->block_size);
        }
        else
        {
            MemStorage *parent = storage->parent;
            MemStoragePos parent_pos;

            SaveMemStoragePos(parent, &parent_pos);
            GoNextMemBlock(parent);

            block = parent->top;
            RestoreMemStoragePos(parent, &parent_pos);

            if(block == parent->top)  /* the single allocated block */
            {
                assert(parent->bottom == block);
                parent->top = parent->bottom = 0;
                parent->free_space = 0;
            }
            else
            {
                /* cut the block from the parent's list of blocks */
                parent->top->next = block->next;
                if(block->next)
                    block->next->prev = parent->top;
            }
        }

        /* link block */
        block->next = 0;
        block->prev = storage->top;

        if(storage->top)
            storage->top->next = block;
        else
            storage->top = storage->bottom = block;
    }

    if(storage->top->next)
        storage->top = storage->top->next;
    storage->free_space = storage->block_size - sizeof(MemBlock);
    assert(storage->free_space%(int)sizeof(double) == 0);
}

/* Initialize allocated storage: */
void icvInitMemStorage(MemStorage* storage, int block_size)
{
    if(!storage)
        fatal("NULL Pointer");

    if(block_size <= 0)
        block_size = (1<<16) - 128;

    block_size = Align(block_size, (int)sizeof(double));
    assert(sizeof(MemBlock)%(int)sizeof(double) == 0);

    memset(storage, 0, sizeof(*storage));
    storage->signature = 0x42890000;
    storage->block_size = block_size;
}

/** Creates new memory storage.
   block_size == 0 means that default,
   somewhat optimal size, is used (currently, it is 64K) */
MemStorage* CreateMemStorage(int block_size)
{
    MemStorage* storage = malloc(sizeof(MemStorage));
    icvInitMemStorage(storage, block_size);
    return storage;
}

/** Creates a memory storage that will borrow memory blocks from parent storage */
MemStorage* CreateChildMemStorage(MemStorage* parent)
{
    MemStorage* storage = CreateMemStorage(parent->block_size);
    storage->parent = parent;

    return storage;
}

/* Release all blocks of the storage (or return them to parent, if any): */
static void icvDestroyMemStorage(MemStorage* storage)
{
    int k = 0;

    MemBlock* block;
    MemBlock* dst_top = 0;

    if(!storage)
        fatal("NULL Pointer Error");

    if(storage->parent)
        dst_top = storage->parent->top;

    for(block = storage->bottom; block != 0; k++)
    {
        MemBlock* temp = block;

        block = block->next;
        if(storage->parent)
        {
            if(dst_top)
            {
                temp->prev = dst_top;
                temp->next = dst_top->next;
                if(temp->next)
                    temp->next->prev = temp;
                dst_top = dst_top->next = temp;
            }
            else
            {
                dst_top = storage->parent->bottom = storage->parent->top = temp;
                temp->prev = temp->next = 0;
                storage->free_space = storage->block_size - sizeof(*temp);
            }
        }
        else
            free(&temp);
    }

    storage->top = storage->bottom = 0;
    storage->free_space = 0;
}

/* Release memory storage: */
void ReleaseMemStorage(MemStorage** storage)
{
    if(!storage)
        fatal("NULL Pointer Error");

    MemStorage* st = *storage;
    *storage = 0;
    if(st)
    {
        icvDestroyMemStorage(st);
        free(&st);
    }
}

