#include "image.h"
#include "bmpheader.h"
#include <cstring>
#include <stdexcept>

using std::istream;
using std::ostream;
using std::ios_base;
using std::runtime_error;

static uint32_t readUncompressedData(Image *image, const BMPHeader& header, const uint32_t *palette, istream& stream)
{
    uint32_t lineSize = ((header.width * header.bpp + 31) / 32) * 4;
    unsigned char *buffer = new unsigned char[lineSize * header.height];
    memset(buffer, 0, lineSize * header.height);
    stream.read(reinterpret_cast<char *>(buffer), lineSize * header.height);
    for(uint32_t y = 0; y < header.height; y++)
    {
        for(uint32_t x = 0; x < header.width; x++)
        {
			uint32_t ry = header.bottomToTop ? y : header.height - y;
            uint32_t color;
            if(header.bpp >= 8)
            {
				color = 0;
                for(int i = 0; i < header.bpp / 8; i++)
                {
                    color = color | (buffer[y * lineSize + x * header.bpp / 8 + i] << i * 8);
                }
            }
			else
			{
				color = (buffer[y * lineSize + x * header.bpp / 8] >> ((8 - (x + 1) * header.bpp) % 8))
                             & (UINT8_MAX >> (8 - (header.bpp % 8)));
			}
            if(palette)
            {
                color = palette[color];
            }
            if(header.bChannel.size == 0)
            {
                (*image)[ry][x][Image::B] = UINT8_MAX;
            }
            else
            {
                (*image)[ry][x][Image::B] = (color >> header.bChannel.offset) & (UINT32_MAX >> (32 - header.bChannel.size))
                                             * UINT8_MAX / ((1 << header.bChannel.size) - 1);
            }
            if(header.gChannel.size == 0)
            {
                (*image)[ry][x][Image::G] = UINT8_MAX;
            }
            else
            {
                (*image)[ry][x][Image::G] = (color >> header.gChannel.offset) & (UINT32_MAX >> (32 - header.gChannel.size))
                                             * UINT8_MAX / ((1 << header.gChannel.size) - 1);
            }
            if(header.rChannel.size == 0)
            {
                (*image)[ry][x][Image::R] = UINT8_MAX;
            }
            else
            {
                (*image)[ry][x][Image::R] = (color >> header.rChannel.offset) & (UINT32_MAX >> (32 - header.rChannel.size))
                                             * UINT8_MAX / ((1 << header.rChannel.size) - 1);
            }
            if(header.aChannel.size == 0)
            {
                (*image)[ry][x][Image::A] = UINT8_MAX;
            }
            else
            {
                (*image)[ry][x][Image::A] = (color >> header.aChannel.offset) & (UINT32_MAX >> (32 - header.aChannel.size))
                                             * UINT8_MAX / ((1 << header.aChannel.size) - 1);
            }
        }
    }
    delete[] buffer;
    return lineSize * header.height;
}

istream& operator>>(istream& stream, Image& image)
{
    if(image.pixels != NULL)
    {
        delete[] image.pixels;
        image.pixels = NULL;
    }
    image.width = 0;
    image.height = 0;
    BMPHeader header;
    stream >> header;
    uint32_t *palette = NULL;
    uint32_t paletteSize = header.paletteSize * header.paletteElementSize;
    if(header.paletteSize > 0)
    {
        unsigned char *paletteRaw = new unsigned char[paletteSize];
        memset(paletteRaw, 0, paletteSize);
        stream.read(reinterpret_cast<char *>(paletteRaw), paletteSize);
        palette = new uint32_t[header.paletteSize];
		memset(palette, 0, header.paletteSize * 4);
        for(uint16_t i = 0; i < header.paletteSize; i++)
        {
            for(int j = 0; j < header.paletteElementSize; j++)
            {
                palette[i] = palette[i] | (paletteRaw[i * header.paletteElementSize + j] << j * 8);
            }
        }
        delete paletteRaw;
    }
    stream.seekg(header.dataOffset - (header.headerSize + paletteSize), ios_base::cur);
    image.width = header.width;
    image.height = header.height;
    image.pixels = new uint8_t[image.width * image.height][4];
    memset(image.pixels, 0, image.width * image.height * 4);
    uint32_t dataSize = 0;
    if(header.compression == NO_COMPRESSION)
    {
        dataSize = readUncompressedData(&image, header, palette, stream);
        stream.seekg(header.fileSize - (header.dataOffset + dataSize), ios_base::cur);
    }
    else
    {
        throw runtime_error("Compression is not supported yet");
        stream.seekg(header.fileSize - (header.dataOffset + dataSize), ios_base::cur);
    }
    return stream;
}

ostream& operator<<(ostream& stream, Image& image)
{
    unsigned char *header = new unsigned char[FILE_HEADER_SIZE + V1_HEADER_SIZE];
    writeBMPHeader(header, image.width, image.height);
    stream.write(reinterpret_cast<char *>(header), FILE_HEADER_SIZE + V1_HEADER_SIZE);
    delete[] header;
    stream.write(reinterpret_cast<char *>(image.pixels), image.width * image.height * 4);
    return stream;
}
