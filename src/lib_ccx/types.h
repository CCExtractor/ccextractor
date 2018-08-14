#ifndef TYPES_H
#define TYPES_H

#include "Mat.h"

void CreateSeqBlock(SeqWriter* writer);
void ChangeSeqBlock(void* _reader, int direction);
static void GoNextMemBlock(MemStorage* storage);

static inline int Align(int size, int align)
{
    return (size+align-1) & -align;
}

static inline unsigned char* alignptr(unsigned char* ptr, int n)
{
    return (unsigned char*)(((size_t)ptr + n-1) & -n);
}

static inline unsigned char** alignPtr(unsigned char** ptr, int n)
{
    return (unsigned char**)(((size_t)ptr + n-1) & -n);
}

int AlignLeft(int size, int align)
{
    return size & -align;
}

static inline void* AlignPtr(const void* ptr, int align/* 32 */)
{
    return (void*)(((size_t)ptr + align - 1) & ~(size_t)(align-1));
}

#define CV_NODE_TYPE(flags)  ((flags) & 7)
#define CV_NODE_IS_INT(flags)       (CV_NODE_TYPE(flags) == 1)
#define CV_NODE_IS_REAL(flags)       (CV_NODE_TYPE(flags) == 2)
#define CV_NODE_IS_MAP(flags)        (CV_NODE_TYPE(flags) == 6)
#define CV_NODE_IS_COLLECTION(flags) (CV_NODE_TYPE(flags) >= 5)
#define CV_NODE_IS_USER(flags)       (((flags) & 16) != 0)
#define CV_IS_SEQ_HOLE(seq)       (((seq)->flags & CV_SEQ_FLAG_HOLE) != 0)

#define CV_SEQ_READER_FIELDS()                                      		\
    int          header_size;                                       		\
    Seq*       seq;        /**< sequence, beign read */             		\
    SeqBlock*  block;      /**< current block */                    		\
    signed char*       ptr;        /**< pointer to element be read next */  \
    signed char*       block_min;  /**< pointer to the beginning of block */\
    signed char*       block_max;  /**< pointer to the end of block */      \
    int          delta_index;/**< = seq->first->start_index   */      		\
    signed char*       prev_elem;  /**< pointer to previous element */

#define CV_SEQ_WRITER_FIELDS()                                     			\
    int          header_size;                                      			\
    Seq*       seq;        /**< the sequence written */            			\
    SeqBlock*  block;      /**< current block */                   			\
    signed char*       ptr;        /**< pointer to free space */           	\
    signed char*       block_min;  /**< pointer to the beginning of block*/	\
    signed char*       block_max;  /**< pointer to the end of block */

/* initializes 8-element array for fast access to 3x3 neighborhood of a pixel */
#define  CV_INIT_3X3_DELTAS(deltas, step, nch)              \
    ((deltas)[0] =  (nch),  (deltas)[1] = -(step) + (nch),  \
     (deltas)[2] = -(step), (deltas)[3] = -(step) - (nch),  \
     (deltas)[4] = -(nch),  (deltas)[5] =  (step) - (nch),  \
     (deltas)[6] =  (step), (deltas)[7] =  (step) + (nch))

#define CV_TREE_NODE_FIELDS(node_type)                                \
    int flags;                  /**< Miscellaneous flags.     */      \
    int header_size;            /**< Size of sequence header. */      \
    struct node_type* h_prev;   /**< Previous sequence.       */      \
    struct node_type* h_next;   /**< Next sequence.           */      \
    struct node_type* v_prev;   /**< 2nd previous sequence.   */      \
    struct node_type* v_next    /**< 2nd next sequence.       */

#define CV_SEQUENCE_FIELDS()                                                      \
    CV_TREE_NODE_FIELDS(Seq);                                                     \
    int total;              /**< Total number of elements.            */          \
    int elem_size;          /**< Size of sequence element in bytes.   */          \
    signed char* block_max; /**< Maximal bound of the last block.     */          \
    signed char* ptr;       /**< Current write pointer.               */          \
    int delta_elems;        /**< Grow seq this many at a time.        */          \
    MemStorage* storage;    /**< Where the seq is stored.             */          \
    SeqBlock* free_blocks;  /**< Free blocks list.                    */          \
    SeqBlock* first;        /**< Pointer to the first sequence block. */

#define CV_CONTOUR_FIELDS()  \
    CV_SEQUENCE_FIELDS()     \
    Rect rect;               \
    int color;               \
    int reserved[3];

#define CV_SET_FIELDS()      \
    CV_SEQUENCE_FIELDS()     \
    SetElem* free_elems;     \
    int active_count;

#define CV_SET_ELEM_FIELDS(elem_type)   \
    int  flags;                         \
    struct elem_type* next_free;

#define CV_WRITE_SEQ_ELEM(elem, writer)                         \
{                                                               \
    assert((writer).seq->elem_size == sizeof(elem));            \
    if((writer).ptr >= (writer).block_max)                      \
    {                                                           \
        CreateSeqBlock(&writer);                                \
    }                                                           \
    assert((writer).ptr <= (writer).block_max - sizeof(elem));  \
    memcpy((writer).ptr, &(elem), sizeof(elem));                \
    (writer).ptr += sizeof(elem);                               \
}

#define CV_IS_MASK_ARR(mat)             \
    (((mat)->type & (4095 & ~1)) == 0)

#define ICV_FREE_PTR(storage)  \
    ((signed char*)(storage)->top + (storage)->block_size - (storage)->free_space)

/** Move reader position forward: */
#define CV_NEXT_SEQ_ELEM(elem_size, reader)                 	\
{                                                             	\
    if( ((reader).ptr += (elem_size)) >= (reader).block_max ) 	\
    {                                                         	\
        ChangeSeqBlock( &(reader), 1 );                     	\
    }                                                         	\
}

#define CV_SEQ_ELTYPE(seq)   ((seq)->flags & 4095)
#define CV_SEQ_KIND(seq)     ((seq)->flags & 12288)
#define CV_IS_SEQ_CLOSED(seq)     (((seq)->flags & 16384) != 0)

#define CV_IS_SEQ_CHAIN(seq)   \
    (CV_SEQ_KIND(seq) == 4096 && (seq)->elem_size == 1)

#define CV_IS_SEQ_CHAIN_CONTOUR(seq) \
    (CV_IS_SEQ_CHAIN(seq) && CV_IS_SEQ_CLOSED(seq))

#define CV_IS_SEQ_POINT_SET(seq) \
    ((CV_SEQ_ELTYPE(seq) == 13 || CV_SEQ_ELTYPE(seq) == 13))

/** Read element and move read position forward: */
#define CV_READ_SEQ_ELEM( elem, reader )                       \
{                                                              \
    assert( (reader).seq->elem_size == sizeof(elem));          \
    memcpy( &(elem), (reader).ptr, sizeof((elem)));            \
    CV_NEXT_SEQ_ELEM( sizeof(elem), reader )                   \
}

#define CV_READ_CHAIN_POINT( _pt, reader )                              \
{                                                                       \
    (_pt) = (reader).pt;                                                \
    if( (reader).ptr )                                                  \
    {                                                                   \
        CV_READ_SEQ_ELEM( (reader).code, (reader));                     \
        assert( ((reader).code & ~7) == 0 );                            \
        (reader).pt.x += (reader).deltas[(int)(reader).code][0];        \
        (reader).pt.y += (reader).deltas[(int)(reader).code][1];        \
    }                                                                   \
}

#define CV_GET_LAST_ELEM( seq, block ) \
    ((block)->data + ((block)->count - 1)*((seq)->elem_size))

static const Point icvCodeDeltas[8] =
    { init_Point(1, 0), init_Point(1, -1), init_Point(0, -1), init_Point(-1, -1), init_Point(-1, 0), init_Point(-1, 1), init_Point(0, 1), init_Point(1, 1) };

typedef struct AttrList
{
    const char** attr;         /**< NULL-terminated array of (attribute_name,attribute_value) pairs. */
    struct AttrList* next;   /**< Pointer to next chunk of the attributes list.                    */
} AttrList ;

typedef struct Seq
{
    CV_SEQUENCE_FIELDS()
} Seq;

typedef struct SeqIterator
{
    CV_SEQ_READER_FIELDS();
    int index;
} SeqIterator ;

typedef struct SeqBlock
{
    struct SeqBlock*  prev;     /**< Previous sequence block.                   */
    struct SeqBlock*  next;     /**< Next sequence block.                       */
    int    start_index;         /**< Index of the first element in the block +  */
                                /**< sequence->first->start_index.              */
    int    count;               /**< Number of elements in the block.           */
    signed char* data;          /**< Pointer to the first element of the block. */
} SeqBlock ;

#define ICV_ALIGNED_SEQ_BLOCK_SIZE  \
    (int)Align(sizeof(SeqBlock), (int)sizeof(double))

typedef struct SeqWriter
{
    CV_SEQ_WRITER_FIELDS()
} SeqWriter;

typedef struct SeqReader
{
    CV_SEQ_READER_FIELDS()
} SeqReader ;

/** Freeman chain reader state */
typedef struct ChainPtReader
{
    CV_SEQ_READER_FIELDS()
    char      code;
    Point   pt;
    signed char     deltas[8][2];
} ChainPtReader ;

typedef struct StringHash
{
    CV_SET_FIELDS()
    int tab_size;
    void** table;
} StringHash ;

typedef struct CvFileStorage
{
    int flags;
    int fmt;
    int write_mode;
    int is_first;
    MemStorage* memstorage;
    MemStorage* dststorage;
    MemStorage* strstorage;
    StringHash* str_hash;
    Seq* roots;
    Seq* write_stack;
    int struct_indent;
    int struct_flags;
    char* struct_tag;
    int space;
    char* filename;
    FILE* file;
    void* gzfile;
    char* buffer;
    char* buffer_start;
    char* buffer_end;
    int wrap_margin;
    int lineno;
    int dummy_eof;
    const char* errmsg;
    char errmsgbuf[128];

    const char* strbuf;
    size_t strbufsize, strbufpos;
    vector* outbuf; //char

    bool is_write_struct_delayed;
    char* delayed_struct_key;
    int   delayed_struct_flags;
    char* delayed_type_name;

    bool is_opened;
} CvFileStorage ;

typedef struct FileStorage
{
    CvFileStorage* fs;
    char* elname; //!< the currently written element
    vector* structs //!< the stack of written structures(char)
    int state; //!< the writer state
} FileStorage ;

/** Basic element of the file storage - scalar or collection: */
typedef struct CvFileNode
{
    int tag;
    struct TypeInfo* info; /**< type information
            (only for user-defined object, for others it is 0) */
    union
    {
        double f; /**< scalar floating-point number */
        int i;    /**< scalar integer number */
        char* str; /**< text string */
        Seq* seq; /**< sequence (ordered collection of file nodes) */
        StringHash* map; /**< map (collection of named file nodes) */
    } data;
} CvFileNode ;

typedef struct FileNode
{
    const CvFileStorage* fs;
    const CvFileNode* node;
} FileNode ;

typedef struct FileNodeIterator
{
    const CvFileStorage* fs;
    const CvFileNode* container;
    SeqReader reader;
    size_t remaining;
} FileNodeIterator ;

typedef struct FileMapNode
{
    CvFileNode value;
    const StringHashNode* key;
    struct FileMapNode* next;
} FileMapNode ;

typedef struct MemStorage
{
    int signature;
    MemBlock* bottom;            /**< First allocated block.                   */
    MemBlock* top;               /**< Current memory block - top of the stack. */
    struct  MemStorage* parent;  /**< We get new blocks from parent as needed. */
    int block_size;              /**< Block size.                              */
    int free_space;              /**< Remaining free space in current block.   */
} MemStorage ;

typedef struct MemStoragePos
{
    MemBlock* top;
    int free_space;
} MemStoragePos ;

typedef struct MemBlock
{
    struct MemBlock*  prev;
    struct MemBlock*  next;
} MemBlock ;

typedef struct FFillSegment
{
    unsigned short y;
    unsigned short l;
    unsigned short r;
    unsigned short prevl;
    unsigned short prevr;
    short dir;
} FFillSegment ;

typedef struct ConnectedComp
{
    Rect rect;
    Point pt;
    int threshold;
    int label;
    int area;
    int harea;
    int carea;
    int perimeter;
    int nholes;
    int ninflections;
    double mx;
    double my;
    Scalar avg;
    Scalar sdv;
} ConnectedComp;

typedef struct Diff8uC1
{
    unsigned char lo, interval;
} Diff8uC1 ;

Diff8uC1 diff8uC1(unsigned char _lo, unsigned char _up)
{
    Diff8uC1 diff;
    diff.lo = _lo;
    diff.interval = _lo + _up;
    return diff;
}

typedef struct ThresholdRunner
{
    Mat src;
    Mat dst;

    double thresh;
    double maxval;
    int thresholdType;
} ThresholdRunner;

typedef struct StringHashNode
{
    unsigned hashval;
    char* str;
    struct StringHashNode* next;
} StringHashNode ;

typedef struct Set
{
    CV_SET_FIELDS()
} Set;

typedef struct SetElem
{
    CV_SET_ELEM_FIELDS(SetElem)
} SetElem;

typedef struct Chain
{
    CV_SEQUENCE_FIELDS()
    Point  origin;
} Chain ;

typedef struct Contour
{
    CV_CONTOUR_FIELDS()
} Contour;

typedef struct PtInfo
{
    Point pt;
    int k;                      /* support region */
    int s;                      /* curvature value */
    struct PtInfo *next;
} PtInfo ;

typedef struct ContourInfo
{
    int flags;
    struct ContourInfo *next;       /* next contour with the same mark value            */
    struct ContourInfo *parent;     /* information about parent contour                 */
    Seq *contour;                   /* corresponding contour (may be 0, if rejected)    */
    Rect rect;                      /* bounding rectangle                               */
    Point origin;                   /* origin point (where the contour was traced from) */
    int is_hole;                    /* hole flag                                        */
} ContourInfo ;

typedef struct ContourScanner
{
    MemStorage *storage1;             /* contains fetched contours                              */
    MemStorage *storage2;             /* contains approximated contours
                                   (!=storage1 if approx_method2 != approx_method1)             */
    MemStorage *cinfo_storage;        /* contains _CvContourInfo nodes                          */
    Set *cinfo_set;                   /* set of _CvContourInfo nodes                            */
    MemStoragePos initial_pos;        /* starting storage pos                                   */
    MemStoragePos backup_pos;         /* beginning of the latest approx. contour                */
    MemStoragePos backup_pos2;        /* ending of the latest approx. contour                   */
    signed char *img0;                /* image origin                                           */
    signed char *img;                 /* current image row                                      */
    int img_step;                     /* image step                                             */
    Point img_size;                   /* ROI size                                               */
    Point offset;                     /* ROI offset: coordinates, added to each contour point   */
    Point pt;                         /* current scanner position                               */
    Point lnbd;                       /* position of the last met contour                       */
    int nbd;                          /* current mark val                                       */
    ContourInfo *l_cinfo;             /* information about latest approx. contour               */
    ContourInfo cinfo_temp;           /* temporary var which is used in simple modes            */
    ContourInfo frame_info;           /* information about frame                                */
    Seq frame;                        /* frame itself                                           */
    int approx_method1;               /* approx method when tracing                             */
    int approx_method2;               /* final approx method                                    */
    int mode;                         /* contour scanning mode:
                                           0 - external only
                                           1 - all the contours w/o any hierarchy
                                           2 - connected components (i.e. two-level structure -
                                           external contours and holes),
                                           3 - full hierarchy;
                                           4 - connected components of a multi-level image
                                      */
    int subst_flag;
    int seq_type1;                    /* type of fetched contours                               */
    int header_size1;                 /* hdr size of fetched contours                           */
    int elem_size1;                   /* elem size of fetched contours                          */
    int seq_type2;                    /*                                                        */
    int header_size2;                 /* the same for approx. contours                          */
    int elem_size2;                   /*                                                        */
    ContourInfo *cinfo_table[128];
} ContourScanner;

typedef struct TreeNodeIterator
{
    const void* node;
    int level;
    int max_level;
} TreeNodeIterator ;

typedef struct CompHistory
{
    CompHistory* child_;
    CompHistory* parent_;
    CompHistory* next_;
    int val;
    int size;
    float var;
    int head;
    bool checked;
} CompHistory;

typedef struct TreeNode
{
    int       flags;            /* micsellaneous flags      */
    int       header_size;      /* size of sequence header  */
    struct    TreeNode* h_prev; /* previous sequence        */
    struct    TreeNode* h_next; /* next sequence            */
    struct    TreeNode* v_prev; /* 2nd previous sequence    */
    struct    TreeNode* v_next; /* 2nd next sequence        */
} TreeNode;

typedef struct TypeInfo
{
    int flags; /**< not used */
    int header_size; /**< sizeof(TypeInfo) */
    struct TypeInfo* prev; /**< previous registered type in the list */
    struct TypeInfo* next; /**< next registered type in the list */
    const char* type_name; /**< type name, written to file storage */
} TypeInfo ;

void fastFree(void* ptr)
{
    if(ptr)
    {
        unsigned char* udata = ((unsigned char**)ptr)[-1];
        free(udata);
    }
}

void cvFree_(void* ptr)
{
    fastFree(ptr);
}

#define cvFree(ptr) (cvFree_(*(ptr)), *(ptr)=0)

void* fastMalloc(size_t size)
{
    unsigned char* udata = (unsigned char*)malloc(size + sizeof(void*) + CV_MALLOC_ALIGN);
    if(!udata)
        fatal("Out of Memory Error");
    unsigned char** adata = alignPtr((unsigned char**)udata + 1, CV_MALLOC_ALIGN);
    adata[-1] = udata;
    return adata;
}

void* cvAlloc(size_t size)
{
    return fastMalloc(size);
}

void SetSeqBlockSize(Seq *seq, int delta_elements);
void StartReadSeq(const Seq *seq, SeqReader* reader, int reverse);
void StartWriteSeq(int seq_flags, int header_size, int elem_size, MemStorage* storage, SeqWriter* writer);
Seq* EndWriteSeq(SeqWriter* writer);
void StartAppendToSeq(Seq *seq, SeqWriter * writer)
void StartReadChainPoints(Chain * chain, ChainPtReader* reader);
void SetSeqReaderPos(SeqReader* reader, int index, int is_relative);
void getNextIterator_Seq(SeqIterator* it);
Seq* TreeToNodeSeq(const void* first, int header_size, MemStorage* storage);
void InitTreeNodeIterator(TreeNodeIterator* treeIterator, const void* first, int max_level);
void* NextTreeNode(TreeNodeIterator* treeIterator);
bool validInterval(Diff8uC1 obj, const unsigned char* a, const unsigned char* b);
static char* icvXMLParseValue(CvFileStorage* fs, char* ptr, CvFileNode* node, int value_type);
static char* icvXMLParseTag(CvFileStorage* fs, char* ptr, StringHashNode** _tag, AttrList** _list, int* _tag_type);
const char* AttrValue(const AttrList* attr, const char* attr_name);
CvFileNode* cvGetFileNode(CvFileStorage* fs, CvFileNode* _map_node, const StringHashNode* key, int create_missing);
TypeInfo* FindType(const char* type_name);
double icv_strtod(CvFileStorage* fs, char* ptr, char** endptr);
FileStorage fileStorage(char* filename, int flags);
FileNode fileNode(const CvFileStorage* _fs, const CvFileNode* _node);
FileNode FileNode_op(const FileNode fn, const char* nodename) const;
int int_op(FileNode fn);
FileNodeIterator begin(FileNode r);
static void icvEndProcessContour(ContourScanner* scanner);
void SaveMemStoragePos(const MemStorage* storage, MemStoragePos* pos);
void RestoreMemStoragePos(MemStorage* storage, MemStoragePos* pos );
void ReleaseMemStorage(MemStorage** storage);
MemStorage* CreateChildMemStorage(MemStorage* parent);
MemStorage* CreateMemStorage(int block_size);

#endif