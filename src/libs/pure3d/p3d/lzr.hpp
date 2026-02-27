/*===========================================================================
    File:: lzr.hpp

    A compression format designed for speedy decompression

    Copyright (c) 2001 Radical Entertainment, Inc.  All rights reserved.

===========================================================================*/

#ifndef _LZR_HPP
#define _LZR_HPP

#define LZR_BLOCK_SIZE 4096

void lzr_decompress (const unsigned char* input, unsigned int inputsize,
                            unsigned char* output, unsigned int outputsize);

#endif

