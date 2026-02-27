#pragma once

#include <stdint.h>

void BlockDecompressImageBC1(uint32_t width, uint32_t height, const uint8_t* blockStorage, unsigned char* image);
void BlockDecompressImageBC2(uint32_t width, uint32_t height, const uint8_t* blockStorage, unsigned char* image);
void BlockDecompressImageBC3(uint32_t width, uint32_t height, const uint8_t* blockStorage, unsigned char* image);
