//
//  PredictionMethods.h
//
//  Created by Mo DeJong on 6/4/18.
//  Copyright © 2018 helpurock. All rights reserved.
//

#ifndef _PredictionMethods_hpp
#define _PredictionMethods_hpp

#include <stdlib.h>

// Encode N byte symbols into and output buffer of at least n.
// Returns the number of bytes written to encodedBytes, this
// number of bytes should be less than the original???

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
  
// Read n bytes from inBytePtr and emit n symbols with fixed bit width
// calculated from input power of 2 value k (0 = 1, 1 = 2, 2 = 4).
// The returned buffer is dynamically allocated with malloc() and
// must be released via free(). NULL is returned if memory cannot be allocted.

    void med_encode_pred32_error(
                                 const uint32_t *inSamplesPtr,
                                 uint32_t *outPredErrPtr,
                                 const int width,
                                 const int height,
                                 int originX,
                                 int originY,
                                 int regionWidth,
                                 int regionHeight
                                 );
    
    void med_decode_pred32_error(
                                 const uint32_t *inPredErrPtr,
                                 uint32_t *outSamplesPtr,
                                 const int width,
                                 const int height,
                                 int originX,
                                 int originY,
                                 int regionWidth,
                                 int regionHeight
                                 );
    
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _PredictionMethods_hpp
