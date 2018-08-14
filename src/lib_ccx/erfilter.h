#ifndef ERFILTER_H
#define ERFILTER_H

#include "Mat.h"
// default 1st and 2nd stage classifier
typedef struct ERClassifierNM
{
    Boost* boost;
} ERClassifierNM ;

/** @brief The ERStat structure represents a class-specific Extremal Region (ER).

An ER is a 4-connected set of pixels with all its grey-level values smaller than the values in its
outer boundary. A class-specific ER is selected (using a classifier) from all the ER's in the
component tree of the image. :
 */
typedef struct ERStat
{
    //! Incrementally Computable Descriptors
    int pixel;
    int level;
    int area;
    int perimeter;
    int euler;                 //!< Euler's number
    Rect rect;                 // Rect_<int>
    double raw_moments[2];     //!< order 1 raw moments to derive the centroid
    double central_moments[3]; //!< order 2 central moments to construct the covariance matrix
    vector* crossings;         //!< horizontal crossings
    float med_crossings;       //!< median of the crossings at three different height levels

    //! stage 2 features
    float hole_area_ratio;
    float convex_hull_ratio;
    float num_inflexion_points;

    //! pixel list after the 2nd stage
    int** pixels;

    //! probability that the ER belongs to the class we are looking for
    double probability;

    //! pointers preserving the tree structure of the component tree --> might be optional
    ERStat* parent;
    ERStat* child;
    ERStat* next;
    ERStat* prev;

    //! whenever the regions is a local maxima of the probability
    bool local_maxima;
    ERStat* max_probability_ancestor;
    ERStat* min_probability_ancestor;
} ERStat ;

/** @brief Base struct for 1st and 2nd stages of Neumann and Matas scene text detection algorithm @cite Neumann12. :

Extracts the component tree (if needed) and filter the extremal regions (ER's) by using a given classifier.
 */
typedef struct ERFilterNM
{
    int type; // type = 1 --> evalNM1, type = 2 --> evalNM2
    float minProbability;   
    bool  nonMaxSuppression;
    float minProbabilityDiff;

    int thresholdDelta;
    float maxArea; 
    float minArea;

    ERClassifierNM* classifier;

    // count of the rejected/accepted regions
    int num_rejected_regions;
    int num_accepted_regions;

    //! Pointer to the Input/output regions
    vector* regions; //ERStat
    //! image mask used for feature calculations
    Mat region_mask;
} ERFilterNM ;

/** @brief Compute the different channels to be processed independently in the N&M algorithm @cite Neumann12.

@param _src Source image. Must be RGB CV_8UC3.

@param _channels Output vector\<Mat\> where computed channels are stored.

@param _mode Mode of operation. Currently the only available options are:
**ERFILTER_NM_RGBLGrad** (used by default) and **ERFILTER_NM_IHSGrad**.

In N&M algorithm, the combination of intensity (I), hue (H), saturation (S), and gradient magnitude
channels (Grad) are used in order to obtain high localization recall. This implementation also
provides an alternative combination of red (R), green (G), blue (B), lightness (L), and gradient
magnitude (Grad).
*/
void computeNMChannels(Mat src, vector* _channels/* Mat */);

/*!
    Allow to implicitly load the default classifier when creating an ERFilterNM object.
    The function takes as parameter the XML or YAML file with the classifier model
    (e.g. trained_classifierNM1.xml or trained_classifierNM2.xml)
    returns a pointer to ERClassifierNM.
*/
ERClassifierNM* loadclassifierNM(ERClassifierNM* erc, const char* filename);



/*!
    Create an Extremal Region Filter for the 1st stage classifier of N&M algorithm
    Neumann L., Matas J.: Real-Time Scene Text Localization and Recognition, CVPR 2012

    The component tree of the image is extracted by a threshold increased step by step
    from 0 to 255, incrementally computable descriptors (aspect_ratio, compactness,
    number of holes, and number of horizontal crossings) are computed for each ER
    and used as features for a classifier which estimates the class-conditional
    probability P(er|character). The value of P(er|character) is tracked using the inclusion
    relation of ER across all thresholds and only the ERs which correspond to local maximum
    of the probability P(er|character) are selected (if the local maximum of the
    probability is above a global limit pmin and the difference between local maximum and
    local minimum is greater than minProbabilityDiff).

    \param  cb                Callback with the classifier.
                              default classifier can be implicitly load with function loadClassifierNM()
                              from file in "trained_classifierNM1.xml"
    \param  thresholdDelta    Threshold step in subsequent thresholds when extracting the component tree
    \param  minArea           The minimum area (% of image size) allowed for retrieved ER's
    \param  minArea           The maximum area (% of image size) allowed for retrieved ER's
    \param  minProbability    The minimum probability P(er|character) allowed for retrieved ER's
    \param  nonMaxSuppression Whenever non-maximum suppression is done over the branch probabilities
    \param  minProbability    The minimum probability difference between local maxima and local minima ERs
*/
ERFilterNM* createERFilterNM1(ERClassifierNM* erc, int thresholdDelta, float minArea, float maxArea, float minArea, float minProbability, bool nonMaxSuppression, float minProbabilityDiff, bool is_dummy);


/*!
    Create an Extremal Region Filter for the 2nd stage classifier of N&M algorithm
    Neumann L., Matas J.: Real-Time Scene Text Localization and Recognition, CVPR 2012

    In the second stage, the ERs that passed the first stage are classified into character
    and non-character classes using more informative but also more computationally expensive
    features. The classifier uses all the features calculated in the first stage and the following
    additional features: hole area ratio, convex hull ratio, and number of outer inflexion points.

    \param  cb             Callback with the classifier
                           default classifier can be implicitly load with function loadClassifierNM()
                           from file in samples/cpp/trained_classifierNM2.xml
    \param  minProbability The minimum probability P(er|character) allowed for retreived ER's
*/
ERFilterNM* createERFilterNM2(ERClassifierNM* erc, float minProbability);

/** @brief The key method of ERFilter algorithm.

    Takes image on input and returns the selected regions in a vector of ERStat only distinctive
    ERs which correspond to characters are selected by a sequential classifier

    @param image Single channel image CV_8UC1

    @param regions Output for the 1st stage and Input/Output for the 2nd. The selected Extremal Regions
    are stored here.

    Extracts the component tree (if needed) and filter the extremal regions (ER's) by using a given
    classifier.
     */
void run(ERFilterNM* filter, Mat m, vector* _regions);

// extract the component tree and store all the ER regions
// uses the algorithm described in
// Linear time maximally stable extremal regions, D Nistér, H Stewénius – ECCV 2008
void er_tree_extract(ERFilterNM* filter, Mat src);

// accumulate a pixel into an ER
void er_add_pixel(ERFilterNM* filter, ERStat* parent, int x, int y, int non_border_neighbours,
                                                            int non_border_neighbours_horiz,
                                                            int d_C1, int d_C2, int d_C3);

// merge an ER with its nested parent
void er_merge(ERFilterNM* filter, ERStat* parent, ERStat* child);

// copy extracted regions into the output vector
ERStat* er_save(ERFilterNM* filter, ERStat *er, ERStat *parent, ERStat *prev);

// recursively walk the tree and filter (remove) regions using the callback classifier
ERStat* er_tree_filter(ERFilterNM* filter, Mat* src, ERStat* stat, ERStat *parent, ERStat *prev);

// recursively walk the tree selecting only regions with local maxima probability
ERStat* er_tree_nonmax_suppression(ERFilterNM* filter, ERStat* stat, ERStat* parent, ERStat* prev);

// Evaluates if a pair of regions is valid or not
// using thresholds learned on training (defined above)
bool isValidPair(Mat grey, Mat lab, Mat mask, vector* channels, vector* regions, Point idx1, Point idx2);

// Evaluates if a set of 3 regions is valid or not
// using thresholds learned on training (defined above)
bool isValidTriplet(vector< vector<ERStat> >& regions, region_pair pair1, region_pair pair2, region_triplet &triplet);

// Evaluates if a set of more than 3 regions is valid or not
// using thresholds learned on training (defined above)
bool isValidSequence(region_sequence &sequence1, region_sequence &sequence2);

//Extracts text regions from image.
void detect_regions(Mat src, vector* channels,/*Mat*/vector* regions_groups/*vector<Point>*/, vector* groups_boxes/*Rect*/);

void init_ERStat(ERStat* erstat, int init_level, int init_pixel, int init_x, int init_y)
{
    erstat->level = init_level;
    erstat->pixel = init_pixel;
    erstat->area = 0;
    erstat->perimeter = 0;
    erstat->euler = 0;
    erstat->probability = 1.0;
    erstat->local_maxima = 0;
    erstat->parent = 0;
    erstat->child = 0;
    erstat->prev = 0;
    erstat->next = 0;
    erstat->max_probability_ancestor = 0;
    erstat->min_probability_ancestor = 0;
    erstat->rect = init_Rect(init_x, init_y, 1, 1);
    (erstat->raw_moments)[0] = 0.0;
    (erstat->raw_moments)[1] = 0.0;
    (erstat->central_moments)[0] = 0.0;
    (erstat->central_moments)[1] = 0.0;
    (erstat->central_moments)[2] = 0.0;
    erstat->crossings = malloc(sizeof(vector));
    vector_init(erstat->crossings);
    int val = 0;
    vector_add(erstat->crossings, &val);
}

// set/get methods to set the algorithm properties,
void setCallback(ERFilterNM* filter,ERClassifierNM *erc)
{
    filter->classifier = *erc;
}

void setMinArea(ERFilterNM* filter, float _minArea)
{
    filter->minArea = _minArea;
}

void setMaxArea(ERFilterNM* filter, float _maxArea)
{
    filter->maxArea = _maxArea;
}

void setMinProbability(ERFilterNM* filter, float _minProbability)
{
    filter->minProbability = _minProbability;
}

void setNonMaxSuppression(ERFilterNM* filter, bool _nonMaxSuppression)
{
    filter->nonMaxSuppression = _nonMaxSuppression;
}

void setMinProbabilityDiff(ERFilterNM* filter, float _minProbabilityDiff)
{
    filter->minProbabilityDiff = _minProbabilityDiff;
}

void setThresholdDeta(ERFilterNM* filter, int thresholdDelta)
{
    filter->thresholdDelta = thresholdDelta;
}

void findContours(Mat image0, vector* contours/* vector<Point> */, vector* hierarchy/* Scalar */, int mode, int method, Point offset);

#endif