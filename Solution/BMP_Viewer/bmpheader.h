#ifndef Y_BMPHEADER_H
#define Y_BMPHEADER_H

#include <cstdint>
#include <istream>

enum FileHeaderSize { FILE_HEADER_SIZE = 14 };

enum InfoHeaderSize { V1_HEADER_SIZE = 40, V2_HEADER_SIZE = 52, V3_HEADER_SIZE = 56, V4_HEADER_SIZE = 108,
                      V5_HEADER_SIZE = 124, CORE_HEADER_SIZE = 12, CORE2_HEADER_SIZE = 64 };

struct Channel
{
    void set(uint32_t mask);
    uint8_t offset;
    uint8_t size;
};

enum Compression { NO_COMPRESSION = 0, RLE8_COMPRESSION = 1, RLE4_COMPRESSION = 2, BITFIELDS_COMPRESSION = 3,
                   JPEG_COMPRESSION = 4, PNG_COMPRESSION = 5, ALPHABITFIELDS_COMPRESSION = 6,
                   HUFFMUN_COMPRESSION = 8 + 3, RLE24_COMPRESSION = 8 + 4 };

struct BMPHeader
{
    uint32_t fileSize;
    uint32_t headerSize;
    bool bottomToTop;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    Compression compression;
    uint16_t paletteSize;
    uint8_t paletteElementSize;
    Channel bChannel;
    Channel gChannel;
    Channel rChannel;
    Channel aChannel;
    uint32_t dataOffset;
};

std::istream& operator>>(std::istream& stream, BMPHeader& header);

void writeBMPHeader(unsigned char *buffer, uint32_t width, uint32_t height);

#endif //Y_BMPHEADER_H
