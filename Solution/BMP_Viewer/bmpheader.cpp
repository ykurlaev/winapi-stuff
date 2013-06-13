#include "bmpheader.h"
#include <cstring>
#include <stdexcept>

using std::istream;
using std::runtime_error;

enum FileHeaderFieldOffset { MAGIC1_OFFSET = 0, MAGIC2_OFFSET = 1, FILE_SIZE_OFFSET = 2, BITMAP_OFFSET_OFFSET = 10 };

enum FileHeaderFieldSize { MAGIC1_SIZE = 1, MAGIC2_SIZE = 1, FILE_SIZE_SIZE = 4, BITMAP_OFFSET_SIZE = 4 };

enum InfoHeaderFieldOffset { INFO_HEADER_SIZE_OFFSET = FILE_HEADER_SIZE + 0,
                             WIDTH_OFFSET = FILE_HEADER_SIZE + 4, HEIGHT_OFFSET = FILE_HEADER_SIZE + 8,
                             PLANES_OFFSET = FILE_HEADER_SIZE + 12, BPP_OFFSET = FILE_HEADER_SIZE + 14,
                             COMPRESSION_OFFSET = FILE_HEADER_SIZE + 16, IMAGE_SIZE_OFFSET = FILE_HEADER_SIZE + 20,
                             XRES_OFFSET = FILE_HEADER_SIZE + 24, YRES_OFFSET = FILE_HEADER_SIZE + 28,
                             COLORS_OFFSET = FILE_HEADER_SIZE + 32, R_MASK_OFFSET = FILE_HEADER_SIZE + 40,
                             G_MASK_OFFSET = FILE_HEADER_SIZE + 44, B_MASK_OFFSET = FILE_HEADER_SIZE + 48,
                             A_MASK_OFFSET = FILE_HEADER_SIZE + 52 };

enum InfoHeaderFieldSize { INFO_HEADER_SIZE_SIZE = 4, WIDTH_SIZE = 4, HEIGHT_SIZE = 4, PLANES_SIZE = 2,
                           BPP_SIZE = 2, COMPRESSION_SIZE = 4, IMAGE_SIZE_SIZE = 4, XRES_SIZE = 4, YRES_SIZE = 4,
                           COLORS_SIZE = 4, R_MASK_SIZE = 4, G_MASK_SIZE = 4, B_MASK_SIZE = 4, A_MASK_SIZE = 4 };

enum CoreHeaderFieldOffset { CORE_WIDTH_OFFSET = FILE_HEADER_SIZE + 4, CORE_HEIGHT_OFFSET = FILE_HEADER_SIZE + 6,
                             CORE_BPP_OFFSET = FILE_HEADER_SIZE + 10 };

enum CoreHeaderFieldSize { CORE_WIDTH_SIZE = 2, CORE_HEIGHT_SIZE = 2, CORE_BPP_SIZE = 2 };

#define WRITE(array, field, value) \
    for(int i = 0; i < field ## _SIZE; i++) \
    { array[field ## _OFFSET + i] = ((static_cast<uint32_t>(value)) >> (8 * i)) & UINT8_MAX; }

#define READ(array, field, value) \
	do { uint32_t uvalue = 0; \
    for(int i = 0; i < field ## _SIZE; i++) \
    { uvalue = uvalue | (array[field ## _OFFSET + i] << (8 * i)); } \
	value = uvalue; } while(0)

void Channel::set(uint32_t mask)
{
    int i = 0;
    while(i < 32 && ((mask & 1) == 0))
    {
        i++;
    }
    if(i == 32)
    {
        offset = 0;
        size = 0;
        return;
    }
    offset = i;
    while(i < 32 && ((mask & 1) == 1))
    {
        i++;
    }
    size = i - offset;
}

istream& operator>>(istream& stream, BMPHeader& header)
{
    memset(&header, 0, sizeof(header));
    unsigned char *buffer = new unsigned char[FILE_HEADER_SIZE + V5_HEADER_SIZE];
    memset(buffer, 0, FILE_HEADER_SIZE + V5_HEADER_SIZE);
    stream.read(reinterpret_cast<char *>(buffer), FILE_HEADER_SIZE + INFO_HEADER_SIZE_SIZE);
    if(!stream.good())
    {
        throw runtime_error(stream.eof() ? "Unexpected end of file (corrupted file)" : "Unknown I/O error");
    }
    header.headerSize += (uint32_t)stream.gcount();
    char magic[3];
    READ(buffer, MAGIC1, magic[0]);
    READ(buffer, MAGIC2, magic[1]);
	magic[2] = '\0';
    if(strcmp(magic, "BM"))
    {
        delete[] buffer;
        throw runtime_error("Not a BMP file");
    }
    READ(buffer, BITMAP_OFFSET, header.dataOffset);
    uint32_t headerSize;
    READ(buffer, INFO_HEADER_SIZE, headerSize);
    bool core = headerSize <= CORE_HEADER_SIZE;
    bool core2 = headerSize == CORE2_HEADER_SIZE;
    bool defaultMask = true;
    stream.read(reinterpret_cast<char *>(buffer + FILE_HEADER_SIZE + INFO_HEADER_SIZE_SIZE), headerSize - INFO_HEADER_SIZE_SIZE);
    if(!stream.good())
    {
        delete[] buffer;
        throw runtime_error(stream.eof() ? "Unexpected end of file (corrupted header)" : "Unknown I/O error");
    }
    header.headerSize += (uint32_t)stream.gcount();
    if(core)
    {
        READ(buffer, CORE_WIDTH, header.width);
        int16_t height;
        READ(buffer, CORE_HEIGHT, height);
        if(height >= 0)
        {
            header.bottomToTop = true;
            header.height = height;
        }
        else
        {
            header.bottomToTop = false;
            header.height = -height;
        }
        READ(buffer, CORE_BPP, header.bpp);
        header.compression = NO_COMPRESSION;
        header.paletteSize = header.bpp > 8 ? 0 : 1 << header.bpp;
        header.paletteElementSize = 3;
        header.aChannel.offset = 0; header.aChannel.size = 0;
    }
    else
    {
        READ(buffer, WIDTH, header.width);
        int32_t height;
        READ(buffer, HEIGHT, height);
		if(height >= 0)
        {
            header.bottomToTop = true;
            header.height = height;
        }
        else
        {
            header.bottomToTop = false;
            header.height = -height;
        }
        READ(buffer, BPP, header.bpp);
        uint32_t compr;
        READ(buffer, COMPRESSION, compr);
        header.compression = static_cast<Compression>(compr);
        if(header.compression > 6 ||
           (header.compression == RLE4_COMPRESSION && header.bpp != 4) ||
           (header.compression == RLE8_COMPRESSION && header.bpp != 8) ||
           (header.compression == RLE24_COMPRESSION && header.bpp != 24))
        {
            delete[] buffer;
            throw runtime_error("Invalid compression method");
        }
        if(core2 && (header.compression == 3 || header.compression == 4))
        {
            header.compression = static_cast<Compression>(static_cast<int>(header.compression) + 8);
        }
        READ(buffer, COLORS, header.paletteSize);
        header.paletteElementSize = 4;
        if((header.paletteSize == 0 || header.paletteSize > 1 << header.bpp) && header.bpp <= 8)
        {
            header.paletteSize = 1 << header.bpp;
        }
        if(header.bpp > 8)
        {
            header.paletteSize = 0;
        }
        if(header.compression == BITFIELDS_COMPRESSION && headerSize < V2_HEADER_SIZE)
        {
            stream.read(reinterpret_cast<char *>(buffer + headerSize), 4 * 3);
            if(!stream.good())
            {
                delete[] buffer;
                throw runtime_error(stream.eof() ? "Unexpected end of file (corrupted header)" : "Unknown I/O error");
            }
            header.headerSize += (uint32_t)stream.gcount();
            headerSize = V2_HEADER_SIZE;
        }
        if(header.compression == ALPHABITFIELDS_COMPRESSION)
        {
            header.compression = BITFIELDS_COMPRESSION;
            if(headerSize < V2_HEADER_SIZE)
            {
                stream.read(reinterpret_cast<char *>(buffer + headerSize), 4 * 4);
                if(!stream.good())
                {
                    delete[] buffer;
                    throw runtime_error(stream.eof() ? "Unexpected end of file (corrupted header)" : "Unknown I/O error");
                }
                header.headerSize += (uint32_t)stream.gcount();
                headerSize = V3_HEADER_SIZE;
            }
        }
        if(header.compression == BITFIELDS_COMPRESSION)
        {
            header.compression = NO_COMPRESSION;
            defaultMask = false;
            uint32_t rMask, gMask, bMask;
            READ(buffer, R_MASK, rMask);
            header.rChannel.set(rMask);
            READ(buffer, G_MASK, gMask);
            header.gChannel.set(gMask);
            READ(buffer, B_MASK, bMask);
            header.bChannel.set(bMask);
        }
        if((headerSize >= V3_HEADER_SIZE && !core2))
        {
            uint32_t aMask;
            READ(buffer, A_MASK, aMask);
            header.aChannel.set(aMask);
        }
        else
        {
            header.aChannel.offset = 0; header.aChannel.size = 0;
        }
    }
    if(header.bpp != 1 && header.bpp != 2 && header.bpp != 4 && header.bpp != 8 &&
       header.bpp != 16 && header.bpp != 24 && header.bpp != 32)
    {
        delete[] buffer;
        throw runtime_error("Invalid BPP value");
    }
    if(defaultMask)
    {
        if(header.bpp == 16)
        {
            header.bChannel.offset = 0; header.bChannel.size = 5;
            header.gChannel.offset = 5; header.bChannel.size = 5;
            header.rChannel.offset = 10; header.bChannel.size = 5;
        }
        else
        {
            header.bChannel.offset = 0; header.bChannel.size = 8;
            header.gChannel.offset = 8; header.gChannel.size = 8;
            header.rChannel.offset = 16; header.rChannel.size = 8;
        }
    }
    delete[] buffer;
    return stream;
}

void writeBMPHeader(unsigned char *buffer, uint32_t width, uint32_t height)
{
    memset(buffer, 0, FILE_HEADER_SIZE + V1_HEADER_SIZE);
    WRITE(buffer, MAGIC1, 'B');
    WRITE(buffer, MAGIC2, 'M');
    uint32_t imageSize = width * height * 4;
    uint32_t size = FILE_HEADER_SIZE + V1_HEADER_SIZE + imageSize;
    WRITE(buffer, FILE_SIZE, size);
    WRITE(buffer, BITMAP_OFFSET, FILE_HEADER_SIZE + V1_HEADER_SIZE);
    WRITE(buffer, INFO_HEADER_SIZE, V1_HEADER_SIZE);
    WRITE(buffer, WIDTH, width);
    WRITE(buffer, HEIGHT, height);
    WRITE(buffer, PLANES, 1);
    WRITE(buffer, BPP, 32);
    WRITE(buffer, COMPRESSION, NO_COMPRESSION);
    WRITE(buffer, IMAGE_SIZE, imageSize);
    WRITE(buffer, XRES, 3780);
    WRITE(buffer, YRES, 3780);
}
