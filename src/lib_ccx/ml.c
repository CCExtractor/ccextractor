#include "ml.h"

static int icvSymbolToType(char c)
{
    static const char symbols[9] = "ucwsifdr";
    const char* pos = strchr( symbols, c );
    return (int)(pos - symbols);
}

int icvDecodeFormat( const char* dt, int* fmt_pairs, int max_len )
{
    int fmt_pair_count = 0;
    int i = 0, k = 0, len = dt ? (int)strlen(dt) : 0;

    if( !dt || !len )
        return 0;

    assert( fmt_pairs != 0 && max_len > 0 );
    fmt_pairs[0] = 0;
    max_len *= 2;

    for( ; k < len; k++ )
    {
        char c = dt[k];

        if( cv_isdigit(c) )
        {
            int count = c - '0';
            if( cv_isdigit(dt[k+1]) )
            {
                char* endptr = 0;
                count = (int)strtol( dt+k, &endptr, 10 );
                k = (int)(endptr - dt) - 1;
            }

            fmt_pairs[i] = count;
        }
        else
        {
            int depth = icvSymbolToType(c);
            if( fmt_pairs[i] == 0 )
                fmt_pairs[i] = 1;
            fmt_pairs[i+1] = depth;
            if( i > 0 && fmt_pairs[i+1] == fmt_pairs[i-1] )
                fmt_pairs[i-2] += fmt_pairs[i];
            else
                i += 2;
            fmt_pairs[i] = 0;
        }
    }

    fmt_pair_count = i/2;
    return fmt_pair_count;
}

int icvCalcElemSize(const char* dt, int initial_size)
{
    int size = 0;
    int fmt_pairs[128], i, fmt_pair_count;
    int comp_size;

    fmt_pair_count = icvDecodeFormat( dt, fmt_pairs, 128);
    fmt_pair_count *= 2;
    for( i = 0, size = initial_size; i < fmt_pair_count; i += 2 )
    {
        comp_size = CV_ELEM_SIZE(fmt_pairs[i+1]);
        size = Align( size, comp_size );
        size += comp_size * fmt_pairs[i];
    }
    if( initial_size == 0 )
    {
        comp_size = CV_ELEM_SIZE(fmt_pairs[1]);
        size = Align( size, comp_size );
    }
    return size;
}

int icvCalcStructSize(const char* dt, int initial_size)
{
    int size = icvCalcElemSize(dt, initial_size);
    size_t elem_max_size = 0;
    for (const char * type = dt; *type != '\0'; type++) {
        switch (*type)
        {
        case 'u': { elem_max_size = max( elem_max_size, sizeof(unsigned char ) ); break; }
        case 'c': { elem_max_size = max( elem_max_size, sizeof(signed char   ) ); break; }
        case 'w': { elem_max_size = max( elem_max_size, sizeof(unsigned short) ); break; }
        case 's': { elem_max_size = max( elem_max_size, sizeof(short ) ); break; }
        case 'i': { elem_max_size = max( elem_max_size, sizeof(int   ) ); break; }
        case 'f': { elem_max_size = max( elem_max_size, sizeof(float ) ); break; }
        case 'd': { elem_max_size = max( elem_max_size, sizeof(double) ); break; }
        default: break;
        }
    }
    size = Align(size, (int)(elem_max_size));
    return size;
}

void cvReadRawDataSlice(const FileStorage* fs, SeqReader* reader, int len, void* _data, const char* dt)
{
    char* data0 = (char*)_data;
    int fmt_pairs[256], k = 0, fmt_pair_count;
    int i = 0, count = 0;

    fmt_pair_count = icvDecodeFormat(dt, fmt_pairs, 128);
    size_t step = icvCalcStructSize(dt, 0);

    for(;;)
    {
        int offset = 0;
        for(k = 0; k < fmt_pair_count; k++)
        {
            int elem_type = fmt_pairs[k*2+1];
            int elem_size = CV_ELEM_SIZE(elem_type);
            char* data;

            count = fmt_pairs[k*2];
            offset = Align(offset, elem_size);
            data = data0 + offset;

            for(i = 0; i < count; i++)
            {
                CvFileNode* node = (CvFileNode*)reader->ptr;
                if( CV_NODE_IS_INT(node->tag) )
                {
                    int ival = node->data.i;
                    switch( elem_type )
                    {
                    case 0:
                        *(unsigned char*)data = (unsigned char)(ival);
                        data++;
                        break;
                    case 1:
                        *(char*)data = (signed char)(ival);
                        data++;
                        break;
                    case 2:
                        *(unsigned short*)data = (unsigned short)(ival);
                        data += sizeof(unsigned short);
                        break;
                    case 3:
                        *(short*)data = (short)(ival);
                        data += sizeof(short);
                        break;
                    case 4:
                        *(int*)data = ival;
                        data += sizeof(int);
                        break;
                    case 5:
                        *(float*)data = (float)ival;
                        data += sizeof(float);
                        break;
                    case 6:
                        *(double*)data = (double)ival;
                        data += sizeof(double);
                        break;
                    case 7: /* reference */
                        *(size_t*)data = ival;
                        data += sizeof(size_t);
                        break;
                    }
                }

                 else if(CV_NODE_IS_REAL(node->tag))
                {
                    double fval = node->data.f;
                    int ival;

                    switch(elem_type)
                    {
                    case 0:
                        ival = round(fval);
                        *(unsigned char*)data = (unsigned char)(ival);
                        data++;
                        break;
                    case 1:
                        ival = round(fval);
                        *(char*)data = (signed char)(ival);
                        data++;
                        break;
                    case 2:
                        ival = round(fval);
                        *(unsigned short*)data = (unsigned short)(ival);
                        data += sizeof(unsigned short);
                        break;
                    case 3:
                        ival = round(fval);
                        *(short*)data = (short)(ival);
                        data += sizeof(short);
                        break;
                    case 4:
                        ival = round(fval);
                        *(int*)data = ival;
                        data += sizeof(int);
                        break;
                    case 5:
                        *(float*)data = (float)fval;
                        data += sizeof(float);
                        break;
                    case 6:
                        *(double*)data = fval;
                        data += sizeof(double);
                        break;
                    case 7: /* reference */
                        ival = round(fval);
                        *(size_t*)data = ival;
                        data += sizeof(size_t);
                        break;
                    }
                }

                CV_NEXT_SEQ_ELEM(sizeof(CvFileNode), *reader);
                if(!--len)
                    goto end_loop;
            }

            offset = (int)(data - data0);
        }
        data0 += step;
    }

end_loop:
    if(!reader->seq)
        reader->ptr -= sizeof(CvFileNode);
}

static void getElemSize(const char* fmt, size_t* elemSize, size_t* cn)
{
    const char* dt = fmt;
    elemSize = malloc(sizeof(size_t));
    cn = malloc(sizeof(size_t));

    *cn = 1;
    if(cv_isdigit(dt[0]))
    {
        *cn = dt[0] - '0';
        dt++;
    }
    char c = dt[0];
    *elemSize = (*cn)*(c == 'u' || c == 'c' ? sizeof(unsigned char) : c == 'w' || c == 's' ? sizeof(unsigned short) :
        c == 'i' ? sizeof(int) : c == 'f' ? sizeof(float) : c == 'd' ? sizeof(double) :
        c == 'r' ? sizeof(void*) : (size_t)0);
}

void readRaw(FileNodeIterator* it, const char* fmt, unsigned char* vec, size_t maxCount)
{
    if(it->fs && it->container && it->remaining > 0)
    {
        size_t elem_size, cn;
        getElemSize(fmt, &elem_size, &cn);
        size_t count = min(it->remaining, maxCount);

        cvReadRawDataSlice(it->fs, (SeqReader*)(&(it->reader)), (int)count, vec, fmt);
        it->remaining -= count*cn;
    }
}

void VecReader_op(FileNodeIterator* it, vector* v/*int*/, size_t count, code)
{
    size_t remaining = it->remaining;
    size_t cn = 1;
    int _fmt = 0;
    if(code == 0)
        _fmt = 105;

    if(code == 1)
        _fmt = 117;

    char fmt[] = { (char)((_fmt >> 8)+'1'), (char)_fmt, '\0' };
    size_t remaining1 = remaining / cn;
    count = count < remaining1 ? count : remaining1;
    vector_resize(v, count);
    if(code == 0)
        readRaw(it, fmt, (unsigned char*)vector_get(vec, 0), count*sizeof(int));

    if(code == 1)
        readRaw(it, fmt, (unsigned char*)vector_get(v, 0), count*sizeof(unsigned char));
}

void rightshift_op_(FileNodeIterator* it, vector* v/*int*/, int code)
{
    
    FileNodeIterator it_;
    VecReader_op(it, v, INT_MAX, code);
}



void rightshift_op(const FileNode node, vector* v/*int*/, int code)
{
    FileNodeIterator it = begin(node);
    rightshift_op_(&it, v, code);
}

static inline void readVectorOrMat(const FileNode node, vector* v/*int*/, int code)
{
    rightshift_op(node, v, code);
}

void initCompVarIdx(DTreesImplForBoost* impl)
{
    int nallvars = vector_size(impl->varType);
    vector_resize(impl->compVarIdx, nallvars);
    int push_val[nallvars];
    for(int i = 0; i < nallvars; i++)
    {
        push_val[i] = -1
        vector_add(impl->compVarIdx, &push_val[i]);
    }
    int i, nvars = vector_size(impl->varIdx), prevIdx = -1;
    for(i = 0; i < nvars; i++)
    {
        int vi = *(int*)vector_get(impl->varIdx, i);
        assert(0 <= vi && vi < nallvars && vi > prevIdx);
        prevIdx = vi;
        *(int*)vector_get(impl->compVarIdx, vi) = i;
    }
}

void readParams_(DTreesImplForBoost* impl, const FileNode fn)
{
    impl->_isClassifier = false;
    FileNode tparams_node = FileNode_op(fn, "training_params");

    TreeParams param0 = init_TreeParams();
    param0.useSurrogates = false;
    param0.maxCategories = 10;
    param0.regressionAccuracy = 0.00999999978;
    param0.maxDepth = 1;
    param0.minSampleCount = 10;
    param0.CVFolds = 0;
    param0.use1SERule = true;
    param0.truncatePrunedTree = true;
    param0.use1SERule = true;   
    release(&(param0.priors));

    readVectorOrMat(FileNode_op(fn, "var_idx"), impl->varIdx, 0);
    rightshift_op(FileNode_op(fn, "varType"), impl->varType, 1);   

    bool isLegacy= true;

    int varAll = int_op(FileNode_op(fn, "var_all"));

    if(isLegacy && vector_size(impl->varType) <= varAll)
    {
        vector* extendedTypes = malloc(sizeof(vector)); //unsigned char
        vector_init_n(extendedTypes, varAll+1);
        unsigned char push_val = (unsigned char)0;
        for(int i = 0; i < varAll+1; i++)
            vector_add(extendedTypes, &push_val);

        int i = 0, n;
        if(!vector_empty(impl->varIdx))
        {
            n = vector_size(impl->varIdx);
            for (; i < n; ++i)
            {
                int var = *(int*)vector_get(varIdx, i);
                *(int*)vector_get(extendedTypes, var) = *(int*)vector_get(impl->varType, i);
            }
        }
        unsigned push_val_ = (unsigned char)1;
        vector_set(extendedTypes, varAll, &push_val_);
        vector_swap(extendedTypes, impl->varType);
    }

    readVectorOrMat(FileNode_op(fn, "cat_map"), impl->catMap, 0);

    if(isLegacy)
    {
        // generating "catOfs" from "cat_count"
        vector_init(impl->catOfs);
        vector_init(impl->classLabels);
        vector* counts; //int
        vector_init(counts);
        readVectorOrMat(fn["cat_count"], counts, 0);
        unsigned int i = 0, j = 0, curShift = 0, size = (int)vector_size(impl->varType) - 1;
        Point newOffsets[size];
        for(; i < size; ++i)
        {
            if((*(int*)vector_get(impl->varType, i)) == 1) // only categorical vars are represented in catMap
            {
                newOffsets[i].x = curShift;
                curShift += (*(int*)vector_get(counts, j));
                newOffsets[i].y = curShift;
                ++j;
            }
            vector_add(impl->catOfs, &newOffsets[i]);
        }
    }

    vector_assign(impl->varIdx, impl->varMapping);
    initCompVarIdx(impl);
    impl->params = params0;
}

void getNextIterator_(FileNodeIterator* it)
{
    if(it->remaining > 0)
    {
        if(it->reader.seq)
        {
            if(((it->reader).ptr += (((Seq*)it->reader.seq)->elem_size)) >= (it->reader).block_max)
            {
                ChangeSeqBlock((SeqReader*)&(it->reader), 1);
            }
        }
        it->remaining--;
    }
}

void readParams(DTreesImplForBoost* impl, const FileNode fn)
{
    readParams_(impl, fn);
    FileNode tparams_node = FileNode_op(fn, "training_params");
    // check for old layout
    impl->bparams.boostType  = 1;
    impl->bparams.weightTrimRate = 0.0;
}

void read_double(FileNode node, double* value, double default_value)
{
    *value = !node.node ? default_value :
        CV_NODE_IS_INT(node.node->tag) ? (float)node.node->data.i : (float)(node.node->data.f);
}

double double_op(const FileNode node)
{
    *value = !node.node ? default_value :
        CV_NODE_IS_INT(node.node->tag) ? (double)node.node->data.i : (double)(node.node->data.f);
}

void read_float(FileNode node, float* value, float default_value)
{
    *value = !node.node ? default_value :
        CV_NODE_IS_INT(node.node->tag) ? (float)node.node->data.i : (float)(node.node->data.f);
}

float float_op(const FileNode fn)
{
    float* value = malloc(sizeof(float));
    read_float(fn, value, 0.f);
    return *value;
}

int readSplit(DTreesImplForBoost* impl, const FileNode fn)
{
    Split split;

    int vi = int_op(fileNode(fn, "var"));
    vi = *(int*)vector_get(impl->varMapping, vi); // convert to varIdx if needed
    split.varIdx = vi;

    FileNode cmpNode = FileNode_op(fn, "le");
    if(!cmpNode.node)
    {
        cmpNode = FileNode_op(fn, "gt");
        split.inversed = true;
    }
    split.c = float_op(cmpNode);

    split.quality = float_op(fn, "quality");
    vector_add(impl->splits, &split);

    return vector_size(impl->splits)-1;
}

int readNode(DTreesImplForBoost* impl, const FileNode fn)
{
    Node node;
    node.value = double_op(FileNode_op(fn, "value"));

    if(impl->_isClassifier)
        node.classIdx = int_op(FileNode_op(fn, "norm_class_idx"));

    FileNode sfn = FileNode_op(fn, "splits");
    if(sfn.node != 0)
    {
        int i, n = sfn.node->data.seq->total, prevsplit = -1;
        FileNodeIterator it = begin(sfn);

        for(i = 0; i < n; i++, getNextIterator_(&it))
        {
            int splitidx = readSplit(impl, fileNode(it.fs, (const CvFileNode*)(const void*)it.reader.ptr));
            if(splitidx < 0)
                break;
            if(prevsplit < 0)
                node.split = splitidx;
            else
                ((Split*)vector_get(impl->splits, prevsplit))->next = splitidx;
            prevsplit = splitidx;
        }
    }
}

int readTree(DTreesImplForBoost* impl, const FileNode fn)
{
    int i, n = (size_t)fn.node->data.seq->total, root = -1, pidx = -1;
    FileNodeIterator it = begin(fn);

    for(i = 0; i < n; i++, getNextIterator_(&t))
    {
        int nidx = readNode(impl, fileNode(it.fs, (const CvFileNode*)(const void*)it.reader.ptr));
        if(nidx < 0)
            break;
        Node* node = vector_get(impl->nodes, nidx);
        node->parent = pidx;
        if(pidx < 0)
            root = nidx;
        else
        {
            Node* parent = vector_get(impl->nodes, pidx);
            if(parent->left < 0)
                parent->left = nidx;
            else
                parent->right = nidx;
        }
        if(node->split >= 0)
            pidx = nidx;
        else
        {
            while(pidx >= 0 && ((Node*)vector_get(impl->nodes, pidx))->right >= 0)
                pidx = ((Node*)vector_get(impl->nodes, pidx))->parent;
        }
    }
    vector_add(impl->roots, &root);
    return root;
}

float predictTrees(DTreesImplForBoost* impl, const Point range, const Mat sample, int flags)
{
	int predictType = flags & PREDICT_MASK;
	int nvars = vector_size(impl->varIdx);
    if(nvars == 0)
    	nvars = vector_size(impl->varIdx);
    int i, ncats = (int)vector_size(impl->catOfs), nclasses = vector_size(impl->classLabels);
    int catbufsize = ncats > 0 ? nvars : 0;
    AutoBuffer buf = init_AutoBuffer(nclasses + catbufsize + 1);
    int* votes = buf.ptr;
    int* catbuf = votes + nclasses;
    const int* cvidx = (int*)vector_get(impl->compVarIdx, 0);
    const unsigned char* vtype = (unsigned char*)vector_get(varType, 0);
    const Point* cofs = (Point*)vector_get(catOfs, 0);
    const int* cmap = 0;
    const float* psample = (float*)ptr(sample, 0);
    const float* missingSubstPtr = 0;
    size_t sstep = 1;
    double sum = 0.;
    int lastClassIdx = -1;
    const float MISSED_VAL = FLT_MAX;

    for(i = 0; i < catbufsize; i++)
        catbuf[i] = -1;

    for(int ridx = range.x; ridx < range.y; ridx++)
    {
    	int nidx = *(int*)vector_get(impl->roots, ridx), prev = nidx, c = 0;

    	for(;;)
    	{
    		prev = nidx;
    		const Node node = (Node*)vector_get(impl->nodes, nidx);
    		if(node.split < 0)
                break;
			const Split split = (Split*)vector_get(impl->splits, node.split);
			int vi = split.varIdx;
            int ci = cvidx ? cvidx[vi] : vi;
            float val = psample[ci*sstep];

            if(val == MISSED_VAL)
            {
                if(!missingSubstPtr)
                {
                    nidx = node.defaultDir < 0 ? node.left : node.right;
                    continue;
                }
                val = missingSubstPtr[vi];
            }

            if(vtype[vi] == 0)
                nidx = val <= split.c ? node.left : node.right;
            else
            {
            	c = catbuf[ci];
                if(c < 0)
                {
                    int a = c = cofs[vi].x;
                    int b = cofs[vi].y;

                    int ival = round(val);

                    while(a < b)
                    {
                        c = (a + b) >> 1;
                        if(ival < cmap[c])
                            b = c;
                        else if(ival > cmap[c])
                            a = c+1;
                        else
                            break;
                    }

                    c -= cofs[vi].x;
                    catbuf[ci] = c;
                }
                const int* subset = (int*)vector_get(impl->subsets, split.subsetOfs);
                unsigned u = c;
                nidx = CV_DTREE_CAT_DIR(u, subset) < 0 ? node.left : node.right;
            }
    	}
		sum += nodes[prev].value;
    }

    return (float)sum;
}

//Creates the empty model
static Boost* createBoost()
{
    Boost* obj = malloc(sizeof(Boost));
    obj->impl._isClassifier = false;
    obj->impl.params = init_TreeParams();
    obj->impl.params.CVFolds = 0;
    obj->impl.params.maxDepth = 1;
    obj->impl.bparams.boostType = 1;
    obj->impl.bparams.weakCount = 100;
    obj->impl.bparams.weightTrimRate = 0.95;
    vector_init(obj->impl.varIdx);
    vector_init(obj->impl.compVarIdx);
    vector_init(obj->impl.varType);
    vector_init(obj->impl.catOfs);
    vector_init(obj->impl.catMap);
    vector_init(obj->impl.roots);
    vector_init(obj->impl.nodes);
    vector_init(obj->impl.splits);
    vector_init(obj->impl.subsets);
    vector_init(obj->impl.classLabels);
    vector_init(obj->impl.missingSubst);
    vector_init(obj->impl.varMapping);
    vector_init(obj->impl.sumResult);
    obj->impl.w = malloc(sizeof(WorkData));
    return obj;
}

void read_ml(Boost* obj, const FileNode fn)
{
    FileNode _fn = FileNode_op(fn, "ntrees");
    int ntrees = int_op(_fn);
    readParams(&(obj->impl), fn);

    FileNode trees_node = FileNode_op(fn, "trees");
    FileNodeIterator it = begin(trees_node);

    for(int treeidx = 0; treeidx < ntrees; treeidx++, getNextIterator_(&it))
    {
        FileNode nfn = FileNode_op(fileNode(it.fs, (const CvFileNode*)(const void*)it.reader.ptr), "nodes");
        readTree(&(obj->impl), nfn);
    }
}


/** @brief Loads algorithm from the file

     @param filename Name of the file to read.
     @param objname The optional name of the node to read (if empty, the first top-level node will be used)

     This is static template method of Algorithm. It's usage is following (in the case of SVM):
     @code
     Ptr<SVM> svm = Algorithm::load<SVM>("my_svm_model.xml");
     @endcode
     In order to make this method work, the derived class must overwrite Algorithm::read(const
     FileNode& fn).
     */
Boost* load_ml(char* filename)
{
    FileStorage fs = fileStorage(filename, 0/*READ*/);
    FileNode fn = getFirstTopLevelNode(fs);
    Boost* obj = createBoost();
    read_ml(obj, fn);
    return obj;
}

//Predicts response(s) for the provided sample(s) 
void predict_ml(DTreesImplForBoost* impl, Mat samples, int flags)
{
	int rtype = 5;
	int i, nsamples = samples.rows;
	float retval = 1.0f;
	bool iscls = true;
	float scale = 1.f;

	nsamples = min(nsamples, 1);
	return predictTrees(init_Point(0, vector_size(impl->roots)), row_op(samples, init_Point(y, y+1)), flags)*scale;
}