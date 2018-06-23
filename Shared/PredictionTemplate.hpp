//
//  PredictionTemplate.hpp
//
//  Created by Mo DeJong on 6/3/18.
//  Copyright © 2018 helpurock. All rights reserved.
//
//  The prediction template logic makes it possible to encode data
//  in row major order using different kinds of prediction. Each
//  prediction operation will read from 1 to N previous pixel
//  values and then these values will be used to generate a prediction.
//  The error amount between the actual value and the prediction value
//  are then encoded as uint8_t values that represent positive and
//  negative deltas. These prediction functions can operate on either
//  8 bit grayscale values or 32 bit BGRA pixels.

#ifndef _PredictionTemplate_hpp
#define _PredictionTemplate_hpp

#include <stdio.h>
#include <cinttypes>
#include <vector>
#include <functional>

using namespace std;

// Function declarations to indicate input and output for prediction

// FIXME: no reason to pass height if it is unused in calculations

typedef uint8_t PredictionTemplate_pred8(const uint8_t * inSamplesPtr, int width, int height, int x, int y, int offset);
typedef uint32_t PredictionTemplate_pred32(const uint32_t * inSamplesPtr, int width, int height, int x, int y, int offset);

#define PT_RSHIFT_MASK(num, shift, mask) ((num >> shift) & mask)
#define PT_MASK_LSHIFT(num, mask, shift) ((num & mask) << shift)

// Generic method that encodes a prediction error

static
inline
uint32_t PredictionTemplate_encode_pred8_error(uint8_t pred, uint8_t sample) {
    const bool debug = false;
    
    if (debug) {
        printf("pred=%d, sample=%d\n", pred, sample);
    }
    
    uint8_t resultByte = sample - pred;
    uint32_t pred_err = ((uint32_t)resultByte) & 0xFF;
    
    if (debug) {
        printf("gradclamp_encode_predict_error : sample %d : prediction %d : prediction_err %d", sample, pred, pred_err);
    }
    
    return pred_err;
}

// This logic will decode a prediction error back into the original sample
// value. This calculation must be done in terms of a signed 8 bit value
// which is then converted back to an unsigned integer value and returned.

static
inline
uint32_t PredictionTemplate_decode_pred8_error(uint8_t pred_err, uint8_t pred) {
    const bool debug = false;
    
    if (debug) {
        printf("pred_err=%d, pred=%d\n", pred_err, pred);
    }
    
    // Undo ( err = pixel - prediction )
    // by calculating:
    // ( pixel = err + prediction ) using unsigned 8bit math.
    
    uint8_t result = pred_err + pred;
    uint32_t sample = ((uint32_t)result) & 0xFF;
    
    if (debug) {
        printf("gradclamp8_decode_predict_error : pred_err %d : prediction %d : sample %d\n", pred_err, pred, sample);
    }
    
    return sample;
}

// Given a buffer of BGRA pixels, do prediction for each value and
// encode the prediction residuals from predictor function P.

template <typename FuncType>
void PredictionTemplate_encode_pred32_error(
                                          FuncType P,
                                          const uint32_t *inSamplesPtr,
                                          uint32_t *outPredErrPtr,
                                          const int width,
                                          const int height,
                                          int originX,
                                          int originY,
                                          int regionWidth,
                                          int regionHeight
                                          )
{
    const bool debug = false;
    
    if (debug) {
        printf("PredictionTemplate_encode_pred32_error() (W x H) (%5d x %5d) (origin %d %d, %d x %d) \n", width, height, originX, originY, regionWidth, regionHeight);
    }
    
    // P : uint32_t P(inSamplesPtr, width, height, x, y, offset)
    
    const int maxX = originX + regionWidth;
    const int maxY = originY + regionHeight;
    
    int ySum = (originY * width);
    
    for (int y = originY; y < maxY; y++) {
        int offset = ySum + originX;
        
        for (int x = originX; x < maxX; x++) {
            uint32_t pred = P(inSamplesPtr, width, height, x, y, offset);
            uint32_t inSamples = inSamplesPtr[offset];
            
            uint8_t c3_sample = PT_RSHIFT_MASK(inSamples, 24, 0xFF);
            uint8_t c3_pred = PT_RSHIFT_MASK(pred, 24, 0xFF);
            uint32_t c3_pred_err = PredictionTemplate_encode_pred8_error(c3_pred, c3_sample);
            
            uint8_t c2_sample = PT_RSHIFT_MASK(inSamples, 16, 0xFF);
            uint8_t c2_pred = PT_RSHIFT_MASK(pred, 16, 0xFF);
            uint32_t c2_pred_err = PredictionTemplate_encode_pred8_error(c2_pred, c2_sample);
            
            uint8_t c1_sample = PT_RSHIFT_MASK(inSamples, 8, 0xFF);
            uint8_t c1_pred = PT_RSHIFT_MASK(pred, 8, 0xFF);
            uint32_t c1_pred_err = PredictionTemplate_encode_pred8_error(c1_pred, c1_sample);
            
            uint8_t c0_sample = PT_RSHIFT_MASK(inSamples, 0, 0xFF);
            uint8_t c0_pred = PT_RSHIFT_MASK(pred, 0, 0xFF);
            uint32_t c0_pred_err = PredictionTemplate_encode_pred8_error(c0_pred, c0_sample);
            
            uint32_t outPredErr = (c3_pred_err << 24) | (c2_pred_err << 16) | (c1_pred_err << 8) | (c0_pred_err);
            
            outPredErrPtr[offset] = outPredErr;
            offset += 1;
        }
        
        ySum += width;
    }
    
    return;
}

// Given a buffer of BGRA pixels, do prediction and then apply
// residual over the prediction to decode the original pixel value.

template <typename FuncType>
void PredictionTemplate_decode_pred32_error(
                                            FuncType P,
                                            const uint32_t *inPredErrPtr,
                                            uint32_t *outSamplesPtr,
                                            const int width,
                                            const int height,
                                            int originX,
                                            int originY,
                                            int regionWidth,
                                            int regionHeight
                                            )
{
    const bool debug = false;
    
    if (debug) {
        printf("PredictionTemplate_decode_pred32_error() (W x H) (%5d x %5d) (origin %d %d, %d x %d) \n", width, height, originX, originY, regionWidth, regionHeight);
    }
    
    // P : uint32_t P(inSamplesPtr, width, height, x, y, offset)

    const int maxX = originX + regionWidth;
    const int maxY = originY + regionHeight;
    
    int ySum = (originY * width);
    
    for (int y = originY; y < maxY; y++) {
        int offset = ySum + originX;
        
        for (int x = originX; x < maxX; x++) {
            uint32_t inPredErr = inPredErrPtr[offset];
            uint32_t pred = P(outSamplesPtr, width, height, x, y, offset);
            
            uint8_t c3_pred_err = PT_RSHIFT_MASK(inPredErr, 24, 0xFF);
            uint8_t c3_pred = PT_RSHIFT_MASK(pred, 24, 0xFF);
            uint32_t c3_sample = PredictionTemplate_decode_pred8_error(c3_pred_err, c3_pred);

            uint8_t c2_pred_err = PT_RSHIFT_MASK(inPredErr, 16, 0xFF);
            uint8_t c2_pred = PT_RSHIFT_MASK(pred, 16, 0xFF);
            uint32_t c2_sample = PredictionTemplate_decode_pred8_error(c2_pred_err, c2_pred);

            uint8_t c1_pred_err = PT_RSHIFT_MASK(inPredErr, 8, 0xFF);
            uint8_t c1_pred = PT_RSHIFT_MASK(pred, 8, 0xFF);
            uint32_t c1_sample = PredictionTemplate_decode_pred8_error(c1_pred_err, c1_pred);

            uint8_t c0_pred_err = PT_RSHIFT_MASK(inPredErr, 0, 0xFF);
            uint8_t c0_pred = PT_RSHIFT_MASK(pred, 0, 0xFF);
            uint32_t c0_sample = PredictionTemplate_decode_pred8_error(c0_pred_err, c0_pred);
            
            uint32_t outSamples = (c3_sample << 24) | (c2_sample << 16) | (c1_sample << 8) | (c0_sample);;
            
            outSamplesPtr[offset] = outSamples;
            offset += 1;
        }
        
        ySum += width;
    }
    
    return;
}

#endif // _PredictionTemplate_hpp
