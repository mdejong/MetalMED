//
//  PredictionMethods.cpp
//
//  Created by Mo DeJong on 6/4/18.
//  Copyright © 2018 helpurock. All rights reserved.
//

#include "PredictionMethods.h"

#include <assert.h>

#include "PredictionTemplate.hpp"

// util methods

static inline
uint32_t min2ui(uint32_t v1, uint32_t v2) {
    if (v1 <= v2) {
        return v1;
    } else {
        return v2;
    }
}

static inline
uint32_t min3ui(uint32_t v1, uint32_t v2, uint32_t v3) {
    uint32_t min = min2ui(v1, v2);
    return min2ui(min, v3);
}

static inline
uint32_t max2ui(uint32_t v1, uint32_t v2) {
    if (v1 >= v2) {
        return v1;
    } else {
        return v2;
    }
}

static inline
uint32_t max3ui(uint32_t v1, uint32_t v2, uint32_t v3) {
    uint32_t max = max2ui(v1, v2);
    return max2ui(max, v3);
}

// Clamp an integer value to a min and max range.
// This method operates only on unsigned integer
// values.

static inline
uint32_t clampui(uint32_t val, uint32_t min, uint32_t max) {
    if (val < min) {
        return min;
    } else if (val > max) {
        return max;
    } else {
        return val;
    }
}

// Clamp an integer value to a min and max range.
// This method operates only on signed integer
// values.

static inline
int32_t clampi(int32_t val, int32_t min, int32_t max) {
    if (val < min) {
        return min;
    } else if (val > max) {
        return max;
    } else {
        return val;
    }
}

static inline
void Convert_offset_to_xy(int offset, const int width, int * x1, int * y1)
{
    int x = offset % width;
    int y = offset / width;
    *x1 = x;
    *y1 = y;
}

// ClampedGradPredictor described here:
// http://cbloomrants.blogspot.com/2010/06/06-20-10-filters-for-png-alike.html
//
// This inlined predictor function operates on 1 component
// of a 4 component word. Note that this method operates
// in terms of 8, 16, or 32 bit integers.

static
inline
uint32_t med_predict(uint32_t a, uint32_t b, uint32_t c) {
    const bool debug = false;
    
    if (debug) {
        printf("a=%d (left), b=%d (up), c=%d (upleft)\n", a, b, c);
    }
    
    // The next couple of calculations make use of signed integer
    // math, though the returned result will never be smaller
    // than the min or larger than the max, so there should
    // not be an issue with this method returning values
    // outside the byte range if the original input is in the
    // byte range.
    
    int p = a + b - c;
    
    if (debug) {
        printf("p = (a + b - c) = (%d + %d - %d) = %d\n", a, b, c, p);
    }
    
    // Find the min and max value of the 3 input values
    
    uint32_t min = min3ui(a, b, c);
    uint32_t max = max3ui(a, b, c);
    
    // FIXME: currently, there appears to be an issue if the
    // values being predicted are as large as the largest integer,
    // but this is ignored for now since only bytes will be predicted.
    
    // Note that the compare here is done in terms of a signed
    // integer but that the min and max are known to be in terms
    // of an unsigned int, so the result will never be less than
    // min or larger than max.
    
    int32_t clamped = clampi(p, (int32_t)min, (int32_t)max);
    
#ifdef DEBUG
    assert(clamped >= 0);
    assert(clamped >= min);
    assert(clamped <= max);
#endif // DEBUG
    
    if (debug) {
        printf("p = %d\n", p);
        printf("(min, max) = (%d, %d)\n", min, max);
        printf("clamped = %d\n", clamped);
    }
    
    return (uint32_t) clamped;
}

// This gradclamp predictor logic operates in terms of bytes
// and not whole pixels. But for the sake of efficient code,
// the logic executes in terms of blocks of 4 byte elements at
// a time read from samplesPtr. This approach makes it efficient
// to predict RGB or RGBA pixels packed into a word (a pixel).
// The caller must make sure to supply values 4 at a time and
// deal with the results if fewer than 4 are needed at the
// end of the buffer.
//
// The width argument indicates the width of the buffer being
// predicted. The offset argument indicates the ith offset
// in samplesPtr to read from.

static inline
uint32_t med_predict32(const uint32_t * samplesPtr, int width, int height, int x, int y, int offset) {
    const bool debug = false;
    
    // indexes must be signed so that they can be negative at top or left edge
    
    int upOffset = offset - width;
    int upLeftOffset = upOffset - 1;
    int leftOffset = offset - 1;
    
    if (debug) {
        printf("upi=%d, upLefti=%d, lefti=%d\n", upOffset, upLeftOffset, leftOffset);
    }
    
    uint32_t upSamples;
    uint32_t upLeftSamples;
    uint32_t leftSamples;
    
    // In the case of column 0, the L and UL samples should be treated as missing
    // as opposed to reading values from the end of the previous row.
    
    if (x == 0) {
        leftOffset = -1;
        upLeftOffset = -1;
    }
    
    // left = a
    
    if (leftOffset < 0) {
        leftSamples = 0x0;
    } else {
        leftSamples = samplesPtr[leftOffset];
    }
    
    // up = b
    
    if (upOffset < 0) {
        upSamples = 0x0;
    } else {
        upSamples = samplesPtr[upOffset];
    }
    
    // upLeft = c
    
    if (upLeftOffset < 0) {
        upLeftSamples = 0x0;
    } else {
        upLeftSamples = samplesPtr[upLeftOffset];
    }
    
    // Execute predictor for each of the 4 components
    
    uint32_t c3 = med_predict(
                                    PT_RSHIFT_MASK(leftSamples, 24, 0xFF),
                                    PT_RSHIFT_MASK(upSamples, 24, 0xFF),
                                    PT_RSHIFT_MASK(upLeftSamples, 24, 0xFF)
                                    );
    
    uint32_t c2 = med_predict(
                                    PT_RSHIFT_MASK(leftSamples, 16, 0xFF),
                                    PT_RSHIFT_MASK(upSamples, 16, 0xFF),
                                    PT_RSHIFT_MASK(upLeftSamples, 16, 0xFF)
                                    );
    
    uint32_t c1 = med_predict(
                                    PT_RSHIFT_MASK(leftSamples, 8, 0xFF),
                                    PT_RSHIFT_MASK(upSamples, 8, 0xFF),
                                    PT_RSHIFT_MASK(upLeftSamples, 8, 0xFF)
                                    );
    uint32_t c0 = med_predict(
                                    PT_RSHIFT_MASK(leftSamples, 0, 0xFF),
                                    PT_RSHIFT_MASK(upSamples, 0, 0xFF),
                                    PT_RSHIFT_MASK(upLeftSamples, 0, 0xFF)
                                    );
    
    if (debug) {
        printf("c3=%d, c2=%d, c1=%d, c0=%d\n", c3, c2, c1, c0);
    }
    
    // It should not be possible for values larger than a byte range
    // to be returned, since gradclamp_predict() returns one of the 3 passed
    // in values and they are all masked to 0xFF.
    
#if defined(DEBUG)
    assert(c3 <= 0xFF);
    assert(c2 <= 0xFF);
    assert(c1 <= 0xFF);
    assert(c0 <= 0xFF);
#endif // DEBUG
    
    // Mask to 8bit value and shift into component position
    
    uint32_t components =
    PT_MASK_LSHIFT(c3, 0xFF, 24) |
    PT_MASK_LSHIFT(c2, 0xFF, 16) |
    PT_MASK_LSHIFT(c1, 0xFF, 8) |
    PT_MASK_LSHIFT(c0, 0xFF, 0);
    
    return components;
}

// For testing only

uint32_t med32_test(uint32_t *samplesPtr, uint32_t width, uint32_t offset) {
    int originX, originY, regionWidth, regionHeight;
    Convert_offset_to_xy(offset, width, &originX, &originY);
    regionWidth = 1;
    regionHeight = 1;
    int height = -1;
    return med_predict32(samplesPtr, width, height, originX, originY, offset);
}

// External API entry point with C linkage

void med_encode_pred32_error(
                             const uint32_t *inSamplesPtr,
                             uint32_t *outPredErrPtr,
                             int width,
                             int height,
                             int originX,
                             int originY,
                             int regionWidth,
                             int regionHeight
                             )
{
    PredictionTemplate_encode_pred32_error<typeof(med_predict32)>(med_predict32, inSamplesPtr, outPredErrPtr, width, height, originX, originY, regionWidth, regionHeight);
}

void med_decode_pred32_error(
                             const uint32_t *inPredErrPtr,
                             uint32_t *outSamplesPtr,
                             int width,
                             int height,
                             int originX,
                             int originY,
                             int regionWidth,
                             int regionHeight
                             )
{
    PredictionTemplate_decode_pred32_error<typeof(med_predict32)>(med_predict32, inPredErrPtr, outSamplesPtr, width, height, originX, originY, regionWidth, regionHeight);
}
