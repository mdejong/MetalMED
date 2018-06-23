# MetalMED

A MED predictor combined with a huffman decoder implemented on GPU with Metal API, adapted from Basic Texturing example provided by Apple. This decoder is known to work on iOS and should work on other Metal capable hardware.

## Overview

Combine simple MED image pixel prediction with huffman decoding on the GPU.

## Decoding Speed

This code attempts to improve processing time of the image decoding process by compressing pixels using the MED predictor from JPEG-LS. Reading from memory seems to be the bottleneck in the GPU decode logic so reducing the amount of memory to be read using the MED predictor should significantly reduce decoding time.

See AAPLRenderer.m and AAPLShaders.metal for the core GPU rendering logic. A table based huffman encoder and decoder are also included.

