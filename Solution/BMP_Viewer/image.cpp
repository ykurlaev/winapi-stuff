#include "image.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>

using namespace YQ;

using std::swap;
using std::istream;
using std::ostream;
using std::runtime_error;

static void readUncompressedData(const uint8_t *buffer, const uint32_t *palette,
                                 uint8_t bpp, bool bottomToTop, Image& image)
{
    long width = image.header.biWidth;
    long height = image.header.biHeight;
    uint32_t lineSize = ((width * bpp + 31) / 32) * 4; // round to 4 bytes up
    for(long y = 0; y < height; y++)
    {
        for(long x = 0; x < width; x++)
        {
            long ry = bottomToTop ? y : height - y;
            uint32_t color = 0;
            if(bpp >= 8)
            {
                for(int i = 0; i < bpp / 8; i++)
                {
                    uint8_t byte = buffer[y * lineSize + x * (bpp / 8) + i];
                    color = color | (byte << i * 8);
                }
            }
            else
            {
                uint8_t byte = buffer[y * lineSize + (x * bpp) / 8];
                color = (byte >> ((8 - (((x + 1) * bpp) % 8)) % 8)) // pixels in byte are stored from high bits to low
                        & (0xFF >> (8 - bpp));
            }
            if(bpp <= 8)
            {
                color = palette[color];
            }
            for(int i = 0; i < 3; i++)
            {
                if(bpp == 16) // 5 bits per channel
                {
                    image[ry][x][i] = (color >> (i * 5) & 0x1F) // 0x1F = binary 11111 (5x1)
                                      * 0xFF / 0x1F; // normalize to 8 bpc
                }
                else // 8 bits per channel
                {
                    image[ry][x][i] = color >> (i * 8) & 0xFF;
                }
            }
            image[ry][x][Image::A] = 0xFF; // alpha channel is ignored
        }
    }
}

istream& YQ::operator>>(istream& stream, Image& image)
{
    if(image.pixels != NULL)
    {
        delete[] image.pixels;
        image = Image();
    }
    BITMAPFILEHEADER fileHeader;
    stream.read(reinterpret_cast<char *>(&fileHeader), sizeof(fileHeader));
    if(!stream.good())
    {
        throw runtime_error(stream.eof() ? "Not a BMP or corrupted file" : "Unknown I/O error");
    }
    if(fileHeader.bfType != 0x4D42) // "BM" signature
    {
        throw runtime_error("Not a BMP file");
    }
    uint32_t bufferSize = fileHeader.bfSize - sizeof(fileHeader);
    char *buffer = new char[bufferSize];
    memset(buffer, 0, bufferSize);
    stream.read(buffer, bufferSize);
    uint32_t infoHeaderSize = *reinterpret_cast<uint32_t *>(buffer);
    switch(infoHeaderSize)
    {
        case sizeof(BITMAPINFOHEADER):
        case sizeof(BITMAPV4HEADER):
        case sizeof(BITMAPV5HEADER): // alpha channel is ignored, even if alpha mask is present
            break;
        default:
            throw runtime_error("Unsupported BMP header type");
    }
    BITMAPINFOHEADER *pInfoHeader = reinterpret_cast<BITMAPINFOHEADER *>(buffer);
    image.header.biWidth = pInfoHeader->biWidth;
    image.header.biHeight = pInfoHeader->biHeight;
    bool bottomToTop = image.header.biHeight >= 0;
    if(!bottomToTop)
    {
        image.header.biHeight = -image.header.biHeight;
    }
    if(pInfoHeader->biCompression != BI_RGB)
    {
        throw runtime_error("Compression is not supported");
    }
    uint8_t bpp = static_cast<uint8_t>(pInfoHeader->biBitCount);
    if(bpp != 1 && bpp != 2 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32)
    {
        throw runtime_error("Unsupported bpp");
    }
    image.pixels = new Image::Pixel[image.header.biWidth * image.header.biHeight * 4];
    uint32_t *palette = reinterpret_cast<uint32_t *>(buffer + infoHeaderSize);
    uint8_t *data = reinterpret_cast<uint8_t *>(buffer + fileHeader.bfOffBits - sizeof(fileHeader));
    readUncompressedData(data, palette, bpp, bottomToTop, image);
    delete[] buffer;
    return stream;
}

ostream& YQ::operator<<(ostream& stream, Image& image)
{
    BITMAPFILEHEADER fileHeader;
    memset(&fileHeader, 0, sizeof(fileHeader));
    fileHeader.bfType = 0x4D42; // "BM"
    uint32_t dataSize = image.header.biWidth * image.header.biHeight * 4;
    fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(image.header);
    fileHeader.bfSize = fileHeader.bfOffBits + dataSize;
    stream.write(reinterpret_cast<char *>(&fileHeader), sizeof(fileHeader));
    if(!stream.good())
    {
        throw runtime_error("Unknown I/O error");
    }
    stream.write(reinterpret_cast<char *>(&image.header), sizeof(image.header));
    if(!stream.good())
    {
        throw runtime_error("Unknown I/O error");
    }
    stream.write(reinterpret_cast<char *>(image.pixels), dataSize);
    if(!stream.good())
    {
        throw runtime_error("Unknown I/O error");
    }
    return stream;
}

Image::Image()
: pixels(NULL)
{
    memset(&header, 0, sizeof(header));
    header.biSize = sizeof(header);
    header.biPlanes = 1;
    header.biBitCount = 32;
    header.biCompression = BI_RGB;
    header.biXPelsPerMeter = 3780; // 96 dpi
    header.biYPelsPerMeter = 3780;
}

Image::~Image()
{
    delete[] pixels;
}

void Image::flipX()
{
    for(long y = 0; y < header.biHeight; y++)
    {
        for(long x = 0; x < header.biWidth / 2; x++)
        {
            for(int i = 0; i < 4; i++)
            {
                swap((*this)[y][x][i], (*this)[y][header.biWidth - x - 1][i]);
            }
        }
    }
}

void Image::flipY()
{
    for(long y = 0; y < header.biHeight / 2; y++)
    {
        for(long x = 0; x < header.biWidth; x++)
        {
            for(int i = 0; i < 4; i++)
            {
                swap((*this)[y][x][i], (*this)[header.biHeight - y - 1][x][i]);
            }
        }
    }
}

void Image::invert(Channel channel)
{
    for(long y = 0; y < header.biHeight; y++)
    {
        for(long x = 0; x < header.biWidth; x++)
        {
            (*this)[y][x][channel] = 0xFF - (*this)[y][x][channel];
        }
    }
}
