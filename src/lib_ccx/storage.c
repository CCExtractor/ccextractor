#include <stdbool.h>
#include <stdio.h>
#include "types.h"

inline char* cv_skip_BOM(char* ptr)
{
    if((unsigned char)ptr[0] == 0xef && (unsigned char)ptr[1] == 0xbb && (unsigned char)ptr[2] == 0xbf) //UTF-8 BOM
    {
      return ptr + 3;
    }
    return ptr;
}

/* The function allocates space for at least one more sequence element.
   If there are free sequence blocks (seq->free_blocks != 0)
   they are reused, otherwise the space is allocated in the storage: */
static void icvGrowSeq(Seq *seq, int in_front_of)
{
    SeqBlock *block;

    if(!seq)
        fatal("NULL Pointer Error");

    block = seq->free_blocks;

    if(!block)
    {
        int elem_size = seq->elem_size;
        int delta_elems = seq->delta_elems;
        MemStorage *storage = seq->storage;

        if(seq->total >= delta_elems*4)
            SetSeqBlockSize(seq, delta_elems*2);

        if(!storage)
            fatal("The sequence has NULL storage pointer");

        /* If there is a free space just after last allocated block
           and it is big enough then enlarge the last block.
           This can happen only if the new block is added to the end of sequence: */
        if( (size_t)(ICV_FREE_PTR(storage) - seq->block_max) < (int)sizeof(double) &&
            storage->free_space >= seq->elem_size && !in_front_of )
        {
            int delta = storage->free_space / elem_size;

            delta = min(delta, delta_elems) * elem_size;
            seq->block_max += delta;
            storage->free_space = AlignLeft((int)(((signed char*)storage->top + storage->block_size) -
                                              seq->block_max), (int)sizeof(double));
            return;
        }
        else
        {
            int delta = elem_size * delta_elems + ICV_ALIGNED_SEQ_BLOCK_SIZE;

            /* Try to allocate <delta_elements> elements: */
            if(storage->free_space < delta)
            {
                int small_block_size = max(1, delta_elems/3)*elem_size +
                                       ICV_ALIGNED_SEQ_BLOCK_SIZE;
                /* try to allocate smaller part */
                if(storage->free_space >= small_block_size + (int)sizeof(double))
                {
                    delta = (storage->free_space - ICV_ALIGNED_SEQ_BLOCK_SIZE)/seq->elem_size;
                    delta = delta*seq->elem_size + ICV_ALIGNED_SEQ_BLOCK_SIZE;
                }
                else
                {
                    GoNextMemBlock(storage);
                    assert(storage->free_space >= delta);
                }
            }

            block = (SeqBlock*)MemStorageAlloc(storage, delta);
            block->data = (signed char*)AlignPtr(block + 1, (int)sizeof(double));
            block->count = delta - ICV_ALIGNED_SEQ_BLOCK_SIZE;
            block->prev = block->next = 0;
        }
    }
    else
    {
        seq->free_blocks = block->next;
    }

    if(!(seq->first))
    {
        seq->first = block;
        block->prev = block->next = block;
    }
    else
    {
        block->prev = seq->first->prev;
        block->next = seq->first;
        block->prev->next = block->next->prev = block;
    }

    /* For free blocks the <count> field means
     * total number of bytes in the block.
     *
     * For used blocks it means current number
     * of sequence elements in the block:
     */
    assert(block->count % seq->elem_size == 0 && block->count > 0);

    if(!in_front_of)
    {
        seq->ptr = block->data;
        seq->block_max = block->data + block->count;
        block->start_index = block == block->prev ? 0 :
            block->prev->start_index + block->prev->count;
    }
    else
    {
        int delta = block->count / seq->elem_size;
        block->data += block->count;

        if(block != block->prev)
        {
            assert(seq->first->start_index == 0);
            seq->first = block;
        }
        else
        {
            seq->block_max = seq->ptr = block->data;
        }

        block->start_index = 0;

        for( ;; )
        {
            block->start_index += delta;
            block = block->next;
            if( block == seq->first )
                break;
        }
    }

    block->count = 0;
}

void* MemStorageAlloc(MemStorage* storage, size_t size)
{
    signed char *ptr = 0;
    if(!storage)
        fatal("NULL storage pointer");

    if(size > INT_MAX)
        fatal("Too large memory block is requested");

    assert(storage->free_space%CV_STRUCT_ALIGN == 0);

    if((size_t)storage->free_space < size)
    {
        size_t max_free_space = AlignLeft(storage->block_size - sizeof(MemBlock), CV_STRUCT_ALIGN);
        if(max_free_space < size)
            fatal("requested size is negative or too big");

        GoNextMemBlock(storage);
    }

    ptr = ICV_FREE_PTR(storage);
    assert((size_t)ptr % (int)sizeof(double) == 0);
    storage->free_space = AlignLeft(storage->free_space - (int)size, CV_STRUCT_ALIGN );

    return ptr;
}

char* MemStorageAllocString(MemStorage* storage, const char* ptr, int len)
{
    char* str;
    memset(&str, 0, sizeof(char**));

    int length = len >= 0 ? len : (int)strlen(ptr);
    str = len >= 0 ? malloc(sizeof(char)*len) : (int)strlen(ptr);
    str = (char*)MemStorageAlloc(storage, length + 1);
    memcpy(str, ptr, strlen(str));
    str[strlen(str)] = '\0';

    return str;
}

char* icvGets(CvFileStorage* fs, char* str, int maxCount)
{
    if(fs->file)
    {
        char* ptr = fgets(str, maxCount, fs->file);
        if(ptr && maxCount > 256 && !(fs->flags & 64))
        {
            size_t sz = strnlen(ptr, maxCount);
            assert(sz < (size_t)(maxCount - 1));
        }
        return ptr;
    }
}

/* Update sequence header: */
void FlushSeqWriter(SeqWriter* writer)
{
    if(!writer)
        fatal("NULL Pointer Error");

    Seq* seq = writer->seq;
    seq->ptr = writer->ptr;

    if(writer->block)
    {
        int total = 0;
        SeqBlock *first_block = writer->seq->first;
        SeqBlock *block = first_block;

        writer->block->count = (int)((writer->ptr - writer->block->data) / seq->elem_size);
        assert( writer->block->count > 0 );

        do
        {
            total += block->count;
            block = block->next;
        }
        while( block != first_block );

        writer->seq->total = total;
    }
}

/* Create new sequence block: */
void CreateSeqBlock(SeqWriter* writer)
{
    if( !writer || !writer->seq )
        fatal("NULL Pointer Error");

    Seq* seq = writer->seq;

    FlushSeqWriter( writer );

    icvGrowSeq( seq, 0 );

    writer->block = seq->first->prev;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;
}

/* Change the current reading block
 * to the previous or to the next:
 */
void ChangeSeqBlock(void* _reader, int direction)
{
    SeqReader* reader = (SeqReader*)_reader;

    if( direction > 0 )
    {
        reader->block = reader->block->next;
        reader->ptr = reader->block->data;
    }
    else
    {
        reader->block = reader->block->prev;
        reader->ptr = CV_GET_LAST_ELEM( reader->seq, reader->block );
    }
    reader->block_min = reader->block->data;
    reader->block_max = reader->block_min + reader->block->count * reader->seq->elem_size;
}

/* Set reader position to given position,
 * either absolute or relative to the
 *  current one:
 */
void SetSeqReaderPos(SeqReader* reader, int index, int is_relative)
{
    SeqBlock *block;
    int elem_size, count, total;

    total = reader->seq->total;
    elem_size = reader->seq->elem_size;

    if( !is_relative )
    {
        if( index < 0 )
        {
            index += total;
        }
        else if( index >= total )
        {
            index -= total;
        }

        block = reader->seq->first;
        if( index >= (count = block->count) )
        {
            if( index + index <= total )
            {
                do
                {
                    block = block->next;
                    index -= count;
                }
                while( index >= (count = block->count) );
            }
            else
            {
                do
                {
                    block = block->prev;
                    total -= block->count;
                }
                while( index < total );
                index -= total;
            }
        }
        reader->ptr = block->data + index * elem_size;
        if( reader->block != block )
        {
            reader->block = block;
            reader->block_min = block->data;
            reader->block_max = block->data + block->count * elem_size;
        }
    }
    else
    {
        signed char* ptr = reader->ptr;
        index *= elem_size;
        block = reader->block;

        if( index > 0 )
        {
            while( ptr + index >= reader->block_max )
            {
                int delta = (int)(reader->block_max - ptr);
                index -= delta;
                reader->block = block = block->next;
                reader->block_min = ptr = block->data;
                reader->block_max = block->data + block->count*elem_size;
            }
            reader->ptr = ptr + index;
        }
        else
        {
            while( ptr + index < reader->block_min )
            {
                int delta = (int)(ptr - reader->block_min);
                index += delta;
                reader->block = block = block->prev;
                reader->block_min = block->data;
                reader->block_max = ptr = block->data + block->count*elem_size;
            }
            reader->ptr = ptr + index;
        }
    }
}

void SetSeqBlockSize(Seq *seq, int delta_elements)
{
    int elem_size;
    int useful_block_size;

    if( !seq || !seq->storage )
        fatal("NULL Pointer");
    if( delta_elements < 0 )
        fatal("Out of Range Error");

    useful_block_size = AlignLeft(seq->storage->block_size - sizeof(MemBlock) -
                                    sizeof(SeqBlock), CV_STRUCT_ALIGN);
    elem_size = seq->elem_size;

    if(delta_elements == 0)
    {
        delta_elements = (1 << 10)/elem_size;
        delta_elements = MAX(delta_elements, 1);
    }
    if( delta_elements * elem_size > useful_block_size )
    {
        delta_elements = useful_block_size / elem_size;
        if( delta_elements == 0 )
            fatal("Storage block size is too small "
                    "to fit the sequence elements");
    }

    seq->delta_elems = delta_elements;
}

Set* CreateSet(int set_flags, int header_size, int elem_size, MemStorage* storage)
{
    if(!storage)
        fatal("NULL Pointer");
    if(header_size < (int)sizeof(Set) ||
        elem_size < (int)sizeof(void*)*2 ||
        (elem_size & (sizeof(void*)-1)) != 0 )
        fatal("NULL Pointer");

    Set* set = (Set*)CreateSeq( set_flags, header_size, elem_size, storage );
    set->flags = (set->flags & ~0xFFFF0000) | 0x42980000;

    return set;
}


StringHash* CreateMap(int flags, int header_size, int elem_size, CvMemStorage* storage, int start_tab_size)
{
    if( header_size < (int)sizeof(StringHash) )
        fatal("Too small map header_size");

    if( start_tab_size <= 0 )
        start_tab_size = 16;

    StringHash* map = (StringHash*)CreateSet(flags, header_size, elem_size, storage);

    map->tab_size = start_tab_size;
    start_tab_size *= sizeof(map->table[0]);
    map->table = (void**)MemStorageAlloc( storage, start_tab_size );
    memset(map->table, 0, start_tab_size);

    return map;
}

Seq* CreateSeq(int seq_flags, size_t header_size, size_t elem_size, MemStorage* storage)
{
    Seq *seq = 0;

    if(!storage)
        fatal("NULL Pointer");
    if(header_size < sizeof(Seq) || elem_size <= 0)
        fatal("Bad sizr error");

    /* allocate sequence header */
    seq = (Seq*)MemStorageAlloc(storage, header_size);
    memset(seq, 0, header_size);

    seq->header_size = (int)header_size;
    seq->flags = (seq_flags & ~CV_MAGIC_MASK) | CV_SEQ_MAGIC_VAL;
    {
        int elemtype = CV_MAT_TYPE(seq_flags);
        int typesize = CV_ELEM_SIZE(elemtype);

        if(elemtype != CV_SEQ_ELTYPE_GENERIC && elemtype != CV_USRTYPE1 &&
            typesize != 0 && typesize != (int)elem_size)
            fatal("Specified element size doesn't match to the size of the specified element type "
            "(try to use 0 for element type)");
    }
    seq->elem_size = (int)elem_size;
    seq->storage = storage;

    SetSeqBlockSize(seq, (int)((1 << 10)/elem_size));

    return seq;
}

signed char* SeqPush(Seq *seq, const void *element)
{
    signed char *ptr = 0;
    size_t elem_size;

    if(!seq)
        fatal("NULL Pointer Error");

    elem_size = seq->elem_size;
    ptr = seq->ptr;

    if(ptr >= seq->block_max)
    {
        icvGrowSeq(seq, 0);

        ptr = seq->ptr;
        assert( ptr + elem_size <= seq->block_max /*&& ptr == seq->block_min */);
    }

    if(element)
        memcpy( ptr, element, elem_size );
    seq->first->prev->count++;
    seq->total++;
    seq->ptr = ptr + elem_size;

    return ptr;
}

/* Initialize sequence reader: */
void StartReadSeq(const Seq *seq, SeqReader* reader, int reverse)
{
    SeqBlock *first_block;
    SeqBlock *last_block;

    if(reader)
    {
        reader->seq = 0;
        reader->block = 0;
        reader->ptr = reader->block_max = reader->block_min = 0;
    }

    if(!seq || !reader)
        fatal("NULL Pointer Error");

    reader->header_size = sizeof(SeqReader);
    reader->seq = (Seq*)seq;

    first_block = seq->first;

    if(first_block)
    {
        last_block = first_block->prev;
        reader->ptr = first_block->data;
        reader->prev_elem = CV_GET_LAST_ELEM(seq, last_block);
        reader->delta_index = seq->first->start_index;

        if(reverse)
        {
            signed char *temp = reader->ptr;

            reader->ptr = reader->prev_elem;
            reader->prev_elem = temp;

            reader->block = last_block;
        }
        else
        {
            reader->block = first_block;
        }

        reader->block_min = reader->block->data;
        reader->block_max = reader->block_min + reader->block->count * seq->elem_size;
    }
    else
    {
        reader->delta_index = 0;
        reader->block = 0;

        reader->ptr = reader->prev_elem = reader->block_min = reader->block_max = 0;
    }
}

/** @brief Initializes Freeman chain reader.

   The reader is used to iteratively get coordinates of all the chain points.
   If the Freeman codes should be read as is, a simple sequence reader should be used
@see cvApproxChains
*/
void StartReadChainPoints(Chain * chain, ChainPtReader* reader)
{
    int i;

    if(!chain || !reader)
        fatal("NULL Pointer Error");

    if(chain->elem_size != 1 || chain->header_size < (int)sizeof(Chain))
        fatal("Bad size error");

    StartReadSeq((Seq*)chain, (SeqReader*)reader, 0);

    reader->pt = chain->origin;
    for( i = 0; i < 8; i++ )
    {
        reader->deltas[i][0] = (signed char) icvCodeDeltas[i].x;
        reader->deltas[i][1] = (signed char) icvCodeDeltas[i].y;
    }
}

/* Initialize sequence writer: */
void StartAppendToSeq(Seq *seq, SeqWriter * writer)
{
    if(!seq || !writer)
        fatal("NULL Pointer Error");

    memset(writer, 0, sizeof(*writer));
    writer->header_size = sizeof(SeqWriter);

    writer->seq = seq;
    writer->block = seq->first ? seq->first->prev : 0;
    writer->ptr = seq->ptr;
    writer->block_max = seq->block_max;
}

/* Initialize sequence writer: */
void StartWriteSeq(int seq_flags, int header_size,
                 int elem_size, MemStorage* storage, SeqWriter* writer)
{
    if( !storage || !writer )
        fatal("NULL Pointer Error");

    Seq* seq = CreateSeq(seq_flags, header_size, elem_size, storage);
    StartAppendToSeq(seq, writer);
}

/* Calls icvFlushSeqWriter and finishes writing process: */
Seq* EndWriteSeq(SeqWriter* writer)
{
    if(!writer)
        fatal("NULL Pointer Error");

    FlushSeqWriter(writer);
    Seq* seq = writer->seq;

    /* Truncate the last block: */
    if(writer->block && writer->seq->storage)
    {
        MemStorage *storage = seq->storage;
        signed char *storage_block_max = (signed char *) storage->top + storage->block_size;

        assert(writer->block->count > 0);

        if((unsigned)((storage_block_max - storage->free_space)
            - seq->block_max) < (int)sizeof(double) )
        {
            storage->free_space = AlignLeft((int)(storage_block_max - seq->ptr), (int)sizeof(double));
            seq->block_max = seq->ptr;
        }
    }

    writer->ptr = 0;
    return seq;
}

SeqReader init_SeqReader()
{
    SeqReader sr;
    sr.header_size = 0;
    sr.seq = 0;
    sr.block = 0;
    sr.ptr = 0;
    sr.block_min = 0
    sr.block_max = 0;
    sr.delta_index = 0;
    sr.prev_elem = 0;
    return sr;
}

SeqIterator begin_it(Seq* seq)
{
    SeqIterator it;
    StartReadSeq(seq, false);
    index = 0;
}

void getNextIterator_Seq(SeqIterator* it)
{
    CV_NEXT_SEQ_ELEM(sizeof(Seq*), it->ptr);
    if(++(it->index) >= it->seq->total*2)
        it->index = 0;
}

/* Add new element to the set: */
int SetAdd(Set* set, SetElem* element, SetElem** inserted_element)
{
    int id = -1;
    SetElem *free_elem;

    if(!set)
        fatal("NULL Pointer Error");

    if(!(set->free_elems))
    {
        int count = set->total;
        int elem_size = set->elem_size;
        signed char *ptr;
        icvGrowSeq((Seq *)set, 0 );

        set->free_elems = (SetElem*)(ptr = set->ptr);
        for( ; ptr + elem_size <= set->block_max; ptr += elem_size, count++ )
        {
            ((SetElem*)ptr)->flags = count | CV_SET_ELEM_FREE_FLAG;
            ((SetElem*)ptr)->next_free = (SetElem*)(ptr + elem_size);
        }
        assert( count <= CV_SET_ELEM_IDX_MASK+1 );
        ((SetElem*)(ptr - elem_size))->next_free = 0;
        set->first->prev->count += count - set->total;
        set->total = count;
        set->ptr = set->block_max;
    }

    free_elem = set->free_elems;
    set->free_elems = free_elem->next_free;

    id = free_elem->flags & CV_SET_ELEM_IDX_MASK;
    if(element)
        memcpy(free_elem, element, set->elem_size);

    free_elem->flags = id;
    set->active_count++;

    if(inserted_element)
        *inserted_element = free_elem;

    return id;
}

/** Fast variant of SetAdd */
inline SetElem* SetNew(Set* set_header)
{
    SetElem* elem = set_header->free_elems;
    if(elem)
    {
        set_header->free_elems = elem->next_free;
        elem->flags = elem->flags & CV_SET_ELEM_IDX_MASK;
        set_header->active_count++;
    }
    else
        SetAdd(set_header, NULL, &elem);
    return elem;
}

StringHashNode* GetHashedKey(CvFileStorage* fs, const char* str, int len, int create_missing)
{
    StringHashNode* node = 0;
    unsigned hashval = 0;
    int i, tab_size;

    if(!fs)
        return 0;

    StringHash* map = fs->str_hash;

    if(len < 0)
    {
        for( i = 0; str[i] != '\0'; i++ )
            hashval = hashval*33 + (unsigned char)str[i];
        len = i;
    }
    else for(i = 0; i < len; i++)
        hashval = hashval*33 + (unsigned char)str[i];

    hashval &= INT_MAX;
    tab_size = map->tab_size;
    if((tab_size & (tab_size - 1)) == 0)
        i = (int)(hashval & (tab_size - 1));
    else
        i = (int)(hashval % tab_size);

    for(node = (StringHashNode*)(map->table[i]); node != 0; node = node->next)
    {
        if(node->hashval == hashval &&
            node->str.len == len &&
            memcmp(node->str.ptr, str, len) == 0)
            break;
    }
    if(!node && create_missing)
    {
        node = (StringHashNode*)SetNew((Set*)map);
        node->hashval = hashval;
        node->str = MemStorageAllocString( map->storage, str, len );
        node->next = (StringHashNode*)(map->table[i]);
        map->table[i] = node;
    }

    return node;
}

/* Find a sequence element by its index: */
signed char* GetSeqElem(const Seq *seq, int index)
{
    SeqBlock *block;
    int count, total = seq->total;

    if((unsigned)index >= (unsigned)total)
    {
        index += index < 0 ? total : 0;
        index -= index >= total ? total : 0;
        if((unsigned)index >= (unsigned)total)
            return 0;
    }

    block = seq->first;
    if(index + index <= total)
    {
        while(index >= (count = block->count))
        {
            block = block->next;
            index -= count;
        }
    }
    else
    {
        do
        {
            block = block->prev;
            total -= block->count;
        }
        while(index < total);
        index -= total;
    }

    return block->data + index * seq->elem_size;
}

void* NextTreeNode(TreeNodeIterator* treeIterator)
{
    TreeNode* prevNode = 0;
    TreeNode* node;
    int level;

    if(!treeIterator)
        fatal("NULL iterator pointer");

    prevNode = node = (TreeNode*)treeIterator->node;
    level = treeIterator->level;

    if(node)
    {
        if(node->v_next && level+1 < treeIterator->max_level)
        {
            node = node->v_next;
            level++;
        }
        else
        {
            while(node->h_next == 0)
            {
                node = node->v_prev;
                if( --level < 0 )
                {
                    node = 0;
                    break;
                }
            }
            node = node && treeIterator->max_level != 0 ? node->h_next : 0;
        }
    }

    treeIterator->node = node;
    treeIterator->level = level;
    return prevNode;
}

void InitTreeNodeIterator(TreeNodeIterator* treeIterator, const void* first, int max_level)
{
    if(!treeIterator || !first)
        fatal("NULL Pointer Error");

    if(max_level < 0)
        fatal("Out of Range Error");

    treeIterator->node = (void*)first;
    treeIterator->level = 0;
    treeIterator->max_level = max_level;
}

Seq* TreeToNodeSeq(const void* first, int header_size, MemStorage* storage)
{
    Seq* allseq = 0;
    TreeNodeIterator iterator;

    if( !storage )
        fatal("NULL storage pointer");

    allseq = CreateSeq(0, header_size, sizeof(first), storage);

    if(first)
    {
        InitTreeNodeIterator(&iterator, first, INT_MAX);

        for(;;)
        {
            void* node = NextTreeNode( &iterator );
            if(!node)
                break;
            SeqPush(allseq, &node);
        }
    }
    return allseq;
}

/****************************************************************************************\
*                            Common macros and type definitions                          *
\****************************************************************************************/
#define CV_FILE_STORAGE ('Y' + ('A' << 8) + ('M' << 16) + ('L' << 24))
#define cv_isprint(c)     ((unsigned char)(c) >= (unsigned char)' ')

inline bool cv_isdigit(char c)
{
    return '0' <= c && c <= '9';
}

inline bool cv_isalpha(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

inline bool cv_isalnum(char c)
{
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

inline bool cv_isspace(char c)
{
    return (9 <= c && c <= 13) || c == ' ';
}

void icvFSCreateCollection(CvFileStorage* fs, int tag, CvFileNode* collection)
{
    if(CV_NODE_IS_MAP(tag))
        collection->data.map = CreateMap(0, sizeof(StringHash), sizeof(FileMapNode), fs->memstorage, 16);
    else
    {
        Seq* seq;
        seq = CreateSeq(0, sizeof(Seq), sizeof(CvFileNode), fs->memstorage);

        // if <collection> contains some scalar element, add it to the newly created collection
        if(CV_NODE_TYPE(collection->tag) != 0)
            SeqPush(seq, collection);

        collection->data.seq = seq;
    }

    collection->tag = tag;
    SetSeqBlockSize(collection->data.seq, 8);
}

/****************************************************************************************\
*                            		XML parser                          				 *
\****************************************************************************************/

static char* icvXMLSkipSpaces(CvFileStorage* fs, char* ptr, int mode)
{
    int level = 0;

    for(;;)
    {
        char c;
        ptr--;

        if(mode == 2 || mode == 0)
        {
            do c = *++ptr;
            while(c == ' ' || c == '\t');

            if(c == '<' && ptr[1] == '!' && ptr[2] == '-' && ptr[3] == '-')
            {
                mode = 1;
                ptr += 4;
            }
            else if(cv_isprint(c))
                break;
        }

        if(!cv_isprint(*ptr))
        {
            int max_size = (int)(fs->buffer_end - fs->buffer_start);

            ptr = icvGets(fs, fs->buffer_start, max_size);
            if(!ptr)
            {
                ptr = fs->buffer_start;  // FIXIT Why do we need this hack? What is about other parsers JSON/YAML?
                *ptr = '\0';
                fs->dummy_eof = 1;
                break;
            }
            else
                int l = (int)strlen(ptr);

            fs->lineno++;  // FIXIT doesn't really work with long lines. It must be counted via '\n' or '\r' symbols, not the number of icvGets() calls.
        }
    }
    return ptr;
}

static char* icvXMLParseTag(CvFileStorage* fs, char* ptr, StringHashNode** _tag, AttrList** _list, int* _tag_type)
{
    int tag_type = 0;
    StringHashNode* tagname = 0;
    AttrList *first = 0, *last = 0;
    int count = 0, max_count = 4;
    int attr_buf_size = (max_count*2 + 1)*sizeof(char*) + sizeof(AttrList);
    char* endptr;
    char c;
    int have_space;

    ptr++;
    if(cv_isalnum(*ptr) || *ptr == '_')
        tag_type = 1;
    else if(*ptr == '/')
    {
        tag_type = 2;
        ptr++;
    }
    else if(*ptr == '?')
    {
        tag_type = 4;
        ptr++;
    }
    else if(*ptr == '!')
    {
        tag_type = 5;
        assert( ptr[1] != '-' || ptr[2] != '-' );
        ptr++;
    }

    for(;;)
    {
        StringHashNode* attrname;

        endptr = ptr - 1;
        do c = *++endptr;
        while(cv_isalnum(c) || c == '_' || c == '-');

        attrname = GetHashedKey(fs, ptr, (int)(endptr - ptr), 1);
        assert(attrname);
        ptr = endptr;

        if(!tagname)
            tagname = attrname;
        else
        {
            if(!last || count >= max_count)
            {
                AttrList* chunk;

                chunk = (AttrList*)MemStorageAlloc(fs->memstorage, attr_buf_size);
                memset(chunk, 0, attr_buf_size);
                chunk->attr = (const char**)(chunk + 1);
                count = 0;
                if(!last)
                    first = last = chunk;
                else
                    last = last->next = chunk;
            }
            last->attr[count*2] = attrname->str.ptr;
        }

        if(last)
        {
            CvFileNode stub;

            if(*ptr != '=')
                ptr = icvXMLSkipSpaces(fs, ptr, 2);

            c = *++ptr;
            if( c != '\"' && c != '\'' )
                ptr = icvXMLSkipSpaces( fs, ptr, 2 );

            ptr = icvXMLParseValue(fs, ptr, &stub, 3);
            assert(stub.tag == 3);
            last->attr[count*2+1] = stub.data.str.ptr;
            count++;
        }

        c = *ptr;
        have_space = cv_isspace(c) || c == '\0';

        if(c != '>')
        {
            ptr = icvXMLSkipSpaces(fs, ptr, 2);
            c = *ptr;
        }

        if(c == '>')
        {
            ptr++;
            break;
        }
        else if(c == '?' && tag_type == 4)
        {
            ptr += 2;
            break;
        }
        else if( c == '/' && ptr[1] == '>' && tag_type == CV_XML_OPENING_TAG )  // FIXIT ptr[1] - out of bounds read without check
        {
            tag_type = 3;
            ptr += 2;
            break;
        }
    }
    *_tag = tagname;
    *_tag_type = tag_type;
    *_list = first;

    return ptr;
}

static char* icvXMLParseValue(CvFileStorage* fs, char* ptr, CvFileNode* node, int value_type)
{
    CvFileNode *elem = node;
    bool have_space = true, is_simple = true;
    int is_user_type = CV_NODE_IS_USER(value_type);
    memset(node, 0, sizeof(*node));

    value_type = CV_NODE_TYPE(value_type);

    for(;;)
    {
        char c = *ptr, d;
        char* endptr;

        if(cv_isspace(c) || c == '\0' || (c == '<' && ptr[1] == '!' && ptr[2] == '-'))
        {
            ptr = icvXMLSkipSpaces(fs, ptr, 0);
            have_space = true;
            c = *ptr;
        }

        d = ptr[1];

        if( c =='<' || c == '\0' )
        {
            StringHashNode *key = 0, *key2 = 0;
            AttrList* list = 0;
            TypeInfo* info = 0;
            int tag_type = 0;
            int is_noname = 0;
            const char* type_name = 0;
            int elem_type = CV_NODE_NONE;

            if( d == '/' || c == '\0' )
                break;

            ptr = icvXMLParseTag(fs, ptr, &key, &list, &tag_type);

            /* for base64 string */
            bool is_binary_string = false;

            type_name = list ? AttrValue(list, "type_id") : 0;
            if(type_name)
            {
                if(strcmp( type_name, "str") == 0)
                    elem_type = CV_NODE_STRING;
                else if(strcmp(type_name, "map") == 0)
                    elem_type = CV_NODE_MAP;
                else if(strcmp(type_name, "seq") == 0)
                    elem_type = CV_NODE_SEQ;
                else if (strcmp(type_name, "binary") == 0)
                {
                    elem_type = CV_NODE_NONE;
                    is_binary_string = true;
                }
                else
                {
                    info = FindType(type_name);
                    if(info)
                        elem_type = CV_NODE_USER;
                }
            }

            is_noname = key->str.len == 1 && key->str.ptr[0] == '_';
            if(!CV_NODE_IS_COLLECTION(node->tag))
            {
                icvFSCreateCollection(fs, is_noname ? CV_NODE_SEQ : CV_NODE_MAP, node);
            }

            if(is_noname)
                elem = (CvFileNode*)SeqPush( node->data.seq, 0 );
            else
                elem = cvGetFileNode(fs, node, key, 1);
            assert(elem);
            if (!is_binary_string)
                ptr = icvXMLParseValue(fs, ptr, elem, elem_type);
            
            if(!is_noname)
                elem->tag |= CV_NODE_NAMED;
            is_simple = is_simple && !CV_NODE_IS_COLLECTION(elem->tag);
            elem->info = info;
            ptr = icvXMLParseTag( fs, ptr, &key2, &list, &tag_type );
            have_space = true;
        }
        else
        {
            elem = node;
            if(node->tag != CV_NODE_NONE)
            {
                if(!CV_NODE_IS_COLLECTION(node->tag))
                    icvFSCreateCollection(fs, CV_NODE_SEQ, node);

                elem = (CvFileNode*)SeqPush(node->data.seq, 0);
                elem->info = 0;
            }

            if(value_type != CV_NODE_STRING &&
                (cv_isdigit(c) || ((c == '-' || c == '+') &&
                (cv_isdigit(d) || d == '.')) || (c == '.' && cv_isalnum(d)))) // a number
            {
                double fval;
                int ival;
                endptr = ptr + (c == '-' || c == '+');
                while(cv_isdigit(*endptr))
                    endptr++;
                if(*endptr == '.' || *endptr == 'e')
                {
                    fval = icv_strtod(fs, ptr, &endptr);
                    elem->tag = CV_NODE_REAL;
                    elem->data.f = fval;
                }
                else
                {
                    ival = (int)strtol( ptr, &endptr, 0 );
                    elem->tag = CV_NODE_INT;
                    elem->data.i = ival;
                }

                ptr = endptr;
            }
            else
            {
                //string
                char buf[CV_FS_MAX_LEN+16] = {0};
                int i = 0, len, is_quoted = 0;
                elem->tag = CV_NODE_STRING;
                if( c == '\"' )
                    is_quoted = 1;
                else
                    --ptr;

                for( ;; )
                {
                    c = *++ptr;
                    if(!cv_isalnum(c))
                    {
                        if( c == '\"' )
                        {
                            ++ptr;
                            break;
                        }
                        else if(!cv_isprint(c) || c == '<' || (!is_quoted && cv_isspace(c)))
                            break;

                        else if( c == '&' )
                        {
                            if( *++ptr == '#' )
                            {
                                int val, base = 10;
                                ptr++;
                                if( *ptr == 'x' )
                                {
                                    base = 16;
                                    ptr++;
                                }
                                val = (int)strtol( ptr, &endptr, base );
                                c = (char)val;
                            }
                            else
                            {
                                endptr = ptr;
                                do c = *++endptr;
                                while( cv_isalnum(c) );
                                len = (int)(endptr - ptr);
                                if( len == 2 && memcmp( ptr, "lt", len ) == 0 )
                                    c = '<';
                                else if( len == 2 && memcmp( ptr, "gt", len ) == 0 )
                                    c = '>';
                                else if( len == 3 && memcmp( ptr, "amp", len ) == 0 )
                                    c = '&';
                                else if( len == 4 && memcmp( ptr, "apos", len ) == 0 )
                                    c = '\'';
                                else if( len == 4 && memcmp( ptr, "quot", len ) == 0 )
                                    c = '\"';
                                else
                                {
                                    memcpy( buf + i, ptr-1, len + 2 );
                                    i += len + 2;
                                }
                            }
                            ptr = endptr;
                        }
                    }
                    buf[i++] = c;
                }
                elem->data.str = MemStorageAllocString(fs->memstorage, buf, i);
            }

            if(!CV_NODE_IS_COLLECTION(value_type) && value_type != 0)
                break;
            have_space = false;
        }
    }

    if((CV_NODE_TYPE(node->tag) == 0 ||
        (CV_NODE_TYPE(node->tag) != value_type &&
        !CV_NODE_IS_COLLECTION(node->tag))) &&
        CV_NODE_IS_COLLECTION(value_type))
    {
        icvFSCreateCollection(fs, CV_NODE_IS_MAP(value_type) ?
                                        6 : 5, node);
    }

    if( CV_NODE_IS_COLLECTION(node->tag) && is_simple )
        node->data.seq->flags |= 256;

    node->tag |= is_user_type ? 16 : 0;
    return ptr;
}

void icvXMLParse(CvFileStorage* fs)
{
    char* ptr = fs->buffer_start;
    StringHashNode *key = 0, *key2 = 0;
    AttrList* list = 0;
    int tag_type = 0;

    //prohibit leading comments
    ptr = icvXMLSkipSpaces(fs, ptr, CV_XML_INSIDE_TAG);

    ptr = icvXMLParseTag(fs, ptr, &key, &list, &tag_type);

    while(*ptr != '\0')
    {
        ptr = icvXMLSkipSpaces(fs, ptr, 0);

        if( *ptr != '\0' )
        {
            CvFileNode* root_node;
            ptr = icvXMLParseTag( fs, ptr, &key, &list, &tag_type );

            root_node = (CvFileNode*)SeqPush(fs->roots, 0);
            ptr = icvXMLParseValue(fs, ptr, root_node, CV_NODE_NONE);
            ptr = icvXMLParseTag(fs, ptr, &key2, &list, &tag_type);
            ptr = icvXMLSkipSpaces(fs, ptr, 0);
        }
    }
}

/****************************************************************************************\
*                            Data Persistence					                         *
\****************************************************************************************/

/** returns attribute value or 0 (NULL) if there is no such attribute */
const char* AttrValue(const AttrList* attr, const char* attr_name)
{
    while(attr && attr->attr)
    {
        int i;
        for(i = 0; attr->attr[i*2] != 0; i++)
        {
            if(strcmp( attr_name, attr->attr[i*2] ) == 0 )
                return attr->attr[i*2+1];
        }
        attr = attr->next;
    }

    return 0;
}
/** @brief Finds a node in a map or file storage.

The function finds a file node. It is a faster version of cvGetFileNodeByName. 
The function can insert a new node, if it is not in the map yet.
@param fs File storage
@param map The parent map. If it is NULL, the function searches a top-level node. If both map and
key are NULLs, the function returns the root file node - a map that contains top-level nodes.
@param key Unique pointer to the node name, retrieved with cvGetHashedKey
@param create_missing Flag that specifies whether an absent node should be added to the map
 */
CvFileNode* cvGetFileNode(CvFileStorage* fs, CvFileNode* _map_node, const StringHashNode* key, int create_missing)
{
    CvFileNode* value = 0;
    int k = 0, attempts = 1;

    if(!fs)
        return 0;

    if(_map_node)
    {
        if(!fs->roots)
            return 0;
        attempts = fs->roots->total;
    }

    for(k = 0; k < attempts; k++)
    {
        int i, tab_size;
        CvFileNode* map_node = _map_node;
        FileMapNode* another;
        StringHash* map;

        if(!map_node)
            map_node = (CvFileNode*)GetSeqElem(fs->roots, k);
        if(!CV_NODE_IS_MAP(map_node->tag))
        {
            return 0;
        }

        map = map_node->data.map;
        tab_size = map->tab_size;

        if( (tab_size & (tab_size - 1)) == 0 )
            i = (int)(key->hashval & (tab_size - 1));
        else
            i = (int)(key->hashval % tab_size);

        for( another = (FileMapNode*)(map->table[i]); another != 0; another = another->next )
            if( another->key == key )
            {
                if(!create_missing)
                {
                    value = &another->value;
                    return value;
                }
            }

        if(k == attempts - 1 && create_missing)
        {
            FileMapNode* node = (FileMapNode*)SetNew((Set*)map);
            node->key = key;

            node->next = (FileMapNode*)(map->table[i]);
            map->table[i] = node;
            value = (CvFileNode*)node;
        }
    }

    return value;
}

CvFileNode* cvGetFileNodeByName(const CvFileStorage* fs, const CvFileNode* _map_node, const char* str)
{
    CvFileNode* value = 0;
    int i, len, tab_size;
    unsigned hashval = 0;
    int k = 0, attempts = 1;

    if(!fs)
        return 0;

    for(i = 0; str[i] != '\0'; i++)
        hashval = hashval*CV_HASHVAL_SCALE + (unsigned char)str[i];
    hashval &= INT_MAX;
    len = i;

    if(!_map_node)
    {
        if( !fs->roots )
            return 0;
        attempts = fs->roots->total;
    }

    for(k = 0; k < attempts; k++)
    {
        StringHash* map;
        const CvFileNode* map_node = _map_node;
        FileMapNode* another;

        if(!map_node)
            map_node = (CvFileNode*)GetSeqElem(fs->roots, k);

        if(!CV_NODE_IS_MAP(map_node->tag))
            return 0;

        map = map_node->data.map;
        tab_size = map->tab_size;

        if( (tab_size & (tab_size - 1)) == 0 )
            i = (int)(hashval & (tab_size - 1));
        else
            i = (int)(hashval % tab_size);

        for(another = (FileMapNode*)(map->table[i]); another != 0; another = another->next)
        {
            const StringHashNode* key = another->key;

            if(key->hashval == hashval &&
                key->str.len == len &&
                memcmp( key->str.ptr, str, len ) == 0)
            {
                value = &another->value;
                return value;
            }
        }
    }

    return value;
}

/** @brief Finds a type by its name.

The function finds a registered type by its name. It returns NULL if there is no type with the
specified name.
@param type_name Type name
 */
TypeInfo* FindType(const char* type_name)
{
    TypeInfo* info = 0;

    if (type_name)
      for(info = 0; info != 0; info = info->next)
        if(strcmp( info->type_name, type_name ) == 0)
      break;

    return info;
}

static void icvProcessSpecialDouble(CvFileStorage* fs, char* buf, double* value, char** endptr)
{
    char c = buf[0];
    int inf_hi = 0x7ff00000;

    if(c == '-' || c == '+')
    {
        inf_hi = c == '-' ? 0xfff00000 : 0x7ff00000;
        c = *++buf;
    }

    union{double d; uint64_t i;} v;
    v.d = 0.;
    if(toupper(buf[1]) == 'I' && toupper(buf[2]) == 'N' && toupper(buf[3]) == 'F')
        v.i = (uint64_t)inf_hi << 32;
    else if(toupper(buf[1]) == 'N' && toupper(buf[2]) == 'A' && toupper(buf[3]) == 'N')
        v.i = (uint64_t)-1;

    *value = v.d;
    *endptr = buf + 4;
}

double icv_strtod(CvFileStorage* fs, char* ptr, char** endptr)
{
    double fval = strtod(ptr, endptr);
    if(**endptr == '.')
    {
        char* dot_pos = *endptr;
        *dot_pos = ',';
        double fval2 = strtod(ptr, endptr);
        *dot_pos = '.';
        if(*endptr > dot_pos)
            fval = fval2;
        else
            *endptr = dot_pos;
    }

    if(*endptr == ptr || cv_isalpha(**endptr))
        icvProcessSpecialDouble( fs, ptr, &fval, endptr );

    return fval;
}

void icvCloseFile(CvFileStorage* fs)
{
    if( fs->file )
        fclose( fs->file );

    fs->file = 0;
    fs->gzfile = 0;
    fs->strbuf = 0;
    fs->strbufpos = 0;
    fs->is_opened = false;
}

FileNode fileNode(const CvFileStorage* _fs, const CvFileNode* _node)
{
    FileNode fn;
    fn.fs = _fs;
    fn.node = _node;
    return fn;
}

/** @brief Opens file storage for reading or writing data.

The function opens file storage for reading data.

The function returns a pointer to the CvFileStorage structure.
If the file cannot be opened then the function returns NULL.
@param filename Name of the file associated with the storage
@param memstorage Memory storage used for temporary data and for
:   storing dynamic structures, such as CvSeq or CvGraph . If it is NULL, a temporary memory
    storage is created and used.
@param flags :
> -   0: the storage is open for reading
@param encoding
 */
CvFileStorage* OpenFileStorage(const char* query, CvMemStorage* dststorage, int flags, const char* encoding)
{ // flags = 0
    CvFileStorage* fs = 0;
    int default_block_size = 1 << 18;
    bool append = (flags & 3) == 2;
    bool mem = (flags & 4) != 0;
    bool write_mode = (flags & 3) != 0;
    bool isGZ = false;
    size_t fnamelen = 0;
    const char* filename = query;

    fnamelen = strlen(filename);

    fs = cvAlloc(sizeof(*fs));
    assert(fs);
    memset(fs, 0, sizeof(*fs));

    fs->memstorage = CreateMemStorage( default_block_size );
    fs->dststorage = dststorage ? dststorage : fs->memstorage;

    fs->flags = CV_FILE_STORAGE;
    fs->write_mode = write_mode;

    if(!mem)
    {
        fs->filename = (char*)MemStorageAlloc(fs->memstorage, fnamelen+1);
        strcpy(fs->filename, filename);

        char compression = '\0';

        fs->file = fopen(fs->filename, !fs->write_mode ? "rt" : !append ? "wt" : "a+t" );
    }

    fs->roots = 0;
    fs->struct_indent = 0;
    fs->struct_flags = 0;
    fs->wrap_margin = 71;

    fs->fmt = 8;

    size_t buf_size = 1 << 20;
    char buf[16];
    icvGets(fs, buf, sizeof(buf)-2);
    char* bufPtr = cv_skip_BOM(buf);
    size_t bufOffset = bufPtr - buf;

    fseek(fs->file, 0, 2);
    buf_size = ftell(fs->file);

    buf_size = min(buf_size, (size_t)(1 << 20));
    buf_size = max(buf_size, (size_t)(CV_FS_MAX_LEN*2 + 1024));

    rewind(fs->file);
    fs->strbufpos = bufOffset;

    fs->str_hash = CreateMap(0, sizeof(StringHash), sizeof(StringHashNode), fs->memstorage, 256);
    fs->roots = CreateSeq(0, sizeof(Seq), sizeof(CvFileNode), fs->memstorage);

    fs->buffer = fs->buffer_start = (char*)cvAlloc(buf_size + 256);
    fs->buffer_end = fs->buffer_start + buf_size;
    fs->buffer[0] = '\n';
    fs->buffer[1] = '\0';

    icvXMLParse(fs);

    // release resources that we do not need anymore
    cvFree(&fs->buffer_start);
    fs->buffer = fs->buffer_end = 0;

    icvCloseFile(fs);
    // we close the file since it's not needed anymore. But icvCloseFile() resets is_opened,
    // which may be misleading. Since we restore the value of is_opened.
    fs->is_opened = true;
    return fs;
}

void isOpened(FileStorage fs)
{
    return fs.fs && fs.is_opened;
}

bool open(FileStorage* fs, char* filename, int flags)
{
    free(fs->fs);
    vector_clear(fs->structs);
    fs->state = 0;

    fs->fs =  OpenFileStorage(filename, 0, flags, 0);
    bool ok = isOpened(*fs);
    fs->state = ok ? 6 : 0;
    return ok;
}

FileStorage fileStorage(char* filename, int flags)
{
    FileStorage fs;
    fs.state = 0;//UNDEFINED
    open(&fs, filename, flags);
    return fs;
}

CvFileNode* cvGetRootFileNode(const CvFileStorage* fs, int stream_index);
{
    if(!fs->roots || (unsigned)stream_index >= (unsigned)fs->roots->total)
        return 0;

    return (CvFileNode*)GetSeqElem(fs->roots, stream_index);
}

FileNode FileNode_op(const FileNode fn, const char* nodename) const
{
    return fileNode(fn.fs, cvGetFileNodeByName(fn.fs, fn.node, nodename));
}

void read(FileNode node, int* value, int default_value)
{
    *value = node.node->data.i;
}

int int_op(FileNode fn)
{
    int* value = malloc(sizeof(int));
    read(fn, value, 0);
    return *value;
}

FileNode root(FileStorage fs, int streamidx)
{
    return fileNode(fs.fs, cvGetRootFileNode(fs.fs, 0));
}

FileNodeIterator fileNodeIterator(const CvFileStorage* _fs, const CvFileNode* _node, size_t _ofs)
{
    FileNodeIterator fn_it;
    fn_it.reader = init_SeqReader();
    if(_fs && _node && CV_NODE_TYPE(_node->tag) != 0)
    {
        int node_type = _node->tag & 7;
        fn_it.fs = _fs;
        fn_it.container = _node;
        if(!(_node->tag & 16) && (node_type == 5 || node_type == 6))
        {
            StartReadSeq(_node->data.seq, (SeqReader*)&reader);
            remaining = size(fileNode(_fs, _node));
        }
    }
}

FileNodeIterator begin(FileNode r)
{
    return fileNodeIterator(r.fs, r.node, 0);
}

inline FileNode getFirstTopLevelNode(FileStorage fs) const 
{ 
    FileNode r = root(fs, 0);
    FileNodeIterator it = begin(r);
    return fileNode(it.fs, (const CvFileNode*)(const void*)it.reader.ptr);
}

size_t size(FileNode fn)
{
    return (size_t)((Set*)fn.node->data.map)->active_count;
}