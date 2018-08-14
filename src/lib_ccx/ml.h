#include "types.h"

#define CV_DTREE_CAT_DIR(idx,subset) \
        (2*((subset[(idx)>>5]&(1 << ((idx) & 31)))==0)-1)

typedef struct TreeParams
{
    bool  useSurrogates;
    bool  use1SERule;
    bool  truncatePrunedTree;
    Mat priors;

    int   maxCategories;
    int   maxDepth;
    int   minSampleCount;
    int   CVFolds;
    float regressionAccuracy;
} TreeParams ;

TreeParams init_TreeParams()
{
    TreeParams p;
    p.maxDepth = INT_MAX;
    p.minSampleCount = 10;
    p.regressionAccuracy = 0.01f;
    p.useSurrogates = false;
    p.maxCategories = 10;
    p.CVFolds = 10;
    p.use1SERule = true;
    p.truncatePrunedTree = true;
    Mat* m = &(p.priors);
    m->rows = m->cols = 0;
    m->flags = MAGIC_VAL;
    m->data = 0;
    m->datastart = 0;
    m->datalimit = 0;
    m->dataend = 0;
    m->step = 0;
    return p;
}

typedef struct TrainData
{
} TrainData ;

typedef struct WorkData
{
    TrainData* data; //No fields, only funcs.
    vector* wnodes; //WNode
    vector* wsplits; //WSplit
    vector* wsubsets; //int
    vector* cv_Tn; //double
    vector* cv_node_risk; //double
    vector* cv_node_error; //double
    vector* cv_labels; //int
    vector* sample_weights; //double
    vector* cat_responses; //int
    vector* ord_responses; //double
    vector* sidx; //int
    int maxSubsetSize;
} WorkData ;

typedef struct BoostTreeParams
{
    int boostType;
    int weakCount;
    double weightTrimRate;
} BoostTreeParams ;

typedef struct DTreesImplForBoost
{
    TreeParams params;

    vector* varIdx; //int
    vector* compVarIdx; //int
    vector* varType; //unsigned char
    vector* catOfs; //Point
    vector* catMap; //int
    vector* roots; //int
    vector* nodes; //Node
    vector* splits; //Split
    vector* subsets; //int
    vector* classLabels; //int
    vector* missingSubst; //float
    vector* varMapping; //int
    bool _isClassifier;

    WorkData* w;

    BoostTreeParams bparams;
    vector* sumResult; //double
} DTreesImplForBoost ;

typedef struct Node
{
    double value; //!< Value at the node: a class label in case of classification or estimated
                      //!< function value in case of regression.
    int classIdx; //!< Class index normalized to 0..class_count-1 range and assigned to the
                  //!< node. It is used internally in classification trees and tree ensembles.
    int parent; //!< Index of the parent node
    int left; //!< Index of the left child node
    int right; //!< Index of right child node
    int defaultDir; //!< Default direction where to go (-1: left or +1: right). It helps in the
                    //!< case of missing values.
    int split; //!< Index of the first split
} Node ;

typedef struct Split
{
    int varIdx; //!< Index of variable on which the split is created.
    bool inversed; //!< If true, then the inverse split rule is used (i.e. left and right
                   //!< branches are exchanged in the rule expressions below).
    float quality; //!< The split quality, a positive number. It is used to choose the best split.
    int next; //!< Index of the next split in the list of splits for the node
    float c; /**< The threshold value in case of split on an ordered variable.
                  The rule is:
                  @code{.none}
                  if var_value < c
                    then next_node <- left
                    else next_node <- right
                  @endcode */
    int subsetOfs; /**< Offset of the bitset used by the split on a categorical variable.
                        The rule is:
                        @code{.none}
                        if bitset[var_value] == 1
                            then next_node <- left
                            else next_node <- right
                        @endcode */
} Split ;

typedef struct Boost
{
    DTreesImplForBoost impl;
} Boost;