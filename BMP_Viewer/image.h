#ifndef YQ_IMAGE_H
#define YQ_IMAGE_H

#include <windows.h>
#include <cstdint>
#include <cstring>
#include <istream>
#include <ostream>

namespace YQ
{

struct Image
{
    enum Channel { B = 0, G = 1, R = 2, A = 3 };
    typedef uint8_t Pixel[4];
    Image();
    ~Image();
    BITMAPINFOHEADER header;
    Pixel *pixels;
    Pixel *operator[](uint32_t y) const { return pixels + header.biWidth * y; }
    void flipX();
    void flipY();
    void invert(Channel channel);
};

std::istream& operator>>(std::istream& stream, Image& image);

std::ostream& operator<<(std::ostream& stream, Image& image);

}

#endif //YQ_IMAGE_H
