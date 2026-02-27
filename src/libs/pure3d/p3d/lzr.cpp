/*===========================================================================
    File:: lzr.cpp

    A compression format designed for speedy decompression

    Copyright (c) 2001 Radical Entertainment, Inc.  All rights reserved.


    The format of LZR (Lempel - Ziv - Radical) is as follows:

    The file consists of sequences of literal runs, or dictionary matches.

    A literal run is specified with a byte from 0-15, specifying the length
    of the run.  If the byte is 0, the length is 15 + the next byte.  If that
    byte is 0, add 255 and continue...

    eg:
    Bytes    Length
    14       14
    0 1      16  (15 + 1)
    0 24     39  (15 + 24)
    0 0 1    271 (15 + 255 + 1)
    0 0 0 1  526 (15 + 255 + 255 + 1)

    A match begins with a byte greater than 15.  Matches encode an offset into
    previously decompressed characters, and a count of characters.

    The count is stored in the low 4 bits of the first byte, and uses the same
    rules as the literal run for extending the count beyond 15.  The offset is
    stored in the high 4 bits, plus 16 * the next byte (after count extensions).

    eg:

    Code bytes            Offset      Count

    17  (0x11) 0          1           1
    234 (0xEA) 1          30          10
    64  (0x40) 43 52      836         58          

    This compressor does NOT encode EOF.  The caller must pass in the correct
    output size.
    
===========================================================================*/



#include <p3d/lzr.hpp>
#include <p3d/error.hpp>

#include <string.h>

void lzr_decompress (const unsigned char* input, unsigned int inputsize,
                            unsigned char* output, unsigned int outputsize)
{
    unsigned int outputcount = 0;

    while(outputcount < outputsize)
    {
        unsigned int code = *input++;

        if(code > 15)
        {
            // a match
            int matchlength = code & 15;

            if(matchlength == 0)
            {
                matchlength += 15;
                while (*input == 0)
                {
                    matchlength += 255;
                    input++;
                }
                matchlength += *input++;
            }

            int offset = (code >> 4) | (*input++) << 4;
            unsigned char* match_ptr = output - offset;

            // shortest match is 4 characters, so we can unroll the loop
            int len = matchlength>>2;
            matchlength -= len<<2;

            do
            {
                *output++ = *match_ptr++;
                *output++ = *match_ptr++;
                *output++ = *match_ptr++;
                *output++ = *match_ptr++;
                outputcount += 4;
            } while(--len);

            while(matchlength)
            {
                *output++ = *match_ptr++;
                outputcount++;
                matchlength--;
            }
        }
        else
        {
            // A literal run
            int runlength = code;
            
            if(runlength == 0)
            {
                while (*input == 0)
                {
                    runlength += 255;
                    input++;
                }
                runlength += *input++;

                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                *output++ = *input++;
                outputcount += 15;
            }

            do
            {
                *output++ = *input++;
                outputcount++;
            } while(--runlength);
        }
    }
}

